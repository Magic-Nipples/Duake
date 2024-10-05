/*
*
*	Copyright (c) 2024, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit is provided! Enjoy!
*
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "shake.h"
#include "env_sprite.h"


LINK_ENTITY_TO_CLASS(furniture_sprite, CSprite);

TYPEDESCRIPTION	CSprite::m_SaveData[] =
{
	DEFINE_FIELD(CSprite, m_maxFrame, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CSprite, CPointEntity);

void CSprite::Spawn(void)
{
	Precache();
	
	SET_MODEL(ENT(pev), STRING(pev->model));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) -1;

	if (pev->spawnflags & SF_SPRITE_SOLID)
	{
		pev->solid = SOLID_SLIDEBOX;
		pev->movetype = MOVETYPE_TOSS;
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
		pev->health = 9999;
	}
	else
	{
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
	}
	pev->iuser4 = 1;

	if (pev->angles.y != 0 && pev->angles.z == 0) // Worldcraft only sets y rotation, copy to Z
	{
		//pev->angles.z = pev->angles.y; //magic nipples - does not work properly?
		//pev->angles.y = 0;
	}

	pev->nextthink = gpGlobals->time + 0.01;
	SetThink(&CSprite::StartThink);
}

void CSprite::Precache(void)
{
	PRECACHE_MODEL((char*)STRING(pev->model));
}

void CSprite::StartThink(void)
{
	if (pev->spawnflags & SF_SPRITE_START_OFF)
		TurnOff();
	else
		TurnOn();
}

void CSprite::AnimateThink(void)
{
	Animate();
	pev->nextthink = gpGlobals->time + 0.1;
}

void CSprite::Animate()
{
	pev->frame++;

	if (pev->frame > m_maxFrame)
	{
		if (pev->spawnflags & SF_SPRITE_PLAY_ONCE)
		{
			TurnOff();
		}
		else
		{
			if (pev->spawnflags & SF_SPRITE_HAS_OFF)
				pev->frame = 1;
			else
				pev->frame = 0;
		}
	}
}

void CSprite::TurnOff(void)
{
	pev->effects &= ~EF_FULLBRIGHT;

	if ((pev->spawnflags & SF_SPRITE_HOLD_FRAME) && (pev->frame >= m_maxFrame))
		pev->frame = m_maxFrame;
	else
		pev->frame = 0;

	pev->iuser1 = 0;
	pev->nextthink = -1;
	SetThink(NULL);
}


void CSprite::TurnOn(void)
{
	if (pev->spawnflags & SF_SPRITE_FULLBRIGHT)
		pev->effects |= EF_FULLBRIGHT;

	pev->iuser1 = 1;

	if (m_maxFrame == 1) //only has on/off state so why think?
	{
		pev->frame = 1;
		pev->nextthink = -1;
	}
	else
	{
		SetThink(&CSprite::AnimateThink);
		pev->nextthink = gpGlobals->time;
	}
}


void CSprite::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pev->iuser1)
		TurnOff();
	else
		TurnOn();
}


#define GARG_IDLE_FRAME		13
#define GARG_FIRE_FRAME		16

LINK_ENTITY_TO_CLASS(monster_gargoyle, CGargoyle);

TYPEDESCRIPTION	CGargoyle::m_SaveData[] =
{
	DEFINE_FIELD(CGargoyle, m_maxFrame, FIELD_FLOAT),
	DEFINE_FIELD(CGargoyle,f_nextAttack,FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CGargoyle, CPointEntity);

void CGargoyle::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "sprites/gargwallalive.spr");

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->iuser4 = 1;
	f_nextAttack = -1;

	pev->nextthink = gpGlobals->time + 0.01;
	SetThink(&CGargoyle::StartThink);
}

void CGargoyle::Precache(void)
{
	PRECACHE_MODEL("sprites/gargwallalive.spr");
	PRECACHE_MODEL("models/lavaball.mdl");
	PRECACHE_SOUND("wall/throw1.wav");
	PRECACHE_SOUND("wall/awake1.wav");
}

void CGargoyle::StartThink(void)
{
	TurnOff();
}

void CGargoyle::TurnOff(void)
{
	pev->frame = 0;
	pev->iuser1 = 0;
	pev->nextthink = -1;
	SetThink(NULL);
}

void CGargoyle::TurnOn(void)
{
	pev->iuser1 = 1;
	EMIT_SOUND(edict(), CHAN_WEAPON, "wall/awake1.wav", 1.0, ATTN_NORM);
	SetThink(&CGargoyle::WakeThink);
	pev->nextthink = gpGlobals->time;
}

void CGargoyle::WakeThink(void)
{
	pev->nextthink = gpGlobals->time + 0.075;

	pev->frame++;

	if (pev->frame >= GARG_IDLE_FRAME)
	{
		pev->frame = GARG_IDLE_FRAME;
		SetThink(&CGargoyle::LookForThink);
		f_nextAttack = gpGlobals->time + RANDOM_LONG(1,5);
		return;
	}
}

void CGargoyle::LookForThink(void)
{
	TraceResult	tr;
	edict_t* pEdict = g_engfuncs.pfnPEntityOfEntIndex(1);
	CBaseEntity* pEntity = NULL;
	Vector vec2LOS;
	float inforwardcone;
	float cone = -0.7;

	pev->nextthink = gpGlobals->time + 0.1;

	if (f_nextAttack >= gpGlobals->time)
		return;

	UTIL_MakeVectorsPrivate(pev->angles, vForward, NULL, NULL);

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		if (pEdict == edict())
			continue;

		pEntity = Instance(pEdict);

		if (pEntity == NULL)
			continue;

		if (!pEntity->IsPlayer())
			continue;

		if (!pEntity->IsAlive())
			continue;

		if (pEntity->pev->health <= 0)
			continue;

		if (pEntity->pev->flags & FL_NOTARGET)
			continue;

		vec2LOS = (pev->origin - pEntity->Center());
		vec2LOS = vec2LOS.Normalize();

		inforwardcone = DotProduct(vec2LOS, vForward);

		if (inforwardcone > 0.5)
		{
			UTIL_TraceLine(pev->origin, pEntity->Center(), ignore_monsters, edict(), &tr);

			if (tr.flFraction == 1.0f)
			{
				//ALERT(at_console, "%0.2f\n", inforwardcone);
				//UTIL_LineTest(pev->origin, pEntity->pev->origin, 255, 1, 1, 5);
				attackOrigin = pEntity->pev->origin;
				SetThink(&CGargoyle::FireThink);
				EMIT_SOUND(edict(), CHAN_WEAPON, "wall/throw1.wav", 1.0, ATTN_NORM);
			}
		}
		else
		{
			pev->nextthink = gpGlobals->time + RANDOM_LONG(1,5);
		}
	}
}

void CGargoyle::FireThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	pev->frame++;

	if (pev->frame == GARG_FIRE_FRAME)
	{
		Vector vecDir = (attackOrigin + Vector(0, 0, 15) - pev->origin).Normalize();
		CNail* pFireBall = CNail::CreateNail(pev->origin + vForward * -4 + Vector(0, 0, -15), vecDir, this);

		if (pFireBall)
		{
			SET_MODEL(ENT(pFireBall->pev), "models/lavaball.mdl");
			pFireBall->SetTouch(&CNail::ExplodeTouch); // rocket explosion
			pFireBall->pev->avelocity = Vector(200, 100, 300);
			pFireBall->pev->velocity = vecDir * 300;
		}
	}

	if (pev->frame > m_maxFrame)
	{
		pev->frame = GARG_IDLE_FRAME;
		SetThink(&CGargoyle::LookForThink);

		f_nextAttack = gpGlobals->time + RANDOM_LONG(2, 6);
		return;
	}
}

void CGargoyle::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pev->iuser1)
		TurnOff();
	else
		TurnOn();
}