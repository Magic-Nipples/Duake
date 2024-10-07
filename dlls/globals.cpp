/*

===== globals.cpp ========================================================

  DLL-wide global variable definitions.
  They're all defined here, for convenient centralization.
  Source files that need them should "extern ..." declare each
  variable, to better document what globals they care about.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

DLL_GLOBAL ULONG		g_ulFrameCount;
DLL_GLOBAL ULONG		g_ulModelIndexEyes;
DLL_GLOBAL ULONG		g_ulModelIndexPlayer;
DLL_GLOBAL Vector		g_vecAttackDir;
DLL_GLOBAL int		g_iSkillLevel;
DLL_GLOBAL int		gDisplayTitle;
DLL_GLOBAL int		g_iWorldType;
DLL_GLOBAL BOOL		g_fGameOver;
DLL_GLOBAL BOOL		g_fXashEngine;
DLL_GLOBAL const Vector	g_vecZero = Vector(0,0,0);
DLL_GLOBAL const Vector	g_bonusColor = Vector( 215, 186, 69 );
DLL_GLOBAL int		g_levelParams[10];	// changelevel params
DLL_GLOBAL BOOL		g_changelevel = FALSE;
DLL_GLOBAL BOOL		g_isDead = FALSE;
DLL_GLOBAL int		g_RespawnParams[10];
DLL_GLOBAL int		g_StoreRune = 0;
DLL_GLOBAL int		g_intermission_running;
DLL_GLOBAL float		g_intermission_exittime;
DLL_GLOBAL char		g_sNextMap[64];
DLL_GLOBAL BOOL		g_registered = FALSE;
DLL_GLOBAL int		g_iXashEngineBuildNumber;
DLL_GLOBAL BOOL		g_progsFound;