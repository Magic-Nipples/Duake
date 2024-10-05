//
// singleplayer_gamerules
//
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"items.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;

//=========================================================
//=========================================================
CQuakeRules::CQuakeRules( void )
{
	RefreshSkillData();
}

//=========================================================
//=========================================================
void CQuakeRules::Think ( void )
{
}

//=========================================================
//=========================================================
BOOL CQuakeRules::IsMultiplayer( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CQuakeRules::IsDeathmatch ( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CQuakeRules::IsCoOp( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CQuakeRules :: ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{
	return TRUE;
}

void CQuakeRules :: InitHUD( CBasePlayer *pl )
{
}

//=========================================================
//=========================================================
void CQuakeRules :: ClientDisconnected( edict_t *pClient )
{
}

//=========================================================
//=========================================================
float CQuakeRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	return 5;
}

//=========================================================
//=========================================================
void CQuakeRules :: PlayerSpawn( CBasePlayer *pPlayer )
{
	// Start with init ammoload
	pPlayer->ammo_nails = 50; // pistol starting ammo

	// Start with shotgun and axe
	pPlayer->m_iItems = (IT_AXE | IT_PISTOL );
	pPlayer->m_iCueWeapon = pPlayer->m_iWeapon = pPlayer->W_BestWeapon();

	if (!g_isDead && !g_changelevel)
	{
		g_StoreRune = 0;
		g_RespawnParams[PARM_HEALTH] = pPlayer->pev->health;
		g_RespawnParams[PARM_NAILS] = pPlayer->ammo_nails;
		g_RespawnParams[PARM_CURWEAPON] = pPlayer->m_iCueWeapon;
		g_RespawnParams[PARM_ITEMS] = pPlayer->m_iItems;
	}

	if(g_isDead)
		pPlayer->DecodeRespawnParms();
	else
		pPlayer->DecodeLevelParms();
	pPlayer->W_SetCurrentAmmo();
}

//=========================================================
//=========================================================
BOOL CQuakeRules :: AllowAutoTargetCrosshair( void )
{
	return ( g_iSkillLevel == SKILL_EASY );
}

//=========================================================
//=========================================================
void CQuakeRules :: PlayerThink( CBasePlayer *pPlayer )
{
}


//=========================================================
//=========================================================
BOOL CQuakeRules :: FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CQuakeRules :: FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CQuakeRules :: IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}

//=========================================================
//=========================================================
BOOL CQuakeRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CQuakeRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CQuakeRules::ItemShouldRespawn( CItem *pItem )
{
	return GR_ITEM_RESPAWN_NO;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CQuakeRules::FlItemRespawnTime( CItem *pItem )
{
	return -1;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CQuakeRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
BOOL CQuakeRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	if( g_iSkillLevel == SKILL_EASY && ( pEntity->pev->spawnflags & SF_NOT_EASY ))
		return FALSE;
	else if( g_iSkillLevel == SKILL_MEDIUM && ( pEntity->pev->spawnflags & SF_NOT_MEDIUM ))
		return FALSE;
	else if( g_iSkillLevel >= SKILL_HARD && ( pEntity->pev->spawnflags & SF_NOT_HARD ))
		return FALSE;

	return TRUE;
}

//=========================================================
//=========================================================
float CQuakeRules::FlHealthChargerRechargeTime( void )
{
	return 0;// don't recharge
}

//=========================================================
//=========================================================
int CQuakeRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CQuakeRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_NO;
}

//=========================================================
//=========================================================
int CQuakeRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// why would a single player need this? 
	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
BOOL CQuakeRules :: FAllowMonsters( void )
{
	return TRUE;
}
