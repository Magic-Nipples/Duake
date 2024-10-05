/*
*
*	Copyright (c) 2024, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit is provided! Enjoy!
*
*/
//=========================================================
// Sprite Based Soldier ?
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"items.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"


// SOLDIER SPRITE ANIM DEFINES
#define SOLDIER_ANIM_WALK_S		12
#define SOLDIER_ANIM_WALK_E		16

#define SOLDIER_ANIM_HURT_S		12

#define SOLDIER_ANIM_ATTACK_S	8
#define SOLDIER_ANIM_ATTACK_E	11

#define SOLDIER_ANIM_DEATH_S	45
#define SOLDIER_ANIM_DEATH_E	49


class CWolfSoldier : public CQuakeMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void MonsterGetAnimOffset(void);

	BOOL MonsterHasMissileAttack(void) { return TRUE; }
	int BloodColor(void) { return BLOOD_COLOR_RED; }

	void MonsterSight(void);
	void MonsterIdle(void);
	void MonsterWalk(void);
	void MonsterRun(void);
	
	void MonsterMissileAttack(void);

	void MonsterAttack(void);
	BOOL MonsterCheckAttack(void);
	void MonsterFire(void);

	void MonsterPain(CBaseEntity* pAttacker, float flDamage);
	void MonsterKilled(entvars_t* pevAttacker, int iGib);
	void MonsterDeathSound(void);

	int m_fAttackFinished;
	int m_fInAttack;
};

LINK_ENTITY_TO_CLASS(monster_wolf_soldier, CWolfSoldier);

//=========================================================
// Spawn
//=========================================================
void CWolfSoldier::Spawn(void)
{
	if (!g_pGameRules->FAllowMonsters())
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}
	Precache();

	SET_MODEL(ENT(pev), "sprites/npc/soldier.spr");
	UTIL_SetSize(pev, Vector(-32, -32, -56), Vector(32, 32, 18)); //( -16, -16, -24 ) | ( 16, 16, 40 )

	
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->health = 25;

	pev->scale = 2;
	pPlayer = UTIL_PlayerByIndex(1);
	sprFlags = FL_MONSTER_SPRITE;
	MonsterGetAnimOffset();

	WalkMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CWolfSoldier::Precache()
{
	PRECACHE_MODEL("sprites/npc/soldier.spr");
	PRECACHE_SOUND("soldier/soldier_alert.wav");
	PRECACHE_SOUND("soldier/soldier_attack.wav");

	PRECACHE_SOUND("soldier/soldier_death1.wav");
	PRECACHE_SOUND("soldier/soldier_death2.wav");
	PRECACHE_SOUND("soldier/soldier_death3.wav");
	PRECACHE_SOUND("soldier/soldier_death4.wav");
	PRECACHE_SOUND("soldier/soldier_death5.wav");
}

void CWolfSoldier::MonsterGetAnimOffset(void)
{
	i_anim_offset[0] = SOLDIER_ANIM_WALK_S;
	i_anim_offset[1] = SOLDIER_ANIM_WALK_E;
	i_anim_offset[2] = SOLDIER_ANIM_HURT_S;
	i_anim_offset[3] = SOLDIER_ANIM_ATTACK_S;
	i_anim_offset[4] = SOLDIER_ANIM_ATTACK_E;
	i_anim_offset[5] = SOLDIER_ANIM_DEATH_S;
	i_anim_offset[6] = SOLDIER_ANIM_DEATH_E;
}

void CWolfSoldier::MonsterSight(void)
{
	EMIT_SOUND(edict(), CHAN_VOICE, "soldier/soldier_alert.wav", 1.0, ATTN_NORM);
}

void CWolfSoldier::MonsterIdle(void)
{
	m_iAIState = STATE_IDLE;
	m_flMonsterSpeed = 0;

	ANIM_STATE = SPRITE_STATE_IDLE;
}

void CWolfSoldier::MonsterWalk(void)
{
	m_iAIState = STATE_WALK;
	m_flMonsterSpeed = 2;

	ANIM_STATE = SPRITE_STATE_WALK;
	ANIM_START = true;
}

void CWolfSoldier::MonsterRun(void)
{
	m_iAIState = STATE_RUN;
	m_flMonsterSpeed = 3;

	ANIM_STATE = SPRITE_STATE_WALK;
	ANIM_START = true;
}

void CWolfSoldier::MonsterMissileAttack(void)
{
	m_iAIState = STATE_ATTACK;

	ANIM_STATE = SPRITE_STATE_ATTACK;
	ANIM_START = true;
}


BOOL CWolfSoldier::MonsterCheckAttack(void)
{
	Vector spot1, spot2;
	CBaseEntity* pTarg;
	float chance;

	pTarg = m_hEnemy;

	// see if any entities are in the way of the shot
	spot1 = EyePosition();
	spot2 = pTarg->EyePosition();

	TraceResult tr;
	UTIL_TraceLine(spot1, spot2, dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr);

	if (tr.fInOpen && tr.fInWater)
		return FALSE;	// sight line crossed contents

	if (tr.pHit != pTarg->edict())
		return FALSE;	// don't have a clear shot


	// missile attack
	if (gpGlobals->time < m_flAttackFinished)
		return FALSE;

	//if (m_iEnemyRange == RANGE_FAR)
		//return FALSE;

	if (m_iEnemyRange == RANGE_MELEE)
		chance = 1.1f;
	else if (m_iEnemyRange == RANGE_NEAR)
		chance = 0.4f;
	else if (m_iEnemyRange == RANGE_MID)
		chance = 0.1f;
	else
		chance = 0.05;

	if (RANDOM_FLOAT(0, 1) < chance)
	{
		MonsterMissileAttack();
		AttackFinished(RANDOM_FLOAT(0.5, 1.0));
		if (RANDOM_FLOAT(0, 1) < 0.3f)
			m_fLeftY = !m_fLeftY;

		return TRUE;
	}

	return FALSE;
}

void CWolfSoldier::MonsterAttack(void)
{
	if (m_fAttackFinished)
	{
		m_fAttackFinished = FALSE;
		m_fInAttack = FALSE;

		if (CheckRefire())
			MonsterMissileAttack();
		else
			MonsterRun();
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.12;

		if (pev->frame == SOLDIER_ANIM_ATTACK_E - 1)
			MonsterFire();

		if (pev->frame == SOLDIER_ANIM_ATTACK_E && ANIM_STATE == SPRITE_STATE_ATTACK)
		{
			m_fAttackFinished = TRUE;
			m_fInAttack = TRUE;

			MonsterRun();
		}
	}
	AI_Face();
}

void CWolfSoldier::MonsterFire(void)
{
	if (m_fInAttack) return;

	AI_Face();

	EMIT_SOUND(edict(), CHAN_VOICE, "soldier/soldier_attack.wav", 1.0, ATTN_NORM);

	// fire somewhat behind the player, so a dodging player is harder to hit
	//Vector vecDir = ((m_hEnemy->pev->origin - m_hEnemy->pev->velocity * 0.1f) - pev->origin).Normalize();
	Vector vecDir = (m_hEnemy->pev->origin - pev->origin).Normalize();

	CBasePlayer::FireBullets(pev, 1, vecDir, Vector(0.01, 0.01, 0), BULLET_FLAG_NPC);

	pev->effects |= EF_MUZZLEFLASH;
	m_fInAttack = TRUE;
}

void CWolfSoldier::MonsterPain(CBaseEntity* pAttacker, float flDamage)
{
	if (pev->pain_finished > gpGlobals->time)
		return;

	ANIM_STATE = SPRITE_STATE_DAMAGE;
	ANIM_START = true;

	//EMIT_SOUND_ARRAY_DYN(CHAN_VOICE, pPainSounds, ATTN_NORM);

	m_iAIState = STATE_PAIN;
	m_flMonsterSpeed = 0;

	pev->pain_finished = gpGlobals->time + 0.1f;
}

void CWolfSoldier::MonsterKilled(entvars_t* pevAttacker, int iGib)
{
	ANIM_STATE = SPRITE_STATE_DEATH;
	ANIM_START = true;

	if (ShouldGibMonster(iGib))
	{
		EMIT_SOUND(edict(), CHAN_VOICE, "player/udeath.wav", 1.0, ATTN_NORM);
		CGib::ThrowHead("progs/h_guard.mdl", pev);
		CGib::ThrowGib("progs/gib1.mdl", pev);
		CGib::ThrowGib("progs/gib2.mdl", pev);
		CGib::ThrowGib("progs/gib3.mdl", pev);
		UTIL_Remove(this);
		return;
	}
}

void CWolfSoldier::MonsterDeathSound(void)
{
	if (pev->frame == SOLDIER_ANIM_DEATH_S + 1)
	{
		switch (RANDOM_LONG(0, 4))
		{
		case 0: EMIT_SOUND(edict(), CHAN_VOICE, "soldier/soldier_death1.wav", 1.0, ATTN_NORM); break;
		case 1: EMIT_SOUND(edict(), CHAN_VOICE, "soldier/soldier_death2.wav", 1.0, ATTN_NORM); break;
		case 2: EMIT_SOUND(edict(), CHAN_VOICE, "soldier/soldier_death3.wav", 1.0, ATTN_NORM); break;
		case 3: EMIT_SOUND(edict(), CHAN_VOICE, "soldier/soldier_death4.wav", 1.0, ATTN_NORM); break;
		case 4: EMIT_SOUND(edict(), CHAN_VOICE, "soldier/soldier_death5.wav", 1.0, ATTN_NORM); break;
		}
	}
}