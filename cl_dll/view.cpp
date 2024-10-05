
// view/refresh setup functions

#include "hud.h"
#include "cl_util.h"
#include "cvardef.h"
#include "usercmd.h"
#include "const.h"
#include <windows.h>
#include "entity_state.h"
#include "cl_entity.h"
#include "ref_params.h"
#include "in_defs.h" // PITCH YAW ROLL
#include "pm_movevars.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "event_api.h"
#include "pmtrace.h"
#include "screenfade.h"
#include "shake.h"
#include "hltv.h"

// Spectator Mode
extern "C" 
{
	float	vecNewViewAngles[3];
	int		iHasNewViewAngles;
	float	vecNewViewOrigin[3];
	int		iHasNewViewOrigin;
	int		iIsSpectator;
}

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

extern "C" 
{
	int CL_IsThirdPerson( void );
	void CL_CameraOffset( float *ofs );

	void DLLEXPORT V_CalcRefdef( struct ref_params_s *pparams );

	void PM_ParticleLine( float *start, float *end, int pcolor, float life, float vert);
	int		PM_GetVisEntInfo( int ent );
	int		PM_GetPhysEntInfo( int ent );
	void	InterpolateAngles(  float * start, float * end, float * output, float frac );
	void	NormalizeAngles( float * angles );
	float	Distance(const float * v1, const float * v2);
	float	AngleBetweenVectors(  const float * v1,  const float * v2 );

	float	vJumpOrigin[3];
	float	vJumpAngles[3];
}

void V_DropPunchAngle ( float frametime, float *ev_punchangle );
void VectorAngles( const float *forward, float *angles );

#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;

/*
The view is allowed to move slightly from it's true position for bobbing,
but if it exceeds 8 pixels linear distance (spherical, not box), the list of
entities sent from the server may not include everything in the pvs, especially
when crossing a water boudnary.
*/

extern cvar_t	*cl_forwardspeed;
extern cvar_t	*chase_active;
extern cvar_t	*scr_ofsx, *scr_ofsy, *scr_ofsz;
extern cvar_t	*cl_vsmoothing;

#define CAM_MODE_RELAX		1
#define CAM_MODE_FOCUS		2

vec3_t		v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles;
float		v_frametime, v_lastDistance;	
float		v_cameraRelaxAngle	= 5.0f;
float		v_cameraFocusAngle	= 35.0f;
int			v_cameraMode = CAM_MODE_FOCUS;
qboolean	v_resetCamera = 1;
qboolean  v_resetCshifts = 0;
ref_params_t *gpViewParams = NULL;

vec3_t ev_punchangle;


cshift_t cshift_empty = {{ 130,80,50 }, 0 };
cshift_t cshift_water = {{ 130,80,50 }, 128 };
cshift_t cshift_slime = {{ 0, 25, 5 }, 150 };
cshift_t cshift_lava =  {{ 255,80,0 }, 150 };

cvar_t	*scr_ofsx;
cvar_t	*scr_ofsy;
cvar_t	*scr_ofsz;

cvar_t	*v_centermove;
cvar_t	*v_centerspeed;

cvar_t	*cl_waterdist;
cvar_t	*cl_chasedist;

cvar_t* cl_bobcycle;
cvar_t* cl_bob;
cvar_t* cl_bobup;

cvar_t* cl_rollspeed;
cvar_t* cl_rollangle;

// These cvars are not registered (so users can't cheat), so set the ->value field directly
// Register these cvars in V_Init() if needed for easy tweaking
cvar_t	v_iyaw_cycle		= {"v_iyaw_cycle", "2", 0, 2};
cvar_t	v_iroll_cycle		= {"v_iroll_cycle", "0.5", 0, 0.5};
cvar_t	v_ipitch_cycle		= {"v_ipitch_cycle", "1", 0, 1};
cvar_t	v_iyaw_level		= {"v_iyaw_level", "0.3", 0, 0.3};
cvar_t	v_iroll_level		= {"v_iroll_level", "0.1", 0, 0.1};
cvar_t	v_ipitch_level		= {"v_ipitch_level", "0.3", 0, 0.3};
cvar_t	gl_cshiftpercent		= {"gl_cshiftpercent", "100", 0, 100};
cvar_t	v_kicktime		= {"v_kicktime", "0.5", 0, 0.5};
cvar_t	v_kickroll		= {"v_kickroll", "0.6", 0, 0.6};
cvar_t	v_kickpitch		= {"v_kickpitch", "0.6", 0, 0.6};

float	v_idlescale;  // used by TFC for concussion grenade effect

float	v_dmg_time, v_dmg_roll, v_dmg_pitch;
float	v_blend[4];   // rgba 0.0 - 1.0

//WOLF 3D - Doom weapon swaying
float V_CalcDoomBob ( struct ref_params_s *pparams )
{
	static	double	bobtime;
	static float	bob, bobx, boby, lasttime;
	float	cycle, cycleweap;
	vec3_t	vel;

	if (pparams->onground == -1 || pparams->time == lasttime)
		return bob;	// just use old value

	lasttime = pparams->time;
	bobtime += pparams->frametime;

	//calc cycle for view bob up/down
	cycle = bobtime - (int)(bobtime / cl_bobcycle->value * 0.25) * cl_bobcycle->value * 0.25;
	cycle /= cl_bobcycle->value * 0.25;

	if (cycle < cl_bobup->value)
		cycle = M_PI * cycle / cl_bobup->value;
	else
		cycle = M_PI + M_PI * (cycle - cl_bobup->value) / (1.0 - cl_bobup->value);

	//calc cycle for weapon model separately
	cycleweap = bobtime - (int)(bobtime / cl_bobcycle->value) * cl_bobcycle->value;
	cycleweap /= cl_bobcycle->value;

	if (cycleweap < cl_bobup->value)
		cycleweap = M_PI * cycleweap / cl_bobup->value;
	else
		cycleweap = M_PI + M_PI * (cycleweap - cl_bobup->value) / (1.0 - cl_bobup->value);

	VectorCopy(pparams->simvel, vel); vel[2] = 0; // get velocity without z axis
	//==================================================================================================
	
	if (gHUD.m_iSpriteFrame == 0 && (HOLSTER_CLIENT_IN == HOLSTER_STATE_NONE) && (pparams->onground > 0))
	{
		bobx = boby = min((sqrt(vel[0] * vel[0] + vel[1] * vel[1]) * cl_bob->value), 14.0f);

		bobx = bobx * cos(cycleweap);
		boby = boby * sin(cycleweap);

		if (gHUD.m_iSpriteCur == IT_CHAINSAW && gHUD.m_fChainsawTime < gEngfuncs.GetClientTime())
		{
			gHUD.bob_x = (int)bobx;
			gHUD.bob_y = (int)fabs(boby);
		}
		else if (gHUD.m_iSpriteCur != IT_CHAINSAW)
		{
			gHUD.bob_x = (int)bobx;
			gHUD.bob_y = (int)fabs(boby);
		}

	}
	else if (gHUD.m_iSpriteFrame == 0 && (HOLSTER_CLIENT_IN == HOLSTER_STATE_NONE))
	{
		gHUD.bob_x = gHUD.SmoothValues(gHUD.bob_x, 0, gHUD.m_flTimeDelta * 6);
		gHUD.bob_y = gHUD.SmoothValues(gHUD.bob_y, 0, gHUD.m_flTimeDelta * 6);
	}

	if (pparams->onground < 1 || pparams->waterlevel >= 2)
	{
		bob = gHUD.SmoothValues(bob, 0, gHUD.m_flTimeDelta * 6);
		return bob;
	}

	bob = min((sqrt(vel[0] * vel[0] + vel[1] * vel[1]) * cl_bob->value), 14.0f);
	bob = bob * 0.3 + bob * 0.7 * sin(cycle);

	return bob * 0.75;
}

/*
===============
V_CalcRoll
Used by view and sv_user
===============
*/
float V_CalcRoll (vec3_t angles, vec3_t velocity, float rollangle, float rollspeed )
{
    float   sign;
    float   side;
    float   value;
	vec3_t  forward, right, up;
    
	AngleVectors ( angles, forward, right, up );
    
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs( side );
    
	value = rollangle;
	if (side < rollspeed)
		side = side * value / rollspeed;
	else
		side = value;

	return side * sign;
}

typedef struct pitchdrift_s
{
	float		pitchvel;
	int			nodrift;
	float		driftmove;
	double		laststop;
} pitchdrift_t;

static pitchdrift_t pd;

void V_StartPitchDrift( void )
{
	if ( pd.laststop == gEngfuncs.GetClientTime() )
	{
		return;		// something else is keeping it from drifting
	}

	if ( pd.nodrift || !pd.pitchvel )
	{
		pd.pitchvel = v_centerspeed->value;
		pd.nodrift = 0;
		pd.driftmove = 0;
	}
}

void V_StopPitchDrift ( void )
{
	pd.laststop = gEngfuncs.GetClientTime();
	pd.nodrift = 1;
	pd.pitchvel = 0;
}

/*
===============
V_DriftPitch

Moves the client pitch angle towards idealpitch sent by the server.

If the user is adjusting pitch manually, either with lookup/lookdown,
mlook and mouse, or klook and keyboard, pitch drifting is constantly stopped.
===============
*/
void V_DriftPitch ( struct ref_params_s *pparams )
{
	float		delta, move;

	if ( gEngfuncs.IsNoClipping() || !pparams->onground || pparams->demoplayback || pparams->spectator )
	{
		pd.driftmove = 0;
		pd.pitchvel = 0;
		return;
	}

	// don't count small mouse motion
	if (pd.nodrift)
	{
		if ( fabs( pparams->cmd->forwardmove ) < cl_forwardspeed->value )
			pd.driftmove = 0;
		else
			pd.driftmove += pparams->frametime;
	
		if ( pd.driftmove > v_centermove->value)
			V_StartPitchDrift ();

		return;
	}
	
	delta = pparams->idealpitch - pparams->cl_viewangles[PITCH];

	if (!delta)
	{
		pd.pitchvel = 0;
		return;
	}

	move = pparams->frametime * pd.pitchvel;
	pd.pitchvel += pparams->frametime * v_centerspeed->value;
	
	if (delta > 0)
	{
		if (move > delta)
		{
			pd.pitchvel = 0;
			move = delta;
		}
		pparams->cl_viewangles[PITCH] += move;
	}
	else if (delta < 0)
	{
		if (move > -delta)
		{
			pd.pitchvel = 0;
			move = -delta;
		}
		pparams->cl_viewangles[PITCH] -= move;
	}
}

void V_CalcDamage( vec3_t from, int count )
{
	cl_entity_t *player = gEngfuncs.GetLocalPlayer();
	vec3_t forward, right, up;
	float side;

	VectorSubtract (from, player->origin, from);
	VectorNormalize (from);
	
	AngleVectors (player->angles, forward, right, up);

	side = DotProduct (from, right);
	//v_dmg_roll = count*side*v_kickroll.value;

	gHUD.m_fFaceDirection = side;

	//side = DotProduct (from, forward);
	//v_dmg_pitch = count*side*v_kickpitch.value;

	//v_dmg_time = v_kicktime.value;
}

/*
==================
V_cshift_f
==================
*/
void V_cshift_f (void)
{
	cshift_empty.destcolor[0] = atoi(gEngfuncs.Cmd_Argv(1));
	cshift_empty.destcolor[1] = atoi(gEngfuncs.Cmd_Argv(2));
	cshift_empty.destcolor[2] = atoi(gEngfuncs.Cmd_Argv(3));
	cshift_empty.percent = atoi(gEngfuncs.Cmd_Argv(4));
}


/*
==================
V_BonusFlash_f

When you run over an item, the server sends this command
==================
*/
void V_BonusFlash_f (void)
{
	gHUD.cshifts[CSHIFT_BONUS].destcolor[0] = 215;
	gHUD.cshifts[CSHIFT_BONUS].destcolor[1] = 186;
	gHUD.cshifts[CSHIFT_BONUS].destcolor[2] = 69;
	gHUD.cshifts[CSHIFT_BONUS].percent = 50;
}

/*
=============
V_SetContentsColor

Underwater, lava, etc each has a color shift
=============
*/
void V_SetContentsColor (int contents)
{
	switch (contents)
	{
	case CONTENTS_EMPTY:
	case CONTENTS_SOLID:
		gHUD.cshifts[CSHIFT_CONTENTS] = cshift_empty;
		break;
	case CONTENTS_LAVA:
		gHUD.cshifts[CSHIFT_CONTENTS] = cshift_lava;
		break;
	case CONTENTS_SLIME:
		gHUD.cshifts[CSHIFT_CONTENTS] = cshift_slime;
		break;
	default:
		gHUD.cshifts[CSHIFT_CONTENTS] = cshift_water;
		break;
	}
}

/*
=============
V_CalcPowerupCshift
=============
*/
void V_CalcPowerupCshift (void)
{
	if (gHUD.items & IT_QUAD)
	{
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[1] = 0;
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[2] = 255;
		gHUD.cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else if (gHUD.items & IT_SUIT)
	{
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[0] = 0;
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		gHUD.cshifts[CSHIFT_POWERUP].percent = 20;
	}
	else if (gHUD.items & IT_INVISIBILITY)
	{
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[0] = 100;
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[1] = 100;
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[2] = 100;
		gHUD.cshifts[CSHIFT_POWERUP].percent = 100;
	}
	else if (gHUD.items & IT_INVULNERABILITY)
	{
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[0] = 255;
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[1] = 255;
		gHUD.cshifts[CSHIFT_POWERUP].destcolor[2] = 0;
		gHUD.cshifts[CSHIFT_POWERUP].percent = 30;
	}
	else
		gHUD.cshifts[CSHIFT_POWERUP].percent = 0;
}

void V_CalcBlend (void)
{
	float	r, g, b, a, a2;

	r = 0;
	g = 0;
	b = 0;
	a = 0;

	for (int j=0 ; j<NUM_CSHIFTS ; j++)	
	{
		if (!gl_cshiftpercent.value)
			continue;

		a2 = ((gHUD.cshifts[j].percent * gl_cshiftpercent.value) / 100.0) / 255.0;

		if (!a2)
			continue;
		a = a + a2*(1-a);

		a2 = a2/a;
		r = r*(1-a2) + gHUD.cshifts[j].destcolor[0]*a2;
		g = g*(1-a2) + gHUD.cshifts[j].destcolor[1]*a2;
		b = b*(1-a2) + gHUD.cshifts[j].destcolor[2]*a2;
	}

	v_blend[0] = r/255.0;
	v_blend[1] = g/255.0;
	v_blend[2] = b/255.0;
	v_blend[3] = a;

	if (v_blend[3] > 1)
		v_blend[3] = 1;
	if (v_blend[3] < 0)
		v_blend[3] = 0;
}

/*
=============
V_UpdatePalette
=============
*/
void V_UpdatePalette (struct ref_params_s *pparams)
{
	int	i, j;
	float	r,g,b,a;

	cl_entity_t *world = gEngfuncs.GetEntityByIndex(0);

	// set contents color
	if (world != NULL && world->model != NULL)
	{
		mleaf_t *viewleaf = Mod_PointInLeaf( pparams->vieworg, world->model->nodes );

		if (viewleaf != NULL)
			V_SetContentsColor (viewleaf->contents);
	}

	V_CalcPowerupCshift ();
	
	for (i=0 ; i<NUM_CSHIFTS ; i++)
	{
		if (gHUD.cshifts[i].percent != gHUD.prev_cshifts[i].percent)
		{
			gHUD.prev_cshifts[i].percent = gHUD.cshifts[i].percent;
		}

		for (j=0 ; j<3 ; j++)
		{
			if (gHUD.cshifts[i].destcolor[j] != gHUD.prev_cshifts[i].destcolor[j])
			{
				gHUD.prev_cshifts[i].destcolor[j] = gHUD.cshifts[i].destcolor[j];
			}
		}
	}
	
	// drop the damage value
	gHUD.cshifts[CSHIFT_DAMAGE].percent -= pparams->frametime*150;
	if (gHUD.cshifts[CSHIFT_DAMAGE].percent <= 0)
		gHUD.cshifts[CSHIFT_DAMAGE].percent = 0;

	// drop the bonus value
	gHUD.cshifts[CSHIFT_BONUS].percent -= pparams->frametime*100;
	if (gHUD.cshifts[CSHIFT_BONUS].percent <= 0)
		gHUD.cshifts[CSHIFT_BONUS].percent = 0;

	if( pparams->intermission )
	{
		memset( gHUD.cshifts, 0, sizeof( gHUD.cshifts ));
		memset( gHUD.prev_cshifts, 0, sizeof( gHUD.prev_cshifts ));
	}

	V_CalcBlend ();

	r = 255*v_blend[0]*v_blend[3];
	g = 255*v_blend[1]*v_blend[3];
	b = 255*v_blend[2]*v_blend[3];
	a = 255 - (255*v_blend[3]);

	screenfade_t sf;

	if(a==255 && r==0 && g==0 && b==0)
	{
		// don't reset fade each frame because this a globals struct
		// that passes values a comes from UTIL_ScreenFade.
		if( v_resetCshifts )
		{
			memset( &sf, 0, sizeof( sf ));
			gEngfuncs.pfnSetScreenFade( &sf );
			v_resetCshifts = 0;
		}
		return;// no cshifts
          }

	sf.fader = (int)r;
	sf.fadeg = (int)g;
	sf.fadeb = (int)b;
	sf.fadealpha = (int)a;
	sf.fadeFlags = FFADE_STAYOUT|FFADE_MODULATE;

	gEngfuncs.pfnSetScreenFade( &sf );	
	v_resetCshifts = 1;
}

/* 
============================================================================== 
						VIEW RENDERING 
============================================================================== 
*/ 

/*
==================
V_CalcGunAngle
==================
*/
void V_CalcGunAngle ( struct ref_params_s *pparams )
{	
	cl_entity_t *viewent;
	
	viewent = gEngfuncs.GetViewModel();
	if ( !viewent )
		return;

	viewent->angles[YAW]   =  pparams->viewangles[YAW]   + pparams->crosshairangle[YAW];
	viewent->angles[PITCH] = -pparams->viewangles[PITCH] + pparams->crosshairangle[PITCH] * 0.25;
	viewent->angles[ROLL]  -= v_idlescale * sin(pparams->time*v_iroll_cycle.value) * v_iroll_level.value;
	
	// don't apply all of the v_ipitch to prevent normally unseen parts of viewmodel from coming into view.
	viewent->angles[PITCH] -= v_idlescale * sin(pparams->time*v_ipitch_cycle.value) * (v_ipitch_level.value * 0.5);
	viewent->angles[YAW]   -= v_idlescale * sin(pparams->time*v_iyaw_cycle.value) * v_iyaw_level.value;

	VectorCopy( viewent->angles, viewent->curstate.angles );
	VectorCopy( viewent->angles, viewent->latched.prevangles );
}

/*
==============
V_AddIdle

Idle swaying
==============
*/
void V_AddIdle ( struct ref_params_s *pparams )
{
	pparams->viewangles[ROLL] += v_idlescale * sin(pparams->time*v_iroll_cycle.value) * v_iroll_level.value;
	pparams->viewangles[PITCH] += v_idlescale * sin(pparams->time*v_ipitch_cycle.value) * v_ipitch_level.value;
	pparams->viewangles[YAW] += v_idlescale * sin(pparams->time*v_iyaw_cycle.value) * v_iyaw_level.value;
}


/*
==============
V_CalcViewRoll

Roll is induced by movement and damage
==============
*/
void V_CalcViewRoll ( struct ref_params_s *pparams )
{
	cl_entity_t *viewentity;
	
	viewentity = gEngfuncs.GetEntityByIndex( pparams->viewentity );
	if ( !viewentity )
		return;

	pparams->viewangles[ROLL] = V_CalcRoll(pparams->viewangles, pparams->simvel, cl_rollangle->value, cl_rollspeed->value) * 4;

	if (v_dmg_time > 0)
	{
		pparams->viewangles[ROLL] += v_dmg_time / v_kicktime.value*v_dmg_roll;
		pparams->viewangles[PITCH] += v_dmg_time / v_kicktime.value*v_dmg_pitch;
		v_dmg_time -= pparams->frametime;
	}

	if ( pparams->health <= 0 && ( pparams->viewheight[2] != 0 ) )
	{
		// only roll the view if the player is dead and the viewheight[2] is nonzero 
		// this is so deadcam in multiplayer will work.
		pparams->viewangles[ROLL] = 0;	// dead view angle
		return;
	}
}


/*
==================
V_CalcIntermissionRefdef

==================
*/
void V_CalcIntermissionRefdef ( struct ref_params_s *pparams )
{
	cl_entity_t	*ent, *view;
	float		old;

	// ent is the player model ( visible when out of body )
	ent = gEngfuncs.GetLocalPlayer();
	
	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	// override all previous settings if the viewent isn't the client
	if ( pparams->viewentity > pparams->maxclients )
	{
		cl_entity_t *viewentity;
		viewentity = gEngfuncs.GetEntityByIndex( pparams->viewentity );
		if ( viewentity )
		{
			VectorCopy( viewentity->origin, pparams->vieworg );
			VectorCopy( viewentity->angles, pparams->viewangles );

			// Store off overridden viewangles
			v_angles = pparams->viewangles;
		}
	}
	else
	{
		VectorCopy ( pparams->simorg, pparams->vieworg );
		VectorCopy ( pparams->cl_viewangles, pparams->viewangles );
	}

	gEngfuncs.V_CalcShake();
	gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0 );

	view->model = NULL;

	// allways idle in intermission
	old = v_idlescale;
	v_idlescale = 1;

	if (pparams->intermission != 2)
		V_AddIdle ( pparams );

	v_idlescale = old;

	v_cl_angles = pparams->cl_viewangles;
	v_origin = pparams->vieworg;
	v_angles = pparams->viewangles;

	pparams->waterlevel = 0;	// no underwater effect in intermission
}

#define ORIGIN_BACKUP 64
#define ORIGIN_MASK ( ORIGIN_BACKUP - 1 )

typedef struct 
{
	float Origins[ ORIGIN_BACKUP ][3];
	float OriginTime[ ORIGIN_BACKUP ];

	float Angles[ ORIGIN_BACKUP ][3];
	float AngleTime[ ORIGIN_BACKUP ];

	int CurrentOrigin;
	int CurrentAngle;
} viewinterp_t;

/*
==================
V_CalcRefdef

==================
*/
void V_CalcNormalRefdef ( struct ref_params_s *pparams )
{
	cl_entity_t		*ent, *view;
	int				i;
	vec3_t			angles;
	float			bob, waterOffset;
	static viewinterp_t		ViewInterp;

	static float oldz = 0;
	static float oldViewz = 0;
	static float lasttime;

	vec3_t camAngles, camForward, camRight, camUp;
	//cl_entity_t *pwater;

	V_DriftPitch ( pparams );

	if ( gEngfuncs.IsSpectateOnly() )
		ent = gEngfuncs.GetEntityByIndex( g_iUser2 );
	else
		ent = gEngfuncs.GetLocalPlayer(); // ent is the player model ( visible when out of body )
	
	// view is the weapon model (only visible from inside body )
	view = gEngfuncs.GetViewModel();

	HOLSTER_CLIENT_IN = ent->curstate.iuser4;

	bob = V_CalcDoomBob ( pparams );

	// refresh position
	VectorCopy ( pparams->simorg, pparams->vieworg );

#ifndef WOLF3DGAME
	pparams->vieworg[2] += ( bob );		//WOLF 3D - no viewbob
#endif

	VectorAdd( pparams->vieworg, pparams->viewheight, pparams->vieworg );

	VectorCopy ( pparams->cl_viewangles, pparams->viewangles );

	gEngfuncs.V_CalcShake();
	gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0 );

	// never let view origin sit exactly on a node line, because a water plane can
	// dissapear when viewed with the eye exactly on it.
	// FIXME, we send origin at 1/128 now, change this?
	// the server protocol only specifies to 1/16 pixel, so add 1/32 in each axis
	
	pparams->vieworg[0] += 1.0/32;
	pparams->vieworg[1] += 1.0/32;
	pparams->vieworg[2] += 1.0/32;

	// Check for problems around water, move the viewer artificially if necessary 
	// -- this prevents drawing errors in GL due to waves

	waterOffset = 0;
#if 0
	if ( pparams->waterlevel >= 2 )
	{
		int		i, contents, waterDist, waterEntity;
		vec3_t	point;
		waterDist = cl_waterdist->value;

		if ( pparams->hardware )
		{
			waterEntity = gEngfuncs.PM_WaterEntity( pparams->simorg );
			if ( waterEntity >= 0 && waterEntity < pparams->max_entities )
			{
				pwater = gEngfuncs.GetEntityByIndex( waterEntity );
				if ( pwater && ( pwater->model != NULL ) )
				{
					waterDist += ( pwater->curstate.scale * 16 );	// Add in wave height
				}
			}
		}
		else
		{
			waterEntity = 0;	// Don't need this in software
		}
		
		VectorCopy( pparams->vieworg, point );

		// Eyes are above water, make sure we're above the waves
		if ( pparams->waterlevel == 2 )	
		{
			point[2] -= waterDist;
			for ( i = 0; i < waterDist; i++ )
			{
				contents = gEngfuncs.PM_PointContents( point, NULL );
				if ( contents > CONTENTS_WATER )
					break;
				point[2] += 1;
			}
			waterOffset = (point[2] + waterDist) - pparams->vieworg[2];
		}
		else
		{
			// eyes are under water.  Make sure we're far enough under
			point[2] += waterDist;

			for ( i = 0; i < waterDist; i++ )
			{
				contents = gEngfuncs.PM_PointContents( point, NULL );
				if ( contents <= CONTENTS_WATER )
					break;
				point[2] -= 1;
			}
			waterOffset = (point[2] - waterDist) - pparams->vieworg[2];
		}
	}
#endif

	pparams->vieworg[2] += waterOffset;
	
	V_CalcViewRoll ( pparams );
	
	V_AddIdle ( pparams );

	// offsets
	VectorCopy( pparams->cl_viewangles, angles );

	AngleVectors ( angles, pparams->forward, pparams->right, pparams->up );

	// don't allow cheats in multiplayer
	if ( pparams->maxclients <= 1 )
	{
		for ( i=0 ; i<3 ; i++ )
			pparams->vieworg[i] += scr_ofsx->value*pparams->forward[i] + scr_ofsy->value*pparams->right[i] + scr_ofsz->value*pparams->up[i];
	}
	
	// Treating cam_ofs[2] as the distance
	if( CL_IsThirdPerson() )
	{
		vec3_t ofs;

		ofs[0] = ofs[1] = ofs[2] = 0.0;

		CL_CameraOffset( (float *)&ofs );

		VectorCopy( ofs, camAngles );
		camAngles[ ROLL ]	= 0;

		AngleVectors( camAngles, camForward, camRight, camUp );

		for ( i = 0; i < 3; i++ )
			pparams->vieworg[ i ] += -ofs[2] * camForward[ i ];
	}
	
	// Give gun our viewangles
	VectorCopy ( pparams->cl_viewangles, view->angles );
	
	// set up gun position
	V_CalcGunAngle ( pparams );

	// Use predicted origin as view origin.
	VectorCopy ( pparams->simorg, view->origin );      
	view->origin[2] += ( waterOffset );
	VectorAdd( view->origin, pparams->viewheight, view->origin );

	// Let the viewmodel shake at about 10% of the amplitude
	gEngfuncs.V_ApplyShake( view->origin, view->angles, 0.9 );

	//for ( i = 0; i < 3; i++ )
		//view->origin[ i ] += bob * 0.4 * pparams->forward[ i ];

	//view->origin[2] += bob;

	// throw in a little tilt.
	view->angles[YAW]   -= bob * 0.5;
	view->angles[ROLL]  -= bob * 1;
	view->angles[PITCH] -= bob * 0.3;

	// pushing the view origin down off of the same X/Z plane as the ent's origin will give the
	// gun a very nice 'shifting' effect when the player looks up/down. If there is a problem
	// with view model distortion, this may be a cause. (SJB). 
	view->origin[2] -= 1;

	// fudge position around to keep amount of weapon visible
	// roughly equal with different FOV
	if (pparams->viewsize == 110)
		view->origin[2] += 1;
	else if (pparams->viewsize == 100)
		view->origin[2] += 2;
	else if (pparams->viewsize == 90)
		view->origin[2] += 1;
	else if (pparams->viewsize == 80)
		view->origin[2] += 0.5;

	gHUD.m_iViewportY = pparams->viewport[1];
	gHUD.m_iViewportHeight = pparams->viewport[3];
	gHUD.m_iWaterLevel = pparams->waterlevel;

	// setup sbar size
	if (pparams->viewsize >= 120)
		gHUD.sb_lines = 0;
	else if (pparams->viewsize >= 110)
		gHUD.sb_lines = 1;
	else
		gHUD.sb_lines = 2;

	// Add in the punchangle, if any
	VectorAdd(pparams->vieworg, pparams->punchangle, pparams->vieworg);
	//VectorAdd ( pparams->viewangles, pparams->punchangle, pparams->viewangles );

	// Include client side punch, too
	VectorAdd ( pparams->viewangles, (float *)&ev_punchangle, pparams->viewangles);

	V_DropPunchAngle ( pparams->frametime, (float *)&ev_punchangle );

	// smooth out stair step ups
	if (!pparams->smoothing && pparams->onground && (pparams->simorg[2] != oldz) && oldViewz == pparams->viewheight[2])
	{
		int dir = (pparams->simorg[2] > oldz) ? 1 : -1;

		float steptime;

		steptime = pparams->time - lasttime;
		if (steptime < 0)
			steptime = 0; //FIXME	I_Error ("steptime < 0");

		oldz += steptime * 150 * dir;

		const float stepSize = pparams->movevars->stepsize;

		if (dir > 0)
		{
			if (oldz > pparams->simorg[2])
				oldz = pparams->simorg[2];

			if (pparams->simorg[2] - oldz > stepSize)
				oldz = pparams->simorg[2] - stepSize;
		}
		else
		{
			if (oldz < pparams->simorg[2])
				oldz = pparams->simorg[2];

			if (pparams->simorg[2] - oldz < -stepSize)
				oldz = pparams->simorg[2] + stepSize;
		}

		pparams->vieworg[2] += oldz - pparams->simorg[2];
		view->origin[2] += oldz - pparams->simorg[2];
	}
	else
	{
		oldz = pparams->simorg[2];
		oldViewz = pparams->viewheight[2];
	}

	{
		static float lastorg[3];
		vec3_t delta;

		VectorSubtract(pparams->simorg, lastorg, delta);

		if (Length(delta) != 0.0)
		{
			VectorCopy(pparams->simorg, ViewInterp.Origins[ViewInterp.CurrentOrigin & ORIGIN_MASK]);
			ViewInterp.OriginTime[ViewInterp.CurrentOrigin & ORIGIN_MASK] = pparams->time;
			ViewInterp.CurrentOrigin++;

			VectorCopy(pparams->simorg, lastorg);
		}
	}

	// Smooth out whole view in multiplayer when on trains, lifts
	if (cl_vsmoothing && cl_vsmoothing->value &&
		(pparams->smoothing && (pparams->maxclients > 1)))
	{
		int foundidx;
		int i;
		float t;

		if (cl_vsmoothing->value < 0.0)
		{
			gEngfuncs.Cvar_SetValue("cl_vsmoothing", 0.0);
		}

		t = pparams->time - cl_vsmoothing->value;

		for (i = 1; i < ORIGIN_MASK; i++)
		{
			foundidx = ViewInterp.CurrentOrigin - 1 - i;
			if (ViewInterp.OriginTime[foundidx & ORIGIN_MASK] <= t)
				break;
		}

		if (i < ORIGIN_MASK && ViewInterp.OriginTime[foundidx & ORIGIN_MASK] != 0.0)
		{
			// Interpolate
			vec3_t delta;
			double frac;
			double dt;
			vec3_t neworg;

			dt = ViewInterp.OriginTime[(foundidx + 1) & ORIGIN_MASK] - ViewInterp.OriginTime[foundidx & ORIGIN_MASK];
			if (dt > 0.0)
			{
				frac = (t - ViewInterp.OriginTime[foundidx & ORIGIN_MASK]) / dt;
				frac = min(1.0, frac);
				VectorSubtract(ViewInterp.Origins[(foundidx + 1) & ORIGIN_MASK], ViewInterp.Origins[foundidx & ORIGIN_MASK], delta);
				VectorMA(ViewInterp.Origins[foundidx & ORIGIN_MASK], frac, delta, neworg);

				// Dont interpolate large changes
				if (Length(delta) < 64)
				{
					VectorSubtract(neworg, pparams->simorg, delta);

					VectorAdd(pparams->simorg, delta, pparams->simorg);
					VectorAdd(pparams->vieworg, delta, pparams->vieworg);
					VectorAdd(view->origin, delta, view->origin);

				}
			}
		}
	}

	// Store off v_angles before munging for third person
	v_angles = pparams->viewangles;
	v_lastAngles = pparams->viewangles;
//	v_cl_angles = pparams->cl_viewangles;	// keep old user mouse angles !
	if ( CL_IsThirdPerson() )
	{
		VectorCopy( camAngles, pparams->viewangles);
		float pitch = camAngles[ 0 ];

		// Normalize angles
		if ( pitch > 180 ) 
			pitch -= 360.0;
		else if ( pitch < -180 )
			pitch += 360;

		// Player pitch is inverted
		pitch /= -3.0;

		// Slam local player's pitch value
		ent->angles[ 0 ] = pitch;
		ent->curstate.angles[ 0 ] = pitch;
		ent->prevstate.angles[ 0 ] = pitch;
		ent->latched.prevangles[ 0 ] = pitch;
	}

	// override all previous settings if the viewent isn't the client
	if ( pparams->viewentity > pparams->maxclients )
	{
		cl_entity_t *viewentity;
		viewentity = gEngfuncs.GetEntityByIndex( pparams->viewentity );
		if ( viewentity )
		{
			VectorCopy( viewentity->origin, pparams->vieworg );
			VectorCopy( viewentity->angles, pparams->viewangles );

			gEngfuncs.V_CalcShake();
			gEngfuncs.V_ApplyShake( pparams->vieworg, pparams->viewangles, 1.0 );

			// Store off overridden viewangles
			v_angles = pparams->viewangles;
		}
	}

	lasttime = pparams->time;

	v_origin = pparams->vieworg;
}

void DLLEXPORT V_CalcRefdef( struct ref_params_s *pparams )
{
	gpViewParams = pparams;

	// intermission / finale rendering
	if ( pparams->intermission )
	{	
		V_CalcIntermissionRefdef ( pparams );	
	}
	else if ( !pparams->paused )
	{
		V_CalcNormalRefdef ( pparams );
	}

	V_UpdatePalette (pparams);
}

/*
=============
V_DropPunchAngle

=============
*/
void V_DropPunchAngle ( float frametime, float *ev_punchangle )
{
	float	len;
	
	len = VectorNormalize ( ev_punchangle );
	len -= (10.0 + len * 0.5) * frametime;
	len = max( len, 0.0 );
	VectorScale ( ev_punchangle, len, ev_punchangle );
}

/*
=============
V_PunchAxis

Client side punch effect
=============
*/
void V_PunchAxis( int axis, float punch )
{
	ev_punchangle[ axis ] = punch;
}

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	gEngfuncs.pfnAddCommand ("centerview", V_StartPitchDrift );
	gEngfuncs.pfnAddCommand ("v_cshift", V_cshift_f);	
	gEngfuncs.pfnAddCommand ("bf", V_BonusFlash_f);
	
	scr_ofsx		= gEngfuncs.pfnRegisterVariable( "scr_ofsx","0", 0 );
	scr_ofsy		= gEngfuncs.pfnRegisterVariable( "scr_ofsy","0", 0 );
	scr_ofsz		= gEngfuncs.pfnRegisterVariable( "scr_ofsz","0", 0 );

	v_centermove	= gEngfuncs.pfnRegisterVariable( "v_centermove", "0.15", 0 );
	v_centerspeed	= gEngfuncs.pfnRegisterVariable( "v_centerspeed","500", 0 );

	cl_waterdist	= gEngfuncs.pfnRegisterVariable( "cl_waterdist","4", 0 );
	cl_chasedist	= gEngfuncs.pfnRegisterVariable( "cl_chasedist","112", 0 );

#ifndef WOLF3DGAME
	cl_bobcycle		= gEngfuncs.pfnRegisterVariable("cl_bobcycle", "2.0", 0);
	cl_bob			= gEngfuncs.pfnRegisterVariable("cl_bob", "0.055", 0);
#else
	cl_bobcycle = gEngfuncs.pfnRegisterVariable("cl_bobcycle", "0.8", 0);
	cl_bob = gEngfuncs.pfnRegisterVariable("cl_bob", "0.0075", 0);
#endif
	cl_bobup		= gEngfuncs.pfnRegisterVariable("cl_bobup", "0.5", 0);

	cl_rollspeed = gEngfuncs.pfnRegisterVariable("cl_rollspeed", "325", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
	cl_rollangle = gEngfuncs.pfnRegisterVariable("cl_rollangle", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);
}