//
// hud.cpp
//
// implementation of CHud class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"

extern client_sprite_t	*GetSpriteList(client_sprite_t *pList, const char *psz, int iRes, int iCount);
hud_player_info_t		g_PlayerInfoList[MAX_PLAYERS+1];	// player info from the engine
extra_player_info_t		g_PlayerExtraInfo[MAX_PLAYERS+1];	// additional player info sent directly to the client dll
team_info_t		g_TeamInfo[MAX_TEAMS+1];

extern cvar_t *sensitivity;
cvar_t *cl_lw = NULL;

void ShutdownInput (void);

int __MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_ResetHUD(pszName, iSize, pbuf );
}

int __MsgFunc_InitHUD(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_InitHUD( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_HideHUD(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_HideHUD(pszName, iSize, pbuf );
}

int __MsgFunc_ViewMode(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_ViewMode( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_Damage(const char *pszName, int iSize, void *pbuf)
{
	gHUD.MsgFunc_Damage( pszName, iSize, pbuf );
	return 1;
}

int __MsgFunc_SetFOV(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_SetFOV( pszName, iSize, pbuf );
}

int __MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	return gHUD.MsgFunc_GameMode( pszName, iSize, pbuf );
}

int __MsgFunc_TempEntity(const char *pszName, int iSize, void *pbuf)
{
	return gHUD.MsgFunc_TempEntity( pszName, iSize, pbuf );
}

int __MsgFunc_SpriteWeapon(const char* pszName, int iSize, void* pbuf)
{
	gHUD.MsgFunc_SpriteWeapon(pszName, iSize, pbuf);
	return 1;
}
 
// This is called every time the DLL is loaded
void CHud :: Init( void )
{
	memset( &nullrc, 0, sizeof( nullrc ));

	HOOK_MESSAGE( ResetHUD );
	HOOK_MESSAGE( GameMode );
	HOOK_MESSAGE( InitHUD );
	HOOK_MESSAGE( HideHUD );
	HOOK_MESSAGE( ViewMode );
	HOOK_MESSAGE( Damage );
	HOOK_MESSAGE( SetFOV );
	HOOK_MESSAGE( TempEntity );

	HOOK_MESSAGE(SpriteWeapon);

	m_iFOV = 0;

	CVAR_CREATE( "zoom_sensitivity_ratio", "1.2", 0 );
	default_fov = CVAR_CREATE( "default_fov", "80", FCVAR_ARCHIVE);
	m_pCvarStealMouse = CVAR_CREATE( "hud_capturemouse", "1", FCVAR_ARCHIVE );
	m_pCvarDraw = CVAR_CREATE( "hud_draw", "1", FCVAR_ARCHIVE );
	m_pCvarCrosshair = CVAR_CREATE( "crosshair", "1", 0 );
	cl_lw = gEngfuncs.pfnGetCvarPointer( "cl_lw" );

	CVAR_CREATE("test", "0", 0);

	CVAR_CREATE("sv_viewlag", "1", FCVAR_ARCHIVE);
	CVAR_CREATE("doom_fog", "0", FCVAR_ARCHIVE);
	CVAR_CREATE("doom_msg", "0", FCVAR_ARCHIVE);

#ifdef WOLF3DGAME
	CVAR_CREATE("_level", "1", 0);
	CVAR_CREATE("_score", "0", 0);
#endif

	m_pSpriteList = NULL;

	// Clear any old HUD list
	if ( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}

	// In case we get messages before the first update -- time will be valid
	m_flTime = 1.0;

	m_sbar.Init();
	m_SayText.Init();
	m_Message.Init();
	m_DeathNotice.Init();
	m_TextMessage.Init();
	m_Scoreboard.Init();

	MsgFunc_ResetHUD( 0, 0, NULL );
}

// CHud destructor
// cleans up memory allocated for m_rg* arrays
CHud :: ~CHud()
{
	delete [] m_rgHLSPRITEs;
	delete [] m_rgrcRects;
	delete [] m_rgszSpriteNames;

	if( m_pHudList )
	{
		HUDLIST *pList;
		while ( m_pHudList )
		{
			pList = m_pHudList;
			m_pHudList = m_pHudList->pNext;
			free( pList );
		}
		m_pHudList = NULL;
	}
}

// GetSpriteIndex()
// searches through the sprite list loaded from hud.txt for a name matching SpriteName
// returns an index into the gHUD.m_rgHLSPRITEs[] array
// returns 0 if sprite not found
int CHud :: GetSpriteIndex( const char *SpriteName )
{
	// look through the loaded sprite name list for SpriteName
	for ( int i = 0; i < m_iSpriteCount; i++ )
	{
		if ( strncmp( SpriteName, m_rgszSpriteNames + (i * MAX_SPRITE_NAME_LENGTH), MAX_SPRITE_NAME_LENGTH ) == 0 )
			return i;
	}

	return -1; // invalid sprite
}

void CHud :: VidInit( void )
{
	int j;
	m_scrinfo.iSize = sizeof(m_scrinfo);
	GetScreenInfo(&m_scrinfo);

	//if (ScreenWidth < 640)
	//	m_iRes = 320;
	//else
		m_iRes = 640;

	// Only load this once
	if ( !m_pSpriteList )
	{
		// we need to load the hud.txt, and all sprites within
		m_pSpriteList = SPR_GetList("sprites/hud.txt", &m_iSpriteCountAllRes);

		if (m_pSpriteList)
		{
			// count the number of sprites of the appropriate res
			m_iSpriteCount = 0;
			client_sprite_t *p = m_pSpriteList;
			for ( j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if ( p->iRes == m_iRes )
					m_iSpriteCount++;
				p++;
			}

			// allocated memory for sprite handle arrays
 			m_rgHLSPRITEs = new HLSPRITE[m_iSpriteCount];
			m_rgrcRects = new wrect_t[m_iSpriteCount];
			m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

			p = m_pSpriteList;
			int index = 0;
			for ( j = 0; j < m_iSpriteCountAllRes; j++ )
			{
				if ( p->iRes == m_iRes )
				{
					char sz[256];
					sprintf(sz, "sprites/%s.spr", p->szSprite);
					m_rgHLSPRITEs[index] = SPR_Load(sz);
					m_rgrcRects[index] = p->rc;
					strncpy( &m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH );

					index++;
				}

				p++;
			}
		}
	}
	else
	{
		// we have already have loaded the sprite reference from hud.txt, but
		// we need to make sure all the sprites have been loaded (we've gone through a transition, or loaded a save game)
		client_sprite_t *p = m_pSpriteList;

		// count the number of sprites of the appropriate res
		m_iSpriteCount = 0;
		for ( int j = 0; j < m_iSpriteCountAllRes; j++ )
		{
			if ( p->iRes == m_iRes )
				m_iSpriteCount++;
			p++;
		}

		delete [] m_rgHLSPRITEs;
		delete [] m_rgrcRects;
		delete [] m_rgszSpriteNames;

		// allocated memory for sprite handle arrays
 		m_rgHLSPRITEs = new HLSPRITE[m_iSpriteCount];
		m_rgrcRects = new wrect_t[m_iSpriteCount];
		m_rgszSpriteNames = new char[m_iSpriteCount * MAX_SPRITE_NAME_LENGTH];

		p = m_pSpriteList;
		int index = 0;
		for ( j = 0; j < m_iSpriteCountAllRes; j++ )
		{
			if ( p->iRes == m_iRes )
			{
				char sz[256];
				sprintf( sz, "sprites/%s.spr", p->szSprite );
				m_rgHLSPRITEs[index] = SPR_Load(sz);
				m_rgrcRects[index] = p->rc;
				strncpy( &m_rgszSpriteNames[index * MAX_SPRITE_NAME_LENGTH], p->szName, MAX_SPRITE_NAME_LENGTH );

				index++;
			}

			p++;
		}
	}

	m_sbar.VidInit();
	m_Message.VidInit();
	m_DeathNotice.VidInit();
	m_SayText.VidInit();
	m_TextMessage.VidInit();
	m_Scoreboard.VidInit();

	CL_ClearBeams();
}

int CHud::MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int newfov = READ_BYTE();
	int def_fov = CVAR_GET_FLOAT( "default_fov" );

	if ( newfov == 0 )
		m_iFOV = def_fov;
	else
		m_iFOV = newfov;

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if ( m_iFOV == def_fov ) // reset to saved sensitivity
		m_flMouseSensitivity = 0;
	else // set a new sensitivity that is proportional to the change from the FOV default
		m_flMouseSensitivity = sensitivity->value * ((float)newfov / (float)def_fov) * CVAR_GET_FLOAT("zoom_sensitivity_ratio");

	return 1;
}


void CHud::AddHudElem(CHudBase *phudelem)
{
	HUDLIST *pdl, *ptemp;

	if (!phudelem)
		return;

	pdl = (HUDLIST *)malloc(sizeof(HUDLIST));
	if (!pdl)
		return;

	memset(pdl, 0, sizeof(HUDLIST));
	pdl->p = phudelem;

	if (!m_pHudList)
	{
		m_pHudList = pdl;
		return;
	}

	ptemp = m_pHudList;

	while (ptemp->pNext)
		ptemp = ptemp->pNext;

	ptemp->pNext = pdl;
}

float CHud::GetSensitivity( void )
{
	return m_flMouseSensitivity;
}

float CHud::SmoothValues(float startValue, float endValue, float speed)
{
	float absd, d, finalValue;

	d = endValue - startValue;
	absd = fabs(d);

	if (absd > 0.01f)
	{
		if (d > 0)
			finalValue = startValue + (absd * speed);
		else
			finalValue = startValue - (absd * speed);
	}
	else
	{
		finalValue = endValue;
	}
	startValue = finalValue;

	return startValue;
}