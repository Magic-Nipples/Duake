
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"


class CHellKnight : public CQuakeMonster
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MonsterHasMeleeAttack( void ) { return TRUE; }
	BOOL MonsterHasMissileAttack( void ) { return TRUE; }
	void MonsterMeleeAttack( void );
	void MonsterMissileAttack( void );

	void MonsterAttack( void );
	void MonsterKilled( entvars_t *pevAttacker, int iGib );
	void MonsterPain( CBaseEntity *pAttacker, float flDamage );
	int BloodColor( void ) { return BLOOD_COLOR_RED; }

	void MonsterSight( void );
	void MonsterIdle( void );
	void MonsterWalk( void );	
	void MonsterRun( void );

	void CheckForCharge( void );
	void CheckContinueCharge( void );

	static const char *pAttackSounds[];

	void ShootSpike( float flOffset );
	void MonsterChargeAttack( void );

	void MonsterEvents(void);
};

LINK_ENTITY_TO_CLASS( monster_hell_knight, CHellKnight );

const char *CHellKnight::pAttackSounds[] = 
{
	"knight/sword1.wav",
	"knight/sword2.wav",
};

void CHellKnight :: MonsterSight( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "hknight/sight1.wav", 1.0, ATTN_NORM );
}

void CHellKnight :: MonsterPain( CBaseEntity *pAttacker, float flDamage )
{
	if( pev->pain_finished > gpGlobals->time )
		return;

	EMIT_SOUND( edict(), CHAN_VOICE, "hknight/pain1.wav", 1.0, ATTN_NORM );

	if(( gpGlobals->time - pev->pain_finished ) > 5 )
	{	
		// allways go into pain frame if it has been a while
		m_iAIState = STATE_PAIN;
		SetActivity( ACT_SMALL_FLINCH );
		pev->pain_finished = gpGlobals->time + 1;
		return;
	}
	
	if( RANDOM_FLOAT( 0.0f, 30.0f ) > flDamage )
		return; // didn't flinch

	m_iAIState = STATE_PAIN;
	SetActivity( ACT_SMALL_FLINCH );
	pev->pain_finished = gpGlobals->time + 1;

}

void CHellKnight :: MonsterChargeAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_RANGE_ATTACK1 );
//	m_flMonsterSpeed = 0;
}

void CHellKnight :: CheckForCharge( void )
{
	// check for mad charge
	if (!m_fEnemyVisible || m_hEnemy == NULL)
		return;

	if (gpGlobals->time < m_flAttackFinished)
		return;	

	if ( fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 20)
		return;	// too much height change

	if ( (pev->origin - m_hEnemy->pev->origin).Length() < 80)
		return;	// use regular attack

	// charge		
	AttackFinished (2);
	MonsterChargeAttack ();
}

void CHellKnight :: CheckContinueCharge( void )
{
	if (gpGlobals->time > m_flAttackFinished)
	{
		AttackFinished (3);
		MonsterRun ();
		return;	// done charging
	}

	EMIT_SOUND_ARRAY_DYN( CHAN_WEAPON, pAttackSounds, ATTN_NORM ); 
}

void CHellKnight :: MonsterMissileAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_SPECIAL_ATTACK1 );
	m_flMonsterSpeed = 0;
}

void CHellKnight :: MonsterMeleeAttack( void )
{
	m_iAIState = STATE_ATTACK;
	EMIT_SOUND( edict(), CHAN_WEAPON, "hknight/slash1.wav", 1.0, ATTN_NORM );
	SetActivity( ACT_MELEE_ATTACK1 );
	m_flMonsterSpeed = 0;
}

void CHellKnight :: ShootSpike( float flOffset )
{
	Vector ang = UTIL_VecToAngles( m_hEnemy->pev->origin - pev->origin );
	ang.y += flOffset * 6;
	
	UTIL_MakeVectors( ang );

	Vector org = pev->origin + pev->mins + pev->size * 0.5f + gpGlobals->v_forward * 20;

	// set missile speed
	Vector vec = gpGlobals->v_forward.Normalize();
	vec.z = -vec.z + RANDOM_FLOAT( -0.05f, 0.05f );

	EMIT_SOUND( edict(), CHAN_WEAPON, "hknight/attack1.wav", 1.0, ATTN_NORM );
	CNail::CreateKnightSpike( org, vec, this );
}

void CHellKnight :: MonsterIdle( void )
{
	m_iAIState = STATE_IDLE;
	SetActivity( ACT_IDLE );
	m_flMonsterSpeed = 0;
}

void CHellKnight :: MonsterWalk( void )
{
	m_iAIState = STATE_WALK;
	SetActivity( ACT_WALK );
	m_flMonsterSpeed = 5;
}

void CHellKnight :: MonsterRun( void )
{
	m_iAIState = STATE_RUN;
	SetActivity( ACT_RUN );
	m_flMonsterSpeed = 18;
}

void CHellKnight :: MonsterAttack( void )
{
	if (m_Activity == ACT_SPECIAL_ATTACK1)
		AI_Face();

	if (m_Activity == ACT_MELEE_ATTACK1)
		AI_Charge(5);

	if (m_Activity == ACT_RANGE_ATTACK1)
		AI_Charge(18);

	if (m_iAIState == STATE_ATTACK && m_fSequenceFinished)
		MonsterRun();
}

void CHellKnight :: MonsterKilled( entvars_t *pevAttacker, int iGib )
{
	if( ShouldGibMonster( iGib ))
	{
		EMIT_SOUND( edict(), CHAN_VOICE, "player/udeath.wav", 1.0, ATTN_NORM );
		CGib::ThrowHead ("models/h_hellkn.mdl", pev);
		CGib::ThrowGib ("models/gib1.mdl", pev);
		CGib::ThrowGib ("models/gib2.mdl", pev);
		CGib::ThrowGib ("models/gib3.mdl", pev);
		UTIL_Remove( this );
		return;
	}

	// regular death
	EMIT_SOUND( edict(), CHAN_VOICE, "hknight/death1.wav", 1.0, ATTN_NORM );
}

//=========================================================
// Spawn
//=========================================================
void CHellKnight :: Spawn( void )
{
	if( !g_pGameRules->FAllowMonsters( ) || !g_registered )
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	Precache( );

	SET_MODEL(ENT(pev), "progs/hknight.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, -24 ), Vector( 16, 16, 40 ));

	pev->solid	= SOLID_SLIDEBOX;
	pev->movetype	= MOVETYPE_STEP;
	pev->health	= 250;

	WalkMonsterInit ();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHellKnight :: Precache()
{
	PRECACHE_MODEL( "progs/hknight.mdl" );
	PRECACHE_MODEL( "models/h_hellkn.mdl" );
	PRECACHE_MODEL( "models/k_spike.mdl" );

	PRECACHE_SOUND_ARRAY( pAttackSounds );

	PRECACHE_SOUND( "hknight/attack1.wav" );
	PRECACHE_SOUND( "hknight/death1.wav" );
	PRECACHE_SOUND( "hknight/pain1.wav" );
	PRECACHE_SOUND( "hknight/sight1.wav" );
	PRECACHE_SOUND( "hknight/hit.wav" );		// used by C code, so don't sound2
	PRECACHE_SOUND( "hknight/slash1.wav" );
	PRECACHE_SOUND( "hknight/idle.wav" );
	PRECACHE_SOUND( "hknight/grunt.wav" );
}

void CHellKnight::MonsterEvents(void)
{
	switch (m_Activity)
	{
	case ACT_MELEE_ATTACK1:
	
		if (pev->frame >= 116 && pev->frame <= 118)
			AI_Melee();

		if (pev->frame >= 129 && pev->frame <= 131)
			AI_Melee();

		if (pev->frame >= 140 && pev->frame <= 142)
			AI_Melee();

		if (pev->frame >= 147 && pev->frame <= 150)
			AI_Melee();

		break;

	case ACT_RANGE_ATTACK1:

		if (pev->frame >= 67 && pev->frame <= 70)
			AI_Melee();

		if (pev->frame >= 74 && pev->frame <= 76)
			AI_Melee();

		if (pev->frame >= 108 && pev->frame <= 110)
			AI_Melee();

		break;

	case ACT_SPECIAL_ATTACK1:
		//magic A
		if (pev->frame == 85)
			ShootSpike(0);
		if (pev->frame == 86)
			ShootSpike(0);
		else if (pev->frame == 87)
			ShootSpike(0);
		else if (pev->frame == 88)
			ShootSpike(0);

		//magic B
		if (pev->frame == 101)
		{
			ShootSpike(-2.0f);
			ShootSpike(-1.1f);
			ShootSpike(0);
			ShootSpike(1.1f);
			ShootSpike(2.0f);
		}

		//magic C
		if (pev->frame == 160)
			ShootSpike(-2.0f);
		if (pev->frame == 161)
			ShootSpike(-1.1f);
		else if (pev->frame == 162)
			ShootSpike(0);
		else if (pev->frame == 163)
			ShootSpike(1.1f);
		else if (pev->frame == 164)
			ShootSpike(2.0f);

		break;

	case ACT_IDLE:
		if (pev->frame == 1)
		{
			if (RANDOM_FLOAT(0.0f, 1.0f) < 0.1f)
				EMIT_SOUND(edict(), CHAN_VOICE, "hknight/idle.wav", 1.0, ATTN_IDLE);
		}
		break;

	}
}