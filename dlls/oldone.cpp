
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monster.h"
#include	"gamerules.h"
#include	"player.h"
#include	"skill.h"
#include	"shake.h"

class CShubNiggurath : public CQuakeMonster//CBaseAnimating
{
public:
	void Spawn(void);
	void Precache(void);
	void Killed(entvars_t* pevAttacker, int iGib);

	void EXPORT DeathThink(void);
	void EXPORT Finale2(void);
	void EXPORT Finale3(void);
	void EXPORT Finale4(void);
	void EXPORT Finale5(void);
	void EXPORT Finale6(void);

	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

private:
	BOOL m_bDelayOn;
	float m_fDelayTime;
};

LINK_ENTITY_TO_CLASS(monster_oldone, CShubNiggurath);

//=========================================================
// Spawn
//=========================================================
void CShubNiggurath::Spawn(void)
{
	if (!g_pGameRules->FAllowMonsters() || !g_registered)
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	Precache();

	SET_MODEL(ENT(pev), "progs/oldone.mdl");
	UTIL_SetSize(pev, Vector(-160, -128, -24), Vector(160, 128, 256));

	pev->solid = SOLID_BBOX;	// g-cont. allow hitbox trace!
	pev->movetype = MOVETYPE_STEP;
	pev->health = 40000;

	pev->takedamage = DAMAGE_YES;
	pev->animtime = gpGlobals->time + 0.1;
	pev->framerate = 1.0;

	SetThink(&CShubNiggurath::MonsterThink);
	pev->nextthink = gpGlobals->time + 0.2;
	gpWorld->total_monsters++;
	m_fDelayTime = -1.0f;
	m_bDelayOn = FALSE;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CShubNiggurath::Precache(void)
{
	PRECACHE_MODEL("progs/oldone.mdl");
	//PRECACHE_MODEL("sprites/player.spr");	// easter egg

	PRECACHE_SOUND("boss2/death.wav");
	PRECACHE_SOUND("boss2/idle.wav");
	PRECACHE_SOUND("boss2/sight.wav");
	PRECACHE_SOUND("boss2/pop2.wav");
	PRECACHE_SOUND("ambience/rumble.wav");
}

void CShubNiggurath::Killed(entvars_t* pevAttacker, int iGib)
{
	g_intermission_exittime = gpGlobals->time + 10000000;	// never allow exit
	g_intermission_running = 1;

	CBaseEntity* pCamera = UTIL_FindEntityByClassname(NULL, "info_intermission");
	CBaseEntity* pTeleport = UTIL_FindEntityByClassname(NULL, "misc_teleporttrain");

	pev->takedamage = DAMAGE_NO;

	if (pTeleport)
		UTIL_Remove(pTeleport);

	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i + 1);

		if (!pPlayer) continue;

		if (pCamera)
		{
			SET_VIEW(pPlayer->edict(), pCamera->edict());
			UTIL_SetOrigin(pPlayer->pev, pCamera->pev->origin);
		}

		pPlayer->EnableControl(FALSE);
		pPlayer->pev->takedamage = DAMAGE_NO;
		pPlayer->pev->solid = SOLID_NOT;
		pPlayer->pev->movetype = MOVETYPE_NONE;
		pPlayer->pev->modelindex = 0;
		pPlayer->m_iHideHUD |= HIDEHUD_HUD;

		MESSAGE_BEGIN(MSG_ONE, gmsgHideHUD, NULL, pPlayer->pev);
		WRITE_BYTE(HIDEHUD_HUD); // hide scoreboard and HUD
		MESSAGE_END();
	}

	// make fake versions of all players as standins, and move the real
	// players to the intermission spot

	// wait for 1 second
	SetThink(&CShubNiggurath::Finale2);
	m_fDelayTime = gpGlobals->time + 1.0f;

	MESSAGE_BEGIN(MSG_ALL, SVC_FINALE);
	MESSAGE_END();
}

void CShubNiggurath::Finale2(void)
{
	float flInterval = StudioFrameAdvance(1.0f);
	pev->nextthink = gpGlobals->time + 0.1f;

	if (m_fDelayTime < gpGlobals->time)
	{
		Vector telePos = pev->origin - Vector(0, 100, 0);

		MESSAGE_BEGIN(MSG_PAS, gmsgTempEntity, telePos);
		WRITE_BYTE(TE_TELEPORT);
		WRITE_COORD(telePos.x);
		WRITE_COORD(telePos.y);
		WRITE_COORD(telePos.z);
		MESSAGE_END();

		//EMIT_SOUND(ENT(pev), CHAN_VOICE, "misc/r_tele1.wav", 1, ATTN_NORM);
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "misc/teleport.wav", 1, ATTN_NORM);


		SetThink(&CShubNiggurath::Finale3);
		m_fDelayTime = gpGlobals->time + 2.0f;
	}
}

void CShubNiggurath::Finale3(void)
{
	float flInterval = StudioFrameAdvance(1.0f);
	pev->nextthink = gpGlobals->time + 0.1f;

	if (m_fDelayTime < gpGlobals->time)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "boss2/death.wav", 1, ATTN_NORM);
		EMIT_SOUND(ENT(pev), CHAN_BODY, "ambience/rumble.wav", 1, ATTN_NORM);
		LIGHT_STYLE(0, "abcdefghijklmlkjihgfedcb");	// apply to world
		UTIL_ScreenShake(pev->origin, 32.0f, 8.0f, 8.0f, 500.0f);

		SetThink(&CShubNiggurath::DeathThink);
		m_fDelayTime = gpGlobals->time + 5.5f; //time before explode

		LIGHT_STYLE(0, "mkkigecacegikmm");	// apply to world
		SetActivity(ACT_USE);	// shake
		ResetSequenceInfo();
	}
}

void CShubNiggurath::DeathThink(void)
{
	float flInterval = StudioFrameAdvance(1.0f);
	pev->nextthink = gpGlobals->time + 0.1f;
	UTIL_ScreenShake(pev->origin, 32.0f, 8.0f, 1.0f, 500.0f);

	/*if (m_fSequenceFinished)
	{
		if( m_Activity == ACT_USE )
		{
			SetThink( NULL );
			Finale4();
			return;
		}

		//ResetSequenceInfo();
		pev->frame = 0.0f;

		// play three times
		if( ++pev->impulse > 2 )
		{
			pev->sequence = 2;	// explode
			//ResetSequenceInfo();
			pev->frame = 0.0f;
		}
	}*/
	if (m_fDelayTime < gpGlobals->time)
	{
		SetThink(NULL);
		Finale4();
		return;
	}
}

void CShubNiggurath::Finale4(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "boss2/pop2.wav", 1, ATTN_NORM);

	Vector vecSrc = pev->origin;
	float x, y, z;

	pev->health = -999;

	z = 16;
	while (z <= 144)
	{
		x = -64;
		while (x <= 64)
		{
			y = -64;
			while (y <= 64)
			{
				pev->origin.x = vecSrc.x + x;
				pev->origin.y = vecSrc.y + y;
				pev->origin.z = vecSrc.z + z;

				float r = RANDOM_FLOAT(0.0f, 1.0f);
				if (r < 0.3f)
					CGib::ThrowGib("models/gib1.mdl", pev);
				else if (r < 0.6f)
					CGib::ThrowGib("models/gib2.mdl", pev);
				else
					CGib::ThrowGib("models/gib3.mdl", pev);
				y = y + 32;
			}
			x = x + 32;
		}
		z = z + 96;
	}

	// start the end text
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex(i + 1);

		if (!pPlayer) continue;
		CenterPrint(pPlayer->pev, "GameFinale");
		g_engfuncs.pfnFadeClientVolume(pPlayer->edict(), 100, 5, 150, 5);
	}

	gpWorld->killed_monsters++;

	// just an event to increase internal client counter
	MESSAGE_BEGIN(MSG_ALL, gmsgKilledMonster);
	MESSAGE_END();

	// g-cont. i can see no reason to remove the oldone and the spawn fake client entity
	// i've just replace model :-)
	SET_MODEL(ENT(pev), "sprites/player.spr");
	UTIL_SetOrigin(pev, vecSrc - Vector(32, 264, -12));

	STOP_SOUND(ENT(pev), CHAN_BODY, "ambience/rumble.wav");

	MESSAGE_BEGIN(MSG_ALL, SVC_CDTRACK);
	WRITE_BYTE(13);
	WRITE_BYTE(12);
	MESSAGE_END();

	LIGHT_STYLE(0, "m");

	SetThink(&CShubNiggurath::Finale5);
	pev->nextthink = gpGlobals->time + 40;
}

void CShubNiggurath::Finale5(void)
{
	UTIL_ScreenFadeAll(g_vecZero, 10.0f, 0.f, 255, FFADE_OUT | FFADE_STAYOUT);

	SetThink(&CShubNiggurath::Finale6);
	pev->nextthink = gpGlobals->time + 14;
}

void CShubNiggurath::Finale6(void)
{
	SERVER_COMMAND("newgame\n");
}


int CShubNiggurath::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (flDamage < 40000)
		return 0;

	pev->health -= flDamage;

	if (pev->health <= 0)
		Killed(pevAttacker, GIB_NORMAL);;

	return 0;
}