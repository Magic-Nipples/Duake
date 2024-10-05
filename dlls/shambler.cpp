
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"

extern DLL_GLOBAL BOOL		g_fXashEngine;

#define SHAM_STATE_START_CHARGING	0
#define SHAM_STATE_CHARGING		2
#define SHAM_STATE_END_CHARGING	4

class CShambler : public CQuakeMonster
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL MonsterHasMeleeAttack(void) { return TRUE; }
	BOOL MonsterHasMissileAttack(void) { return TRUE; }
	void MonsterMeleeAttack(void);
	void MonsterMissileAttack(void);
	//void HandleAnimEvent( MonsterEvent_t *pEvent );
	void MonsterEvents(void);

	void MonsterAttack(void);
	BOOL MonsterCheckAttack(void);
	void MonsterKilled(entvars_t* pevAttacker, int iGib);
	void MonsterPain(CBaseEntity* pAttacker, float flDamage);
	int BloodColor(void) { return BLOOD_COLOR_RED; }

	void MonsterSight(void);
	void MonsterIdle(void);
	void MonsterWalk(void);
	void MonsterRun(void);

	// magic stuff
	void CastLightning(void);
	void BeginCharging(void);
	void EXPORT Charge(void);
	void EndCharging(void);

	int m_iChargeState;
	int m_iAttackCount;

	void ShamblerSmash(void);
	void ShamblerClaw(float side);

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( monster_shambler, CShambler );

class CShamLightning : public CBaseEntity
{
public:
	void Spawn( void );
	void ChangeAnim( int iAnim );
};

LINK_ENTITY_TO_CLASS( sham_lightning, CShamLightning );

TYPEDESCRIPTION CShambler :: m_SaveData[] = 
{
	DEFINE_FIELD( CShambler, m_iChargeState, FIELD_INTEGER ),
	DEFINE_FIELD( CShambler, m_iAttackCount, FIELD_INTEGER ),
}; IMPLEMENT_SAVERESTORE( CShambler, CQuakeMonster );

void CShamLightning :: Spawn( void )
{
	PRECACHE_MODEL( "progs/s_light.mdl" );

	// Safety removal
	pev->nextthink = gpGlobals->time + 0.7f;
	SetThink( &CShamLightning::SUB_Remove );

	SET_MODEL( ENT(pev), "progs/s_light.mdl" );
	UTIL_SetOrigin( pev, pev->origin );

	ChangeAnim( 0 );
}

void CShamLightning :: ChangeAnim( int iAnim )
{
	pev->sequence = iAnim;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
	pev->frame = 0.0f;
}

void CShambler :: MonsterSight( void )
{
	EMIT_SOUND( edict(), CHAN_VOICE, "shambler/ssight.wav", 1.0, ATTN_NORM );
}

void CShambler :: MonsterIdle( void )
{
	m_iAIState = STATE_IDLE;
	SetActivity( ACT_IDLE );
	m_flMonsterSpeed = 0;
}

void CShambler :: MonsterWalk( void )
{
	m_iAIState = STATE_WALK;
	SetActivity( ACT_WALK );
	m_flMonsterSpeed = 8;
}

void CShambler :: MonsterRun( void )
{
	m_iAIState = STATE_RUN;
	SetActivity( ACT_RUN );
	m_flMonsterSpeed = 20;
}

void CShambler :: MonsterMissileAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_RANGE_ATTACK1 );
}

void CShambler :: MonsterMeleeAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_MELEE_ATTACK1 );
}

BOOL CShambler::MonsterCheckAttack( void )
{
	if (m_iEnemyRange == RANGE_MELEE)
	{
		if (Q_CanDamage (m_hEnemy, this))
		{
			m_iAttackState = ATTACK_MELEE;
			return TRUE;
		}
	}

	if (gpGlobals->time < m_flAttackFinished)
		return FALSE;
	
	if (!m_fEnemyVisible)
		return FALSE;
	
	// see if any entities are in the way of the shot
	Vector spot1 = EyePosition();
	Vector spot2 = m_hEnemy->EyePosition();

	if ((spot1 - spot2).Length() > 600)
		return FALSE;

	TraceResult tr;
	UTIL_TraceLine(spot1, spot2, dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr);

	if (tr.fInOpen && tr.fInWater)
		return FALSE;		// sight line crossed contents

	if (tr.pHit != m_hEnemy->edict())
		return FALSE;	// don't have a clear shot
			
	// missile attack
	if (m_iEnemyRange == RANGE_FAR)
		return FALSE;
		
	m_iAttackState = ATTACK_MISSILE;
	AttackFinished (2 + RANDOM_FLOAT( 0.0f, 2.0f ));

	return TRUE;
}

void CShambler :: BeginCharging( void )
{
	// we already in charging
	if( m_pfnThink == &CShambler::Charge )
		return;

	StopAnimation ();
	SetThink(&CShambler::Charge );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->effects |= EF_MUZZLEFLASH;

	CShamLightning *pLightning = (CShamLightning *)CBaseEntity::Create( "sham_lightning", pev->origin, pev->angles, NULL );
	EMIT_SOUND( edict(), CHAN_WEAPON, "shambler/sattck1.wav", 1.0, ATTN_NORM );

	if( pLightning )
	{
		pev->owner = pLightning->edict();

		if( g_fXashEngine )
		{ 
			// link lightning with shambler to make sure it's will
			// not loosing on the moving platforms
			pLightning->pev->movetype = MOVETYPE_COMPOUND;	// set movewith type
			pLightning->pev->aiment = edict();		// set parent
		}
	}

	m_iChargeState = SHAM_STATE_START_CHARGING; // reset charge counter
	m_iAttackCount = 0;
}

void CShambler :: Charge( void )
{
	if( m_iChargeState == SHAM_STATE_CHARGING || m_iChargeState == SHAM_STATE_END_CHARGING )
	{
		// continue animation
		ResetSequenceInfo( );

		if( m_iChargeState == SHAM_STATE_END_CHARGING )
		{
			SetThink(&CShambler::MonsterThink );
			m_iChargeState = SHAM_STATE_START_CHARGING;
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}
	}

	pev->nextthink = gpGlobals->time + 0.1;
	pev->effects |= EF_MUZZLEFLASH;
	m_iChargeState++;
}

void CShambler :: EndCharging( void )
{
	// we not in charging
	if( m_pfnThink != &CShambler::Charge )
		return;

	if( pev->owner )
	{
		// just change lightning animation
		CShamLightning *pLightning = (CShamLightning *)CBaseEntity::Instance( pev->owner );

		if( pLightning )
		{
			pLightning->pev->nextthink = gpGlobals->time + 0.2; // remove after 0.1 secs
			pLightning->ChangeAnim( 1 ); 
		}
	}

	// #frame 4
	StopAnimation();
}

void CShambler :: CastLightning( void )
{
	if( !m_iAttackCount )
	{
		// plays a ligtning sound
		EMIT_SOUND( edict(), CHAN_WEAPON, "shambler/sboom.wav", 1.0, ATTN_NORM );
	}

	if( pev->owner )
	{
		// remove lightning (if present)
		CBaseEntity *pLightning = CBaseEntity::Instance( pev->owner );
		if( pLightning ) UTIL_Remove( pLightning );
		pev->owner = NULL;
	}

	if( m_iAttackCount > 2 && g_iSkillLevel != SKILL_NIGHTMARE )
		return;

	if (m_hEnemy == NULL)
		return;

	pev->effects |= EF_MUZZLEFLASH;
	m_iAttackCount++; // count  shoots

	AI_Face ();

	// Lightning bolt effect
	TraceResult trace;
	Vector vecOrg = pev->origin + Vector(0,0,40);

	// adjust lightning origin
	Vector vecDir = (m_hEnemy->pev->origin + Vector(0,0,16) - vecOrg).Normalize();
	UTIL_TraceLine( vecOrg, pev->origin + vecDir * 600, ignore_monsters, ENT(pev), &trace );

	MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
		WRITE_BYTE( TE_LIGHTNING1 );
		WRITE_ENTITY( entindex() );
		WRITE_COORD( vecOrg.x );
		WRITE_COORD( vecOrg.y );
		WRITE_COORD( vecOrg.z );
		WRITE_COORD( trace.vecEndPos.x );
		WRITE_COORD( trace.vecEndPos.y );
		WRITE_COORD( trace.vecEndPos.z );
	MESSAGE_END();

	// Do damage
	CBasePlayer::LightningDamage(pev->origin, trace.vecEndPos, this, 10, vecDir );
}

void CShambler :: ShamblerClaw( float side )
{
	if (m_hEnemy == NULL)
		return;

	AI_Charge(10);

	Vector delta = m_hEnemy->pev->origin - pev->origin;

	if (delta.Length() > 100)
		return;
		
	float ldmg = (RANDOM_FLOAT(0,1) + RANDOM_FLOAT(0,1) + RANDOM_FLOAT(0,1)) * 20;
	m_hEnemy->TakeDamage( pev, pev, ldmg, DMG_SLASH);
	EMIT_SOUND( edict(), CHAN_VOICE, "shambler/smack.wav", 1, ATTN_NORM);

	if (side)
	{
		UTIL_MakeVectors( pev->angles );
		SpawnMeatSpray (pev->origin + gpGlobals->v_forward * 16, side * gpGlobals->v_right);
	}
}

void CShambler :: ShamblerSmash( void )
{
	if (m_hEnemy == NULL)
		return;

	AI_Charge(0);

	Vector delta = m_hEnemy->pev->origin - pev->origin;

	if (delta.Length() > 100)
		return;

	if (!Q_CanDamage (m_hEnemy, this))
		return;
		
	float ldmg = (RANDOM_FLOAT(0,1) + RANDOM_FLOAT(0,1) + RANDOM_FLOAT(0,1)) * 40;
	m_hEnemy->TakeDamage( pev, pev, ldmg, DMG_SLASH);
	EMIT_SOUND( edict(), CHAN_VOICE, "shambler/smack.wav", 1, ATTN_NORM);

	UTIL_MakeVectors( pev->angles );

	SpawnMeatSpray (pev->origin + gpGlobals->v_forward * 16, RANDOM_FLOAT( -100, 100 ) * gpGlobals->v_right);
	SpawnMeatSpray (pev->origin + gpGlobals->v_forward * 16, RANDOM_FLOAT( -100, 100 ) * gpGlobals->v_right);
}

void CShambler :: MonsterAttack( void )
{
	AI_Charge( 5 );

	if( m_iAIState == STATE_ATTACK && m_fSequenceFinished )
		MonsterRun();
}

void CShambler :: MonsterPain( CBaseEntity *pAttacker, float flDamage )
{
	if( pev->owner )
	{
		// remove lightning (if present)
		CBaseEntity *pLightning = CBaseEntity::Instance( pev->owner );
		if( pLightning ) UTIL_Remove( pLightning );
		pev->owner = NULL;
	}

	EMIT_SOUND( edict(), CHAN_VOICE, "shambler/shurt2.wav", 1.0, ATTN_NORM );

	if( pev->health <= 0 )
		return; // allready dying, don't go into pain frame

	if (RANDOM_FLOAT( 0.0f, 1.0f ) * 400 > flDamage )
		return; // didn't flinch

	if (pev->pain_finished > gpGlobals->time)
		return;

	pev->pain_finished = gpGlobals->time + 2;

	m_iAIState = STATE_PAIN;
	SetActivity( ACT_SMALL_FLINCH );
}

void CShambler :: MonsterKilled( entvars_t *pevAttacker, int iGib )
{
	if( pev->owner )
	{
		// remove lightning (if present)
		CBaseEntity *pLightning = CBaseEntity::Instance( pev->owner );
		if( pLightning ) UTIL_Remove( pLightning );
		pev->owner = NULL;
	}

	if( ShouldGibMonster( iGib ))
	{
		EMIT_SOUND( edict(), CHAN_VOICE, "player/udeath.wav", 1.0, ATTN_NORM );
		CGib::ThrowHead ("models/h_shams.mdl", pev);
		CGib::ThrowGib ("models/gib1.mdl", pev);
		CGib::ThrowGib ("models/gib2.mdl", pev);
		CGib::ThrowGib ("models/gib3.mdl", pev);
		UTIL_Remove( this );
		return;
	}

	// regular death
	EMIT_SOUND( edict(), CHAN_VOICE, "shambler/sdeath.wav", 1.0, ATTN_NORM );
}

//=========================================================
// Spawn
//=========================================================
void CShambler :: Spawn( void )
{
	if( !g_pGameRules->FAllowMonsters( ))
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	Precache( );

	SET_MODEL(ENT(pev), "progs/shambler.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, -24 ), Vector( 32, 32, 64 ));

	pev->solid	= SOLID_SLIDEBOX;
	pev->movetype	= MOVETYPE_STEP;
	pev->health	= 600;

	WalkMonsterInit ();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CShambler :: Precache()
{
	PRECACHE_MODEL( "progs/shambler.mdl" );
	PRECACHE_MODEL( "models/h_shams.mdl" );
	PRECACHE_MODEL( "progs/s_light.mdl" );
	PRECACHE_MODEL( "progs/bolt.mdl" );

	PRECACHE_SOUND ("shambler/sattck1.wav");
	PRECACHE_SOUND ("shambler/sboom.wav");
	PRECACHE_SOUND ("shambler/sdeath.wav");
	PRECACHE_SOUND ("shambler/shurt2.wav");
	PRECACHE_SOUND ("shambler/sidle.wav");
	PRECACHE_SOUND ("shambler/ssight.wav");
	PRECACHE_SOUND ("shambler/melee1.wav");
	PRECACHE_SOUND ("shambler/melee2.wav");
	PRECACHE_SOUND ("shambler/smack.wav");

	PRECACHE_SOUND ("shambler/step1.wav");
	PRECACHE_SOUND ("shambler/step2.wav");
}

void CShambler::MonsterEvents(void)
{
	switch (m_Activity)
	{
	case ACT_WALK:
	case ACT_RUN:
		if (pev->frame == 23 || pev->frame == 34)
		{
			if (RANDOM_FLOAT(0.0f, 1.0f) > 0.8f)
				EMIT_SOUND(edict(), CHAN_VOICE, "shambler/sidle.wav", 1.0, ATTN_IDLE);

			if (m_hEnemy == NULL || (m_hEnemy->pev->origin - pev->origin).Length() > 100)
			{
				EMIT_SOUND(edict(), CHAN_BODY, "shambler/step1.wav", 0.6, ATTN_IDLE);
				UTIL_ScreenShake(pev->origin + Vector(0, 0, -24), 2.0f, 2.0f, 0.5f, 250.0f);
			}
		}
		if (pev->frame == 18 || pev->frame == 31)
		{
			if (m_hEnemy == NULL || (m_hEnemy->pev->origin - pev->origin).Length() > 100)
			{
				EMIT_SOUND(edict(), CHAN_BODY, "shambler/step2.wav", 0.6, ATTN_IDLE);
				UTIL_ScreenShake(pev->origin + Vector(0, 0, -24), 2.0f, 2.0f, 0.5f, 250.0f);
			}
		}
		break;

	case ACT_MELEE_ATTACK1:
		//smash
		if (pev->frame == 36)
			EMIT_SOUND(edict(), CHAN_VOICE, "shambler/melee1.wav", 1.0, ATTN_NORM);

		if (pev->frame == 43)
			ShamblerSmash();

		//swing right/left
		if (pev->frame == 48 || pev->frame == 57)
			EMIT_SOUND(edict(), CHAN_VOICE, "shambler/melee2.wav", 1.0, ATTN_NORM);

		//swing right
		if (pev->frame == 52)
			ShamblerClaw(-250);

		//swing left
		if (pev->frame == 61)
			ShamblerClaw(250);

		break;

	case ACT_RANGE_ATTACK1:
		if(pev->frame == 68)
			BeginCharging();

		if(pev->frame == 69)
			EndCharging();

		if (pev->frame >= 70 && pev->frame <= 74)
			CastLightning();

		break;
	}
}