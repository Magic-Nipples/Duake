
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"weapons.h"
#include	"skill.h"
#include	"player.h"
#include	"gamerules.h"
#include	"decals.h"

#define TAR_TIME_TO_EXLPODE		45.0f	// makes a tarbaby like a snark :-)

class CTarBaby : public CQuakeMonster
{
public:
	void Spawn(void);
	void Precache(void);
	BOOL MonsterHasMeleeAttack(void) { return TRUE; }
	BOOL MonsterHasMissileAttack(void) { return TRUE; }
	//BOOL MonsterHasPain( void ) { return FALSE; }
	void MonsterMeleeAttack(void);
	void MonsterMissileAttack(void);

	BOOL MonsterCheckAttack(void);
	void MonsterAttack(void);
	void MonsterPain(CBaseEntity* pAttacker, float flDamage);
	void MonsterKilled(entvars_t* pevAttacker, int iGib);
	int BloodColor(void) { return BLOOD_COLOR_TAR; }

	void EXPORT JumpTouch(CBaseEntity* pOther);
	void EXPORT TarExpand(void);
	void EXPORT TarExplosion(void);

	void AI_Run_Melee(void);
	void AI_Run_Missile(void);

	void MonsterSight(void);
	void MonsterIdle(void);
	void MonsterWalk(void);
	void MonsterRun(void);
	void MonsterBounce(void);

	void DecalPaint(void);

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];

	float m_fAttackDelay;
};

LINK_ENTITY_TO_CLASS(monster_tarbaby, CTarBaby);

TYPEDESCRIPTION CTarBaby::m_SaveData[] =
{
	DEFINE_FIELD(CTarBaby, m_fAttackDelay, FIELD_TIME)
}; IMPLEMENT_SAVERESTORE(CTarBaby, CQuakeMonster);

void CTarBaby::MonsterSight(void)
{
	EMIT_SOUND(edict(), CHAN_VOICE, "blob/sight1.wav", 1.0, ATTN_NORM);
}

void CTarBaby::MonsterMissileAttack(void)
{
	m_iAIState = STATE_ATTACK;
	SetActivity(ACT_LEAP);
}

void CTarBaby::MonsterMeleeAttack(void)
{
	m_iAIState = STATE_ATTACK;
	SetActivity(ACT_LEAP);
}

void CTarBaby::MonsterIdle(void)
{
	m_iAIState = STATE_IDLE;
	SetActivity(ACT_WALK);
	m_flMonsterSpeed = 0;
	pev->pain_finished = 0;
	pev->framerate = 0.0f;
}

void CTarBaby::MonsterWalk(void)
{
	m_iAIState = STATE_WALK;
	SetActivity(ACT_WALK);
	m_flMonsterSpeed = 2;
	pev->pain_finished = 0;
	pev->framerate = 1.0f;
}

void CTarBaby::MonsterRun(void)
{
	m_iAIState = STATE_RUN;
	SetActivity(ACT_RUN);
	m_flMonsterSpeed = 2;
	pev->framerate = 1.0f;
}

void CTarBaby::AI_Run_Melee(void)
{
	AI_Face();
}

void CTarBaby::AI_Run_Missile(void)
{
	AI_Face();
}

BOOL CTarBaby::MonsterCheckAttack(void)
{
	if (m_fAttackDelay < gpGlobals->time)
	{
		MonsterMissileAttack();
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void CTarBaby::MonsterAttack(void)
{
	if (!pev->pain_finished)
		pev->pain_finished = gpGlobals->time + TAR_TIME_TO_EXLPODE;

	float speedFactor = 2.0f - ((pev->pain_finished - gpGlobals->time) / (TAR_TIME_TO_EXLPODE));

	// multiply framerate by time to explode
	pev->framerate = 1.0f * speedFactor;

	if (speedFactor >= 2.0f)
	{
		// time to self-destruction
		MonsterKilled(gpWorld->pev, GIB_ALWAYS);
		return;
	}

	if (m_Activity == ACT_LEAP)
		AI_Face(); // doesn't change when is flying

	if (!m_fSequenceFinished) return;

	if (m_Activity == ACT_LEAP)
	{
		MonsterBounce();
		SetActivity(ACT_FLY);
	}
	else if (m_Activity == ACT_FLY)
	{
		if (++pev->impulse == 4)
			SetActivity(ACT_LEAP);
	}
}

void CTarBaby::MonsterPain(CBaseEntity* pAttacker, float flDamage)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, "blob/sight1.wav", 1.0, ATTN_NORM, 0, RANDOM_LONG(80, 95));
}

void CTarBaby::MonsterBounce(void)
{
	pev->movetype = MOVETYPE_BOUNCE;
	SetTouch(&CTarBaby::JumpTouch);

	UTIL_MakeVectors(pev->angles);

	pev->velocity = gpGlobals->v_forward * 600.0f + Vector(0, 0, 200.0f);
	pev->velocity.z += RANDOM_FLOAT(0.0f, 1.0f) * 150.0f;
	pev->flags &= ~FL_ONGROUND;
	pev->impulse = 0;
	pev->origin.z++;

	if (g_iSkillLevel == SKILL_EASY)
	{
		m_fAttackDelay = gpGlobals->time + RANDOM_FLOAT(1.0f, 4.0f);
	}
	else if (g_iSkillLevel == SKILL_MEDIUM)
	{
		m_fAttackDelay = gpGlobals->time + RANDOM_FLOAT(0.5f, 2.5f);
	}
	else if (g_iSkillLevel == SKILL_HARD)
	{
		if (RANDOM_LONG(0, 2) == 2)
			m_fAttackDelay = gpGlobals->time + RANDOM_FLOAT(0.5f, 2.5f);
		else
			m_fAttackDelay = -1;
	}
	else //nightmare
	{
		m_fAttackDelay = -1;
	}
}

void CTarBaby::JumpTouch(CBaseEntity* pOther)
{
	if (pev->health <= 0)
		return;

	if (pev == pOther->pev)
		return;

	if (pOther->pev->takedamage && !FStrEq(STRING(pev->classname), STRING(pOther->pev->classname)))
	{
		if (pev->velocity.Length() > 400)
		{
			float ldmg = 10 + RANDOM_FLOAT(0.0f, 10.0f);
			pOther->TakeDamage(pev, pev, ldmg, DMG_GENERIC);
		}
	}
	else
	{
		EMIT_SOUND(edict(), CHAN_WEAPON, "blob/land1.wav", 1.0, ATTN_NORM);
	}

	DecalPaint();

	TraceResult trace;
	Vector end = pev->origin + pev->velocity * 1.5;
	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	TRACE_MONSTER_HULL(edict(), pev->origin, end, dont_ignore_monsters, edict(), &trace);

	if (pOther->IsBSPModel() && (trace.vecPlaneNormal.z > 0.7f) && m_fAttackDelay > gpGlobals->time)
	{
		SetTouch(NULL);
		pev->impulse = 0;
		pev->movetype = MOVETYPE_STEP;
		MonsterRun();

		return;
	}

	if (!ENT_IS_ON_FLOOR(edict()))
	{
		if (pev->flags & FL_ONGROUND)
		{
			pev->movetype = MOVETYPE_STEP;

			// jump randomly to not get hung up
			SetTouch(NULL);
			MonsterRun();
		}
		return;	// not on ground yet
	}

	SetTouch(NULL);

	if (m_hEnemy != NULL && m_hEnemy->pev->health > 0)
	{
		m_iAIState = STATE_ATTACK;
		SetActivity(ACT_LEAP);
	}
	else
	{
		pev->movetype = MOVETYPE_STEP;
		pev->pain_finished = 0;	// explode cancelling
		MonsterRun();
	}
}

void CTarBaby::TarExpand(void)
{
	float flInterval = StudioFrameAdvance(1.0f);
	pev->nextthink = gpGlobals->time + 0.1f;

	SetThink(&CTarBaby::TarExplosion);
}

void CTarBaby::TarExplosion(void)
{
	Q_RadiusDamage(this, this, 120, gpWorld);

	EMIT_SOUND(edict(), CHAN_VOICE, "blob/death1.wav", 1.0, ATTN_NORM);

	Vector vecSrc = pev->origin - (8 * pev->velocity.Normalize());

	DecalPaint();

	MESSAGE_BEGIN(MSG_BROADCAST, gmsgTempEntity);
	WRITE_BYTE(TE_TAREXPLOSION);
	WRITE_COORD(vecSrc.x);
	WRITE_COORD(vecSrc.y);
	WRITE_COORD(vecSrc.z);
	MESSAGE_END();

	UTIL_Remove(this);
}

void CTarBaby::MonsterKilled(entvars_t* pevAttacker, int iGib)
{
	SetThink(&CTarBaby::TarExpand);
	pev->nextthink = gpGlobals->time + 0.1f;
	SetActivity(ACT_DIESIMPLE);
}

//=========================================================
// Spawn
//=========================================================
void CTarBaby::Spawn(void)
{
	if (!g_pGameRules->FAllowMonsters() || !g_registered)
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	Precache();

	SET_MODEL(ENT(pev), "progs/tarbaby.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, -24), Vector(16, 16, 40));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->health = 80;

	m_fAttackDelay = -1;

	WalkMonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CTarBaby::Precache()
{
	PRECACHE_MODEL("progs/tarbaby.mdl");

	PRECACHE_SOUND("blob/death1.wav");
	PRECACHE_SOUND("blob/hit1.wav");
	PRECACHE_SOUND("blob/land1.wav");
	PRECACHE_SOUND("blob/sight1.wav");
}

#define TARBABYSIDETRACE 24
#define TARBABYVERTICALTRACE 32
void CTarBaby::DecalPaint(void)
{
	TraceResult tr;

	for (int i = 0; i < 6; i++)
	{
		switch (i)
		{
		case 0:
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, TARBABYVERTICALTRACE), ignore_monsters, ENT(pev), &tr);
			UTIL_DecalTrace(&tr, DECAL_TARS1 + RANDOM_LONG(0, 3));
			break;

		case 1:
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -TARBABYVERTICALTRACE), ignore_monsters, ENT(pev), &tr);
			UTIL_DecalTrace(&tr, DECAL_TARS1 + RANDOM_LONG(0, 3));
			break;

		case 2:
			UTIL_TraceLine(pev->origin, pev->origin + Vector(TARBABYSIDETRACE, 0, 0), ignore_monsters, ENT(pev), &tr);
			UTIL_DecalTrace(&tr, DECAL_TARS1 + RANDOM_LONG(0, 3));
			break;

		case 3:
			UTIL_TraceLine(pev->origin, pev->origin + Vector(-TARBABYSIDETRACE, 0, 0), ignore_monsters, ENT(pev), &tr);
			UTIL_DecalTrace(&tr, DECAL_TARS1 + RANDOM_LONG(0, 3));
			break;

		case 4:
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, TARBABYSIDETRACE, 0), ignore_monsters, ENT(pev), &tr);
			UTIL_DecalTrace(&tr, DECAL_TARS1 + RANDOM_LONG(0, 3));
			break;

		case 5:
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, -TARBABYSIDETRACE, 0), ignore_monsters, ENT(pev), &tr);
			UTIL_DecalTrace(&tr, DECAL_TARS1 + RANDOM_LONG(0, 3));
			break;
		}
	}
}