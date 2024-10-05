/*

===== plats.cpp ========================================================

  spawn, think, and touch functions for trains, etc
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

/*QUAKED func_plat (0 .5 .8) ? PLAT_LOW_TRIGGER
speed	default 150

Plats are always drawn in the extended position, so they will light correctly.

If the plat is the target of another trigger or button, it will start out disabled in
the extended position until it is trigger, when it will lower and become a normal plat.
If the "height" key is set, that will determine the amount the plat moves, instead of
being implicitly determined by the model's height.
Set "sounds" to one of the following:
1) base fast
2) chain slow
*/
class CFuncPlat : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData* pkvd);
	void Blocked( CBaseEntity *pOther );

	void EXPORT PlatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TriggerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void EXPORT GoUp( void );
	void EXPORT GoDown( void );
	void EXPORT HitTop( void );
	void EXPORT HitBottom( void );
};
LINK_ENTITY_TO_CLASS( func_plat, CFuncPlat );

class CPlatTrigger : public CBaseEntity
{
public:
	void SpawnInsideTrigger( CFuncPlat *pPlatform );
	void Touch( CBaseEntity *pOther );
	void Precache( void );
	CFuncPlat *m_pPlatform;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( plat_trigger, CPlatTrigger );

TYPEDESCRIPTION	CPlatTrigger::m_SaveData[] = 
{
	DEFINE_FIELD( CPlatTrigger, m_pPlatform, FIELD_CLASSPTR ),
}; IMPLEMENT_SAVERESTORE( CPlatTrigger, CBaseEntity );

static void PlatSpawnInsideTrigger( CFuncPlat *pPlatform )
{
	GetClassPtr( (CPlatTrigger *)NULL)->SpawnInsideTrigger( pPlatform );
}

//
// Create a trigger entity for a platform.
//
void CPlatTrigger :: SpawnInsideTrigger( CFuncPlat *pPlatform )
{
	m_pPlatform = pPlatform;

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER;
	pev->classname = MAKE_STRING ("plat_trigger");

	// Establish the trigger field's size
	Vector vecTMin = m_pPlatform->pev->mins + Vector ( 25, 25,  0 );
	Vector vecTMax = m_pPlatform->pev->maxs - Vector ( 25, 25, -8 );
	vecTMin.z = vecTMax.z - ( m_pPlatform->m_vecPosition1.z - m_pPlatform->m_vecPosition2.z + 8 );

	if (m_pPlatform->pev->spawnflags & PLAT_LOW_TRIGGER)
		vecTMax.z = vecTMin.z + 8;

	if (m_pPlatform->pev->size.x <= 50)
	{
		vecTMin.x = (m_pPlatform->pev->mins.x + m_pPlatform->pev->maxs.x) / 2;
		vecTMax.x = vecTMin.x + 1;
	}
	if (m_pPlatform->pev->size.y <= 50)
	{
		vecTMin.y = (m_pPlatform->pev->mins.y + m_pPlatform->pev->maxs.y) / 2;
		vecTMax.y = vecTMin.y + 1;
	}

	UTIL_SetSize( pev, vecTMin, vecTMax );
}

//
// plat_trigger isn't have model, so we need restore them size here
//
void CPlatTrigger :: Precache( void )
{
	UTIL_SetSize ( pev, pev->mins, pev->maxs );
}

//
// When the platform's trigger field is touched, the platform ???
//
void CPlatTrigger :: Touch( CBaseEntity *pOther )
{
	// Ignore touches by non-players
	if (!FClassnameIs(pOther->pev, "player"))
		return;

	// Ignore touches by corpses
	if (!pOther->IsAlive())
		return;
	
	if (m_pPlatform->m_toggle_state == TS_AT_BOTTOM)
		m_pPlatform->GoUp();
	else if (m_pPlatform->m_toggle_state == TS_AT_TOP)
		m_pPlatform->pev->nextthink = m_pPlatform->pev->ltime + 1;// delay going down
}

void CFuncPlat :: Precache( void )
{
	switch (m_sounds)
	{
	case 1:
		PRECACHE_SOUND ("plats/plat1.wav");
		PRECACHE_SOUND ("plats/plat2.wav");
		pev->noise = MAKE_STRING("plats/plat1.wav");
		pev->noise1 = MAKE_STRING("plats/plat2.wav");
		break;
	case 2:
		PRECACHE_SOUND ("plats/medplat1.wav");
		PRECACHE_SOUND ("plats/medplat2.wav");
		pev->noise = MAKE_STRING("plats/medplat1.wav");
		pev->noise1 = MAKE_STRING("plats/medplat2.wav");
		break;
	}
}

void CFuncPlat :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_flHeight = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CFuncPlat :: Spawn( void )
{
	if (m_flTLength == 0)
		m_flTLength = 80;
	if (m_flTWidth == 0)
		m_flTWidth = 10;

	if (!m_sounds)
		m_sounds = 2;

	Precache();

	pev->angles = g_vecZero;
	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), STRING(pev->model) );

	if (pev->speed == 0)
		pev->speed = 150;

	// vecPosition1 is the top position, vecPosition2 is the bottom
	m_vecPosition1 = pev->origin;
	m_vecPosition2 = pev->origin;

	if (m_flHeight != 0)
		m_vecPosition2.z = pev->origin.z - m_flHeight;
	else
		m_vecPosition2.z = pev->origin.z - pev->size.z + 8;

	SetUse( &CFuncPlat::TriggerUse );

	PlatSpawnInsideTrigger( this );

	if ( !FStringNull(pev->targetname) )
	{
		m_toggle_state = TS_GOING_UP;
		SetUse( &CFuncPlat::PlatUse );
	}
	else
	{
		UTIL_SetOrigin (pev, m_vecPosition2);
		m_toggle_state = TS_AT_BOTTOM;
	}
}

//
// Used by SUB_UseTargets, when a platform is the target of a button.
// Start bringing platform down.
//
void CFuncPlat :: PlatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetUse( NULL );

	if (m_toggle_state != TS_GOING_UP)
		ALERT( at_error, "PlatUse: not in up state\n" );

	GoDown();
}

void CFuncPlat :: TriggerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_pfnThink)
		return;	// allready activated
	GoDown();
}

//
// Platform is at top, now starts moving down.
//
void CFuncPlat :: GoDown( void )
{
	if(pev->noise)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);

	ASSERT(m_toggle_state == TS_AT_TOP || m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_GOING_DOWN;
	SetMoveDone( &CFuncPlat::HitBottom );
	LinearMove( m_vecPosition2, pev->speed );
}

//
// Platform is at bottom, now starts moving up
//
void CFuncPlat :: GoUp( void )
{
	if (pev->noise)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);
	
	ASSERT(m_toggle_state == TS_AT_BOTTOM || m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_GOING_UP;
	SetMoveDone( &CFuncPlat::HitTop );
	LinearMove( m_vecPosition1, pev->speed );
}

//
// Platform has hit top.  Pauses, then starts back down again.
//
void CFuncPlat :: HitTop( void )
{
	if(pev->noise)
		STOP_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise));

	if (pev->noise1)
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, (char*)STRING(pev->noise1), 1, ATTN_NORM);
	
	ASSERT(m_toggle_state == TS_GOING_UP);
	m_toggle_state = TS_AT_TOP;

	// After a delay, the platform will automatically start going down again.
	SetThink( &CFuncPlat::GoDown );
	pev->nextthink = pev->ltime + 3.0;
}

//
// Platform has hit bottom.  Stops and waits forever.
//
void CFuncPlat :: HitBottom( void )
{
	if(pev->noise)
		STOP_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise));

	if (pev->noise1)
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, (char*)STRING(pev->noise1), 1, ATTN_NORM);

	ASSERT(m_toggle_state == TS_GOING_DOWN);
	m_toggle_state = TS_AT_BOTTOM;
}

void CFuncPlat :: Blocked( CBaseEntity *pOther )
{
	pOther->TakeDamage(pev, pev, 1, DMG_CRUSH);

	if(pev->noise)
		STOP_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise));
	
	// Send the platform back where it came from
	ASSERT(m_toggle_state == TS_GOING_UP || m_toggle_state == TS_GOING_DOWN);

	if (m_toggle_state == TS_GOING_UP)
		GoDown();
	else if (m_toggle_state == TS_GOING_DOWN)
		GoUp ();
}

/*QUAKED func_train (0 .5 .8) ?
Trains are moving platforms that players can ride.
The targets origin specifies the min point of the train at each corner.
The train spawns at the first target it is pointing at.
If the train is the target of a button or trigger, it will not begin moving until activated.
speed	default 100
dmg		default	2
sounds
1) ratchet metal

*/
class CFuncTrain : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	void Blocked( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void EXPORT Wait( void );
	void EXPORT Next( void );
	void EXPORT Find( void );
};

LINK_ENTITY_TO_CLASS( func_train, CFuncTrain );

void CFuncTrain::Precache( void )
{
	switch (m_sounds)
	{
	case 0:
		PRECACHE_SOUND ("misc/null.wav");
		pev->noise = MAKE_STRING("misc/null.wav");
		pev->noise1 = MAKE_STRING("misc/null.wav");
		break;
	case 1:
		PRECACHE_SOUND ("plats/train2.wav");
		PRECACHE_SOUND ("plats/train1.wav");
		pev->noise = MAKE_STRING("plats/train2.wav");
		pev->noise1 = MAKE_STRING("plats/train1.wav");
		break;
	}
}

void CFuncTrain :: Spawn( void )
{
	if (pev->speed == 0)
		pev->speed = 100;

	if ( FStringNull(pev->target) )
	{
		ALERT( at_error, "func_train without a target. Removed\n" );

		pev->nextthink = pev->ltime + 0.1;
		SetThink( &CFuncTrain::SUB_Remove );
		return; 
	}	

	if (!pev->dmg)
		pev->dmg = 2;

	Precache();

	pev->solid = SOLID_BSP;	
	pev->movetype = MOVETYPE_PUSH;

	SET_MODEL( ENT(pev), STRING(pev->model) );
	UTIL_SetSize (pev, pev->mins, pev->maxs);
	UTIL_SetOrigin(pev, pev->origin);

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	pev->nextthink = pev->ltime + 0.1;
	SetThink( &CFuncTrain::Find );
}

void CFuncTrain :: Blocked( CBaseEntity *pOther )
{
	if (pev->pain_finished > gpGlobals->time)
		return;

	pev->pain_finished = gpGlobals->time + 0.5;
	pOther->TakeDamage(pev, pev, pev->dmg, DMG_CRUSH);
}

void CFuncTrain :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_pfnThink != &CFuncTrain::Find)
		return; // already activated
	Next();
}

void CFuncTrain :: Find( void )
{
	edict_t	*pentTarg = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ));

	if( FNullEnt( pentTarg ))
	{
		ALERT( at_error, "func_train couldn't find target %s. Removed\n", STRING( pev->target ));

		pev->nextthink = pev->ltime + 0.1;
		SetThink( &CFuncTrain::SUB_Remove );
		return; 
	}
		
	pev->target = VARS( pentTarg )->target;
	UTIL_SetOrigin( pev, VARS( pentTarg )->origin - pev->mins );
		
	if ( FStringNull(pev->targetname) )
	{	// not triggered, so start immediately
		pev->nextthink = pev->ltime + 0.1;
		SetThink( &CFuncTrain::Next );
	}
}

void CFuncTrain :: Wait( void )
{
	if (m_flWait)
	{
		STOP_SOUND (ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise1));
		EMIT_SOUND (ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);
		pev->nextthink = pev->ltime + m_flWait;
	}
	else
		pev->nextthink = pev->ltime + 0.1;

	SetThink(&CFuncTrain::Next );
}

void CFuncTrain :: Next( void )
{
	CBaseEntity	*pTarg;
	
	// now find our next target
	pTarg = GetNextTarget();

	if( !pTarg )
	{
		ALERT( at_error, "func_train: no next target %s.\n", STRING( pev->target ));
		SetThink( NULL );
		return; 
	}

	pev->target = pTarg->pev->target;

	m_flWait = pTarg->GetDelay();

	EMIT_SOUND (ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise1), 1, ATTN_NORM);

	SetMoveDone( &CFuncTrain::Wait );
	LinearMove( pTarg->pev->origin - pev->mins, pev->speed );
}

class CTeleTrain : public CFuncTrain
{
public:
	void Spawn( void );
	void Precache( void );
};

LINK_ENTITY_TO_CLASS( misc_teleporttrain, CTeleTrain );

void CTeleTrain :: Precache( void )
{
	PRECACHE_SOUND ("misc/null.wav");
	pev->noise = MAKE_STRING("misc/null.wav");
	pev->noise1 = MAKE_STRING("misc/null.wav");

	PRECACHE_MODEL ("progs/teleport.mdl");
}

void CTeleTrain :: Spawn( void )
{
	Precache();

	if (pev->speed == 0)
		pev->speed = 100;

	if ( FStringNull(pev->target) )
	{
		ALERT( at_error, "misc_teleporttrain without a target. Removed\n" );

		pev->nextthink = pev->ltime + 0.1;
		SetThink( &CTeleTrain::SUB_Remove );
		return; 
	}

	pev->solid = SOLID_NOT;	
	pev->movetype = MOVETYPE_PUSH;
	pev->avelocity = Vector( 100, 200, 300 );

	SET_MODEL( ENT(pev), "progs/teleport.mdl" );
	UTIL_SetSize (pev, pev->mins, pev->maxs);
	UTIL_SetOrigin(pev, pev->origin);

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	pev->nextthink = pev->ltime + 0.1;
	SetThink(&CTeleTrain::Find );
}