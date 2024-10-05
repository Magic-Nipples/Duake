
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"

extern int gmsgTempEntity;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define CHTHON_OUT_SOUND		1
#define CHTHON_SIGHT_SOUND		2
#define CHTHON_LAUNCH_LEFT_BALL	3
#define CHTHON_LAUNCH_RIGHT_BALL	4
#define CHTHON_DEATH_SOUND		5
#define CHTHON_DEATH_SPLASH		6

class CChthon : public CQuakeMonster
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MonsterHasMissileAttack( void ) { return TRUE; }
	void AI_Idle( void );
	void AI_Face( void );
	void AI_Walk( float flDist );
	void AI_Run_Missile( void );

	void EXPORT Awake( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void LaunchMissile( const Vector &p, int iAttachment );
	void MonsterEvents(void);
};

LINK_ENTITY_TO_CLASS( monster_boss, CChthon );

//=========================================================
// Spawn
//=========================================================
void CChthon :: Spawn( void )
{
	if( !g_pGameRules->FAllowMonsters( ))
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	Precache( );

	SET_MODEL(ENT(pev), "progs/boss.mdl"); //set model here because progs
	pev->frame = 0;
	pev->framerate = 0.0f;
	SetBits(pev->effects, EF_NODRAW); //just hide for now

	SetUse( &CChthon::Awake );
	SetThink( &CChthon::MonsterThink );

	// add one monster to stat
	gpWorld->total_monsters++;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CChthon :: Precache()
{
	PRECACHE_MODEL( "progs/boss.mdl" );
	PRECACHE_MODEL( "models/lavaball.mdl" );

	PRECACHE_SOUND( "weapons/rocket1i.wav" );
	PRECACHE_SOUND( "boss1/out1.wav" );
	PRECACHE_SOUND( "boss1/sight1.wav" );
	PRECACHE_SOUND( "misc/power.wav" );
	PRECACHE_SOUND( "boss1/throw.wav" );
	PRECACHE_SOUND( "boss1/pain.wav" );
	PRECACHE_SOUND( "boss1/death.wav" );
}

void CChthon :: Awake( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->solid	= SOLID_BBOX;
	pev->movetype	= MOVETYPE_STEP;
	pev->takedamage	= DAMAGE_NO;

	UTIL_SetSize( pev, Vector( -128, -128, -24 ), Vector( 128, 128, 256 ));

	if( g_iSkillLevel == SKILL_EASY )
		pev->health = 1;
	else
		pev->health = 3;

	m_hEnemy = pActivator;

	MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
		WRITE_BYTE( TE_LAVASPLASH );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();

	pev->yaw_speed = 20;

	SetUse( NULL );

	ClearBits(pev->effects, EF_NODRAW);
	SetBits(pev->effects, EF_FULLBRIGHT);
	m_iAIState = STATE_IDLE;
	SetActivity( ACT_USE ); // rise Chthon
	MonsterThink();
}

void CChthon :: AI_Idle( void )
{
	if( m_fSequenceFinished )
		AI_Run_Missile (); // do a first attack!
}

void CChthon :: AI_Face( void )
{
	// go for another player if multi player
	if( m_hEnemy != NULL && m_hEnemy->pev->health <= 0.0f || RANDOM_FLOAT( 0, 1.0f ) < 0.02f )
	{
		m_hEnemy = UTIL_FindEntityByClassname( m_hEnemy, "player" );
		if( m_hEnemy == NULL )
			m_hEnemy = UTIL_FindEntityByClassname( m_hEnemy, "player" );
	}

	CQuakeMonster::AI_Face();
}

void CChthon :: AI_Walk( float flDist )
{
	if (m_iAttackState == ATTACK_MISSILE)
	{
		AI_Run_Missile();
		return;
	}

	if( m_fSequenceFinished )
	{
		// just a switch between walk and attack
		if( m_hEnemy == NULL || m_hEnemy->pev->health <= 0 )
		{
			m_iAttackState = ATTACK_NONE;
			m_iAIState = STATE_WALK;
			SetActivity( ACT_WALK ); // play walk animation
		}
		else if( m_Activity == ACT_MELEE_ATTACK1 || m_Activity == ACT_SMALL_FLINCH )
		{
			m_iAIState = STATE_WALK;
			m_iAttackState = ATTACK_MISSILE;
		}
		else if( m_Activity == ACT_BIG_FLINCH )
		{
			m_iAIState = STATE_WALK;
			m_iAttackState = ATTACK_NONE;
			SetActivity( ACT_DIEVIOLENT );
		}
		else if( m_Activity == ACT_DIEVIOLENT )
		{
			m_iAIState = STATE_DEAD;
			gpWorld->killed_monsters++;

			// just an event to increase internal client counter
			MESSAGE_BEGIN( MSG_ALL, gmsgKilledMonster );
			MESSAGE_END();

			if( m_hEnemy == NULL ) m_hEnemy = gpWorld;
			SUB_UseTargets( m_hEnemy, USE_TOGGLE, 0 );
			UTIL_Remove( this );
			return;
		}
          }

	AI_Face();
}

void CChthon :: AI_Run_Missile( void )
{
	m_iAIState = STATE_WALK;
	m_iAttackState = ATTACK_NONE;	// wait for sequence ends
	SetActivity( ACT_MELEE_ATTACK1 );
}

int CChthon :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "boss1/pain.wav", 1, ATTN_NORM );

	pev->health--;

	if( pev->health > 0 )
	{
		SetActivity( ACT_SMALL_FLINCH ); 
		m_iAIState = STATE_WALK;
		m_iAttackState = ATTACK_NONE;
	}
	else
	{
		SetActivity( ACT_BIG_FLINCH ); 
		m_iAIState = STATE_WALK;
		m_iAttackState = ATTACK_NONE;
	}

	return 1;
}

void CChthon :: LaunchMissile( const Vector &p, int iAttachment )
{
	if( m_hEnemy == NULL ) return;

	Vector vecAngles = UTIL_VecToAngles( m_hEnemy->pev->origin - pev->origin );
	UTIL_MakeVectors( vecAngles );

	Vector vecSrc = pev->origin + p.x * gpGlobals->v_forward + p.y * gpGlobals->v_right + p.z * Vector( 0, 0, 1 );
	Vector vecEnd, vecDir;

	// lead the player on hard mode
	/*if (g_iSkillLevel > SKILL_MEDIUM)
	{
		float t = ( m_hEnemy->pev->origin - vecSrc ).Length() / 300.0f;
		Vector vec = m_hEnemy->pev->velocity;
		vec.z = 0;
		vecEnd = m_hEnemy->pev->origin + t * vec;
	}
	else*/
	{
		vecEnd = m_hEnemy->pev->origin;
	}

	vecDir = (vecEnd - vecSrc).Normalize();

	CNail *pFireBall = CNail::CreateNail( vecSrc, vecDir, this );

	if( pFireBall )
	{
		EMIT_SOUND( edict(), CHAN_WEAPON, "boss1/throw.wav", 1.0, ATTN_NORM );
		SET_MODEL( ENT(pFireBall->pev), "models/lavaball.mdl" );
		pFireBall->SetTouch( &CNail::ExplodeTouch ); // rocket explosion
		pFireBall->pev->avelocity = Vector( 200, 100, 300 );
		pFireBall->pev->velocity = vecDir * 300;
	}
}

void CChthon::MonsterEvents(void)
{
	switch (m_Activity)
	{
	case ACT_USE:
		if(pev->frame == 1)
			EMIT_SOUND(edict(), CHAN_WEAPON, "boss1/out1.wav", 1.0, ATTN_NORM);

		if(pev->frame == 2)
			EMIT_SOUND(edict(), CHAN_VOICE, "boss1/sight1.wav", 1.0, ATTN_NORM);

		break;

	case ACT_MELEE_ATTACK1:
		//right
		if (pev->frame == 65)
			LaunchMissile(Vector(100, 100, 200), 2);

		//left
		if (pev->frame == 75)
			LaunchMissile(Vector(100, -100, 200), 1);
		break;

	case ACT_DIEVIOLENT:
		if (pev->frame == 48)
			EMIT_SOUND(edict(), CHAN_WEAPON, "boss1/death.wav", 1.0, ATTN_NORM);

		if (pev->frame == 53)
			EMIT_SOUND(edict(), CHAN_WEAPON, "boss1/out1.wav", 1.0, ATTN_NORM);

		MESSAGE_BEGIN(MSG_BROADCAST, gmsgTempEntity);
		WRITE_BYTE(TE_LAVASPLASH);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		MESSAGE_END();

		break;
	}
}