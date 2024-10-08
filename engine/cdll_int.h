/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  cdll_int.h
//
// 4-23-98  
// JOHN:  client dll interface declarations
//

#ifndef CDLL_INT_H
#define CDLL_INT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "const.h"

// this file is included by both the engine and the client-dll,
// so make sure engine declarations aren't done twice

typedef int HLSPRITE;	// handle to a graphic
typedef int (*pfnUserMsgHook)( const char *pszName, int iSize, void *pbuf );

#include "wrect.h"

#include "../common/demo_api.h"

#define SCRINFO_SCREENFLASH	1
#define SCRINFO_STRETCHED	2

typedef struct SCREENINFO_s
{
	int		iSize;
	int		iWidth;
	int		iHeight;
	int		iFlags;
	int		iCharHeight;
	short	charWidths[256];

	//WOLF - new variables for sprite weapon
#ifdef WOLF3DGAME
	int		iWeaponHeight;
	int		iWeaponWidth;
	int		iWeaponScale;
#endif

	int		iMessageWidth;
	int		iMessageHeight;
} SCREENINFO;

typedef struct client_data_s
{
	// fields that cannot be modified  (ie. have no effect if changed)
	vec3_t		origin;

	// fields that can be changed by the cldll
	vec3_t		viewangles;
	int		iWeaponBits;
	float		fov;		// field of view
} client_data_t;

typedef struct client_sprite_s
{
	char		szName[64];
	char		szSprite[64];
	int		hspr;
	int		iRes;
	wrect_t		rc;
} client_sprite_t;

typedef struct client_textmessage_s
{
	int		effect;
	byte		r1, g1, b1, a1;	// 2 colors for effects
	byte		r2, g2, b2, a2;
	float		x;
	float		y;
	float		fadein;
	float		fadeout;
	float		holdtime;
	float		fxtime;
	const char	*pName;
	const char	*pMessage;
} client_textmessage_t;

typedef struct hud_player_info_s
{
	char		*name;
	short		ping;
	byte		thisplayer;	// TRUE if this is the calling player

	// stuff that's unused at the moment,  but should be done
	byte		spectator;
	byte		packetloss;
	char		*model;
	short		topcolor;
	short		bottomcolor;
} hud_player_info_t;

typedef struct cl_enginefuncs_s
{
	// sprite handlers
	HLSPRITE	(*pfnSPR_Load)( const char *szPicName );
	int		(*pfnSPR_Frames)( HLSPRITE hPic );
	int		(*pfnSPR_Height)( HLSPRITE hPic, int frame );
	int		(*pfnSPR_Width)( HLSPRITE hPic, int frame );
	void	(*pfnSPR_Set)( HLSPRITE hPic, int r, int g, int b );
	void	(*pfnSPR_SetAlpha)(HLSPRITE hPic, int r, int g, int b, int a); //DOOM
	void	(*pfnSPR_Draw)( int frame, int x, int y, const wrect_t *prc );
	void	(*pfnSPR_DrawHoles)( int frame, int x, int y, const wrect_t *prc );
	void	(*pfnSPR_DrawAdditive)( int frame, int x, int y, const wrect_t *prc );
	void	(*pfnSPR_DrawHolesWeapon)(int frame, int x, int y, const wrect_t* prc); //WOLF
	void	(*pfnSPR_EnableScissor)( int x, int y, int width, int height );
	void	(*pfnSPR_DisableScissor)( void );
	client_sprite_t *(*pfnSPR_GetList)( char *psz, int *piCount );

	// screen handlers
	void	(*pfnFillRGBA)( int x, int y, int width, int height, int r, int g, int b, int a );
	int		(*pfnGetScreenInfo)( SCREENINFO *pscrinfo );
	void	(*pfnSetCrosshair)( HLSPRITE hspr, wrect_t rc, int r, int g, int b );

	// cvar handlers
	struct cvar_s *(*pfnRegisterVariable)( char *szName, char *szValue, int flags );
	float	(*pfnGetCvarFloat)( char *szName );
	char*	(*pfnGetCvarString)( char *szName );

	// command handlers
	int	(*pfnAddCommand)( char *cmd_name, void (*function)(void) );
	int	(*pfnHookUserMsg)( char *szMsgName, pfnUserMsgHook pfn );
	int	(*pfnServerCmd)( char *szCmdString );
	int	(*pfnClientCmd)( char *szCmdString );

	void	(*pfnGetPlayerInfo)( int ent_num, hud_player_info_t *pinfo );

	// sound handlers
	void	(*pfnPlaySoundByName)( char *szSound, float volume );
	void	(*pfnPlaySoundByIndex)( int iSound, float volume );

	// vector helpers
	void	(*pfnAngleVectors)( const float *vecAngles, float *forward, float *right, float *up );

	// text message system
	client_textmessage_t *(*pfnTextMessageGet)( const char *pName );
	int	(*pfnDrawCharacter)( int x, int y, int number, int r, int g, int b );
	int	(*pfnDrawConsoleString)( int x, int y, char *string );
	void	(*pfnDrawSetTextColor)( float r, float g, float b );
	void	(*pfnDrawConsoleStringLen)(  const char *string, int *length, int *height );

	void	(*pfnConsolePrint)( const char *string );
	void	(*pfnCenterPrint)( const char *string );

	// Added for user input processing
	int	(*GetWindowCenterX)( void );
	int	(*GetWindowCenterY)( void );
	void	(*GetViewAngles)( float * );
	void	(*SetViewAngles)( float * );
	int	(*GetMaxClients)( void );
	void	(*Cvar_SetValue)( char *cvar, float value );

	int       (*Cmd_Argc)( void );	
	char	*(*Cmd_Argv)( int arg );
	void	(*Con_Printf)( char *fmt, ... );
	void	(*Con_DPrintf)( char *fmt, ... );
	void	(*Con_NPrintf)( int pos, char *fmt, ... );
	void	(*Con_NXPrintf)( struct con_nprint_s *info, char *fmt, ... );

	const char* (*PhysInfo_ValueForKey)( const char *key );
	const char* (*ServerInfo_ValueForKey)( const char *key );
	float	(*GetClientMaxspeed)( void );
	int	(*CheckParm)( char *parm, char **ppnext );

	void	(*Key_Event)( int key, int down );
	void	(*GetMousePosition)( int *mx, int *my );
	int	(*IsNoClipping)( void );

	struct cl_entity_s *(*GetLocalPlayer)( void );
	struct cl_entity_s *(*GetViewModel)( void );
	struct cl_entity_s *(*GetEntityByIndex)( int idx );

	float	(*GetClientTime)( void );
	void	(*V_CalcShake)( void );
	void	(*V_ApplyShake)( float *origin, float *angles, float factor );

	int	(*PM_PointContents)( float *point, int *truecontents );
	int	(*PM_WaterEntity)( float *p );
	struct pmtrace_s *(*PM_TraceLine)( float *start, float *end, int flags, int usehull, int ignore_pe );

	struct model_s *(*CL_LoadModel)( const char *modelname, int *index );
	int	(*CL_CreateVisibleEntity)( int type, struct cl_entity_s *ent );

	const struct model_s* (*GetSpritePointer)( HLSPRITE HLSPRITE );
	void	(*pfnPlaySoundByNameAtLocation)( char *szSound, float volume, float *origin );
	
	unsigned short (*pfnPrecacheEvent)( int type, const char* psz );
	void	(*pfnPlaybackEvent)( int flags, const struct edict_s *pInvoker, unsigned short eventindex, float delay, float *origin, float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
	void	(*pfnWeaponAnim)( int iAnim, int body );
	float	(*pfnRandomFloat)( float flLow, float flHigh );	
	long	(*pfnRandomLong)( long lLow, long lHigh );
	void	(*pfnHookEvent)( char *name, void ( *pfnEvent )( struct event_args_s *args ));
	int	(*Con_IsVisible) ();
	const char *(*pfnGetGameDirectory)( void );
	struct cvar_s *(*pfnGetCvarPointer)( const char *szName );
	const char *(*Key_LookupBinding)( const char *pBinding );
	const char *(*pfnGetLevelName)( void );
	void	(*pfnGetScreenFade)( struct screenfade_s *fade );
	void	(*pfnSetScreenFade)( struct screenfade_s *fade );
	void*	(*VGui_GetPanel)( );
	void	(*VGui_ViewportPaintBackground)( int extents[4] );

	byte*	(*COM_LoadFile)( char *path, int usehunk, int *pLength );
	char*	(*COM_ParseFile)( char *data, char *token );
	void	(*COM_FreeFile)( void *buffer );

	struct triangleapi_s	*pTriAPI;
	struct efx_api_s		*pEfxAPI;
	struct event_api_s		*pEventAPI;	
	struct demo_api_s		*pDemoAPI;
	struct net_api_s		*pNetAPI;
	struct IVoiceTweak_s	*pVoiceTweak;

	// returns 1 if the client is a spectator only (connected to a proxy), 0 otherwise or 2 if in dev_overview mode	
	int	(*IsSpectateOnly)( void );
	struct model_s *(*LoadMapSprite)( const char *filename );

	// file search functions
	void	 (*COM_AddAppDirectoryToSearchPath)( const char *pszBaseDir, const char *appName );
	int	 (*COM_ExpandFilename)( const char *fileName, char *nameOutBuffer, int nameOutBufferSize );

	// User info
	// playerNum is in the range (1, MaxClients)
	// returns NULL if player doesn't exit
	// returns "" if no value is set
	const char *( *PlayerInfo_ValueForKey )( int playerNum, const char *key );
	void	(*PlayerInfo_SetValueForKey )( const char *key, const char *value );

	// Gets a unique ID for the specified player. This is the same even if you see the player on a different server.
	// iPlayer is an entity index, so client 0 would use iPlayer=1.
	// Returns false if there is no player on the server in the specified slot.
	qboolean	(*GetPlayerUniqueID)(int iPlayer, char playerID[16]);

	// TrackerID access
	int	(*GetTrackerIDForPlayer)(int playerSlot);
	int	(*GetPlayerForTrackerID)(int trackerID);

	// Same as pfnServerCmd, but the message goes in the unreliable stream so it can't clog the net stream
	// (but it might not get there).
	int	( *pfnServerCmdUnreliable )( char *szCmdString );

	void	(*pfnGetMousePos)( struct tagPOINT *ppt );
	void	(*pfnSetMousePos)( int x, int y );
	void	(*pfnSetMouseEnable)( qboolean fEnable );
	void	(*pfnUnused1)( void );
	void	(*pfnUnused2)( void );
	void	(*pfnUnused3)( void );
	void	(*pfnUnused4)( void );
	float	(*pfnGetClientOldTime)( void );
	float	(*pfnGetGravity)( void );
	struct model_s*(*pfnGetModelByIndex)( int index );
	void	(*pfnUnused5)( void );
	void	(*pfnUnused6)( void );
	void	(*pfnUnused7)( void );
	void	(*pfnUnused8)( void );
	void	(*pfnUnused9)( void );
	void	(*pfnUnused10)( void );
	void	(*pfnUnused11)( void );
	void	(*pfnUnused12)( void );
	const char*(*LocalPlayerInfo_ValueForKey)( const char* key );
	void	(*pfnUnused13)( void );
	void	(*pfnUnused14)( void );
	void	(*pfnUnused15)( void );
	void	(*pfnUnused16)( void );
	void	(*Cvar_Set)( char *name, char *value );
	void	(*pfnUnused17)( void );
	void	(*pfnUnused18)( void );
	void	(*pfnUnused19)( void );
	double	(*pfnSys_FloatTime)( void );
	void	(*pfnUnused20)( void );
	void	(*pfnUnused21)( void );
	void	(*pfnUnused22)( void );
	void	(*pfnUnused23)( void );
	void	(*pfnFillRGBABlend)( int x, int y, int width, int height, int r, int g, int b, int a );
	int	(*pfnGetAppID)( void );
	void	(*pfnUnused24)( void );
	void	(*pfnUnused25)( void );
} cl_enginefunc_t;

#define CLDLL_INTERFACE_VERSION	7

#ifdef __cplusplus
}
#endif

#endif//CDLL_INT_H