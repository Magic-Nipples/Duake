
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"
#include	"weapons.h"

class CShalrath : public CQuakeMonster
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MonsterHasMeleeAttack(void) { return false; }
	BOOL MonsterHasMissileAttack( void ) { return true; }
	int BloodColor( void ) { return BLOOD_COLOR_RED; }
	void MonsterMissileAttack( void );

	void MonsterPain( CBaseEntity *pAttacker, float flDamage );
	void MonsterKilled( entvars_t *pevAttacker, int iGib );
	void MonsterSight( void );
	void CreateMissile( void );

	void MonsterIdle( void );
	void MonsterWalk( void );	
	void MonsterRun( void );
	void MonsterAttack( void );
	void MonsterEvents(void);

	static const char *pAttackSounds[];
};

LINK_ENTITY_TO_CLASS( monster_shalrath, CShalrath );

const char *CShalrath::pAttackSounds[] = 
{
	"shalrath/attack.wav",
	"shalrath/attack2.wav",
};

void CShalrath :: MonsterIdle( void )
{
	m_iAIState = STATE_IDLE;
	SetActivity(ACT_WALK);
	pev->framerate = 0.0f;
	m_flMonsterSpeed = 0;
}

void CShalrath :: MonsterWalk( void )
{
	m_iAIState = STATE_WALK;
	SetActivity( ACT_WALK );
	pev->framerate = 1.0f;
	m_flMonsterSpeed = 4;
}

void CShalrath :: MonsterRun( void )
{
	m_iAIState = STATE_RUN;
	SetActivity(ACT_WALK);
	pev->framerate = 2.0f;
	m_flMonsterSpeed = 4;
}

void CShalrath :: MonsterMissileAttack( void )
{
	// don't launch more than 5 missiles at one time
	if( pev->impulse > 5 ) return;

	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_MELEE_ATTACK1 );
}

void CShalrath :: MonsterAttack( void )
{
	if( m_iAIState == STATE_ATTACK && m_fSequenceFinished )
		MonsterRun();
}

void CShalrath :: MonsterKilled( entvars_t *pevAttacker, int iGib )
{
	if( ShouldGibMonster( iGib ))
	{
		CGib::ThrowHead ("models/h_shal.mdl", pev);
		CGib::ThrowGib ("models/gib1.mdl", pev);
		CGib::ThrowGib ("models/gib2.mdl", pev);
		CGib::ThrowGib ("models/gib3.mdl", pev);
		UTIL_Remove( this );
		return;
	}

	// regular death
	EMIT_SOUND( edict(), CHAN_VOICE, "shalrath/death.wav", 1.0, ATTN_NORM );
}

void CShalrath :: CreateMissile( void )
{
	Vector vecDir = ((m_hEnemy->pev->origin + Vector( 0, 0, 10 )) - pev->origin).Normalize();
	Vector vecSrc = pev->origin + Vector( 0, 0, 10 );

	float flDist = (m_hEnemy->pev->origin - pev->origin).Length();

	float flytime = flDist * 0.002;
	if( flytime < 0.1f ) flytime = 0.1f;

	pev->effects |= EF_MUZZLEFLASH;
	EMIT_SOUND( edict(), CHAN_WEAPON, "shalrath/attack2.wav", 1.0, ATTN_IDLE );

	CShalMissile *pMiss = CShalMissile::CreateMissile( vecSrc, vecDir * 400.0f );

	if( pMiss )
	{
		pMiss->pev->owner = edict();
		pMiss->pev->enemy = m_hEnemy->edict();
		pMiss->pev->nextthink = gpGlobals->time + flytime;
		pev->impulse++;
	}
}

void CShalrath :: MonsterSight( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "shalrath/sight.wav", 1.0, ATTN_NORM );
}

void CShalrath :: MonsterPain( CBaseEntity *pAttacker, float flDamage )
{
	if( pev->pain_finished > gpGlobals->time )
		return;

	EMIT_SOUND( edict(), CHAN_VOICE, "shalrath/pain.wav", 1.0, ATTN_NORM );

	m_iAIState = STATE_PAIN;
	SetActivity( ACT_SMALL_FLINCH );
	pev->pain_finished = gpGlobals->time + 3;
}

//=========================================================
// Spawn
//=========================================================
void CShalrath :: Spawn( void )
{
	if( !g_pGameRules->FAllowMonsters( ) || !g_registered )
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	Precache( );

	SET_MODEL(ENT(pev), "progs/shalrath.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, -24 ), Vector( 32, 32, 64 ));

	pev->solid	= SOLID_SLIDEBOX;
	pev->movetype	= MOVETYPE_STEP;
	pev->health	= 400;

	WalkMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CShalrath :: Precache()
{
	PRECACHE_MODEL( "progs/shalrath.mdl" );
	PRECACHE_MODEL( "models/h_shal.mdl" );
	PRECACHE_MODEL( "progs/v_spike.mdl" );

	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND( "shalrath/death.wav" );
	PRECACHE_SOUND( "shalrath/idle.wav" );
	PRECACHE_SOUND( "shalrath/pain.wav" );
	PRECACHE_SOUND( "shalrath/sight.wav" );
}

void CShalrath::MonsterEvents(void)
{
	if (m_iAIState == STATE_IDLE)
	{
		if (RANDOM_FLOAT(0.0f, 1.0f) < 0.05f)
			EMIT_SOUND(edict(), CHAN_VOICE, "shalrath/idle.wav", 1.0, ATTN_IDLE);
	}

	switch (m_Activity)
	{
	case ACT_MELEE_ATTACK1:
		if(pev->frame == 7)
			CreateMissile();

		if(pev->frame == 1)
			EMIT_SOUND(edict(), CHAN_VOICE, "shalrath/attack.wav", 1.0, ATTN_IDLE);

		break;
	}
}