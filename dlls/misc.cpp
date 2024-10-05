
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "shake.h"
#include "decals.h"
#include "doors.h"

extern DLL_GLOBAL BOOL		g_fXashEngine;

/*QUAKED misc_fireball (0 .5 .8) (-8 -8 -8) (8 8 8)
Lava Balls
*/
class CFireBallSource : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	Think( void );
};

class CFireBall : public CBaseEntity
{
public:
	void EXPORT LavaTouch( CBaseEntity *pOther )
	{
		if( pev->waterlevel > 0 )
		{
			UTIL_Remove( this );
			return;
		}

		pOther->TakeDamage( pev, pev, 20, DMG_BURN );

		TraceResult tr = UTIL_GetGlobalTrace();

		if( pOther->pev->solid != SOLID_BSP )
		{
			UTIL_Remove( this );
			return;
		}

		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 255;
		pev->pain_finished = gpGlobals->time;

		pev->vuser1 = tr.vecPlaneNormal * -1;
		UTIL_DecalTrace( &tr, DECAL_SCORCH1 + RANDOM_LONG( 0, 1 ));

		SetThink(&CFireBall::DieThink);
		SetTouch( NULL );

		pev->renderfx = kRenderLavaDeform;
		pev->movetype = MOVETYPE_NONE;

		pev->nextthink = gpGlobals->time + 0.001;
		pev->animtime = gpGlobals->time + 0.1;
	}

	void EXPORT DieThink( void )
	{
		float flDegree = (gpGlobals->time - pev->pain_finished) / 1.0f;

		pev->renderamt = 255 - (255 * flDegree);
		pev->nextthink = gpGlobals->time + 0.001;

		if( pev->renderamt <= 200 ) pev->effects &= ~EF_FULLBRIGHT;

		if( pev->renderamt <= 0 ) UTIL_Remove( this );
	}
};

LINK_ENTITY_TO_CLASS( misc_fireball, CFireBallSource );
LINK_ENTITY_TO_CLASS( fireball, CFireBall );

void CFireBallSource :: Precache( void )
{
	PRECACHE_MODEL ("models/lavaball.mdl");
}

void CFireBallSource :: Spawn( void )
{
	Precache ();

	if (!pev->speed)
		pev->speed = 1000;
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1, 5.0 );
}

void CFireBallSource :: Think( void )
{
	CFireBall *pFireBall = GetClassPtr( (CFireBall *)NULL );

	pFireBall->pev->solid = SOLID_TRIGGER;
	pFireBall->pev->movetype = MOVETYPE_TOSS;
	pFireBall->pev->velocity.x = RANDOM_FLOAT( -50, 50 );
	pFireBall->pev->velocity.y = RANDOM_FLOAT( -50, 50 );
	pFireBall->pev->velocity.z = pev->speed + RANDOM_FLOAT( 0, 200 );
	pFireBall->pev->classname = MAKE_STRING( "fireball" );
	pFireBall->pev->avelocity.x = RANDOM_FLOAT( -50, 50 );
	pFireBall->pev->avelocity.y = RANDOM_FLOAT( -50, 50 );
	pFireBall->pev->avelocity.z = RANDOM_FLOAT( -50, 50 );
	pFireBall->SetTouch( &CFireBall::LavaTouch );
	pFireBall->pev->vuser1 = Vector( 1, 1, 1 );

	if( g_fXashEngine )
		pFireBall->pev->effects = EF_FULLBRIGHT; // NOTE: this has effect only in Xash3D

	SET_MODEL( ENT(pFireBall->pev), "models/lavaball.mdl" );
	UTIL_SetOrigin( pFireBall->pev, pev->origin );

	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 3, 8 );
}

/*QUAKED misc_explobox (0 .5 .8) (0 0 0) (32 32 64)
TESTING THING
*/
class CExploBox : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void EXPORT FallInit( void );
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	virtual void Killed( entvars_t *pevAttacker, int iGib );

	int	m_idShard;
};

LINK_ENTITY_TO_CLASS( misc_explobox, CExploBox );
LINK_ENTITY_TO_CLASS( misc_explobox2, CExploBox );

void CExploBox :: Precache( void )
{
	if( FClassnameIs( pev, "misc_explobox2" ))
		PRECACHE_MODEL ("models/b_exbox2.bsp");
	else PRECACHE_MODEL ("models/b_explob.bsp");

	m_idShard = PRECACHE_MODEL( "models/crategib.mdl" );
}

void CExploBox :: Spawn( void )
{
	Precache();

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->origin.z += 6;	// quake code

	if( FClassnameIs( pev, "misc_explobox2" ))
		SET_MODEL( ENT(pev), "models/b_exbox2.bsp" );
	else SET_MODEL( ENT(pev), "models/b_explob.bsp" );

	pev->health = 20;
	pev->takedamage = DAMAGE_AIM;

	SetThink( &CExploBox::FallInit );
	if (g_fXashEngine)
		pev->nextthink = gpGlobals->time + 0.2;
	else pev->nextthink = gpGlobals->time + 1.0; // make sure what client is changed hulls
}

void CExploBox :: FallInit( void )
{
	if (UTIL_DropToFloor(this) == 0)
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
		return;
	}

	pev->movetype = MOVETYPE_PUSHSTEP;
	UTIL_SetOrigin( pev, pev->origin );
}

int CExploBox :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
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

void CExploBox :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->takedamage = DAMAGE_NO;

	Vector vecSrc = pev->origin + Vector( 0, 0, 32 );

	Q_RadiusDamage(this, this, 160, gpWorld);

	UTIL_ScreenShake( pev->origin, 16.0f, 4.0f, 0.8f, 500.0f );

	MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
	MESSAGE_END();

	// spawn gibs
	Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_BREAKMODEL );

		// position
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z );

		// size
		WRITE_COORD( pev->size.x );
		WRITE_COORD( pev->size.y );
		WRITE_COORD( pev->size.z );

		// velocity
		WRITE_COORD( 0 ); 
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );

		// randomization
		WRITE_BYTE( 10 ); 

		// Model
		WRITE_SHORT( m_idShard );	//model id#

		// # of shards
		WRITE_BYTE( 0 );	// let client decide

		// duration
		WRITE_BYTE( 50 );	// 5 seconds

		// flags
		WRITE_BYTE( BREAK_METAL );
	MESSAGE_END();

	SetThink( &CExploBox::SUB_Remove );
	pev->nextthink = pev->nextthink + 0.1;
}

class CBubble : public CBaseEntity
{
public:
	void Touch( CBaseEntity *pOther );
	void Think( void );
	void Split( void );
	void Spawn( void );
};

void CBubble :: Spawn( void )
{
	pev->classname = MAKE_STRING( "bubble" );

	SET_MODEL(ENT(pev), "sprites/s_bubble.spr" );
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ));

	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->velocity = Vector( 0, 0, 15 );
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.5;
}

void CBubble :: Touch( CBaseEntity *pOther )
{
	if( FStrEq( STRING( pOther->pev->classname ), STRING( pev->classname )))
		return;	// touch another bubble
	UTIL_Remove( this );
}

void CBubble :: Split( void )
{
	CBubble *pBubble = GetClassPtr( (CBubble *)NULL);

	pBubble->pev->origin = pev->origin;
	pBubble->Spawn();
	pBubble->pev->velocity = pev->velocity;
	pBubble->pev->frame = 1;
	pBubble->pev->impulse = 10;

	// split one big bubble on two half-sized
	pev->frame = 1;
	pev->impulse = 10;
}

void CBubble :: Think( void )
{
	float	rnd1, rnd2, rnd3;

	pev->impulse++;

	if( pev->impulse == 4 )
		Split();

	if( pev->impulse >= 20 || pev->waterlevel != 3 )
	{
		// lifetime expired
		UTIL_Remove( this );
		return;
	}

	rnd1 = pev->velocity.x + RANDOM_FLOAT( -10, 10 );
	rnd2 = pev->velocity.y + RANDOM_FLOAT( -10, 10 );
	rnd3 = pev->velocity.z + RANDOM_FLOAT( 10, 20 );

	if( rnd1 > 10 ) rnd1 = 5;
	if( rnd1 < -10 ) rnd1 = -5;
		
	if( rnd2 > 10 ) rnd2 = 5;
	if( rnd2 < -10 ) rnd2 = -5;
		
	if( rnd3 < 10 ) rnd3 = 15;
	if( rnd3 > 30 ) rnd3 = 25;
	
	pev->velocity.x = rnd1;
	pev->velocity.y = rnd2;
	pev->velocity.z = rnd3;
		
	pev->nextthink = gpGlobals->time + 0.5;
}

LINK_ENTITY_TO_CLASS( bubble, CBubble );

/*QUAKED air_bubbles (0 .5 .8) (-8 -8 -8) (8 8 8)

testing air bubbles
*/
LINK_ENTITY_TO_CLASS( air_bubbles, CBubbleSource );

void CBubbleSource :: Spawn( void )
{
	if( g_pGameRules->IsDeathmatch())
	{
		// g-cont. how many traffic requires this pretty small bubbles???
		REMOVE_ENTITY( ENT(pev));
		return;
	}

	Precache ();

	pev->nextthink = gpGlobals->time + (pev->button) ? 0.1 : 1;
}

void CBubbleSource :: Think( void )
{
	CBubble *pBubble = GetClassPtr( (CBubble *)NULL);

	pBubble->pev->origin = pev->origin;
	pBubble->Spawn();

	if( pev->button )
	{
		if( ++pev->impulse >= pev->air_finished )
		{
			REMOVE_ENTITY( edict( ));
			return;
		}
	}

	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.5, 1.5 );
}

#define SF_SUPERSPIKE	1
#define SF_LASER		2

class CSpikeShooter : public CBaseToggle
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	Think( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

/*QUAKED trap_shooter (0 .5 .8) (-8 -8 -8) (8 8 8) superspike laser
Continuously fires spikes.
"wait" time between spike (1.0 default)
"nextthink" delay before firing first spike, so multiple shooters can be stagered.
*/
LINK_ENTITY_TO_CLASS( trap_shooter, CSpikeShooter );

/*QUAKED trap_spikeshooter (0 .5 .8) (-8 -8 -8) (8 8 8) superspike laser
When triggered, fires a spike in the direction set in QuakeEd.
Laser is only for REGISTERED.
*/
LINK_ENTITY_TO_CLASS( trap_spikeshooter, CSpikeShooter );

void CSpikeShooter::Precache( void )
{
	if (pev->spawnflags & SF_LASER)
	{
		PRECACHE_MODEL ("progs/laser.mdl");
		PRECACHE_SOUND ("enforcer/enfire.wav");
		PRECACHE_SOUND ("enforcer/enfstop.wav");
	}
}

void CSpikeShooter::Spawn( void )
{
	Precache ();

	SetMovedir(pev);

	if (FClassnameIs( pev, "trap_shooter" ))
	{
		if (m_flWait == 0.0f) m_flWait = 1;
		pev->nextthink = gpGlobals->time + m_flWait;
	}
}

void CSpikeShooter::Think( void )
{
	Use( this, this, USE_TOGGLE, 0 );
	pev->nextthink = gpGlobals->time + m_flWait;
}

void CSpikeShooter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (pev->spawnflags & SF_LASER)
	{
		CLaser *pLaser;

		EMIT_SOUND( edict(), CHAN_VOICE, "enforcer/enfire.wav", 1, ATTN_NORM );

		pLaser = CLaser::LaunchLaser( pev->origin, pev->movedir, this );
	}
	else
	{
		CNail *pNail;

		EMIT_SOUND( edict(), CHAN_VOICE, "weapons/spike2.wav", 1, ATTN_NORM );

		if (pev->spawnflags & SF_SUPERSPIKE)
			pNail = CNail::CreateSuperNail( pev->origin, pev->movedir, this );
		else pNail = CNail::CreateNail( pev->origin, pev->movedir, this );

		if (pNail) pNail->pev->velocity = pev->movedir * 500;
	}
}

class CEventLighting : public CBaseEntity
{
public:
	void	EXPORT LightingFire( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( event_lightning, CEventLighting );

void CEventLighting :: LightingFire( void )
{
	Vector	p1, p2;
	CBaseDoor	*pDoor1, *pDoor2;

	pDoor1 = (CBaseDoor *)CBaseEntity::Instance( pev->enemy );
	pDoor2 = (CBaseDoor *)CBaseEntity::Instance( pev->owner );

	if (gpGlobals->time >= pev->dmgtime)
	{	
		// done here, put the terminals back up
		pDoor1->DoorGoDown();
		pDoor2->DoorGoDown();
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink( &CEventLighting::LightingFire );

	p1 = (pDoor1->pev->mins + pDoor1->pev->maxs) * 0.5f;
	p1.z = pDoor1->pev->absmin.z + 3;// -16;

	p2 = (pDoor2->pev->mins + pDoor2->pev->maxs) * 0.5f;
	p2.z = pDoor2->pev->absmin.z + 3;// -16;

	UTIL_Sparks( p1 );
	UTIL_Sparks( p2 );

	// compensate for length of bolt
	p2 = p2 - (p2 - p1).Normalize() * 110;

	MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
		WRITE_BYTE( TE_LIGHTNING3 );
		WRITE_ENTITY( 0 );
		WRITE_COORD( p1.x );
		WRITE_COORD( p1.y );
		WRITE_COORD( p1.z );
		WRITE_COORD( p2.x );
		WRITE_COORD( p2.y );
		WRITE_COORD( p2.z );
	MESSAGE_END();
}

void CEventLighting :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (pev->dmgtime >= gpGlobals->time + 1)
		return;

	CBaseDoor	*pDoor1, *pDoor2;

	pDoor1 = (CBaseDoor *)UTIL_FindEntityByString( NULL, "target", "lightning" );
	pDoor2 = (CBaseDoor *)UTIL_FindEntityByString( pDoor1, "target", "lightning" );

	if (!pDoor1 || !pDoor2)
	{
		ALERT( at_error, "event_lightning: missing lightning targets\n" );
		return;
	}
	
	if((pDoor1->m_toggle_state != TS_AT_TOP && pDoor1->m_toggle_state != TS_AT_BOTTOM)
	|| (pDoor2->m_toggle_state != TS_AT_TOP && pDoor2->m_toggle_state != TS_AT_BOTTOM)
	|| (pDoor1->m_toggle_state != pDoor2->m_toggle_state ))
	{
		return;
	}

	// don't let the electrodes go back up until the bolt is done
	pDoor1->pev->nextthink = -1;
	pDoor2->pev->nextthink = -1;
	pev->dmgtime = gpGlobals->time + 1;

	// store door pointers into standard fields
	pev->enemy = ENT( pDoor1->pev );
	pev->owner = ENT( pDoor2->pev );

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "misc/power.wav", 1, ATTN_NORM);
	LightingFire();		

	// advance the boss pain if down
	CBaseEntity *pBoss = UTIL_FindEntityByClassname( NULL, "monster_boss" );
	if (!pBoss) return;

	// update enemy (who activated the shock)
	pBoss->m_hEnemy = pActivator;

	if (pDoor1->m_toggle_state != TS_AT_BOTTOM && pBoss->pev->health > 0)
	{
		pBoss->TakeDamage( pev, pActivator->pev, 1, DMG_SHOCK );
	}	
}