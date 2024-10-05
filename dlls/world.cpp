/*

===== world.cpp ========================================================

  precaches and defs for entities and other data that must always be available.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "client.h"
#include "decals.h"
#include "skill.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

extern CBaseEntity		*g_pLastSpawn;
DLL_GLOBAL edict_t		*g_pBodyQueueHead;
extern DLL_GLOBAL	int	gDisplayTitle, g_iWorldType;
extern DLL_GLOBAL BOOL	g_fXashEngine;
CWorld			*gpWorld;

extern void W_Precache(void);

//
// This must match the list in util.h
//
DLL_DECALLIST gDecals[] = {
	{ "{shot1", 0 },		// DECAL_GUNSHOT1 
	{ "{shot2", 0 },		// DECAL_GUNSHOT2
	{ "{shot3",0 },			// DECAL_GUNSHOT3
	{ "{shot4", 0 },		// DECAL_GUNSHOT4
	{ "{shot5", 0 },		// DECAL_GUNSHOT5
	{ "{scorch1", 0 },		// DECAL_SCORCH1
	{ "{scorch2", 0 },		// DECAL_SCORCH2
	{ "{blood1", 0 },		// DECAL_BLOOD1
	{ "{blood2", 0 },		// DECAL_BLOOD2
	{ "{blood3", 0 },		// DECAL_BLOOD3
	{ "{blood4", 0 },		// DECAL_BLOOD4
	{ "{blood5", 0 },		// DECAL_BLOOD5
	{ "{blood6", 0 },		// DECAL_BLOOD6
	{ "{yblood1", 0 },		// DECAL_YBLOOD1
	{ "{yblood2", 0 },		// DECAL_YBLOOD2
	{ "{yblood3", 0 },		// DECAL_YBLOOD3
	{ "{yblood4", 0 },		// DECAL_YBLOOD4
	{ "{yblood5", 0 },		// DECAL_YBLOOD5
	{ "{yblood6", 0 },		// DECAL_YBLOOD6
	{ "{spit1", 0 },		// DECAL_SPIT1
	{ "{spit2", 0 },		// DECAL_SPIT2
	{ "{smscorch1", 0 },	// DECAL_SMORCH1
	{ "{smscorch2", 0 },	// DECAL_SMORCH2
	{ "{tarsplat1", 0 },	// DECAL_TARS1
	{ "{tarsplat2", 0 },	// DECAL_TARS2
	{ "{tarsplat3", 0 },	// DECAL_TARS3
	{ "{tarsplat4", 0 },	// DECAL_TARS4
};

void ClientPrecache( void )
{
// sounds used from C physics code
	PRECACHE_SOUND ("demon/dland2.wav");		// landing thud
	PRECACHE_SOUND ("misc/h2ohit1.wav");		// landing splash

	// setup precaches always needed
	PRECACHE_SOUND ("items/itembk2.wav");		// item respawn sound	
	PRECACHE_SOUND ("player/use1.wav");		// player landing
	PRECACHE_SOUND ("player/land2.wav");		// player hurt landing
	PRECACHE_SOUND ("player/drown1.wav");		// drowning pain
	PRECACHE_SOUND ("player/drown2.wav");		// drowning pain
	PRECACHE_SOUND ("player/gasp1.wav");		// gasping for air
	PRECACHE_SOUND ("player/gasp2.wav");		// taking breath
	PRECACHE_SOUND ("player/h2odeath.wav");		// drowning death

	//jump
	PRECACHE_SOUND("player/jump1.wav");		// player jump
	PRECACHE_SOUND("player/jump_ft1.wav");
	PRECACHE_SOUND("player/jump_ft2.wav");

	PRECACHE_SOUND ("misc/talk.wav");		// talk
	PRECACHE_SOUND ("player/teledth1.wav");		// telefrag
	//PRECACHE_SOUND ("misc/r_tele1.wav");		// teleport sounds
	//PRECACHE_SOUND ("misc/r_tele2.wav");
	//PRECACHE_SOUND ("misc/r_tele3.wav");
	//PRECACHE_SOUND ("misc/r_tele4.wav");
	//PRECACHE_SOUND ("misc/r_tele5.wav");
	PRECACHE_SOUND("misc/teleport.wav");

	PRECACHE_SOUND ("items/damage3.wav");

	PRECACHE_SOUND ("misc/power.wav");		//lightning for boss

	PRECACHE_SOUND("player/swim1.wav");		// breathe bubbles
	PRECACHE_SOUND("player/swim2.wav");
	PRECACHE_SOUND("player/swim3.wav");
	PRECACHE_SOUND("player/swim4.wav");

// player gib sounds
	PRECACHE_SOUND ("player/gib.wav");		// player gib sound
	PRECACHE_SOUND ("player/udeath.wav");		// player gib sound
	PRECACHE_SOUND ("player/tornoff2.wav");		// gib sound

// player pain sounds
	PRECACHE_SOUND ("player/pain1.wav");

// player death sounds
	PRECACHE_SOUND ("player/die1.wav");
	PRECACHE_SOUND ("player/die2.wav");

// ax sounds	
	PRECACHE_SOUND ("weapons/ax1.wav");			// ax swoosh
	PRECACHE_SOUND ("player/axhit1.wav");		// ax hit meat
	PRECACHE_SOUND ("weapons/punch.wav");		// ax hit world

	PRECACHE_SOUND ("player/h2ojump.wav");		// player jumping into water
	PRECACHE_SOUND ("player/slimbrn2.wav");		// player enter slime
	PRECACHE_SOUND ("player/inh2o.wav");		// player enter water
	PRECACHE_SOUND ("player/inlava.wav");		// player enter lava
	PRECACHE_SOUND ("misc/outwater.wav");		// leaving water sound

	PRECACHE_SOUND ("player/lburn1.wav");		// lava burn
	PRECACHE_SOUND ("player/lburn2.wav");		// lava burn

	PRECACHE_SOUND ("misc/water1.wav");		// swimming
	PRECACHE_SOUND ("misc/water2.wav");		// swimming

	PRECACHE_MODEL("sprites/player.spr");
	PRECACHE_MODEL("progs/eyes.mdl");
	PRECACHE_MODEL("sprites/s_bubble.spr");

	PRECACHE_SOUND("player/pl_wade1.wav");		// wade in water
	PRECACHE_SOUND("player/pl_wade2.wav");
	PRECACHE_SOUND("player/pl_wade3.wav");
	PRECACHE_SOUND("player/pl_wade4.wav");
}

// called by worldspawn
void W_Precache(void)
{
	g_sModelIndexBubbles = PRECACHE_MODEL ("sprites/bubble.spr");//bubbles
	//g_sModelIndexBloodSpray = PRECACHE_MODEL ("sprites/bloodspray.spr"); // initial blood
	//g_sModelIndexBloodDrop = PRECACHE_MODEL ("sprites/blood.spr"); // splattered blood 

	PRECACHE_MODEL("sprites/s_explod.spr");

	// lightning
	PRECACHE_MODEL("progs/bolt.mdl");
	PRECACHE_MODEL("progs/bolt2.mdl");
	PRECACHE_MODEL("progs/bolt3.mdl");

	// used by explosions
	PRECACHE_MODEL ("models/grenade.mdl");
	PRECACHE_MODEL ("models/missile.mdl");
	PRECACHE_MODEL ("progs/spike.mdl");
	PRECACHE_MODEL ("progs/backpack.mdl");

	PRECACHE_MODEL("models/gib1.mdl");
	PRECACHE_MODEL("models/gib2.mdl");
	PRECACHE_MODEL("models/gib3.mdl");

	// Weapon sounds
	PRECACHE_SOUND("player/axhit1.wav");
	PRECACHE_SOUND("weapons/explode1.wav");			// new rocket explosion
	
	
	PRECACHE_SOUND("weapons/rocket_launch.wav");
	PRECACHE_SOUND("weapons/ric1.wav");    // ricochet (used in c code)
	PRECACHE_SOUND("weapons/ric2.wav");    // ricochet (used in c code)
	PRECACHE_SOUND("weapons/ric3.wav");    // ricochet (used in c code)
	PRECACHE_SOUND("weapons/spike2.wav");  // super spikes
	PRECACHE_SOUND("weapons/tink1.wav");   // spikes tink (used in c code)
	PRECACHE_SOUND("weapons/grenade.wav"); // grenade launcher
	PRECACHE_SOUND("weapons/bounce.wav");  // grenade bounce
	PRECACHE_SOUND("weapons/supershot_fire.wav"); // super shotgun

	PRECACHE_SOUND( "items/damage.wav" );
	PRECACHE_SOUND( "items/damage2.wav" );
	PRECACHE_SOUND( "items/damage3.wav" );

	//weapon models
	PRECACHE_MODEL("models/null.mdl"); //DOOM - we need a view model to get lighting info so just make it null

	//doom weapon sounds
	PRECACHE_SOUND("weapons/pistol_fire.wav");
	PRECACHE_SOUND("weapons/shotgun_fire.wav");
	PRECACHE_SOUND("weapons/supershot_fire.wav");
	PRECACHE_SOUND("weapons/chaingun_fire.wav");
	PRECACHE_SOUND("weapons/plasma_fire.wav");
	PRECACHE_SOUND("weapons/plasma_stop.wav");
	PRECACHE_SOUND("weapons/chainsaw_hit.wav");
	PRECACHE_SOUND("weapons/chainsaw_rev.wav");
	PRECACHE_SOUND("weapons/bfg_fire.wav");
	PRECACHE_SOUND("weapons/bfg_hit.wav");

	//doom item sounds
	PRECACHE_SOUND("items/pickup_item.wav");
	PRECACHE_SOUND("items/pickup_powerup.wav");
	PRECACHE_SOUND("items/pickup_weapon.wav");

	//doom sprites
	PRECACHE_MODEL("sprites/items/health-misc.spr");
	PRECACHE_MODEL("sprites/weapons/plasma.spr");
	PRECACHE_MODEL("sprites/weapons/bfgball.spr");
	PRECACHE_MODEL("sprites/weapons/bfghit.spr");
	PRECACHE_MODEL("sprites/teleport.spr");
}

/*
==============================================================================

BODY QUE

==============================================================================
*/
// Body queue class here.... It's really just CBaseEntity
class CCorpse : public CBaseEntity
{
	virtual int ObjectCaps( void ) { return FCAP_DONT_SAVE; }	
};

LINK_ENTITY_TO_CLASS( bodyque, CCorpse );

static void InitBodyQue(void)
{
	string_t	istrClassname = MAKE_STRING("bodyque");

	g_pBodyQueueHead = CREATE_NAMED_ENTITY( istrClassname );
	entvars_t *pev = VARS(g_pBodyQueueHead);
	
	// Reserve 3 more slots for dead bodies
	for ( int i = 0; i < 3; i++ )
	{
		pev->owner = CREATE_NAMED_ENTITY( istrClassname );
		pev = VARS(pev->owner);
	}
	
	pev->owner = g_pBodyQueueHead;
}


//
// make a body que entry for the given ent so the ent can be respawned elsewhere
//
// GLOBALS ASSUMED SET:  g_eoBodyQueueHead
//
void CopyToBodyQue(entvars_t *pev) 
{
	if (pev->effects & EF_NODRAW)
		return;

	entvars_t *pevHead	= VARS(g_pBodyQueueHead);

	pevHead->angles		= pev->angles;
	pevHead->model		= pev->model;
	pevHead->modelindex	= pev->modelindex;
	pevHead->frame		= pev->frame;
	pevHead->colormap	= pev->colormap;
	pevHead->movetype	= MOVETYPE_TOSS;
	pevHead->velocity	= pev->velocity;
	pevHead->flags		= 0;
	pevHead->deadflag	= pev->deadflag;
	pevHead->renderfx	= kRenderFxDeadPlayer;
	pevHead->renderamt	= ENTINDEX( ENT( pev ) );

	pevHead->effects    = pev->effects | EF_NOINTERP;
	pevHead->sequence = pev->sequence;
	pevHead->animtime = pev->animtime;

	UTIL_SetOrigin(pevHead, pev->origin);
	UTIL_SetSize(pevHead, pev->mins, pev->maxs);
	g_pBodyQueueHead = pevHead->owner;
}



// moved CWorld class definition to cbase.h
//=======================
// CWorld
//
// This spawns first when each level begins.
//=======================

LINK_ENTITY_TO_CLASS( worldspawn, CWorld );

extern DLL_GLOBAL BOOL		g_fGameOver;
float g_flWeaponCheat; 

#define SF_MESSAGE_SHOWN		1

void CWorld :: Spawn( void )
{
	g_fGameOver = FALSE;
	Precache( );

	serverflags = g_levelParams[PARM_SERVERFLAGS];
}

TYPEDESCRIPTION CWorld::m_SaveData[] = 
{
	DEFINE_FIELD(CWorld, serverflags, FIELD_INTEGER),
	DEFINE_FIELD(CWorld, total_secrets, FIELD_INTEGER),
	DEFINE_FIELD(CWorld, total_monsters, FIELD_INTEGER),
	DEFINE_FIELD(CWorld, found_secrets, FIELD_INTEGER),
	DEFINE_FIELD(CWorld, killed_monsters, FIELD_INTEGER),
	DEFINE_FIELD(CWorld, levelname, FIELD_STRING),
}; IMPLEMENT_SAVERESTORE( CWorld, CBaseEntity );

void CWorld :: Precache( void )
{
	gpWorld = this;	// setup the global world pointer
	g_pLastSpawn = NULL;

	// reset intermission stuff here
	g_intermission_running = 0;
	g_intermission_exittime = 0;

	g_sNextMap[0] = '\0';

	if( FStrEq( STRING( pev->model ), "maps/e1m8.bsp" ))	
		CVAR_SET_STRING("sv_gravity", "100"); // 8.4 ft/sec
	else CVAR_SET_STRING("sv_gravity", "800"); // 67 ft/sec

	CVAR_SET_STRING("sv_stepsize", "18");
	CVAR_SET_STRING("room_type", "0");// clear DSP
	//CVAR_SET_FLOAT("v_dark", 1.0f);

	// Set up game rules
	if (g_pGameRules)
		delete g_pGameRules;

	g_pGameRules = InstallGameRules( );

	InitBodyQue();

	// player precaches     
	W_Precache ();									// get weapon precaches

	ClientPrecache();
//
// Setup light animation tables. 'a' is total darkness, 'z' is maxbright.
//
	// 0 normal
	LIGHT_STYLE(0, "m");
	
	// 1 FLICKER (first variety)
	LIGHT_STYLE(1, "mmnmmommommnonmmonqnmmo");
	
	// 2 SLOW STRONG PULSE
	LIGHT_STYLE(2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	
	// 3 CANDLE (first variety)
	LIGHT_STYLE(3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	
	// 4 FAST STROBE
	LIGHT_STYLE(4, "mamamamamama");
	
	// 5 GENTLE PULSE 1
	LIGHT_STYLE(5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	
	// 6 FLICKER (second variety)
	LIGHT_STYLE(6, "nmonqnmomnmomomno");
	
	// 7 CANDLE (second variety)
	LIGHT_STYLE(7, "mmmaaaabcdefgmmmmaaaammmaamm");
	
	// 8 CANDLE (third variety)
	LIGHT_STYLE(8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	
	// 9 SLOW STROBE (fourth variety)
	LIGHT_STYLE(9, "aaaaaaaazzzzzzzz");
	
	// 10 FLUORESCENT FLICKER
	LIGHT_STYLE(10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	LIGHT_STYLE(11, "abcdefghijklmnopqrrqponmlkjihgfedcba");
	
	// 12 UNDERWATER LIGHT MUTATION
	// this light only distorts the lightmap - no contribution
	// is made to the brightness of affected surfaces
	LIGHT_STYLE(12, "mmnnmmnnnmmnn");
	
	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	LIGHT_STYLE(63, "a");

	for ( int i = 0; i < ARRAYSIZE(gDecals); i++ )
		gDecals[i].index = DECAL_INDEX( gDecals[i].name );

	if ( pev->speed > 0 )
		CVAR_SET_FLOAT( "sv_zmax", pev->speed );
	else
		CVAR_SET_FLOAT( "sv_zmax", 4096 );

	// g-cont. moved here to right restore global WaveHeight on save\restore level
	CVAR_SET_FLOAT( "sv_wateramp", pev->scale );

	if (!FBitSet( pev->spawnflags, SF_MESSAGE_SHOWN ))
	{
		if ( pev->message && !(FStrEq( STRING( pev->model ), "maps/start.bsp" ) && g_levelParams[PARM_SERVERFLAGS] ))
		{
			ALERT( at_aiconsole, "Chapter title: %s\n", STRING(pev->netname) );
			pev->spawnflags |= SF_MESSAGE_SHOWN;
			levelname = pev->message;
			pev->nextthink = gpGlobals->time + 0.3;
		}
	}

	// g-cont. moved here so cheats still working on restore level
	g_flWeaponCheat = CVAR_GET_FLOAT( "sv_cheats" );  // Is the impulse 9 command allowed?

	// g-cont. share worldtype
	g_iWorldType = pev->button;

	if( g_fXashEngine )
	{
		if( !_stricmp( CVAR_GET_STRING( "sv_skyname" ), "desert" ))
			CVAR_SET_STRING( "sv_skyname", "" );
	}
	else
	{
		if (g_iWorldType == WORLDTYPE_PRESENT)
			CVAR_SET_STRING( "sv_skyname", "dmcp" );
		else CVAR_SET_STRING( "sv_skyname", "dmcw" );
	}

	if( g_iXashEngineBuildNumber >= 2009 )
		UPDATE_PACKED_FOG( pev->impulse );
}

void CWorld :: Think( void )
{
	UTIL_ShowMessageAll( STRING(pev->message) );
	pev->nextthink = -1;
}

//
// Just to ignore the "wad" field.
//
void CWorld :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq(pkvd->szKeyName, "skyname") || FStrEq(pkvd->szKeyName, "sky"))
	{
		// Sent over net now.
		CVAR_SET_STRING( "sv_skyname", pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "sounds") )
	{
		gpGlobals->cdAudioTrack = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "fog") || FStrEq(pkvd->szKeyName, "fog_"))
	{
		float fog_settings[4];
		int packed_fog[4];
		UTIL_StringToFloatArray( fog_settings, 4, pkvd->szValue );

		for( int i = 0; i < 4; i++)
			packed_fog[i] = fog_settings[i] * 255;

		// temporare place for store fog settings
		pev->impulse = (packed_fog[1]<<24)|(packed_fog[2]<<16)|(packed_fog[3]<<8)|packed_fog[0];
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "WaveHeight") )
	{
		// Sent over net now.
		pev->scale = atof(pkvd->szValue) * (1.0/8.0);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "MaxRange") )
	{
		pev->speed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "worldtype") )
	{
		// always reset globals
		CVAR_SET_FLOAT( "sv_newunit", 1 );
		pev->button = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "floornumber"))
	{
		// always reset globals
		CVAR_SET_FLOAT("_level", atoi(pkvd->szValue));
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


#define SF_DECAL_NOTINDEATHMATCH		2048

class CDecal : public CBaseEntity
{
public:
	void	Spawn(void);
	void	KeyValue(KeyValueData* pkvd);
	void	EXPORT StaticDecal(void);
	void	EXPORT TriggerDecal(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(infodecal, CDecal);

// UNDONE:  These won't get sent to joining players in multi-player
void CDecal::Spawn(void)
{
	if (pev->skin < 0 || (gpGlobals->deathmatch && FBitSet(pev->spawnflags, SF_DECAL_NOTINDEATHMATCH)))
	{
		REMOVE_ENTITY(ENT(pev));
		return;
	}

	if (FStringNull(pev->targetname))
	{
		SetThink(&CDecal::StaticDecal);
		// if there's no targetname, the decal will spray itself on as soon as the world is done spawning.
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		// if there IS a targetname, the decal sprays itself on when it is triggered.
		SetThink(&CDecal::SUB_DoNothing);
		SetUse(&CDecal::TriggerDecal);
	}
}

void CDecal::TriggerDecal(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// this is set up as a USE function for infodecals that have targetnames, so that the
	// decal doesn't get applied until it is fired. (usually by a scripted sequence)
	TraceResult trace;
	int			entityIndex;

	UTIL_TraceLine(pev->origin - Vector(5, 5, 5), pev->origin + Vector(5, 5, 5), ignore_monsters, ENT(pev), &trace);

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BSPDECAL);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_SHORT((int)pev->skin);
	entityIndex = (short)ENTINDEX(trace.pHit);
	WRITE_SHORT(entityIndex);
	if (entityIndex)
		WRITE_SHORT((int)VARS(trace.pHit)->modelindex);
	MESSAGE_END();

	SetThink(&CDecal::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1;
}


void CDecal::StaticDecal(void)
{
	TraceResult trace;
	int			entityIndex, modelIndex;

	UTIL_TraceLine(pev->origin - Vector(5, 5, 5), pev->origin + Vector(5, 5, 5), ignore_monsters, ENT(pev), &trace);

	entityIndex = (short)ENTINDEX(trace.pHit);
	if (entityIndex)
		modelIndex = (int)VARS(trace.pHit)->modelindex;
	else
		modelIndex = 0;

	g_engfuncs.pfnStaticDecal(pev->origin, (int)pev->skin, entityIndex, modelIndex);

	SUB_Remove();
}


void CDecal::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "texture"))
	{
		pev->skin = DECAL_INDEX(pkvd->szValue);

		// Found
		if (pev->skin >= 0)
			return;
		ALERT(at_console, "Can't find decal %s\n", pkvd->szValue);
	}
	else
		CBaseEntity::KeyValue(pkvd);
}
