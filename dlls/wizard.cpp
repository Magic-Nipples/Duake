
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define WIZARD_START_ATTACK		1
#define WIZARD_END_ATTACK		2
#define WIZARD_IDLE_SOUND		3

class CWizard : public CQuakeMonster
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MonsterHasMissileAttack( void ) { return TRUE; }
	void MonsterMissileAttack( void );
	int BloodColor( void ) { return BLOOD_COLOR_YELLOW; }

	void WizardFastFire( void );
	void MonsterAttack(void);
	void WizardAttackFinished( void );

	BOOL MonsterCheckAttack( void );
	void MonsterPain( CBaseEntity *pAttacker, float flDamage );
	void MonsterKilled( entvars_t *pevAttacker, int iGib );
	void MonsterSight( void );

	void MonsterIdle( void );
	void MonsterWalk( void );	
	void MonsterRun( void );
	void MonsterSide( void );

	void IdleSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];

	void MonsterIdleSound(void);
	void MonsterWalkSound(void);
	void MonsterRunSound(void);

	Vector m_vecEstVelocity;
};

LINK_ENTITY_TO_CLASS( monster_wizard, CWizard );

class CWizardSpikeDelay : public CBaseEntity
{
	void Think( void )
	{
		if (!FNullEnt( pev->owner ) && !FNullEnt( pev->enemy ) && VARS( pev->owner )->health > 0.0)
		{
			VARS( pev->owner )->effects |= EF_MUZZLEFLASH;
                              
			UTIL_MakeVectors( VARS( pev->enemy )->angles );
			Vector dst = VARS( pev->enemy )->origin - 13 * pev->movedir;
	
			Vector vec = (dst - pev->origin).Normalize();
			EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "wizard/wattack.wav", 1, ATTN_NORM );
			
			CNail *pNail = CNail::CreateNail( pev->origin, vec, CBaseEntity::Instance( pev->owner ));

			// FIXME: make a new static function into CNail class
			if( pNail )
			{
				SET_MODEL( ENT(pNail->pev), "models/w_spike.mdl" );
				pNail->pev->velocity = vec * 600;
			}
		}

		UTIL_Remove( this );
	}
};

LINK_ENTITY_TO_CLASS( wizard_spike_delay, CWizardSpikeDelay );

const char *CWizard::pAttackSounds[] = 
{
	"wizard/wattack.wav",
};

const char *CWizard::pIdleSounds[] = 
{
	"wizard/widle1.wav",
	"wizard/widle2.wav",
};

const char *CWizard::pAlertSounds[] = 
{
	"wizard/wsight.wav",
};

const char *CWizard::pPainSounds[] = 
{
	"wizard/wpain.wav",
};

const char *CWizard::pDeathSounds[] = 
{
	"wizard/wdeath.wav",
};

void CWizard :: MonsterIdle( void )
{
	m_iAttackState = ATTACK_NONE;
	m_iAIState = STATE_IDLE;
	SetActivity( ACT_IDLE );
	m_flMonsterSpeed = 0;
}

void CWizard :: MonsterWalk( void )
{
	m_iAttackState = ATTACK_NONE;
	m_iAIState = STATE_WALK;
	SetActivity( ACT_RUN );
	m_flMonsterSpeed = 8;
}

void CWizard :: MonsterRun( void )
{
	m_iAIState = STATE_RUN;
	SetActivity( ACT_RUN );
	m_flMonsterSpeed = 16;
}

void CWizard :: MonsterSide( void )
{
	m_iAIState = STATE_RUN;
	SetActivity( ACT_IDLE );
	m_flMonsterSpeed = 8;
}

BOOL CWizard::MonsterCheckAttack( void )
{
	Vector spot1, spot2;
	CBaseEntity *pTarg;
	float chance;

	if (gpGlobals->time < m_flAttackFinished)
		return FALSE;

	if (!m_fEnemyVisible)
		return FALSE;

	if (m_iEnemyRange == RANGE_FAR)
	{
		if (m_iAttackState != ATTACK_STRAIGHT)
		{
			m_iAttackState = ATTACK_STRAIGHT;
			MonsterRun();
		}
		return FALSE;
	}
		
	pTarg = m_hEnemy;

	// see if any entities are in the way of the shot
	spot1 = EyePosition();
	spot2 = pTarg->EyePosition();

	TraceResult tr;
	UTIL_TraceLine(spot1, spot2, dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr);

	if (tr.pHit != pTarg->edict())
	{	
		// don't have a clear shot, so move to a side
		if (m_iAttackState != ATTACK_STRAIGHT)
		{
			m_iAttackState = ATTACK_STRAIGHT;
			MonsterRun();
		}
		return FALSE;
	}
			
	if (m_iEnemyRange == RANGE_MELEE)
		chance = 0.9;
	else if (m_iEnemyRange == RANGE_NEAR)
		chance = 0.6;
	else if (m_iEnemyRange == RANGE_MID)
		chance = 0.2;
	else
		chance = 0.0;

	if (RANDOM_FLOAT(0,1) < chance)
	{
		m_iAttackState = ATTACK_MISSILE;
		return TRUE;
	}

	if (m_iEnemyRange == RANGE_MID)
	{
		if (m_iAttackState != ATTACK_STRAIGHT)
		{
			m_iAttackState = ATTACK_STRAIGHT;
			MonsterRun();
		}
	}
	else
	{
		if (m_iAttackState != ATTACK_SLIDING)
		{
			m_iAttackState = ATTACK_SLIDING;
			MonsterSide();
		}
	}
	
	return FALSE;
}

void CWizard :: MonsterMissileAttack( void )
{
	m_iAIState = STATE_ATTACK;
	SetActivity( ACT_MELEE_ATTACK1 );
}

void CWizard :: MonsterKilled( entvars_t *pevAttacker, int iGib )
{
	if( ShouldGibMonster( iGib ))
	{
		EMIT_SOUND( edict(), CHAN_VOICE, "player/udeath.wav", 1.0, ATTN_NORM );
		CGib::ThrowHead ("models/h_wizard.mdl", pev);
		CGib::ThrowGib ("models/gib2.mdl", pev);
		CGib::ThrowGib ("models/gib2.mdl", pev);
		CGib::ThrowGib ("models/gib2.mdl", pev);
		UTIL_Remove( this );
		return;
	}

	pev->velocity.x = RANDOM_FLOAT( -200, 200 );
	pev->velocity.y = RANDOM_FLOAT( -200, 200 );
	pev->velocity.z = RANDOM_FLOAT( 100, 200 );
	pev->flags &= ~(FL_ONGROUND|FL_FLY);

	// a bit of a hack. If a corpses' bbox is positioned such that being left solid so that
	// it can be attacked will block the player on a slope or stairs, the corpse is made nonsolid. 
	//UTIL_SetSize ( pev, Vector ( -16, -16, -16 ), Vector ( 16, 16, 16 ) );

	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pDeathSounds, ATTN_NORM ); 
}

void CWizard::MonsterAttack(void)
{
	if (m_Activity == ACT_MELEE_ATTACK1 && pev->frame == 31 && !wizardAttack)
		WizardFastFire();

	if (m_iAIState == STATE_ATTACK && m_fSequenceFinished)
		WizardAttackFinished();
}

/*
=================
WizardAttackFinished
=================
*/
void CWizard :: WizardAttackFinished( void )
{
	AttackFinished(2);

	if (m_iEnemyRange >= RANGE_MID || !m_fEnemyVisible)
	{
		m_iAttackState = ATTACK_STRAIGHT;
		MonsterRun();
	}
	else
	{
		m_iAttackState = ATTACK_SLIDING;
		MonsterSide();
	}
}

void CWizard :: IdleSound( void )
{
	// time-based idle sound
	if( pev->radsuit_finished < gpGlobals->time )
	{
		EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pIdleSounds, ATTN_IDLE ); 
		pev->radsuit_finished = gpGlobals->time + RANDOM_FLOAT( 2, 8 );
	}
}

void CWizard :: MonsterSight( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pAlertSounds, ATTN_NORM ); 
}

void CWizard :: MonsterPain( CBaseEntity *pAttacker, float flDamage )
{
	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pPainSounds, ATTN_NORM ); 

	if( RANDOM_LONG( 0, 70 ) > flDamage )
		return; // didn't flinch

	m_iAIState = STATE_PAIN;
	SetActivity( ACT_SMALL_FLINCH );
}

void CWizard :: WizardFastFire( void )
{
	UTIL_MakeVectors( pev->angles );

	CBaseEntity *pShootDelay;

	if( m_hEnemy == NULL )
	{
		// g-cont. is even possible?
		ALERT( at_console, "No enemy!!!\n" );
		return;
	}

	pShootDelay = GetClassPtr( (CWizardSpikeDelay *)NULL );
	pShootDelay->pev->classname = MAKE_STRING ("wizard_spike_delay");
	pShootDelay->pev->owner = ENT (pev);
	pShootDelay->pev->enemy = m_hEnemy->edict();
	UTIL_SetOrigin( pShootDelay->pev, pev->origin + Vector( 0, 0, 30 ) + gpGlobals->v_forward * 14 + gpGlobals->v_right * 14);
	pShootDelay->pev->nextthink = gpGlobals->time + 0.8;
	pShootDelay->pev->movedir = gpGlobals->v_right;

	pShootDelay = GetClassPtr( (CWizardSpikeDelay *)NULL );
	pShootDelay->pev->classname = MAKE_STRING ("wizard_spike_delay");
	pShootDelay->pev->owner = ENT (pev);
	pShootDelay->pev->enemy = m_hEnemy->edict();
	UTIL_SetOrigin( pShootDelay->pev, pev->origin + Vector( 0, 0, 30 ) + gpGlobals->v_forward * 14 + gpGlobals->v_right * -14);
	pShootDelay->pev->nextthink = gpGlobals->time + 0.3;
	pShootDelay->pev->movedir = -gpGlobals->v_right;
}

//=========================================================
// Spawn
//=========================================================
void CWizard :: Spawn( void )
{
	if( !g_pGameRules->FAllowMonsters( ))
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	Precache( );

	SET_MODEL(ENT(pev), "progs/wizard.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, -24 ), Vector( 16, 16, 40 ));

	pev->solid	= SOLID_SLIDEBOX;
	pev->movetype	= MOVETYPE_STEP;
	pev->flags	|= FL_FLY;
	pev->health	= 80;

	FlyMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CWizard :: Precache()
{
	PRECACHE_MODEL( "progs/wizard.mdl" );
	PRECACHE_MODEL( "models/h_wizard.mdl" );
	PRECACHE_MODEL( "models/w_spike.mdl" );

	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );
}

void CWizard::MonsterIdleSound(void)
{
	if (m_fNextIdleSound < gpGlobals->time)
	{
		if (RANDOM_FLOAT(0.0f, 1.0f) < 0.2f)
			IdleSound();

		m_fNextIdleSound = gpGlobals->time + RANDOM_LONG(2, 4);
	}
}

void CWizard::MonsterWalkSound(void)
{
	if (m_fNextIdleSound < gpGlobals->time)
	{
		if (RANDOM_FLOAT(0.0f, 1.0f) < 0.2f)
			IdleSound();

		m_fNextIdleSound = gpGlobals->time + RANDOM_LONG(2, 4);
	}
}

void CWizard::MonsterRunSound(void)
{
	if (m_fNextIdleSound < gpGlobals->time)
	{
		if (RANDOM_FLOAT(0.0f, 1.0f) < 0.2f)
			IdleSound();

		m_fNextIdleSound = gpGlobals->time + RANDOM_LONG(2, 4);
	}
}