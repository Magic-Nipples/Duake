
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"items.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"

class CFish : public CQuakeMonster
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MonsterHasMeleeAttack( void ) { return TRUE; }
	void MonsterMeleeAttack( void );
	int BloodColor( void ) { return BLOOD_COLOR_RED; }

	void MonsterPain( CBaseEntity *pAttacker, float flDamage );
	void MonsterKilled( entvars_t *pevAttacker, int iGib );

	void MonsterIdle( void );
	void MonsterWalk( void );	
	void MonsterRun( void );
	void MonsterAttack( void );
	void FishMelee( void );
	void MonsterEvents(void);
};

LINK_ENTITY_TO_CLASS( monster_fish, CFish );

void CFish :: MonsterIdle( void )
{
	m_iAIState = STATE_IDLE;
	SetActivity( ACT_SWIM );
	m_flMonsterSpeed = 0;
}

void CFish :: MonsterWalk( void )
{
	m_iAIState = STATE_WALK;
	SetActivity( ACT_SWIM );
	m_flMonsterSpeed = 8;
}

void CFish :: MonsterRun( void )
{
	m_iAIState = STATE_RUN;
	SetActivity( ACT_SWIM );
	m_flMonsterSpeed = 12;
}

void CFish :: MonsterPain( CBaseEntity *pAttacker, float flDamage )
{
	m_iAIState = STATE_PAIN;
	SetActivity( ACT_BIG_FLINCH );

	AI_Pain( 6 );
}

void CFish :: MonsterMeleeAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_MELEE_ATTACK1 );
}

void CFish :: MonsterAttack( void )
{
	AI_Charge( 10 );

	if( m_iAIState == STATE_ATTACK && m_fSequenceFinished )
		MonsterRun();
}

void CFish :: FishMelee( void )
{
	if (m_hEnemy == NULL)
		return; // removed before stroke
		
	Vector delta = m_hEnemy->pev->origin - pev->origin;

	if (delta.Length() > 72)
		return;

	EMIT_SOUND( edict(), CHAN_VOICE, "fish/bite.wav", 1.0, ATTN_NORM );		
	float ldmg = (RANDOM_FLOAT( 0.0f, 1.0f ) + RANDOM_FLOAT( 0.0f, 1.0f )) * 3;
	m_hEnemy->TakeDamage (pev, pev, ldmg, DMG_GENERIC);	
}

void CFish :: MonsterKilled( entvars_t *pevAttacker, int iGib )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "fish/death.wav", 1.0, ATTN_NORM );

	pev->flags &= ~(FL_SWIM);
	//pev->gravity = -0.08f; // underwater gravity

	// a bit of a hack. If a corpses' bbox is positioned such that being left solid so that
	// it can be attacked will block the player on a slope or stairs, the corpse is made nonsolid. 
	UTIL_SetSize ( pev, Vector ( -16, -16, -24 ), Vector ( 16, 16, 16 ) );
}

//=========================================================
// Spawn
//=========================================================
void CFish :: Spawn( void )
{
	if( !g_pGameRules->FAllowMonsters( ) || !g_registered )
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	Precache( );

	SET_MODEL(ENT(pev), "progs/fish.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, -24 ), Vector( 16, 16, 24 ));

	pev->solid	= SOLID_SLIDEBOX;
	pev->movetype	= MOVETYPE_STEP;
	pev->health	= 25;

	SwimMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFish :: Precache()
{
	PRECACHE_MODEL( "progs/fish.mdl" );

	PRECACHE_SOUND( "fish/death.wav" );
	PRECACHE_SOUND( "fish/bite.wav" );
	PRECACHE_SOUND( "fish/idle.wav" );
}

void CFish::MonsterEvents(void)
{
	switch (m_Activity)
	{
	case ACT_MELEE_ATTACK1:
		if (pev->frame == 3 || pev->frame == 8 || pev->frame == 14)
			FishMelee();

	case ACT_SWIM:
		if (pev->frame == 40)
		{
			if (m_iAIState == STATE_RUN && RANDOM_FLOAT(0.0f, 1.0f) < 0.5f)
				EMIT_SOUND(edict(), CHAN_VOICE, "fish/idle.wav", 1.0, ATTN_NORM);
		}
		break;
	}

	if (pev->health <= 0)
	{
		pev->movetype = MOVETYPE_FLY;

		pev->velocity = pev->velocity * 0.8;
		if (UTIL_PointContents(pev->origin - Vector(0,0,20)) == CONTENTS_EMPTY)
			pev->velocity.z -= 8;
		else
			pev->velocity.z += 8;
		
		if (pev->velocity.z >= 64) pev->velocity.z = 64;
		if (pev->velocity.z <= -64) pev->velocity.z = -64;
	}
}