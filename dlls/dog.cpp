
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"

#define DOG_ATTACK		4

class CDog : public CQuakeMonster
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MonsterHasMeleeAttack( void ) { return TRUE; }
	BOOL MonsterHasMissileAttack( void ) { return TRUE; }
	void MonsterMeleeAttack( void );
	void MonsterMissileAttack( void );
	void MonsterIdleSound(void);

	void MonsterAttack( void );
	BOOL MonsterCheckAttack( void );
	void MonsterKilled( entvars_t *pevAttacker, int iGib );
	void MonsterPain( CBaseEntity *pAttacker, float flDamage );
	int BloodColor( void ) { return BLOOD_COLOR_RED; }

	void EXPORT JumpTouch( CBaseEntity *pOther );
	BOOL CheckMelee( void );
	BOOL CheckJump( void );

	void MonsterSight( void );
	void MonsterIdle( void );
	void MonsterWalk( void );	
	void MonsterRun( void );
	void MonsterLeap( void );
	void MonsterBite( void );

	int m_fAttackFinished;
};

LINK_ENTITY_TO_CLASS( monster_dog, CDog );

void CDog :: MonsterSight( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "dog/dsight.wav", 1.0, ATTN_NORM );
}

void CDog :: MonsterIdle( void )
{
	m_iAIState = STATE_IDLE;
	SetActivity( ACT_IDLE );
	m_flMonsterSpeed = 0;
	m_fAttackFinished = false;
}

void CDog :: MonsterWalk( void )
{
	m_iAIState = STATE_WALK;
	SetActivity( ACT_WALK );
	m_flMonsterSpeed = 7;
	m_fAttackFinished = false;
}

void CDog :: MonsterRun( void )
{
	m_iAIState = STATE_RUN;
	SetActivity( ACT_RUN );
	m_flMonsterSpeed = 32;
	m_fAttackFinished = false;
}

void CDog :: MonsterPain( CBaseEntity *pAttacker, float flDamage )
{
	m_iAIState = STATE_PAIN;
	SetActivity( ACT_SMALL_FLINCH );
	m_fAttackFinished = false;

	EMIT_SOUND( edict(), CHAN_VOICE, "dog/dpain1.wav", 1.0, ATTN_NORM );

	AI_Pain( 4 );
}

void CDog :: MonsterAttack( void )
{
	if (!m_fAttackFinished)
	{
		if (pev->impulse == ATTACK_MELEE)
		{
			if (!m_pfnTouch)
				AI_Face();

			if (pev->frame == DOG_ATTACK)
			{
				MonsterBite();
				m_fAttackFinished = true;
			}
		}
		else if (pev->impulse == ATTACK_MISSILE)
		{
			AI_Charge(10);

			MonsterLeap();

			m_fAttackFinished = true;
		}	
	}

	if( m_iAIState == STATE_ATTACK && m_fSequenceFinished )
	{
		pev->impulse = ATTACK_NONE;	// reset shadow of attack state
		MonsterRun();
		m_fAttackFinished = false;
	}
}

void CDog :: JumpTouch( CBaseEntity *pOther )
{
	if (pev->health <= 0)
		return;
		
	if (pOther->pev->takedamage)
	{
		if ( pev->velocity.Length() > 300 )
		{
			float ldmg = 10 + RANDOM_FLOAT( 0.0f, 10.0f );
			pOther->TakeDamage (pev, pev, ldmg, DMG_GENERIC);	
		}
	}

	if (!ENT_IS_ON_FLOOR( edict() ))
	{
		if (pev->flags & FL_ONGROUND)
		{
			// jump randomly to not get hung up
			SetTouch( NULL );
			m_iAIState = STATE_ATTACK;
			SetActivity(ACT_RANGE_ATTACK1);
		}
		return;	// not on ground yet
	}

	SetTouch( NULL );
	MonsterRun();
}

BOOL CDog::CheckMelee( void )
{
	if (m_iEnemyRange == RANGE_MELEE)
	{	
		// FIXME: check canreach
		m_iAttackState = ATTACK_MELEE;
		return TRUE;
	}

	return FALSE;
}

BOOL CDog::CheckJump( void )
{
	if (pev->origin.z + pev->mins.z > m_hEnemy->pev->origin.z + m_hEnemy->pev->mins.z + 0.75f * m_hEnemy->pev->size.z)
		return FALSE;
		
	if (pev->origin.z + pev->maxs.z < m_hEnemy->pev->origin.z + m_hEnemy->pev->mins.z + 0.25f * m_hEnemy->pev->size.z)
		return FALSE;
		
	Vector dist = m_hEnemy->pev->origin - pev->origin;
	dist.z = 0;
	
	float d = dist.Length();
	
	if (d < 80)
		return FALSE;
		
	if (d > 150)
		return FALSE;
		
	return TRUE;
}

BOOL CDog::MonsterCheckAttack( void )
{
	// if close enough for slashing, go for it
	if (CheckMelee ())
	{
		m_iAttackState = ATTACK_MELEE;
		pev->impulse = ATTACK_MELEE;
		return TRUE;
	}
	
	if (CheckJump ())
	{
		m_iAttackState = ATTACK_MISSILE;
		pev->impulse = ATTACK_MISSILE;
		return TRUE;
	}
	
	return FALSE;
}

void CDog :: MonsterKilled( entvars_t *pevAttacker, int iGib )
{
	if( ShouldGibMonster( iGib ))
	{
		EMIT_SOUND( edict(), CHAN_VOICE, "player/udeath.wav", 1.0, ATTN_NORM );
		CGib::ThrowHead ("models/h_dog.mdl", pev);
		CGib::ThrowGib ("models/gib3.mdl", pev);
		CGib::ThrowGib ("models/gib3.mdl", pev);
		CGib::ThrowGib ("models/gib3.mdl", pev);
		UTIL_Remove( this );
		return;
	}

	// regular death
	EMIT_SOUND( edict(), CHAN_VOICE, "dog/ddeath.wav", 1.0, ATTN_NORM );
}

void CDog :: MonsterLeap( void )
{
	AI_Face();
	
	SetTouch( &CDog::JumpTouch );
	UTIL_MakeVectors( pev->angles );

	pev->origin.z++;
	pev->velocity = gpGlobals->v_forward * 300 + Vector( 0, 0, 200 );
	pev->flags &= ~FL_ONGROUND;
}

void CDog :: MonsterBite( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "dog/dattack1.wav", 1.0, ATTN_NORM );

	if (m_hEnemy == NULL)
		return;

	AI_Charge(10);

	if (!Q_CanDamage( m_hEnemy, this ))
		return;

	Vector delta = m_hEnemy->pev->origin - pev->origin;

	if (delta.Length() > 100)
		return;
		
	float ldmg = (RANDOM_FLOAT(0, 1 ) + RANDOM_FLOAT(0, 1 ) + RANDOM_FLOAT(0, 1 )) * 8;
	m_hEnemy->TakeDamage (pev, pev, ldmg, DMG_GENERIC);	
}

void CDog :: MonsterMissileAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_RANGE_ATTACK1 );
}

void CDog :: MonsterMeleeAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_MELEE_ATTACK1 );
}

//=========================================================
// Spawn
//=========================================================
void CDog :: Spawn( void )
{
	if( !g_pGameRules->FAllowMonsters( ))
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	Precache( );

	SET_MODEL(ENT(pev), "progs/dog.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, -24 ), Vector( 32, 32, 40 ));

	pev->solid	= SOLID_SLIDEBOX;
	pev->movetype	= MOVETYPE_STEP;
	pev->health	= 25;

	m_fAttackFinished = false;

	WalkMonsterInit ();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDog :: Precache()
{
	PRECACHE_MODEL( "progs/dog.mdl" );
	PRECACHE_MODEL( "models/h_dog.mdl" );

	PRECACHE_SOUND( "dog/dattack1.wav" );
	PRECACHE_SOUND( "dog/ddeath.wav" );
	PRECACHE_SOUND( "dog/dpain1.wav" );
	PRECACHE_SOUND( "dog/dsight.wav" );
	PRECACHE_SOUND( "dog/idle.wav" );
}

void CDog::MonsterIdleSound(void)
{
	if (m_fNextIdleSound < gpGlobals->time)
	{
		if (RANDOM_FLOAT(0.0f, 1.0f) < 0.05f)
			EMIT_SOUND(edict(), CHAN_VOICE, "dog/idle.wav", 1.0, ATTN_IDLE);

		m_fNextIdleSound = gpGlobals->time + RANDOM_LONG(2, 4);
	}
}