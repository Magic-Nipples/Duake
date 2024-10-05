
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"


#define KNIGHT_START_RUNATTACK		20
#define KNIGHT_END_RUNATTACK		25
#define KNIGHT_START_ATTACK			48
#define KNIGHT_END_ATTACK			51

class CKnight : public CQuakeMonster
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL MonsterHasMeleeAttack(void) { return TRUE; }
	void MonsterMeleeAttack(void);

	void MonsterAttack(void);
	void MonsterKilled(entvars_t* pevAttacker, int iGib);
	void MonsterPain(CBaseEntity* pAttacker, float flDamage);
	int BloodColor(void) { return BLOOD_COLOR_RED; }

	void MonsterSight(void);
	void MonsterIdle(void);
	void MonsterWalk(void);
	void MonsterRun(void);
	void MonsterIdleSound(void);

	static const char* pAttackSounds[];

	int m_iInAttack;
	int m_fAttackFinished;

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( monster_knight, CKnight );

TYPEDESCRIPTION CKnight :: m_SaveData[] = 
{
	DEFINE_FIELD( CKnight, m_iInAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CKnight, m_fAttackFinished, FIELD_BOOLEAN ),
}; IMPLEMENT_SAVERESTORE( CKnight, CQuakeMonster );

const char *CKnight::pAttackSounds[] = 
{
	"knight/sword1.wav",
	"knight/sword2.wav",
};

void CKnight :: MonsterSight( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "knight/ksight.wav", 1.0, ATTN_NORM );
}

void CKnight :: MonsterPain( CBaseEntity *pAttacker, float flDamage )
{
	if( pev->pain_finished > gpGlobals->time )
		return;

	m_iAIState = STATE_PAIN;
	SetActivity( ACT_SMALL_FLINCH );

	EMIT_SOUND( edict(), CHAN_VOICE, "knight/khurt.wav", 1.0, ATTN_NORM );
	pev->pain_finished = gpGlobals->time + 1;
}

void CKnight :: MonsterMeleeAttack( void )
{
	m_iAIState = STATE_ATTACK;

	float flDist = (m_hEnemy->EyePosition() - EyePosition()).Length();

	EMIT_SOUND_ARRAY_DYN( CHAN_WEAPON, pAttackSounds, ATTN_NORM ); 

	if( flDist < 80 )
	{
		SetActivity( ACT_MELEE_ATTACK1 );
		m_flMonsterSpeed = 0;
	}
	else
	{
		SetActivity( ACT_MELEE_ATTACK2 );
		m_flMonsterSpeed = 18;
	}
}

void CKnight :: MonsterIdle( void )
{
	m_iAIState = STATE_IDLE;
	SetActivity( ACT_IDLE );
	m_flMonsterSpeed = 0;
}

void CKnight :: MonsterWalk( void )
{
	m_iAIState = STATE_WALK;
	SetActivity( ACT_WALK );
	m_flMonsterSpeed = 5;
}

void CKnight :: MonsterRun( void )
{
	m_iAIState = STATE_RUN;
	SetActivity( ACT_RUN );
	m_flMonsterSpeed = 18;
}

void CKnight :: MonsterAttack( void )
{
	if (m_fSequenceFinished)
	{
		m_fAttackFinished = TRUE;
		m_iInAttack = FALSE;
	}

	if( m_fAttackFinished )
	{
		MonsterRun();
		m_fAttackFinished = FALSE;
	}

	if (m_Activity == ACT_MELEE_ATTACK2 && (pev->frame >= KNIGHT_START_RUNATTACK && pev->frame <= KNIGHT_END_RUNATTACK))
		m_iInAttack = TRUE;
	else if (m_Activity == ACT_MELEE_ATTACK1 && (pev->frame >= KNIGHT_START_ATTACK && pev->frame <= KNIGHT_END_ATTACK))
		m_iInAttack = TRUE;
	else
		m_iInAttack = FALSE;

	if( m_flMonsterSpeed != 0 )
	{
		// we in runattack!
		if( m_iInAttack )
			AI_Melee_Side ();
		else
			AI_Charge_Side ();
	}
	else
	{
		AI_Charge( 5 );

		// standard attack
		if( m_iInAttack )
			AI_Melee();
	}
}

void CKnight :: MonsterKilled( entvars_t *pevAttacker, int iGib )
{
	if( ShouldGibMonster( iGib ))
	{
		EMIT_SOUND( edict(), CHAN_VOICE, "player/udeath.wav", 1.0, ATTN_NORM );
		CGib::ThrowHead ("models/h_knight.mdl", pev);
		CGib::ThrowGib ("models/gib1.mdl", pev);
		CGib::ThrowGib ("models/gib2.mdl", pev);
		CGib::ThrowGib ("models/gib3.mdl", pev);
		UTIL_Remove( this );
		return;
	}

	// regular death
	EMIT_SOUND( edict(), CHAN_VOICE, "knight/kdeath.wav", 1.0, ATTN_NORM );
}

//=========================================================
// Spawn
//=========================================================
void CKnight :: Spawn( void )
{
	if( !g_pGameRules->FAllowMonsters( ))
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	Precache( );

	SET_MODEL(ENT(pev), "progs/knight.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, -24 ), Vector( 16, 16, 40 ));

	pev->solid	= SOLID_SLIDEBOX;
	pev->movetype	= MOVETYPE_STEP;
	pev->health	= 75;

	WalkMonsterInit ();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CKnight :: Precache()
{
	PRECACHE_MODEL( "progs/knight.mdl" );
	PRECACHE_MODEL( "models/h_knight.mdl" );

	PRECACHE_SOUND_ARRAY( pAttackSounds );

	PRECACHE_SOUND( "knight/idle.wav" );
	PRECACHE_SOUND( "knight/ksight.wav" );
	PRECACHE_SOUND( "knight/khurt.wav" );
	PRECACHE_SOUND( "knight/kdeath.wav" );
}

void CKnight::MonsterIdleSound(void)
{
	if (m_fNextIdleSound < gpGlobals->time)
	{
		if (RANDOM_FLOAT(0.0f, 1.0f) < 0.1f)
			EMIT_SOUND(edict(), CHAN_VOICE, "knight/idle.wav", 1.0, ATTN_IDLE);

		m_fNextIdleSound = gpGlobals->time + RANDOM_LONG(2, 4);
	}
}