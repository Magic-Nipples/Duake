
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"

extern int gmsgTempEntity;

#define SF_SPAWN_CRUCIFIED	1


class CZombie : public CQuakeMonster
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MonsterHasMissileAttack( void ) { return TRUE; }
	void MonsterMissileAttack( void );

	void MonsterPain( CBaseEntity *pAttacker, float flDamage );
	int BloodColor( void ) { return BLOOD_COLOR_RED; }

	void MonsterSight( void );
	void MonsterIdle( void );
	void MonsterWalk( void );	
	void MonsterRun( void );

	virtual void AI_Idle( void );
	void ThrowMeat( int iAttachment, Vector vecOffset );

	static const char *pIdleSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];

	void EXPORT ZombieDefeated( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	void MonsterWalkSound(void);
	void MonsterRunSound(void);
	void MonsterEvents(void);
};

LINK_ENTITY_TO_CLASS( monster_zombie, CZombie );

const char *CZombie::pIdleSounds[] = 
{
	"zombie/z_idle.wav",
	"zombie/z_idle1.wav",
};

const char *CZombie::pPainSounds[] = 
{
	"zombie/z_pain.wav",
	"zombie/z_pain1.wav",
};

void CZombie :: AI_Idle( void )
{
	if (FBitSet(pev->spawnflags, SF_SPAWN_CRUCIFIED))
		return;	// stay idle

	if (FindTarget ())
		return;
	
	if (gpGlobals->time > m_flPauseTime)
	{
		MonsterWalk();
		return;
	}

	// change angle slightly
}

void CZombie :: MonsterIdle( void )
{
	m_iAIState = STATE_IDLE;
	SetActivity( ACT_IDLE );
	m_flMonsterSpeed = 0;
}

void CZombie :: MonsterWalk( void )
{
	m_iAIState = STATE_WALK;
	SetActivity( ACT_WALK );
	m_flMonsterSpeed = 1;
}

void CZombie :: MonsterRun( void )
{
	m_iAIState = STATE_RUN;
	SetActivity( ACT_RUN );
	m_flMonsterSpeed = 4;
	pev->impulse = 0;	// not in pain
	pev->frags = 0;	// not dead
}

void CZombie :: MonsterMissileAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_MELEE_ATTACK1 );
}

void CZombie :: MonsterSight( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "zombie/z_idle.wav", 1.0, ATTN_NORM );
}

void CZombie :: ThrowMeat( int iAttachment, Vector vecOffset )
{
	Vector vecOrigin = pev->origin;
	EMIT_SOUND( edict(), CHAN_WEAPON, "zombie/z_shot1.wav", 1.0, ATTN_NORM );
	CZombieMissile :: CreateMissile( vecOrigin, vecOffset, pev->angles, this );

	MonsterRun ();
}

void CZombie :: MonsterPain( CBaseEntity *pAttacker, float flDamage )
{
	pev->health = 60;		// allways reset health

	if( flDamage < 9 )
		return;		// totally ignore

	if( m_iAIState != STATE_PAIN )
	{
		// play pain sounds if not in pain
		EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pPainSounds, ATTN_NORM ); 
	}

	m_iAIState = STATE_PAIN;

	if( pev->impulse == 2 )
		return; // down on ground, so don't reset any counters

	// go down immediately if a big enough hit
	if( flDamage >= 25 )
	{
		SetActivity( ACT_BIG_FLINCH );
		pev->impulse = 2;
		AI_Pain( 2 );
		return;
	}
	
	if( pev->impulse )
	{
		// if hit again in next gre seconds while not in pain frames, definately drop
		pev->pain_finished = gpGlobals->time + 3;
		return; // currently going through an animation, don't change
	}
	
	if( pev->pain_finished > gpGlobals->time )
	{
		// hit again, so drop down
		SetActivity( ACT_BIG_FLINCH );
		pev->impulse = 2;
		AI_Pain( 2 );
		return;
	}

	// go into one of the fast pain animations	
	pev->impulse = 1;

	SetActivity( ACT_SMALL_FLINCH );
	AI_PainForward( 3 );
}

void CZombie :: Killed( entvars_t *pevAttacker, int iGib )
{
	gpWorld->killed_monsters++;

	if( m_hEnemy == NULL )
		m_hEnemy = CBaseEntity::Instance( pevAttacker );
	MonsterDeathUse( m_hEnemy, this, USE_TOGGLE, 0.0f );

	// just an event to increase internal client counter
	MESSAGE_BEGIN( MSG_ALL, gmsgKilledMonster );
	MESSAGE_END();

	EMIT_SOUND( edict(), CHAN_VOICE, "zombie/z_gib.wav", 1.0, ATTN_NORM );
	CGib::ThrowHead ("models/h_zombie.mdl", pev);
	CGib::ThrowGib ("models/gib1.mdl", pev);
	CGib::ThrowGib ("models/gib2.mdl", pev);
	CGib::ThrowGib ("models/gib3.mdl", pev);
	UTIL_Remove( this );
}

//=========================================================
// Spawn
//=========================================================
void CZombie :: Spawn( void )
{
	if( !g_pGameRules->FAllowMonsters( ))
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	Precache( );

	SET_MODEL(ENT(pev), "progs/zombie.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, -24 ), Vector( 16, 16, 40 ));

	pev->health	= 60;

	if( FBitSet( pev->spawnflags, SF_SPAWN_CRUCIFIED ))
	{
		pev->solid = SOLID_NOT;
		pev->movetype	= MOVETYPE_NONE;
		pev->takedamage	= DAMAGE_NO;
		
		pev->frame = 192; //start on right frame so you aren't in idle pose
		m_iAIState = STATE_PROP;

		SetThink( &CZombie::MonsterThink );
		pev->nextthink = gpGlobals->time + (RANDOM_LONG( 1, 10 ) * 0.1f);
	}
	else
	{
		pev->solid = SOLID_SLIDEBOX;
		pev->movetype = MOVETYPE_STEP;
		WalkMonsterInit ();
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie :: Precache()
{
	PRECACHE_MODEL( "progs/zombie.mdl" );
	PRECACHE_MODEL( "models/h_zombie.mdl" );
	PRECACHE_MODEL( "progs/zom_gib.mdl" );

	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );

	PRECACHE_SOUND( "zombie/z_fall.wav" );
	PRECACHE_SOUND( "zombie/z_miss.wav" );
	PRECACHE_SOUND( "zombie/z_hit.wav" );
	PRECACHE_SOUND( "zombie/z_shot1.wav" );
	PRECACHE_SOUND( "zombie/z_gib.wav" );
	PRECACHE_SOUND( "zombie/idle_w2.wav" );	// crucified
}

void CZombie :: ZombieDefeated( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "zombie/z_idle.wav", 1.0, ATTN_IDLE );

	pev->health = 60;
	pev->solid = SOLID_SLIDEBOX;

	//magic nipples - set collision back to normal first so move check and make sure nothing is above zombie.
	UTIL_SetSize(pev, Vector(-16, -16, -24), Vector(16, 16, 40));
	UTIL_SetOrigin(pev, pev->origin);

	if( !WALK_MOVE( ENT(pev), 0, 0, WALKMOVE_NORMAL ))
	{
		// no space to standing up (e.g. player blocked)
		UTIL_SetSize(pev, Vector(-16, -16, -24), Vector(16, 16, -16)); //pev->solid = SOLID_NOT;
		UTIL_SetOrigin(pev, pev->origin);
		pev->nextthink = gpGlobals->time + 5.0f;
	}
	else
	{
		ResetSequenceInfo( );
		SetThink( &CZombie::MonsterThink );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CZombie::MonsterWalkSound(void)
{
	if (pev->frame == 33)
	{
		if (RANDOM_FLOAT(0.0f, 1.0f) < 0.05f)
			EMIT_SOUND(edict(), CHAN_VOICE, "zombie/z_idle.wav", 1.0, ATTN_IDLE);
	}
}

void CZombie::MonsterRunSound(void)
{
	if (pev->frame == 51)
	{
		if (RANDOM_FLOAT(0.0f, 1.0f) < 0.05f)
			EMIT_SOUND(edict(), CHAN_VOICE, "zombie/z_idle.wav", 1.0, ATTN_IDLE);
	}
}

void CZombie::MonsterEvents(void)
{
	switch (m_Activity)
	{
	case ACT_MELEE_ATTACK1:

		if(pev->frame == 64 || pev->frame == 89)
			ThrowMeat(1, Vector(-10, 22, 30));
		else if (pev->frame == 77)
			ThrowMeat(2, Vector(-10, -24, 29));

		break;

	case ACT_SMALL_FLINCH:

		if (pev->frame == 110)
			if (!pev->frags)
				EMIT_SOUND(edict(), CHAN_AUTO, "zombie/z_fall.wav", 1.0, ATTN_NORM);

		break;

	case ACT_BIG_FLINCH:

		if (pev->frame == 168)
			if (!pev->frags)
				EMIT_SOUND(edict(), CHAN_AUTO, "zombie/z_fall.wav", 1.0, ATTN_NORM);


		if (pev->frame == 173)
			if (!pev->frags)
			{
				SetThink(&CZombie::ZombieDefeated);
				pev->nextthink = gpGlobals->time + 5.0f;
				StopAnimation(); // stop the animation!
				UTIL_SetSize(pev, Vector(-16, -16, -24), Vector(16, 16, -16)); //pev->solid = SOLID_NOT;
				UTIL_SetOrigin(pev, pev->origin);
				pev->frags = 1.0f;
			}

		break;

	case ACT_SLEEP:
		if (m_iAIState != STATE_IDLE)
			m_iAIState = STATE_IDLE;

		if (pev->frame == 194)
		{
			if (RANDOM_FLOAT(0.0f, 1.0f) < 0.05f)
				EMIT_SOUND(edict(), CHAN_VOICE, "zombie/idle_w2.wav", 1.0, ATTN_STATIC);

			pev->framerate = RANDOM_FLOAT(0.5f, 1.1f); // randomize animation speed
		}
		break;
	}
}