
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"items.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define OGRE_DROP_BACKPACK		1
#define OGRE_IDLE_SOUND		2
#define OGRE_IDLE_SOUND2		3
#define OGRE_DRAG_SOUND		4
#define OGRE_SHOOT_GRENADE		5
#define OGRE_CHAINSAW		6
#define OGRE_CHAINSAW_SOUND		7

class COgre : public CQuakeMonster
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

	void CornerReached( void );
	void ChainSaw( float side );
	void ThrowGrenade( void );

	void MonsterIdleSound(void);
	void MonsterWalkSound(void);
	void MonsterRunSound(void);

	static const char *pIdleSounds[];
};

LINK_ENTITY_TO_CLASS( monster_ogre, COgre );
LINK_ENTITY_TO_CLASS( monster_ogre_marksman, COgre );	// UNDONE: see code in Nehahra Total Conversion

const char *COgre::pIdleSounds[] = 
{
	"ogre/ogidle.wav",
	"ogre/ogidle2.wav",
};

void COgre :: MonsterSight( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "ogre/ogwake.wav", 1.0, ATTN_NORM );
}

void COgre :: CornerReached( void )
{
	// play chainsaw drag sound
	EMIT_SOUND( edict(), CHAN_VOICE, "ogre/ogdrag.wav", 1, ATTN_IDLE );
}

void COgre :: ChainSaw( float side )
{
	if( m_hEnemy == NULL )
		return;

	if (!Q_CanDamage( m_hEnemy, this ))
		return;

	AI_Charge( 10 );

	Vector delta = m_hEnemy->pev->origin - pev->origin;

	if (delta.Length() > 100)
		return;

	float ldmg = (RANDOM_FLOAT( 0.0f, 1.0f ) + RANDOM_FLOAT( 0.0f, 1.0f ) + RANDOM_FLOAT( 0.0f, 1.0f )) * 4;
	m_hEnemy->TakeDamage (pev, pev, ldmg, DMG_GENERIC);

	if( side )
	{
		UTIL_MakeVectors( pev->angles );
		if (side == 1)
			SpawnMeatSpray (pev->origin + gpGlobals->v_forward * 16, RANDOM_FLOAT( -100.0f, 100.0f ) * gpGlobals->v_right);
		else
			SpawnMeatSpray (pev->origin + gpGlobals->v_forward * 16, side * gpGlobals->v_right);
	}
}

void COgre :: ThrowGrenade( void )
{
	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/grenade.wav", 1, ATTN_NORM );
	pev->effects |= EF_MUZZLEFLASH;

	Vector vecVelocity = (m_hEnemy->pev->origin - pev->origin).Normalize() * 600.0f;
	vecVelocity.z = 200;

	// Create the grenade
	CRocket *pRocket = CRocket::CreateGrenade( pev->origin, vecVelocity, this );
}

void COgre :: MonsterPain( CBaseEntity *pAttacker, float flDamage )
{
	if( pev->pain_finished > gpGlobals->time )
		return;

	m_iAIState = STATE_PAIN;
	SetActivity( ACT_SMALL_FLINCH );

	EMIT_SOUND( edict(), CHAN_VOICE, "ogre/ogpain1.wav", 1.0, ATTN_NORM );

	alias_sequence_t* psequence = &m_modelExtraData.sequences[pev->sequence];

	pev->pain_finished = gpGlobals->time + (float(psequence->numframes) * 0.1) + RANDOM_FLOAT( 0.5f, 1.0f );
}

void COgre :: MonsterMeleeAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_MELEE_ATTACK1 );
}

void COgre :: MonsterMissileAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_RANGE_ATTACK1 );
}

void COgre :: MonsterIdle( void )
{
	m_iAIState = STATE_IDLE;
	SetActivity( ACT_IDLE );
	m_flMonsterSpeed = 0;
}

void COgre :: MonsterWalk( void )
{
	m_iAIState = STATE_WALK;
	SetActivity( ACT_WALK );
	m_flMonsterSpeed = 5;
}

void COgre :: MonsterRun( void )
{
	m_iAIState = STATE_RUN;
	SetActivity( ACT_RUN );
	m_flMonsterSpeed = 18;
}

void COgre :: MonsterAttack( void )
{
	if( m_Activity == ACT_RANGE_ATTACK1 )
		AI_Face();

	if( m_Activity == ACT_MELEE_ATTACK1 )
		AI_Charge( 4 );

	if (m_Activity == ACT_RANGE_ATTACK1 && pev->frame == 63)
	{
		AI_Face();
		ThrowGrenade();
	}

	if (m_Activity == ACT_MELEE_ATTACK1 && (pev->frame == 34 || pev->frame == 48))
		EMIT_SOUND(edict(), CHAN_WEAPON, "ogre/ogsawatk.wav", 1.0, ATTN_NORM);

	if (m_Activity == ACT_MELEE_ATTACK1 && ((pev->frame >= 36 && pev->frame <= 40) || (pev->frame >= 43 && pev->frame <= 46)))
		ChainSaw(1);

	if (m_Activity == ACT_MELEE_ATTACK1 && (pev->frame >= 54 && pev->frame <= 57))
		ChainSaw(1);

	if( m_iAIState == STATE_ATTACK && m_fSequenceFinished )
		MonsterRun();
}

void COgre :: MonsterKilled( entvars_t *pevAttacker, int iGib )
{
	//ammo_rockets = 2;
	//CWeaponBox::DropBackpack(this, 0);

	if( ShouldGibMonster( iGib ))
	{
		EMIT_SOUND( edict(), CHAN_VOICE, "player/udeath.wav", 1.0, ATTN_NORM );
		CGib::ThrowHead ("models/h_ogre.mdl", pev);
		CGib::ThrowGib ("models/gib3.mdl", pev);
		CGib::ThrowGib ("models/gib3.mdl", pev);
		CGib::ThrowGib ("models/gib3.mdl", pev);
		UTIL_Remove( this );
		return;
	}

	// regular death
	EMIT_SOUND( edict(), CHAN_VOICE, "ogre/ogdth.wav", 1.0, ATTN_NORM );
}

//=========================================================
// Spawn
//=========================================================
void COgre :: Spawn( void )
{
	if( !g_pGameRules->FAllowMonsters( ))
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	Precache( );

	SET_MODEL(ENT(pev), "progs/ogre.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, -24 ), Vector( 32, 32, 64 ));

	pev->solid	= SOLID_SLIDEBOX;
	pev->movetype	= MOVETYPE_STEP;
	pev->health	= 200;

	WalkMonsterInit ();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COgre :: Precache()
{
	PRECACHE_MODEL( "progs/ogre.mdl" );
	PRECACHE_MODEL( "models/h_ogre.mdl" );

	PRECACHE_SOUND_ARRAY( pIdleSounds );

	PRECACHE_SOUND( "ogre/ogdrag.wav" );
	PRECACHE_SOUND( "ogre/ogdth.wav" );
	PRECACHE_SOUND( "ogre/ogpain1.wav" );
	PRECACHE_SOUND( "ogre/ogsawatk.wav" );
	PRECACHE_SOUND( "ogre/ogwake.wav" );

	PRECACHE_MODEL("progs/zom_gib.mdl");
	PRECACHE_SOUND("zombie/z_hit.wav");
	PRECACHE_SOUND("zombie/z_miss.wav");
}

void COgre::MonsterIdleSound(void)
{
	if (m_fNextIdleSound < gpGlobals->time)
	{
		if (RANDOM_FLOAT(0.0f, 1.0f) < 0.1f)
			EMIT_SOUND(edict(), CHAN_VOICE, "ogre/ogidle.wav", 1.0, ATTN_IDLE);

		m_fNextIdleSound = gpGlobals->time + RANDOM_LONG(2, 4);
	}
}

void COgre::MonsterWalkSound(void)
{
	if (m_fNextIdleSound < gpGlobals->time)
	{
		if (pev->frame == 13)
		{
			if (RANDOM_FLOAT(0.0f, 1.0f) < 0.1f)
			{
				EMIT_SOUND(edict(), CHAN_VOICE, "ogre/ogidle.wav", 1.0, ATTN_IDLE);
				m_fNextIdleSound = gpGlobals->time + 0.8f;
			}
		}

		if (pev->frame == 17)
		{
			if (RANDOM_FLOAT(0.0f, 1.0f) < 0.05f)
			{
				EMIT_SOUND(edict(), CHAN_VOICE, "ogre/ogdrag.wav", 1.0, ATTN_IDLE);
				m_fNextIdleSound = gpGlobals->time + 0.8f;
			}
		}
	}
}

void COgre::MonsterRunSound(void)
{
	if (m_fNextIdleSound < gpGlobals->time)
	{
		if (RANDOM_FLOAT(0.0f, 1.0f) < 0.1f)
			EMIT_SOUND(edict(), CHAN_VOICE, "ogre/ogidle2.wav", 1.0, ATTN_IDLE);

		m_fNextIdleSound = gpGlobals->time + RANDOM_LONG(2, 4);
	}
}