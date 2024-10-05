/*
*
*	Copyright (c) 2024, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit is provided! Enjoy!
*
*/
//=========================================================
// Sprite Based Dog
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"


// DOG SPRITE ANIM DEFINES
#define DOG_ANIM_WALK_S		4
#define DOG_ANIM_WALK_E		8

#define DOG_ANIM_HURT_S		37

#define DOG_ANIM_ATTACK_S	0
#define DOG_ANIM_ATTACK_E	4

#define DOG_ANIM_DEATH_S	37
#define DOG_ANIM_DEATH_E	40


class CWolfDog : public CQuakeMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void MonsterGetAnimOffset(void);

	BOOL MonsterHasMeleeAttack(void) { return TRUE; }
	int BloodColor(void) { return BLOOD_COLOR_RED; }

	void MonsterSight(void);
	void MonsterIdle(void);
	void MonsterWalk(void);
	void MonsterRun(void);

	void MonsterMeleeAttack(void);
	//void MonsterMissileAttack(void);

	BOOL CheckMelee(void);
	void MonsterAttack(void);
	void MonsterBite(void);

	void MonsterPain(CBaseEntity* pAttacker, float flDamage);
	void MonsterKilled(entvars_t* pevAttacker, int iGib);
	void MonsterDeathSound(void);
};

LINK_ENTITY_TO_CLASS(monster_wolf_dog, CWolfDog);


//=========================================================
// Spawn
//=========================================================
void CWolfDog::Spawn(void)
{
	if (!g_pGameRules->FAllowMonsters())
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}
	Precache();

	SET_MODEL(ENT(pev), "sprites/npc/dog.spr");
	UTIL_SetSize(pev, Vector(-32, -32, -56), Vector(32, 32, 20));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->health = 10;

	pev->scale = 2;

	pPlayer = UTIL_PlayerByIndex(1);
	sprFlags = FL_MONSTER_SPRITE;
	MonsterGetAnimOffset();

	WalkMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CWolfDog::Precache()
{
	PRECACHE_MODEL("sprites/npc/dog.spr");

	PRECACHE_SOUND("dog/dog_alert.wav");
	PRECACHE_SOUND("dog/dog_attack.wav");
	PRECACHE_SOUND("dog/dog_death.wav");
}

void CWolfDog::MonsterGetAnimOffset(void)
{
	i_anim_offset[0] = DOG_ANIM_WALK_S;
	i_anim_offset[1] = DOG_ANIM_WALK_E;
	i_anim_offset[2] = DOG_ANIM_HURT_S;
	i_anim_offset[3] = DOG_ANIM_ATTACK_S;
	i_anim_offset[4] = DOG_ANIM_ATTACK_E;
	i_anim_offset[5] = DOG_ANIM_DEATH_S;
	i_anim_offset[6] = DOG_ANIM_DEATH_E;
}

void CWolfDog::MonsterSight(void)
{
	EMIT_SOUND(edict(), CHAN_VOICE, "dog/dog_alert.wav", 1.0, ATTN_NORM);
}

void CWolfDog::MonsterIdle(void)
{
	m_iAIState = STATE_IDLE;
	m_flMonsterSpeed = 0;

	ANIM_STATE = SPRITE_STATE_IDLE;
}

void CWolfDog::MonsterWalk(void)
{
	m_iAIState = STATE_WALK;
	m_flMonsterSpeed = 4;

	ANIM_STATE = SPRITE_STATE_WALK;
	ANIM_START = true;
}

void CWolfDog::MonsterRun(void)
{
	m_iAIState = STATE_RUN;
	m_flMonsterSpeed = 5;

	ANIM_STATE = SPRITE_STATE_WALK;
	ANIM_START = true;
}

void CWolfDog::MonsterMeleeAttack(void)
{
	m_iAIState = STATE_ATTACK;

	ANIM_STATE = SPRITE_STATE_ATTACK;
	ANIM_START = true;
}

/*void CWolfDog : MonsterMissileAttack(void)
{
	m_iAIState = STATE_ATTACK;
}*/


BOOL CWolfDog::CheckMelee(void)
{
	if (m_iEnemyRange == RANGE_MELEE)
	{
		// FIXME: check canreach
		m_iAttackState = ATTACK_MELEE;
		return TRUE;
	}

	return FALSE;
}

void CWolfDog::MonsterAttack(void)
{
	if (pev->impulse == ATTACK_MELEE)
	{
		if (!m_pfnTouch)
			AI_Face();
	}
	/*else if (pev->impulse == ATTACK_MISSILE)
	{
		AI_Charge(10);
	}*/

	if (m_iAIState == STATE_ATTACK)
	{
		pev->nextthink = gpGlobals->time + 0.12;

		if (pev->frame == DOG_ANIM_ATTACK_S + 2)
			MonsterBite();

		if (pev->frame == DOG_ANIM_ATTACK_E && ANIM_STATE == SPRITE_STATE_ATTACK)
		{
			pev->impulse = ATTACK_NONE;	// reset shadow of attack state
			MonsterRun();
		}
	}
}

void CWolfDog::MonsterBite(void)
{
	EMIT_SOUND(edict(), CHAN_VOICE, "dog/dog_attack.wav", 1.0, ATTN_NORM);

	if (m_hEnemy == NULL)
		return;

	AI_Charge(10);

	if (!Q_CanDamage(m_hEnemy, this))
		return;

	Vector delta = m_hEnemy->pev->origin - pev->origin;

	if (delta.Length() > 100)
		return;

	float ldmg = (RANDOM_FLOAT(0, 1) + RANDOM_FLOAT(0, 1) + RANDOM_FLOAT(0, 1)) * 8;
	m_hEnemy->TakeDamage(pev, pev, ldmg, DMG_GENERIC);
}

//=========================================================
// Take Damage | Pain
//=========================================================
void CWolfDog::MonsterPain(CBaseEntity* pAttacker, float flDamage)
{
	//ANIM_STATE = SPRITE_STATE_DAMAGE;
	//ANIM_START = true;

	m_iAIState = STATE_PAIN;

	//EMIT_SOUND(edict(), CHAN_VOICE, "dog/dpain1.wav", 1.0, ATTN_NORM);

	AI_Pain(4);
}

//=========================================================
// Death
//=========================================================
void CWolfDog::MonsterKilled(entvars_t* pevAttacker, int iGib)
{
	ANIM_STATE = SPRITE_STATE_DEATH;
	ANIM_START = true;

	if (ShouldGibMonster(iGib))
	{
		EMIT_SOUND(edict(), CHAN_VOICE, "player/udeath.wav", 1.0, ATTN_NORM);
		CGib::ThrowHead("models/h_dog.mdl", pev);
		CGib::ThrowGib("models/gib3.mdl", pev);
		CGib::ThrowGib("models/gib3.mdl", pev);
		CGib::ThrowGib("models/gib3.mdl", pev);
		UTIL_Remove(this);
		return;
	}	
}

void CWolfDog::MonsterDeathSound(void)
{
	if (pev->frame == DOG_ANIM_DEATH_S + 2)
	{
		EMIT_SOUND(edict(), CHAN_VOICE, "dog/dog_death.wav", 1.0, ATTN_NORM);
	}
}