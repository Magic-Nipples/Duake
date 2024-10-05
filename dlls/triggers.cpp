/*

===== triggers.cpp ========================================================

  spawn and use functions for editor-placed triggers              

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "gamerules.h"

#define SF_TRIGGER_NOMESSAGE		1
#define SF_TRIGGER_NOTOUCH		1
#define SF_PUSH_ONCE		1

extern DLL_GLOBAL BOOL		g_fGameOver;

class CBaseTrigger : public CBaseToggle
{
public:
	void KeyValue( KeyValueData *pkvd );
	void InitTrigger( void );
};

/*
================
InitTrigger
================
*/
void CBaseTrigger::InitTrigger( void )
{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if (pev->angles != g_vecZero)
		SetMovedir(pev);
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;

	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world

	if( CVAR_GET_FLOAT("showtriggers") == 0 )
		SetBits( pev->effects, EF_NODRAW );
}

void CBaseTrigger :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "count"))
	{
		// tigger_counter stuff
		pev->impulse = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities. 
If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)	secret
2)	beep beep
3)	large switch
4)
set "message" to text string
*/
class CTriggerMultiple : public CBaseTrigger
{
public:
	void Spawn( void );
	void Precache( void );
	void ActivateMultiTrigger( CBaseEntity *pActivator );

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	virtual void Killed( entvars_t *pevAttacker, int iGib );

	void EXPORT MultiTouch( CBaseEntity *pOther );
	void EXPORT MultiWaitOver( void );
};

LINK_ENTITY_TO_CLASS( trigger_multiple, CTriggerMultiple );

void CTriggerMultiple :: Precache( void )
{
	switch (m_sounds)
	{
	case 1:
		PRECACHE_SOUND ("misc/secret.wav");
		pev->noise = MAKE_STRING( "misc/secret.wav" );
		break;

	case 2:
		PRECACHE_SOUND ("misc/talk.wav");
		pev->noise = MAKE_STRING( "misc/talk.wav" );
		break;
	case 3:
		PRECACHE_SOUND ("misc/trigger1.wav");
		pev->noise = MAKE_STRING( "misc/trigger1.wav" );
		break;
	}
}

void CTriggerMultiple :: Spawn( void )
{
	Precache ();

	if (!m_flWait)
		m_flWait = 0.2;

	InitTrigger();

	if (pev->health > 0)
	{
		if (FBitSet(pev->spawnflags, SF_TRIGGER_NOTOUCH))
			ALERT(at_error, "trigger_multiple spawn: health and notouch don't make sense\n");

		pev->max_health = pev->health;
		pev->takedamage = DAMAGE_YES;
		pev->solid = SOLID_BBOX;
		UTIL_SetOrigin(pev, pev->origin);  // make sure it links into the world
	}
	else if (!FBitSet(pev->spawnflags, SF_TRIGGER_NOTOUCH))
	{
		SetTouch( &CTriggerMultiple::MultiTouch );
	}
}

void CTriggerMultiple :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	ActivateMultiTrigger( pActivator );
}

void CTriggerMultiple::Killed( entvars_t *pevAttacker, int iGib )
{
	ActivateMultiTrigger( CBaseEntity::Instance( pevAttacker ));
}

void CTriggerMultiple :: ActivateMultiTrigger( CBaseEntity *pActivator )
{
	if (pev->nextthink > gpGlobals->time)
		return;		// allready been triggered

	if (FClassnameIs(pev, "trigger_secret"))
	{
		if (!pActivator->IsPlayer( ))
			return;

		gpWorld->found_secrets++;

		// just an event to increase internal client counter
		MESSAGE_BEGIN( MSG_ALL, gmsgFoundSecret );
		MESSAGE_END();
	}

	if (!FStringNull(pev->noise))
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);

	// don't trigger again until reset
	pev->takedamage = DAMAGE_NO;

	m_hActivator = pActivator;
	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 );

	if (m_flWait > 0)
	{
		SetThink(&CTriggerMultiple::MultiWaitOver );
		pev->nextthink = gpGlobals->time + m_flWait;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch( NULL );
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(&CTriggerMultiple::SUB_Remove );
	}
}

// the wait time has passed, so set back up for another activation
void CTriggerMultiple :: MultiWaitOver( void )
{
	if (pev->max_health)
	{
		pev->health = pev->max_health;
		pev->takedamage = DAMAGE_YES;
	}

	SetThink( NULL );
}

int CTriggerMultiple::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
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

void CTriggerMultiple :: MultiTouch( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer( ))
		return;

	// if the trigger has an angles field, check player's facing direction
	if (pev->movedir != g_vecZero)
	{
		Vector vForward;
		UTIL_MakeVectorsPrivate( pOther->pev->angles, vForward, NULL, NULL );
		if ( DotProduct( vForward, pev->movedir ) < 0 )
			return; // not facing the right way
	}

	ActivateMultiTrigger( pOther );
}

/*QUAKED trigger_once (.5 .5 .5) ? notouch
Variable sized trigger. Triggers once, then removes itself.
You must set the key "target" to the name of another object in the level that has a matching
"targetname".  If "health" is set, the trigger must be killed to activate.
If notouch is set, the trigger is only fired by other entities, not by touching.
if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.
Use "360" for an angle of 0.
sounds
1)	secret
2)	beep beep
3)	large switch
4)
set "message" to text string
*/
class CTriggerOnce : public CTriggerMultiple
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_once, CTriggerOnce );

void CTriggerOnce::Spawn( void )
{
	m_flWait = -1;
	CTriggerMultiple :: Spawn();
}

/*QUAKED trigger_relay (.5 .5 .5) (-8 -8 -8) (8 8 8)
This fixed size trigger cannot be touched, it can only be fired by other events.
It can contain killtargets, targets, delays, and messages.
*/
class CTriggerRelay : public CBaseDelay
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( trigger_relay, CTriggerRelay );

void CTriggerRelay :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SUB_UseTargets( this, USE_TOGGLE, 0 );
}

/*QUAKED trigger_secret (.5 .5 .5) ?
secret counter trigger
sounds
1)	secret
2)	beep beep
3)
4)
set "message" to text string
*/
class CTriggerSecret : public CTriggerMultiple
{
public:
	void Spawn( void );
	void Precache( void );
};

LINK_ENTITY_TO_CLASS( trigger_secret, CTriggerSecret );

void CTriggerSecret :: Precache( void )
{
	switch (m_sounds)
	{
	case 1:
		PRECACHE_SOUND ("misc/secret.wav");
		pev->noise = MAKE_STRING( "misc/secret.wav" );
		break;

	case 2:
		PRECACHE_SOUND ("misc/talk.wav");
		pev->noise = MAKE_STRING( "misc/talk.wav" );
		break;
	}
}

void CTriggerSecret::Spawn( void )
{
	gpWorld->total_secrets++;

	m_flWait = -1;

	if (FStringNull( pev->message ))
		pev->message = MAKE_STRING( "You found a secret area!" );
	if (!m_sounds)
		m_sounds = 1;

	CTriggerMultiple :: Spawn();
}

/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.

If nomessage is not set, t will print "1 more.. " etc when triggered and "sequence complete" when finished.

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.
*/
class CTriggerCounter : public CTriggerMultiple
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};
LINK_ENTITY_TO_CLASS( trigger_counter, CTriggerCounter );

void CTriggerCounter :: Spawn( void )
{
	m_flWait = -1;

	if (!pev->impulse)
		pev->impulse = 2;
}

void CTriggerCounter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->impulse--;

	if (pev->impulse < 0)
		return;

	m_hActivator = pActivator;
	
	if (pev->impulse != 0)
	{
		if (m_hActivator->IsPlayer() && !FBitSet(pev->spawnflags, SF_TRIGGER_NOMESSAGE))
		{
			switch (pev->impulse)
			{
			case 1: CenterPrint( m_hActivator->pev, "Only 1 more to go..." ); break;
			case 2: CenterPrint( m_hActivator->pev, "Only 2 more to go..." ); break;
			case 3: CenterPrint( m_hActivator->pev, "Only 3 more to go..." ); break;
			default: CenterPrint( m_hActivator->pev, "There are more to go..." ); break;
			}
		}
		return;
	}

	if (m_hActivator->IsPlayer() && !FBitSet(pev->spawnflags, SF_TRIGGER_NOMESSAGE))
		CenterPrint( m_hActivator->pev, "Sequence completed!" );
	
	ActivateMultiTrigger( m_hActivator );
}

/*
==============================================================================

TELEPORT TRIGGERS

==============================================================================
*/

#define SF_TELEPORT_PLAYER_ONLY	1
#define SF_TELEPORT_SILENT		2

void CTeleFog::CreateFog( const Vector pos )
{
	CTeleFog *pFog = GetClassPtr( (CTeleFog *)NULL );
	pFog->pev->origin = pos;
	pFog->Spawn();
}

void CTeleFog::Spawn( void )
{
	UTIL_SetOrigin( pev, pev->origin + Vector(0,0,8) );
	pev->nextthink = gpGlobals->time + 0.1;
	//pev->nextthink = gpGlobals->time + 0.1;
	pev->frame = 0;
	pev->renderfx = kRenderFxNoDissipation;

	/*MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE( TE_TELEPORT );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();*/

	SET_MODEL(ENT(pev), "sprites/teleport.spr");
	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;

	pev->effects |= EF_FULLBRIGHT;

	//char	tmpstr[64];
	//sprintf(tmpstr, "misc/r_tele%i.wav", RANDOM_LONG(1, 5));
	//EMIT_SOUND(ENT(pev), CHAN_VOICE, tmpstr, 1, ATTN_NORM);
}

void CTeleFog::Think( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if(pev->frame == 0)
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "misc/teleport.wav", 1, ATTN_NORM);

	pev->frame++;

	if (pev->frame > m_maxFrame)
		UTIL_Remove( this );
}

void CTeleFrag::CreateTDeath( const Vector pos, const CBaseEntity *pOwner )
{
	CTeleFrag *pTDeath = GetClassPtr( (CTeleFrag *)NULL );

	pTDeath->pev->classname = MAKE_STRING( "teledeath" );
	pTDeath->pev->movetype = MOVETYPE_NONE;
	pTDeath->pev->solid = SOLID_TRIGGER;

	UTIL_SetSize( pTDeath->pev, pOwner->pev->mins - Vector( 1, 1, 1 ), pOwner->pev->maxs + Vector( 1, 1, 1 ));
	UTIL_SetOrigin( pTDeath->pev, pos );
	pTDeath->pev->nextthink = gpGlobals->time + 0.2;
	pTDeath->SetThink( &CTeleFrag::SUB_Remove );
	pTDeath->pev->owner = ENT( pOwner->pev );

	gpGlobals->force_retouch++;	// make sure even still objects get hit
}

void CTeleFrag::Touch( CBaseEntity *pOther )
{
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );

	if (pOther == pOwner)
		return;

	// frag anyone who teleports in on top of an invincible player
	if (pOther->IsPlayer())
	{
		//if (((CBasePlayer *)pOther)->m_flInvincibleTime > gpGlobals->time)
			//pev->classname = MAKE_STRING( "teledeath2" );

		//if (pOwner && !pOwner->IsPlayer())
		{	
			// other monsters explode themselves
			if (pOwner->pev->health)
			{
				UTIL_Remove(this);
				pOwner->TakeDamage(pOther->pev, pOther->pev, 50000, DMG_GENERIC | DMG_ALWAYSGIB);
			}
			return;
		}
		
	}

	if (pOther->pev->health)
		pOther->TakeDamage(pev, pev, 50000, DMG_GENERIC|DMG_ALWAYSGIB );
}

/*QUAKED info_teleport_destination (.5 .5 .5) (-8 -8 -8) (8 8 32)
This is the destination marker for a teleporter.  It should have a "targetname" field with the same value as a teleporter's "target" field.
*/
class CTriggerTeleport : public CBaseTrigger
{
public:
	void Spawn( void );
	void Precache( void );
	void Touch( CBaseEntity *pOther );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Activate( void );
};

LINK_ENTITY_TO_CLASS( trigger_teleport, CTriggerTeleport );

void CTriggerTeleport :: Precache( void )
{
	if (!FBitSet(pev->spawnflags, SF_TELEPORT_SILENT))
	{
		PRECACHE_SOUND( "ambience/hum1.wav" );
	}
}

void CTriggerTeleport :: Activate( void )
{
	// g-cont. at this point we have valid bmodel origin
	if (!FBitSet(pev->spawnflags, SF_TELEPORT_SILENT))
	{
		UTIL_EmitAmbientSound( ENT(pev), VecBModelOrigin( pev ), "ambience/hum1.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
	}
}

void CTriggerTeleport :: Spawn( void )
{
	Precache();

	InitTrigger();
}

void CTriggerTeleport::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->nextthink = gpGlobals->time + 0.2;
	gpGlobals->force_retouch++; // make sure even still objects get hit
	SetThink( NULL );
}

#include "shake.h"
void CTriggerTeleport :: Touch( CBaseEntity *pOther )
{
	if (!FStringNull(pev->targetname))
	{
		if (pev->nextthink < gpGlobals->time)
			return;	// not fired yet
	}

	if (FBitSet(pev->spawnflags, SF_TELEPORT_PLAYER_ONLY))
	{
		if (!pOther->IsPlayer())
			return;
	}

	// only teleport living creatures
	if (pOther->pev->health <= 0 || pOther->pev->solid != SOLID_SLIDEBOX)
		return;

	SUB_UseTargets( pOther, USE_TOGGLE, 0 );

	// put a tfog where the player was
	//CTeleFog::CreateFog( pOther->pev->origin );

	edict_t *pentTarget = NULL;
	pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING( pev->target ));

	if (FNullEnt(pentTarget))
	{
		SetTouch( NULL );
		ALERT( at_error, "trigger_teleport: couldn't find target!\n" );
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(  &CTriggerTeleport::SUB_Remove );
		return;	
	}

	CBaseEntity *pTarg = CBaseEntity::Instance( pentTarget );
		
	// spawn a tfog flash in front of the destination
	UTIL_MakeVectors( pTarg->pev->angles );
	Vector org = pTarg->pev->origin + 32 * gpGlobals->v_forward;

	if (pOther->IsPlayer())
	{
		UTIL_ScreenFade(pOther, Vector(75, 255, 0), 0.25f, 0.025f, 128, FFADE_IN);
		CTeleFog::CreateFog(org);
	}
	else
	{
		CTeleFog::CreateFog(org + gpGlobals->v_forward * -20);
	}

	CTeleFrag::CreateTDeath( pTarg->pev->origin, pOther );

	// move the player and lock him down for a little while
	if (!pOther->pev->health)
	{
		pOther->pev->origin = pTarg->pev->origin;
		pOther->pev->velocity = (gpGlobals->v_forward * pOther->pev->velocity.x) + (gpGlobals->v_forward * pOther->pev->velocity.y);
		return;
	}

	UTIL_SetOrigin( pOther->pev, pTarg->pev->origin );
	pOther->pev->angles = pTarg->pev->angles;

	if( pOther->pev->flags & FL_MONSTER )
	{
		TraceResult tr;
		UTIL_TraceHull( pTarg->pev->origin, pTarg->pev->origin, ignore_monsters, human_hull, pOther->edict(), &tr );

		if( tr.fStartSolid || tr.fAllSolid )
		{
			ALERT( at_aiconsole, "%s stuck after teleportation. Adjusting position...\n", STRING( pOther->pev->classname ));
			UTIL_SetOrigin( pOther->pev, pTarg->pev->origin - Vector( 0, 0, 27 ));
          	}
	}

	if( FClassnameIs( pTarg->pev, "misc_teleporttrain" ))
		pOther->pev->angles.z = 0;	// clear ROLL

	if (pOther->IsPlayer())
	{
		pOther->pev->fixangle = 1; // turn this way immediately
		//pOther->pev->teleport_time = gpGlobals->time + 0.7; //not needed with updated pm_shared

		//if (FBitSet(pOther->pev->flags, FL_ONGROUND))
			pOther->pev->flags &= ~FL_ONGROUND;

		pOther->pev->velocity = gpGlobals->v_forward * 300;
	}

	pOther->pev->flags &= ~FL_ONGROUND;
}

/*QUAKED info_teleport_destination (.5 .5 .5) (-8 -8 -8) (8 8 32)
This is the destination marker for a teleporter.  It should have a "targetname" field with the same value as a teleporter's "target" field.
*/
class CTeleportDest : public CBaseEntity
{
public:
	void Spawn( void ) { pev->origin.z += 27; }
};

LINK_ENTITY_TO_CLASS( info_teleport_destination, CTeleportDest );

/*
==============================================================================

trigger_setskill

==============================================================================
*/

/*QUAKED trigger_setskill (.5 .5 .5) ?
sets skill level to the value of "message".
Only used on start map.
*/
class CTriggerSkill : public CBaseTrigger
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_setskill, CTriggerSkill );

void CTriggerSkill::Spawn( void )
{
	InitTrigger ();
}

void CTriggerSkill::Touch( CBaseEntity *pOther )
{
	if (!pOther->IsPlayer())
		return;

	// g-cont. we can set strings here instead of skill value :-)
	CVAR_SET_STRING ( "skill", STRING( pev->message ));
}

/*QUAKED trigger_onlyregistered (.5 .5 .5) ?
Only fires if playing the registered version, otherwise prints the message
*/
class CTriggerOnlyRegistered : public CBaseTrigger
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_onlyregistered, CTriggerOnlyRegistered );

void CTriggerOnlyRegistered::Spawn( void )
{
	InitTrigger ();
}

void CTriggerOnlyRegistered::Touch( CBaseEntity *pOther )
{
	if (!pOther->IsPlayer())
		return;

	if (pev->pain_finished > gpGlobals->time)
		return;

	pev->pain_finished = gpGlobals->time + 2;

	if (g_registered)
	{
		pev->message = iStringNull;
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );

		SetTouch( NULL );
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(  &CTriggerOnlyRegistered::SUB_Remove );
	}
	else
	{
		if (!FStringNull( pev->message ))
		{
			CenterPrint( pOther->pev, STRING( pev->message ));
			EMIT_SOUND(ENT(pOther->pev), CHAN_VOICE, "misc/talk.wav", 1, ATTN_NORM);
		}
	}
}

/*QUAKED trigger_hurt (.5 .5 .5) ?
Any object touching this will be hurt
set dmg to damage amount
defalt dmg = 5
*/
class CTriggerHurt : public CBaseTrigger
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_hurt, CTriggerHurt );

void CTriggerHurt :: Spawn( void )
{
	InitTrigger();

	if (!pev->dmg)
		pev->dmg = 5;
}

void CTriggerHurt :: Touch( CBaseEntity *pOther )
{
	if (!pOther->pev->takedamage)
		return;

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if (g_pGameRules->IsMultiplayer( ))
	{
		if (pev->dmgtime > gpGlobals->time)
		{
			if (gpGlobals->time != pev->pain_finished)
			{
				// too early to hurt again, and not same frame with a different entity
				if (pOther->IsPlayer( ))
				{
					int playerMask = 1 << (pOther->entindex() - 1);

					// If I've already touched this player (this time), then bail out
					if (pev->impulse & playerMask)
						return;

					// Mark this player as touched
					pev->impulse |= playerMask;
				}
				else
				{
					// too early to hurt again, and not same frame with a different entity
					return;
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if ( pOther->IsPlayer() )
			{
				int playerMask = 1 << (pOther->entindex() - 1);

				// Mark this player as touched
				pev->impulse |= playerMask;
			}
		}
	}
	else
	{
		// Original code -- single player
		if (pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished)
		{
			// too early to hurt again, and not same frame with a different entity
			return;
		}
	}

	if (pev->dmg < 0)
		pOther->TakeHealth( -pev->dmg, DMG_GENERIC );
	else
		pOther->TakeDamage( pev, pev, pev->dmg, DMG_GENERIC );

	// store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5;// half second delay until this trigger can hurt toucher again
}

/*QUAKED trigger_push (.5 .5 .5) ? PUSH_ONCE
Pushes the player
*/
class CTriggerPush : public CBaseTrigger
{
public:
	void Spawn( void );
	void Precache( void );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( trigger_push, CTriggerPush );

void CTriggerPush :: Precache( void )
{
	PRECACHE_SOUND ("ambience/windfly.wav");
}

void CTriggerPush :: Spawn( void )
{
	Precache();

	InitTrigger();

	if (!pev->speed)
		pev->speed = 1000;
}

void CTriggerPush :: Touch( CBaseEntity *pOther )
{
	entvars_t* pevToucher = pOther->pev;

	if (FClassnameIs( pevToucher, "grenade" ))
		pevToucher->velocity = pev->speed * pev->movedir * 10;
	else if (pevToucher->health > 0)
	{
		pevToucher->velocity = pev->speed * pev->movedir * 10;
		if (pOther->IsPlayer())
		{
			CBasePlayer *pPlayer = (CBasePlayer *)pOther;

			if (pPlayer->m_flFlySound < gpGlobals->time)
			{
				pPlayer->m_flFlySound = gpGlobals->time + 1.5;
				EMIT_SOUND( ENT(pevToucher), CHAN_AUTO, "ambience/windfly.wav", 1, ATTN_NORM );
			}
		}
	}

	if (FBitSet( pev->spawnflags, SF_PUSH_ONCE ))
		UTIL_Remove( this );
}

/*QUAKED trigger_monsterjump (.5 .5 .5) ?
Walking monsters that touch this will jump in the direction of the trigger's angle
"speed" default to 200, the speed thrown forward
"height" default to 200, the speed thrown upwards
*/
class CTriggerMonsterJump : public CBaseTrigger
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
};

LINK_ENTITY_TO_CLASS( trigger_monsterjump, CTriggerMonsterJump );

void CTriggerMonsterJump :: Spawn ( void )
{
	if (!pev->speed)
		pev->speed = 200;
	if (!m_flHeight)
		m_flHeight = 200;
	if (pev->angles == g_vecZero)
		pev->angles = Vector( 0, 360, 0 );

	InitTrigger ();
}

void CTriggerMonsterJump :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "height"))
	{
		m_flHeight = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

void CTriggerMonsterJump :: Touch( CBaseEntity *pOther )
{
	entvars_t *pevOther = pOther->pev;

	if (pOther->IsPlayer())
		return;	// player can't use this trigger

	if ( !( pevOther->flags & FL_MONSTER) )
		return;

	// set XY even if not on ground, so the jump will clear lips
	pevOther->velocity.x = pev->movedir.x * pev->speed;
	pevOther->velocity.y = pev->movedir.y * pev->speed;
	
	if ( !(pevOther->flags & FL_ONGROUND) )
		return;
	
	pevOther->flags &= ~FL_ONGROUND;
	pevOther->velocity.z = m_flHeight;
}

// ====================== TRIGGER_CHANGELEVEL ================================
#define SF_NO_INTERMISSION		0x0001

class CChangeLevel : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT TouchChangeLevel( CBaseEntity *pOther );
	void EXPORT ExecuteChangeLevel( void );
	void ChangeLevelNow( CBaseEntity *pActivator );
	CBaseEntity *FindIntermission( void );

}; LINK_ENTITY_TO_CLASS( trigger_changelevel, CChangeLevel );

void CChangeLevel :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "map"))
	{
		pev->netname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}

/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable.
Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
*/
void CChangeLevel :: Spawn( void )
{
	if ( FStringNull( pev->netname ))
		ALERT( at_console, "a trigger_changelevel doesn't have a map" );

	InitTrigger();
	SetTouch( &CChangeLevel::TouchChangeLevel );
}

void CChangeLevel :: TouchChangeLevel( CBaseEntity *pOther )
{
	if (!FClassnameIs(pOther->pev, "player"))
		return;

	SERVER_COMMAND("killsave TEMP\n");

	if (CVAR_GET_FLOAT("noexit"))
	{
		pOther->TakeDamage( pev, pev, 50000, DMG_GENERIC );
		return;
	}

	//CLIENT_PRINTF( pOther->edict(), print_console, UTIL_VarArgs( "%s exited the level\n", STRING( pOther->pev->netname )));	
	SUB_UseTargets( pOther, USE_TOGGLE, 0 );
	m_hActivator = pOther;

	SetTouch( NULL );	// disable touch

	// move mapname into static memory
	strcpy( g_sNextMap, STRING( pev->netname ));
	gpGlobals->vecLandmarkOffset = g_vecZero;

	if (!g_pGameRules->IsMultiplayer( ))
	{
		// save changelevel parms
		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
		pPlayer->SetChangeParms();
	}

	if (FBitSet( pev->spawnflags, SF_NO_INTERMISSION ) && !g_pGameRules->IsDeathmatch( ))
	{	
		// NO_INTERMISSION
		ChangeLevelNow( pOther );
		return;
	}

	SetThink( &CChangeLevel::ExecuteChangeLevel );
	pev->nextthink = gpGlobals->time + 0.1;
}
/*
============
FindIntermission

Returns the entity to view from
============
*/
CBaseEntity *CChangeLevel :: FindIntermission( void )
{
	CBaseEntity *pSpot = NULL;

	// look for info_intermission first
	pSpot = UTIL_FindEntityByClassname( NULL, "info_intermission" );

	if (pSpot)
	{	// pick a random one
		int cycle = RANDOM_LONG( 0, 4 );

		while( cycle > 1 )
		{
			pSpot = UTIL_FindEntityByClassname( pSpot, "info_intermission" );
			if (!pSpot) pSpot = UTIL_FindEntityByClassname( NULL, "info_intermission" );

			cycle--;
		}
		return pSpot;
	}

	// then look for the start position
	pSpot = UTIL_FindEntityByClassname( NULL, "info_player_start" );
	if (pSpot)
		return pSpot;
	
	// testinfo_player_start is only found in regioned levels
	pSpot = UTIL_FindEntityByClassname( NULL, "testplayerstart" );
	if (pSpot)
		return pSpot;

	return NULL;	
}

void CChangeLevel :: ExecuteChangeLevel( void )
{
	g_intermission_running = 1;
	
	// enforce a wait time before allowing changelevel
	if (g_pGameRules->IsDeathmatch())
		g_intermission_exittime = gpGlobals->time + 5.0;
	else g_intermission_exittime = gpGlobals->time + 2.0;

	MESSAGE_BEGIN( MSG_ALL, SVC_CDTRACK );
		WRITE_BYTE( 3 );
		WRITE_BYTE( 3 );
	MESSAGE_END();

	CBaseEntity *pCamera = FindIntermission();
	CBasePlayer *pPlayer = ((CBasePlayer *)((CBaseEntity *)m_hActivator));

	if( pCamera ) SET_VIEW( pPlayer->edict(), pCamera->edict() );
//	pPlayer->EnableControl( FALSE );
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->pev->solid = SOLID_NOT;
	pPlayer->pev->movetype = MOVETYPE_NONE;
	pPlayer->pev->modelindex = 0;

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();
}

void CChangeLevel :: ChangeLevelNow( CBaseEntity *pActivator )
{
	ALERT( at_console, "CHANGE LEVEL: %s\n", g_sNextMap );
	CHANGE_LEVEL( g_sNextMap, NULL );
}


//===========================================================
//LRC - trigger_inout, a trigger which fires _only_ when
//	the player enters or leaves it.
//	If there's more than one entity it can trigger off, then
//	it will trigger for each one that enters and leaves.
//===========================================================
#ifdef WOLF3DGAME
class CTriggerSpriteColor;

class CInOutRegister : public CPointEntity
{
public:
	// returns true if found in the list
	BOOL IsRegistered(CBaseEntity* pValue);
	// remove all invalid entries from the list, trigger their targets as appropriate
	// returns the new list
	CInOutRegister* Prune(void);
	CInOutRegister* Add(CBaseEntity* pValue); // adds a new entry to the list
	BOOL IsEmpty(void) { return m_pNext ? FALSE : TRUE; };

	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CTriggerSpriteColor* m_pField;
	CInOutRegister* m_pNext;
	EHANDLE m_hValue;
};

class CTriggerSpriteColor : public CBaseTrigger
{
public:
	void Spawn(void);
	void EXPORT Touch(CBaseEntity* pOther);
	void EXPORT Think(void);
	void FireOnEntry(CBaseEntity* pOther);
	void FireOnLeaving(CBaseEntity* pOther);

	void KeyValue(KeyValueData* pkvd);

	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CInOutRegister* m_pRegister;
	int m_iszBrightness;
};

// CInOutRegister method bodies:
TYPEDESCRIPTION	CInOutRegister::m_SaveData[] =
{
	DEFINE_FIELD(CInOutRegister, m_pField, FIELD_CLASSPTR),
	DEFINE_FIELD(CInOutRegister, m_pNext, FIELD_CLASSPTR),
	DEFINE_FIELD(CInOutRegister, m_hValue, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CInOutRegister, CPointEntity);
LINK_ENTITY_TO_CLASS(inout_register, CInOutRegister);

BOOL CInOutRegister::IsRegistered(CBaseEntity* pValue)
{
	if (m_hValue == pValue)
		return TRUE;
	else if (m_pNext)
		return m_pNext->IsRegistered(pValue);
	else
		return FALSE;
}

CInOutRegister* CInOutRegister::Add(CBaseEntity* pValue)
{
	if (m_hValue == pValue)
	{
		return this; //it's already in the list, don't need to do anything
	}
	else if (m_pNext)
	{	
		m_pNext = m_pNext->Add(pValue); //keep looking
		return this;
	}
	else
	{
		// reached the end of the list; add the new entry, and trigger
		CInOutRegister* pResult = GetClassPtr((CInOutRegister*)NULL);
		pResult->m_hValue = pValue;
		pResult->m_pNext = this;
		pResult->m_pField = m_pField;
		pResult->pev->classname = MAKE_STRING("inout_register");

		//ALERT(at_console, "adding; max %.2f %.2f %.2f, min %.2f %.2f %.2f is inside max %.2f %.2f %.2f, min %.2f %.2f %.2f\n", pResult->m_hValue->pev->absmax.x, pResult->m_hValue->pev->absmax.y, pResult->m_hValue->pev->absmax.z, pResult->m_hValue->pev->absmin.x, pResult->m_hValue->pev->absmin.y, pResult->m_hValue->pev->absmin.z, pResult->m_pField->pev->absmax.x, pResult->m_pField->pev->absmax.y, pResult->m_pField->pev->absmax.z, pResult->m_pField->pev->absmin.x, pResult->m_pField->pev->absmin.y, pResult->m_pField->pev->absmin.z); //LRCT

		m_pField->FireOnEntry(pValue);
		return pResult;
	}
}

CInOutRegister* CInOutRegister::Prune(void)
{
	if (m_hValue)
	{
		ASSERTSZ(m_pNext != NULL, "invalid InOut registry terminator\n");
		if (m_pField->Intersects(m_hValue))
		{
			m_pNext = m_pNext->Prune(); // this entity is still inside the field, do nothing
			return this;
		}
		else
		{
			//ALERT(at_console, "removing; max %.2f %.2f %.2f, min %.2f %.2f %.2f is outside max %.2f %.2f %.2f, min %.2f %.2f %.2f\n", m_hValue->pev->absmax.x, m_hValue->pev->absmax.y, m_hValue->pev->absmax.z, m_hValue->pev->absmin.x, m_hValue->pev->absmin.y, m_hValue->pev->absmin.z, m_pField->pev->absmax.x, m_pField->pev->absmax.y, m_pField->pev->absmax.z, m_pField->pev->absmin.x, m_pField->pev->absmin.y, m_pField->pev->absmin.z); //LRCT

			m_pField->FireOnLeaving(m_hValue); // this entity has just left the field, trigger
			SetThink(&CInOutRegister::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.1;
			return m_pNext->Prune();
		}
	}
	else
	{
		// this register has a missing or null value
		if (m_pNext)
		{
			// this is an invalid list entry, remove it
			SetThink(&CInOutRegister::SUB_Remove);
			pev->nextthink = gpGlobals->time + 0.1;
			return m_pNext->Prune();
		}
		else
		{	
			return this; // this is the list terminator, leave it.
		}
	}
}

LINK_ENTITY_TO_CLASS(trigger_sprite_color, CTriggerSpriteColor);

TYPEDESCRIPTION	CTriggerSpriteColor::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerSpriteColor, m_pRegister, FIELD_CLASSPTR),
	DEFINE_FIELD(CTriggerSpriteColor, m_pRegister, FIELD_CLASSPTR),
	DEFINE_FIELD(CTriggerSpriteColor, m_iszBrightness, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CTriggerSpriteColor, CBaseTrigger);

void  CTriggerSpriteColor::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "m_ibrightness"))
	{
		m_iszBrightness = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue(pkvd);
}

void CTriggerSpriteColor::Spawn(void)
{
	InitTrigger();
	// create a null-terminator for the registry
	m_pRegister = GetClassPtr((CInOutRegister*)NULL);
	m_pRegister->m_hValue = NULL;
	m_pRegister->m_pNext = NULL;
	m_pRegister->m_pField = this;
	m_pRegister->pev->classname = MAKE_STRING("inout_register");
}

void CTriggerSpriteColor::Touch(CBaseEntity* pOther)
{
	m_pRegister = m_pRegister->Add(pOther);

	if (pev->nextthink <= 0 && !m_pRegister->IsEmpty())
		pev->nextthink = gpGlobals->time + 0.1;
}

void CTriggerSpriteColor::Think(void)
{
	ALERT(at_console, "THINKTIME\n");

	// Prune handles all Intersects tests and fires targets as appropriate
	m_pRegister = m_pRegister->Prune();

	if (m_pRegister->IsEmpty())
		pev->nextthink = -1;
	else
		pev->nextthink = gpGlobals->time + 0.1;
}

void CTriggerSpriteColor::FireOnEntry(CBaseEntity* pOther)
{
	pOther->pev->iuser2 = m_iszBrightness;

	if (pOther->IsPlayer())
	{
		pOther->pev->iuser4 = m_iszBrightness;
	}
	else
	{
		pOther->pev->rendermode = kRenderTransColor;
		pOther->pev->renderamt = 255;
		pOther->pev->rendercolor[0] = m_iszBrightness;
		pOther->pev->rendercolor[1] = m_iszBrightness;
		pOther->pev->rendercolor[2] = m_iszBrightness;
	}

	ALERT(at_console, "THIS ENTERED: %s\n", STRING(VARS(pOther->pev)->classname));
}

void CTriggerSpriteColor::FireOnLeaving(CBaseEntity* pEnt)
{
	ALERT(at_console, "THIS LEFT: %s\n", STRING(VARS(pEnt->pev)->classname));

	if (pEnt->pev->iuser2 != m_iszBrightness)
	{
		ALERT(at_console, "RETURNED\n");
		return;
	}

	if (pEnt->IsPlayer())
	{
		pEnt->pev->iuser4 = 255;
	}
	else
	{
		pEnt->pev->rendermode = kRenderNormal;
		pEnt->pev->renderamt = 0;
		pEnt->pev->rendercolor[0] = 0;
		pEnt->pev->rendercolor[1] = 0;
		pEnt->pev->rendercolor[2] = 0;
	}
}
#endif