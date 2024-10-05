/*

===== doors.cpp ========================================================

Doors are similar to buttons, but can spawn a fat trigger field around them
to open without a touch, and they link together to form simultanious
double/quad doors.
 
Door->m_pOwner is the master door.  If there is only one door, it points to itself.
If multiple doors, all will point to a single one.

Door->m_pNextDoor chains from the master door through all doors linked in the chain.
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "doors.h"
#include "items.h"
#include "player.h"
#include "gamerules.h"

extern DLL_GLOBAL int		g_iWorldType;

/*QUAKED func_door (0 .5 .8) ? START_OPEN x DOOR_DONT_LINK GOLD_KEY SILVER_KEY TOGGLE
if two doors touch, they are assumed to be connected and operate as a unit.

TOGGLE causes the door to wait in both the start and end states for a trigger event.

START_OPEN causes the door to move to its destination when spawned, and operate in reverse.
It is used to temporarily or permanently close off an area when triggered (not usefull for touch or takedamage doors).

Key doors are allways wait -1.

"message"	is printed when the door is touched if it is a trigger door and it hasn't been fired yet
"angle"	determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"health"	if set, door must be shot open
"speed"	movement speed (100 default)
"wait"	wait before returning (3 default, -1 = never return)
"lip"	lip remaining at end of move (8 default)
"dmg"	damage to inflict when blocked (2 default)
"sounds"
0)	no sound
1)	stone
2)	base
3)	stone chain
4)	screechy metal
*/
LINK_ENTITY_TO_CLASS(func_door, CBaseDoor);
LINK_ENTITY_TO_CLASS(func_water, CBaseDoor); //DOOM

#define SF_WATER_SIDES	2

class CDoorTrigger : public CBaseEntity
{
public:
	void SpawnInsideTrigger( CBaseDoor *pDoor, Vector mins, Vector maxs );
	void Touch( CBaseEntity *pOther );
	void Precache( void );
	CBaseDoor *m_pDoor;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( door_trigger, CDoorTrigger );	// g-cont. need for save\restore

TYPEDESCRIPTION	CDoorTrigger::m_SaveData[] = 
{
	DEFINE_FIELD( CDoorTrigger, m_pDoor, FIELD_CLASSPTR ),
}; IMPLEMENT_SAVERESTORE( CDoorTrigger, CBaseEntity );

//
// Create a trigger entity for a door.
//
void CDoorTrigger :: SpawnInsideTrigger( CBaseDoor *pDoor, Vector mins, Vector maxs )
{
	m_pDoor = pDoor;

	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->classname = MAKE_STRING ("door_trigger");

	// establish the trigger field's size
	Vector vecTMin = mins - Vector ( 60, 60, 8 );
	Vector vecTMax = maxs + Vector ( 60, 60, 8 );

	UTIL_SetSize ( pev, vecTMin, vecTMax );
}

//
// door_trigger isn't have model, so we need restore them size here
//
void CDoorTrigger :: Precache( void )
{
	UTIL_SetSize ( pev, pev->mins, pev->maxs );
}

//
// When the platform's trigger field is touched, the platform ???
//
void CDoorTrigger :: Touch( CBaseEntity *pOther )
{
	if( !m_pDoor )
	{
		// g-cont. in case of stupid level-designer set this entity on map manually
		ALERT( at_error, "%s with no linked door. Removed\n", STRING( pev->classname ));

		pev->nextthink = gpGlobals->time + 0.1;
		SetThink( &CBaseEntity::SUB_Remove );
		return; 
	}

	// g-cont. all monsters can open the doors in Quake!
	if (!pOther->IsAlive())
		return;

	if (pev->pain_finished > gpGlobals->time)
		return;
	pev->pain_finished = gpGlobals->time + 1.0;

	m_pDoor->Use( pOther, this, USE_TOGGLE, 0 );
}

static void DoorSpawnInsideTrigger( CBaseDoor *pDoor, Vector mins, Vector maxs )
{
	GetClassPtr( (CDoorTrigger *)NULL)->SpawnInsideTrigger( pDoor, mins, maxs );
}

TYPEDESCRIPTION CBaseDoor::m_SaveData[] =
{
	DEFINE_FIELD(CBaseDoor, m_pOwner, FIELD_CLASSPTR),
	DEFINE_FIELD(CBaseDoor, m_pNextDoor, FIELD_CLASSPTR),
	DEFINE_FIELD(CBaseDoor, lightstyle, FIELD_INTEGER),
	DEFINE_FIELD(CBaseDoor, lightstyletoggle, FIELD_BOOLEAN),
}; IMPLEMENT_SAVERESTORE(CBaseDoor, CBaseToggle);

void CBaseDoor::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "WaveHeight"))
	{
		pev->scale = atof(pkvd->szValue) * (1.0/8.0);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "switchshadstyle"))
	{
		lightstyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "FogDensity")) //water fog
	{
		pev->iuser4 = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CBaseDoor::Spawn( void )
{
	Precache();

	SetMovedir (pev);

	if (pev->skin == 0)
		pev->solid = SOLID_BSP; // normal door
	else
		pev->solid = SOLID_NOT; // special contents

	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT(pev), STRING(pev->model) );

	// g-cont. allow to lock door with both gold and silver keys
	if (pev->spawnflags & SF_DOOR_SILVER_KEY)
		pev->team |= IT_KEY1;
	if (pev->spawnflags & SF_DOOR_GOLD_KEY)
		pev->team |= IT_KEY2;
	if (pev->spawnflags & SF_DOOR_RED_KEY)
		pev->team |= IT_KEY3;
	
	if (!pev->speed)
		pev->speed = 100;

	if (!m_flWait)
		m_flWait = 3;

	if (!m_flLip)
		m_flLip = 8;

	if (!pev->dmg)
		pev->dmg = 2;
	
	m_vecPosition1 = pev->origin;
	
	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + (pev->movedir * (fabs(pev->movedir.x * (pev->size.x - 2)) + fabs(pev->movedir.y * (pev->size.y - 2)) + fabs(pev->movedir.z * (pev->size.z - 2)) - m_flLip));
	//m_vecPosition2 = m_vecPosition1 + pev->movedir * (fabs(DotProduct( pev->movedir, pev->size )) - m_flLip);

	ASSERTSZ(m_vecPosition1 != m_vecPosition2, "door start/end positions are equal");

	// SF_DOOR_START_OPEN is to allow an entity to be lighted in the closed position
	// but spawn in the open position
	if ( FBitSet (pev->spawnflags, SF_DOOR_START_OPEN) )
	{	
		UTIL_SetOrigin(pev, m_vecPosition2);
		m_vecPosition2 = m_vecPosition1;
		m_vecPosition1 = pev->origin;
	}

	if (FBitSet(pev->spawnflags, SF_DOOR_SHADOW_ON))
		lightstyletoggle = true;

	if (lightstyle)
	{
		if (lightstyletoggle)
		{
			//ALERT(at_console, "turned off %i\n", lightstyle);
			LIGHT_STYLE(lightstyle, "m");
			lightstyletoggle = false;
		}
		else
		{
			//ALERT(at_console, "turned on %i\n", lightstyle);
			LIGHT_STYLE(lightstyle, "a");
			lightstyletoggle = true;
		}
	}

	m_toggle_state = TS_AT_BOTTOM;

	if (FBitSet(pev->spawnflags, SF_DOOR_USE_ONLY))
		SetTouch(NULL);
	else
		SetTouch(&CBaseDoor::DoorTouch); // touchable button
	
	// if the door is flagged for USE button activation only, use NULL touch function
	if (pev->health > 0)
	{
		pev->takedamage = DAMAGE_YES;
		pev->max_health = pev->health;

		SetTouch ( NULL );
	}

	if (pev->team)
		m_flWait = -1;

	if (!FClassnameIs(pev, "func_water")) //func_water dont need this..
	{
		SetThink(&CBaseDoor::LinkDoors);
		pev->nextthink = pev->ltime + 0.1;
	}
	
	if (FBitSet(pev->spawnflags, SF_WATER_SIDES))
		pev->effects |= EF_WATERSIDES;
}

int CBaseDoor :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (!pev->takedamage)
		return 0;

	pev->health -= flDamage;

	if (pev->health <= 0)
	{
		Killed( pevAttacker, GIB_NORMAL );
		return 0;
	}

	return 1;
}

void CBaseDoor::Killed( entvars_t *pevAttacker, int iGib )
{
	m_hActivator = CBaseEntity::Instance( pevAttacker );

	m_pOwner->pev->health = m_pOwner->pev->max_health;
	m_pOwner->pev->takedamage = DAMAGE_NO;	// will be reset upon return
	m_pOwner->DoorActivate( );
}

void CBaseDoor::Precache( void )
{
	if (g_iWorldType == WORLDTYPE_MEDIEVAL)
	{
		PRECACHE_SOUND ("doors/medtry.wav");
		PRECACHE_SOUND ("doors/meduse.wav");
		pev->noise2 = MAKE_STRING( "doors/medtry.wav" );
		pev->noise3 = MAKE_STRING( "doors/meduse.wav" );
	}
	else if (g_iWorldType == WORLDTYPE_RUNIC)
	{
		PRECACHE_SOUND ("doors/runetry.wav");
		PRECACHE_SOUND ("doors/runeuse.wav");
		pev->noise2 = MAKE_STRING( "doors/runetry.wav" );
		pev->noise3 = MAKE_STRING( "doors/runeuse.wav" );
	}
	else if (g_iWorldType == WORLDTYPE_PRESENT)
	{
		PRECACHE_SOUND ("doors/basetry.wav");
		PRECACHE_SOUND ("doors/baseuse.wav");
		pev->noise2 = MAKE_STRING( "doors/basetry.wav" );
		pev->noise3 = MAKE_STRING( "doors/baseuse.wav" );
	}

	// set the door's "in-motion" sound
	switch (m_sounds)
	{
	case 1:
		PRECACHE_SOUND ("doors/drclos4.wav");
		PRECACHE_SOUND ("doors/doormv1.wav");
		pev->noise = MAKE_STRING("doors/drclos4.wav");
		pev->noise1 = MAKE_STRING("doors/doormv1.wav");
		break;
	case 2:
		PRECACHE_SOUND ("doors/hydro1.wav");
		PRECACHE_SOUND ("doors/hydro2.wav");
		pev->noise = MAKE_STRING("doors/hydro2.wav");
		pev->noise1 = MAKE_STRING("doors/hydro1.wav");
		break;
	case 3:
		PRECACHE_SOUND ("doors/stndr1.wav");
		PRECACHE_SOUND ("doors/stndr2.wav");
		pev->noise = MAKE_STRING("doors/stndr2.wav");
		pev->noise1 = MAKE_STRING("doors/stndr1.wav");
		break;
	case 4:
		PRECACHE_SOUND ("doors/ddoor1.wav");
		PRECACHE_SOUND ("doors/ddoor2.wav");
		pev->noise = MAKE_STRING("doors/ddoor2.wav");
		pev->noise1 = MAKE_STRING("doors/ddoor1.wav");
		break;
	case 5:
		PRECACHE_SOUND("doors/door_close.wav");
		PRECACHE_SOUND("doors/door_open.wav");
		pev->noise = MAKE_STRING("doors/door_close.wav");
		pev->noise1 = MAKE_STRING("doors/door_open.wav");
		break;
	default:
		PRECACHE_SOUND ("misc/null.wav");
		pev->noise = MAKE_STRING("misc/null.wav");
		pev->noise1 = MAKE_STRING("misc/null.wav");
		break;
	}
}

void CBaseDoor :: LinkDoors( void )
{
	edict_t	*pentNext;
	Vector	cmins, cmaxs;
	CBaseDoor	*pStart, *pNext, *pCurrent;

	if (m_pNextDoor)
		return;		// already linked by another door

	if (pev->spawnflags & SF_DOOR_DONT_LINK)
	{
		m_pOwner = m_pNextDoor = this;
		return;		// don't want to link this door
	}

	cmins = pev->mins;
	cmaxs = pev->maxs;
	
	pentNext = ENT(pev);
	pStart = pCurrent = this;
	
	while (1)
	{
		pCurrent->m_pOwner = pStart; // master door

		if (pCurrent->pev->health)
			pStart->pev->health = pCurrent->pev->health;
		if (!FStringNull( pCurrent->pev->targetname ))
			pStart->pev->targetname = pCurrent->pev->targetname;
		if (!FStringNull( pCurrent->pev->message ))
			pStart->pev->message = pCurrent->pev->message;

		pentNext = FIND_ENTITY_BY_CLASSNAME(pentNext, STRING(pCurrent->pev->classname));

		if (FNullEnt(pentNext))
		{
			pCurrent->m_pNextDoor = pStart;	// make the chain a loop

			// shootable, fired, or key doors just needed the owner/enemy links,
			// they don't spawn a field
			pCurrent = pCurrent->m_pOwner;

			if (pCurrent->pev->health)
				return;
			if (!FStringNull( pCurrent->pev->targetname ))
				return;
			if (pCurrent->pev->team)
				return;

			DoorSpawnInsideTrigger( pCurrent, cmins, cmaxs );
			return;
		}

		pNext = GetClassPtr( (CBaseDoor *) VARS(pentNext) );

		if (pCurrent->Intersects(pNext))
		{
			if (pNext->m_pNextDoor)
			{
				ALERT( at_error, "cross connected doors\n" );
				pCurrent->pev->nextthink = pev->ltime + 0.1;
				pCurrent->SetThink( &CBaseDoor::SUB_Remove );
				return;
			}			
                              
			pCurrent->m_pNextDoor = pNext;
			pCurrent = pNext;

			if (pNext->pev->mins.x < cmins.x)
				cmins.x = pNext->pev->mins.x;
			if (pNext->pev->mins.y < cmins.y)
				cmins.y = pNext->pev->mins.y;
			if (pNext->pev->mins.z < cmins.z)
				cmins.z = pNext->pev->mins.z;
			if (pNext->pev->maxs.x > cmaxs.x)
				cmaxs.x = pNext->pev->maxs.x;
			if (pNext->pev->maxs.y > cmaxs.y)
				cmaxs.y = pNext->pev->maxs.y;
			if (pNext->pev->maxs.z > cmaxs.z)
				cmaxs.z = pNext->pev->maxs.z;
		}
	}
}

//
// Doors not tied to anything (e.g. button, another door) can be touched, to make them activate.
//
void CBaseDoor::DoorTouch( CBaseEntity *pOther )
{
	// Ignore touches by anything but players
	if (!FClassnameIs(pOther->pev, "player"))
		return;

	if (m_pOwner->pev->pain_finished > gpGlobals->time)
		return;

	m_pOwner->pev->pain_finished = gpGlobals->time + 2.0;

	if (!FStringNull( m_pOwner->pev->message ))
	{
		CenterPrint( pOther->pev, STRING( m_pOwner->pev->message ));
		EMIT_SOUND(ENT(pOther->pev), CHAN_BODY, "misc/talk.wav", 1, ATTN_NORM);
	}

	// key door stuff
	if (!pev->team)
		return;

	// FIXME: blink key on player's status bar
	if (( pev->team & pOther->m_iItems ) != pev->team )
	{
		if (m_pOwner->pev->team == IT_KEY1)
		{
			if (g_iWorldType == WORLDTYPE_PRESENT)
				CenterPrint( pOther->pev, "You need the blue keycard" );
			else
				CenterPrint( pOther->pev, "You need the blue skull key" );
		}
		else if (m_pOwner->pev->team == IT_KEY2)
		{
			if (g_iWorldType == WORLDTYPE_PRESENT)
				CenterPrint( pOther->pev, "You need the yellow keycard" );
			else
				CenterPrint( pOther->pev, "You need the yellow skull key" );;
		}
		else if (m_pOwner->pev->team == IT_KEY3)
		{
			if (g_iWorldType == WORLDTYPE_PRESENT)
				CenterPrint(pOther->pev, "You need the red keycard");
			else
				CenterPrint(pOther->pev, "You need the red skull key");
		}
		else if (m_pOwner->pev->team & IT_KEY1 && m_pOwner->pev->team & IT_KEY2)
		{
			if (g_iWorldType == WORLDTYPE_PRESENT)
				CenterPrint( pOther->pev, "You need the blue and yellow keycards" );
			else
				CenterPrint( pOther->pev, "You need the blue and yellow skull keys" );
		}
		else if (m_pOwner->pev->team & IT_KEY2 && m_pOwner->pev->team & IT_KEY3)
		{
			if (g_iWorldType == WORLDTYPE_PRESENT)
				CenterPrint(pOther->pev, "You need the yellow and red keycards");
			else
				CenterPrint(pOther->pev, "You need the yellow and red skull keys");
		}
		else if (m_pOwner->pev->team & IT_KEY1 && m_pOwner->pev->team & IT_KEY3)
		{
			if (g_iWorldType == WORLDTYPE_PRESENT)
				CenterPrint(pOther->pev, "You need the blue and red keycards");
			else
				CenterPrint(pOther->pev, "You need the blue and red skull keys");
		}
		else if (m_pOwner->pev->team & IT_KEY1 && m_pOwner->pev->team & IT_KEY2 && m_pOwner->pev->team & IT_KEY3)
		{
			if (g_iWorldType == WORLDTYPE_PRESENT)
				CenterPrint(pOther->pev, "You need all 3 keycards");
			else
				CenterPrint(pOther->pev, "You need all 3 skull keys");
		}

		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise2), 1, ATTN_NORM);
		return;
	}

	// remove player keys
	pOther->m_iItems &= ~pev->team;

	SetTouch( NULL );

	if( m_pNextDoor )
		m_pNextDoor->SetTouch( NULL );	// get paired door

	DoorActivate( );
}


//
// Used by SUB_UseTargets, when a door is the target of a button.
//
void CBaseDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;

 	pev->message = iStringNull; // door message are for touch only
	m_pOwner->pev->message = iStringNull;
	m_pNextDoor->pev->message = iStringNull;

	m_pOwner->DoorActivate();
}

//
// Causes the door to "do its thing", i.e. start moving, and cascade activation.
//
void CBaseDoor::DoorActivate( void )
{
/*
	if (m_pOwner != this)
	{
		ALERT( at_error, "DoorActivate: %s invalid link. Removed\n", STRING( pev->classname ));

		pev->nextthink = pev->ltime + 0.1;
		SetThink( SUB_Remove );
		return; 
	}
*/
	// play use key sound
	if (pev->team)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise3), 1, ATTN_NORM);

	pev->message = iStringNull;	// no more message

	CBaseDoor	*pCurrent, *pStart;

	if (pev->spawnflags & SF_DOOR_TOGGLE)
	{
		if (m_toggle_state == TS_GOING_UP || m_toggle_state == TS_AT_TOP)
		{
			pStart = pCurrent = this;
			do
			{
				pCurrent->DoorGoDown();
				pCurrent = pCurrent->m_pNextDoor;

			} while (( pCurrent != pStart ) && (pCurrent != NULL) );
			return;
		}
	}

	// trigger all paired doors
	pStart = pCurrent = this;
	do
	{
		pCurrent->DoorGoUp();
		pCurrent = pCurrent->m_pNextDoor;

	} while (( pCurrent != pStart ) && (pCurrent != NULL) );
}

//
// The door has reached the "up" position.  Either go back down, or wait for another activation.
//
void CBaseDoor::DoorHitTop( void )
{
	if (m_sounds != 5)
	{
		STOP_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise1));
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);
	}

	m_toggle_state = TS_AT_TOP;
	
	// toggle-doors don't come down automatically
	if (FBitSet(pev->spawnflags, SF_DOOR_TOGGLE))
		return;

	SetThink( &CBaseDoor::DoorGoDown );
	pev->nextthink = pev->ltime + m_flWait;
}

//
// The door has reached the "down" position.  Back to quiescence.
//
void CBaseDoor::DoorHitBottom( void )
{
	if (m_sounds != 5)
	{
		STOP_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise1));
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);
	}

	m_toggle_state = TS_AT_BOTTOM;
}

//
// Starts the door going to its "down" position (simply ToggleData->vecPosition1).
//
void CBaseDoor::DoorGoDown( void )
{
	if (lightstyle)
	{
		if (lightstyletoggle)
		{
			//ALERT(at_console, "turned off %i\n", lightstyle);
			LIGHT_STYLE(lightstyle, "m");
			lightstyletoggle = false;
		}
		else
		{
			//ALERT(at_console, "turned on %i\n", lightstyle);
			LIGHT_STYLE(lightstyle, "a");
			lightstyletoggle = true;
		}
	}

	if(m_sounds == 5)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise1), 1, ATTN_NORM);

	if (pev->max_health)
	{
		pev->takedamage = DAMAGE_YES;
		pev->health = pev->max_health;
	}	

	m_toggle_state = TS_GOING_DOWN;

	SetMoveDone( &CBaseDoor::DoorHitBottom );
	LinearMove( m_vecPosition1, pev->speed);
}

//
// Starts the door going to its "up" position (simply ToggleData->vecPosition2).
//
void CBaseDoor::DoorGoUp( void )
{
	// It could be going-down, if blocked.
	if (m_toggle_state == TS_GOING_UP)
		return; // allready going up

	if (m_toggle_state == TS_AT_TOP)
	{	
		// reset top wait time
		pev->nextthink = pev->ltime + m_flWait;
		return;
	}

	if (lightstyle)
	{
		if (lightstyletoggle)
		{
			//ALERT(at_console, "turned off %i\n", lightstyle);
			LIGHT_STYLE(lightstyle, "m");
			lightstyletoggle = false;
		}
		else
		{
			//ALERT(at_console, "turned on %i\n", lightstyle);
			LIGHT_STYLE(lightstyle, "a");
			lightstyletoggle = true;
		}
	}

	// emit door moving and stop sounds on CHAN_VOICE so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise1), 1, ATTN_NORM);

	m_toggle_state = TS_GOING_UP;
	
	SetMoveDone( &CBaseDoor::DoorHitTop );
	LinearMove(m_vecPosition2, pev->speed);

	SUB_UseTargets( this, USE_TOGGLE, 0 );
}

void CBaseDoor::Blocked( CBaseEntity *pOther )
{
	// Hurt the blocker a little.
	if ( pev->dmg )
		pOther->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );

	// if a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast
	if (m_flWait >= 0)
	{
		if (m_toggle_state == TS_GOING_DOWN)
			DoorGoUp();
		else
			DoorGoDown();
	}
}

/*QUAKED func_door_secret (0 .5 .8) ? open_once 1st_left 1st_down no_shoot always_shoot
Basic secret door. Slides back, then to the side. Angle determines direction.
wait  = # of seconds before coming back
1st_left = 1st move is left of arrow
1st_down = 1st move is down from arrow
always_shoot = even if targeted, keep shootable
t_width = override WIDTH to move back (or height if going down)
t_length = override LENGTH to move sideways
"dmg"		damage to inflict when blocked (2 default)

If a secret door has a targetname, it will only be opened by it's botton or trigger, not by damage.
"sounds"
1) medieval
2) metal
3) base
*/
class CSecretDoor : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void Blocked( CBaseEntity *pOther );

	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );

	// used to selectivly override defaults
	void Touch( CBaseEntity *pOther );

	// local functions
	void EXPORT Move1( void );
	void EXPORT Move2( void );
	void EXPORT Move3( void );
	void EXPORT Move4( void );
	void EXPORT Move5( void );
	void EXPORT Move6( void );
	void EXPORT MoveDone( void );
};

LINK_ENTITY_TO_CLASS( func_door_secret, CSecretDoor );

void CSecretDoor::Precache( void )
{
	// set the door's "in-motion" sound
	switch (m_sounds)
	{
	case 1:
		PRECACHE_SOUND ("doors/latch2.wav");
		PRECACHE_SOUND ("doors/winch2.wav");
		PRECACHE_SOUND ("doors/drclos4.wav");
		pev->noise = MAKE_STRING("doors/latch2.wav");
		pev->noise1 = MAKE_STRING("doors/winch2.wav");
		pev->noise2 = MAKE_STRING("doors/drclos4.wav");
		break;
	case 2:
		PRECACHE_SOUND ("doors/airdoor1.wav");
		PRECACHE_SOUND ("doors/airdoor2.wav");
		pev->noise = MAKE_STRING("doors/airdoor2.wav");
		pev->noise1 = MAKE_STRING("doors/airdoor1.wav");
		pev->noise2 = MAKE_STRING("doors/airdoor2.wav");
		break;
	case 3:
		PRECACHE_SOUND ("doors/basesec1.wav");
		PRECACHE_SOUND ("doors/basesec2.wav");
		pev->noise = MAKE_STRING("doors/basesec2.wav");
		pev->noise1 = MAKE_STRING("doors/basesec1.wav");
		pev->noise2 = MAKE_STRING("doors/basesec2.wav");
		break;
	default:
		PRECACHE_SOUND ("misc/null.wav");
		pev->noise = MAKE_STRING("misc/null.wav");
		pev->noise1 = MAKE_STRING("misc/null.wav");
		pev->noise2 = MAKE_STRING("misc/null.wav");
		break;
	}
}

void CSecretDoor::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "t_width"))
	{
		m_flTWidth = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "t_length"))
	{
		m_flTLength = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CSecretDoor::Spawn( void )
{
	if(!m_sounds)
		m_sounds = 3;

	Precache();

	if (!pev->dmg)
		pev->dmg = 2;
		
	// magic formula...
	pev->v_angle = pev->angles;
	pev->angles = g_vecZero;
	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	SET_MODEL( ENT(pev), STRING(pev->model) );
	UTIL_SetOrigin( pev, pev->origin );
	
	pev->speed = 50;

	if (FStringNull( pev->targetname ) || pev->spawnflags & SF_SECRET_YES_SHOOT)
	{
		pev->health = 10000;
		pev->takedamage = DAMAGE_YES;
	}

	pev->oldorigin = pev->origin;

	if (!m_flWait)
		m_flWait = 5;		// 5 seconds before closing
}

int CSecretDoor :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (!pev->takedamage)
		return 0;

	Use( this, this, USE_TOGGLE, 0 );

	return 1;
}

void CSecretDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->health = 10000;	// g-cont. no effect just in case

	// exit if still moving around...
	if( pev->origin != pev->oldorigin )
		return;

	pev->message = iStringNull; // no more message

	SUB_UseTargets( this, USE_TOGGLE, 0 ); // fire all targets / killtargets

	if( !FBitSet( pev->spawnflags, SF_SECRET_NO_SHOOT ))
		pev->takedamage = DAMAGE_NO;

	pev->velocity = g_vecZero;

	// Make a sound, wait a little...

	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING( pev->noise ), 1, ATTN_NORM);
	pev->nextthink = pev->ltime + 0.1;

	float temp = 1.0 - ( pev->spawnflags & SF_SECRET_1ST_LEFT );	// 1 or -1
	UTIL_MakeVectors( pev->v_angle );
	
	if (!m_flTWidth)
	{
		if (pev->spawnflags & SF_SECRET_1ST_DOWN)
			m_flTWidth = fabs(DotProduct( gpGlobals->v_up, pev->size ));
		else
			m_flTWidth = fabs(DotProduct( gpGlobals->v_right, pev->size ));
	}

	if (!m_flTLength)
		m_flTLength = fabs(DotProduct( gpGlobals->v_forward, pev->size ));

	if (pev->spawnflags & SF_SECRET_1ST_DOWN)
		m_vecPosition1 = pev->origin - gpGlobals->v_up * m_flTWidth;
	else
		m_vecPosition1 = pev->origin + gpGlobals->v_right * (m_flTWidth * temp);

	m_vecPosition2 = m_vecPosition1 + gpGlobals->v_forward * m_flTLength;

	SetMoveDone(&CSecretDoor::Move1 );
	LinearMove(m_vecPosition1, pev->speed);

	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING( pev->noise1 ), 1, ATTN_NORM);
}

void CSecretDoor :: Touch( CBaseEntity *pOther )
{
	// Ignore touches by anything but players
	if (!FClassnameIs(pOther->pev, "player"))
		return;

	if (pev->pain_finished > gpGlobals->time)
		return;
	pev->pain_finished = gpGlobals->time + 2.0;

	if (!FStringNull( pev->message ))
	{
		CenterPrint( pOther->pev, STRING( pev->message));
		EMIT_SOUND(ENT(pOther->pev), CHAN_BODY, "misc/talk.wav", 1, ATTN_NORM);
	}
}

// Wait after first movement...
void CSecretDoor::Move1( void )
{
	pev->nextthink = pev->ltime + 1.0;
	SetThink(&CSecretDoor::Move2 );
	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING( pev->noise2 ), 1, ATTN_NORM);
}

// Start moving sideways w/sound...
void CSecretDoor::Move2( void )
{
	SetMoveDone(&CSecretDoor::Move3 );
	LinearMove(m_vecPosition2, pev->speed);
	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING( pev->noise1 ), 1, ATTN_NORM);
}

// Wait here until time to go back...
void CSecretDoor::Move3( void )
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING( pev->noise2 ), 1, ATTN_NORM);
	if (!(pev->spawnflags & SF_SECRET_OPEN_ONCE))
	{
		pev->nextthink = pev->ltime + m_flWait;
		SetThink(&CSecretDoor::Move4 );
	}
}

// Move backward...
void CSecretDoor::Move4( void )
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING( pev->noise1 ), 1, ATTN_NORM);
	SetMoveDone(&CSecretDoor::Move5 );
	LinearMove(m_vecPosition1, pev->speed);
}

// Wait 1 second...
void CSecretDoor::Move5( void )
{
	pev->nextthink = pev->ltime + 1.0;
	SetThink(&CSecretDoor::Move6 );
	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING( pev->noise2 ), 1, ATTN_NORM);
}

void CSecretDoor::Move6( void )
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING( pev->noise1 ), 1, ATTN_NORM);
	SetMoveDone(&CSecretDoor::MoveDone );
	LinearMove(pev->oldorigin, pev->speed);
}

void CSecretDoor::MoveDone( void )
{
	if (FStringNull( pev->targetname ) || pev->spawnflags & SF_SECRET_YES_SHOOT)
	{
		pev->health = 10000;
		pev->takedamage = DAMAGE_YES;
	}
	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING( pev->noise2 ), 1, ATTN_NORM);
}

void CSecretDoor::Blocked( CBaseEntity *pOther )
{
	if (pev->pain_finished > gpGlobals->time)
		return;
	pev->pain_finished = gpGlobals->time + 0.5;

	// Hurt the blocker a little.
	if ( pev->dmg )
		pOther->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );
}