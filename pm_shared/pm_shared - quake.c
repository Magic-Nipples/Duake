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

#include <assert.h>
#include "mathlib.h"
#include "const.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_shared.h"
#include "pm_movevars.h"
#include "pm_debug.h"
#include <stdio.h>  // NULL
#include <math.h>   // sqrt
#include <string.h> // strcpy
#include <stdlib.h> // atoi
#include <ctype.h>  // isspace
#include "com_model.h"
#include "quakedef.h"


#ifdef CLIENT_DLL
	// Spectator Mode
	int		iJumpSpectator;
	float	vJumpOrigin[3];
	float	vJumpAngles[3];
#endif

static int pm_shared_initialized = 0;

#pragma warning( disable : 4305 )

playermove_t *pmove = NULL;

#define PM_DEAD_VIEWHEIGHT	-8
#define VEC_HULL_MIN	-24
#define VEC_HULL_MAX	32
#define VEC_VIEW		22
#define STOP_EPSILON	0.1

//#define PLAYER_FATAL_FALL_SPEED		1024// approx 60 feet
//#define PLAYER_MAX_SAFE_FALL_SPEED	650// approx 20 feet
//#define DAMAGE_FOR_FALL_SPEED		(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )// damage per unit per second.
//#define PLAYER_MIN_BOUNCE_SPEED		200
//#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

#define PLAYER_LONGJUMP_SPEED 350 // how fast we longjump

// double to float warning
#pragma warning(disable : 4244)
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))
// up / down
#define	PITCH	0
// left / right
#define	YAW		1
// fall over
#define	ROLL	2 

#define MAX_CLIENTS 32

#define CONTENTS_TRANSLUCENT	-15

static vec3_t rgv3tStuckTable[54];
static int rgStuckLast[MAX_CLIENTS][2];

char PM_FindTextureType( char *name )
{
	return '\0';
}

/*
================
PM_AddToTouched

Add's the trace result to touch list, if contact is not already in list.
================
*/
qboolean PM_AddToTouched(pmtrace_t tr, vec3_t impactvelocity)
{
	int i;

	for (i = 0; i < pmove->numtouch; i++)
	{
		if (pmove->touchindex[i].ent == tr.ent)
			break;
	}
	if (i != pmove->numtouch)  // Already in list.
		return false;

	VectorCopy( impactvelocity, tr.deltavelocity );

	if (pmove->numtouch >= MAX_PHYSENTS)
		pmove->Con_DPrintf("Too many entities were touched!\n");

	pmove->touchindex[pmove->numtouch++] = tr;
	return true;
}

/*
================
PM_CheckVelocity

See if the player has a bogus velocity value.
================
*/
void PM_CheckVelocity ()
{
	int		i;

//
// bound velocity
//
	for (i=0 ; i<3 ; i++)
	{
		// See if it's bogus.
		if (IS_NAN(pmove->velocity[i]))
		{
			pmove->Con_Printf ("PM  Got a NaN velocity %i\n", i);
			pmove->velocity[i] = 0;
		}
		if (IS_NAN(pmove->origin[i]))
		{
			pmove->Con_Printf ("PM  Got a NaN origin on %i\n", i);
			pmove->origin[i] = 0;
		}

		// Bound it.
		if (pmove->velocity[i] > pmove->movevars->maxvelocity) 
		{
			pmove->Con_DPrintf ("PM  Got a velocity too high on %i\n", i);
			pmove->velocity[i] = pmove->movevars->maxvelocity;
		}
		else if (pmove->velocity[i] < -pmove->movevars->maxvelocity)
		{
			pmove->Con_DPrintf ("PM  Got a velocity too low on %i\n", i);
			pmove->velocity[i] = -pmove->movevars->maxvelocity;
		}
	}
}

/*
==================
PM_ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
int PM_ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	float	change;
	int		i, blocked;
	
	blocked = 0;
	if (normal[2] > 0)
		blocked |= 1;		// floor
	if (!normal[2])
		blocked |= 2;		// step

	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change;
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
	
	return blocked;
}

void PM_AddCorrectGravity ()
{
	float	ent_gravity;

	if ( pmove->waterjumptime )
		return;

	if (pmove->gravity)
		ent_gravity = pmove->gravity;
	else
		ent_gravity = 1.0;

	if (pmove->movetype == MOVETYPE_FLY)
		ent_gravity = 0.1f;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	pmove->velocity[2] -= (ent_gravity * pmove->movevars->gravity * 0.5 * pmove->frametime );
	pmove->velocity[2] += pmove->basevelocity[2] * pmove->frametime;
	pmove->basevelocity[2] = 0;

	PM_CheckVelocity();
}

/*
============
PM_FlyMove

The basic solid body movement clip that slides along multiple planes
============
*/
int PM_FlyMove (void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity, original_velocity;
	int			i, j;
	pmtrace_t		trace;
	vec3_t		end;
	float		time_left;
	int			blocked;
		
	numbumps  = 4;           // Bump up to four times

	numplanes = 0;           //  and not sliding along any planes
	VectorCopy (pmove->velocity, original_velocity);  // Store original velocity
	VectorCopy (pmove->velocity, primal_velocity);
	blocked   = 0;           // Assume not blocked	

	time_left = pmove->frametime;

	for (bumpcount=0 ; bumpcount<numbumps ; bumpcount++)
	{
		for (i=0 ; i<3 ; i++)
			end[i] = pmove->origin[i] + time_left * pmove->velocity[i];

		trace = pmove->PM_PlayerTrace (pmove->origin, end, PM_NORMAL, -1 );

		if (trace.startsolid || trace.allsolid)
		{	// entity is trapped in another solid
			VectorCopy (vec3_origin, pmove->velocity);
			return 3;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy (trace.endpos, pmove->origin);
			numplanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// moved the entire distance

		// save entity for contact
		PM_AddToTouched(trace, pmove->velocity);


		if (trace.plane.normal[2] > 0.7)
			blocked |= 1;		// floor
		if (!trace.plane.normal[2])
			blocked |= 2;		// step

		time_left -= time_left * trace.fraction;
		
	// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			VectorCopy (vec3_origin, pmove->velocity);
			break;
		}

		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

//
// modify original_velocity so it parallels all of the clip planes
//
		for (i=0 ; i<numplanes ; i++)
		{
			PM_ClipVelocity (original_velocity, planes[i], pmove->velocity, 1);
			for (j=0 ; j<numplanes ; j++)
				if (j != i)
				{
					if (DotProduct (pmove->velocity, planes[j]) < 0)
						break;	// not ok
				}
			if (j == numplanes)
				break;
		}
			
		if (i != numplanes)
		{	// go along this plane
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
				//Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
				VectorCopy (vec3_origin, pmove->velocity);
				break;
			}
			CrossProduct (planes[0], planes[1], dir);
			d = DotProduct (dir, pmove->velocity);
			VectorScale (dir, d, pmove->velocity );
		}

//
// if original velocity is against the original velocity, stop dead
// to avoid tiny occilations in sloping corners
//
		if (DotProduct (pmove->velocity, primal_velocity) <= 0)
		{
			VectorCopy (vec3_origin, pmove->velocity);
			break;
		}
	}

	if (pmove->waterjumptime)
	{
		VectorCopy (primal_velocity, pmove->velocity);
	}
	return blocked;
}

/*
=====================
PM_StayOnGround

Try to keep a walking player on the ground when running down slopes etc
======================
*/
void PM_StayOnGround(void)
{
	pmtrace_t trace;
	vec3_t start, end;

	VectorCopy(pmove->origin, start);
	start[2] += 2;

	VectorCopy(pmove->origin, end);
	end[2] -= pmove->movevars->stepsize;

	// See how far up we can go down without getting stuck
	trace = pmove->PM_PlayerTrace(pmove->origin, start, PM_NORMAL, -1);
	VectorCopy(trace.endpos, start);

	// Now trace down from a known safe position
	trace = pmove->PM_PlayerTrace(start, end, PM_NORMAL, -1);

	if (trace.fraction > 0.0f &&            // must go somewhere
		trace.fraction < 1.0f &&            // must hit something
		!trace.startsolid &&                // can't be embedded in a solid
		trace.plane.normal[2] >= 0.7)        // can't hit a steep slope that we can't stand on anyway
	{
		//This is incredibly hacky. The real problem is that trace returning that strange value we can't network over.
		float flDelta = fabs(pmove->origin[2] - trace.endpos[2]);

		if (flDelta > 0.5)
			VectorCopy(trace.endpos, pmove->origin);
	}
}

/*
=====================
PM_GroundMove

Only used by players.  Moves along the ground when player is a MOVETYPE_WALK.
======================
*/
void PM_GroundMove ()
{
	vec3_t	start, dest;
	pmtrace_t	trace;
	vec3_t	original, originalvel, down, up, downvel;
	float	downdist, updist;

	pmove->velocity[2] = 0;
	if (!pmove->velocity[0] && !pmove->velocity[1] && !pmove->velocity[2])
		return;

	// first try just moving to the destination	
	dest[0] = pmove->origin[0] + pmove->velocity[0]*pmove->frametime;
	dest[1] = pmove->origin[1] + pmove->velocity[1]*pmove->frametime;	
	dest[2] = pmove->origin[2];

	// first try moving directly to the next spot
	VectorCopy (dest, start);
	trace = pmove->PM_PlayerTrace (pmove->origin, dest, PM_NORMAL, -1 );
	if (trace.fraction == 1)
	{
		VectorCopy (trace.endpos, pmove->origin);
		PM_StayOnGround();
		return;
	}

	// try sliding forward both on ground and up 16 pixels
	// take the move that goes farthest
	VectorCopy (pmove->origin, original);
	VectorCopy (pmove->velocity, originalvel);

	// slide move
	PM_FlyMove ();

	VectorCopy (pmove->origin, down);
	VectorCopy (pmove->velocity, downvel);

	VectorCopy (original, pmove->origin);
	VectorCopy (originalvel, pmove->velocity);

// move up a stair height
	VectorCopy (pmove->origin, dest);
	dest[2] += pmove->movevars->stepsize;
	trace = pmove->PM_PlayerTrace (pmove->origin, dest, PM_NORMAL, -1 );
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, pmove->origin);
	}

// slide move
	PM_FlyMove ();

// press down the stepheight
	VectorCopy (pmove->origin, dest);
	dest[2] -= pmove->movevars->stepsize;
	trace = pmove->PM_PlayerTrace (pmove->origin, dest, PM_NORMAL, -1 );
	if ( trace.plane.normal[2] < 0.7)
		goto usedown;
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, pmove->origin);
	}
	VectorCopy (pmove->origin, up);

	// decide which one went farther
	downdist = (down[0] - original[0])*(down[0] - original[0])
		+ (down[1] - original[1])*(down[1] - original[1]);
	updist = (up[0] - original[0])*(up[0] - original[0])
		+ (up[1] - original[1])*(up[1] - original[1]);

	if (downdist > updist)
	{
usedown:
		VectorCopy (down, pmove->origin);
		VectorCopy (downvel, pmove->velocity);
	} else // copy z value from slide move
		pmove->velocity[2] = downvel[2];

// if at a dead stop, retry the move with nudges to get around lips
}



/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
void PM_Friction (void)
{
	float	*vel;
	float	speed, newspeed, control;
	float	friction;
	float	drop;
	vec3_t	start, stop;
	pmtrace_t		trace;
	
	if (pmove->waterjumptime)
		return;

	vel = pmove->velocity;
	
	speed = sqrt(vel[0]*vel[0] +vel[1]*vel[1] + vel[2]*vel[2]);
	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	friction = pmove->movevars->friction;

// if the leading edge is over a dropoff, increase friction
	if (pmove->onground != -1) {
		start[0] = stop[0] = pmove->origin[0] + vel[0]/speed*16;
		start[1] = stop[1] = pmove->origin[1] + vel[1]/speed*16;
		start[2] = pmove->origin[2] + pmove->player_mins[pmove->usehull][2];
		stop[2] = start[2] - 34;

		trace = pmove->PM_PlayerTrace (start, stop, PM_NORMAL, -1 );

		if (trace.fraction == 1) {
			friction *= pmove->movevars->edgefriction;
		}
	}

	drop = 0;

	if (pmove->waterlevel >= 2) // apply water friction
		drop += speed*pmove->movevars->waterfriction*pmove->waterlevel*pmove->frametime;
	else if (pmove->onground != -1) // apply ground friction
	{
		control = speed < pmove->movevars->stopspeed ? pmove->movevars->stopspeed : speed;
		drop += control*friction*pmove->frametime;
	}


// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate
==============
*/
void PM_Accelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	if (pmove->dead)
		return;
	if (pmove->waterjumptime)
		return;

#ifndef WOLF3DGAME //WOLF 3D - no accelerate at all
	currentspeed = DotProduct(pmove->velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * pmove->frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
		
	for (i=0 ; i<3 ; i++)
		pmove->velocity[i] += accelspeed * wishdir[i];		
#else
	for (i=0 ; i<3 ; i++)
		pmove->velocity[i] = wishspeed * wishdir[i];
#endif
}

void PM_AirAccelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;
		
	if (pmove->dead)
		return;
	if (pmove->waterjumptime)
		return;

	if (wishspd > 30)
		wishspd = 30;
	currentspeed = DotProduct (pmove->velocity, wishdir);
	addspeed = wishspd - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * wishspeed * pmove->frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	for (i=0 ; i<3 ; i++)
		pmove->velocity[i] += accelspeed*wishdir[i];	
}



/*
===================
PM_WaterMove

===================
*/
void PM_WaterMove (void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;
	vec3_t	start, dest;
	pmtrace_t	trace;

//
// user intentions
//
	for (i = 0; i < 3; i++)
		wishvel[i] = pmove->forward[i] * pmove->cmd.forwardmove + pmove->right[i] * pmove->cmd.sidemove;

	if (!pmove->cmd.forwardmove && !pmove->cmd.sidemove && !pmove->cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += pmove->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > pmove->maxspeed)
	{
		VectorScale (wishvel, pmove->maxspeed/wishspeed, wishvel);
		wishspeed = pmove->maxspeed;
	}
	wishspeed *= 0.7;

	PM_Accelerate (wishdir, wishspeed, pmove->movevars->wateraccelerate);

// assume it is a stair or a slope, so press down from stepheight above
	VectorMA (pmove->origin, pmove->frametime, pmove->velocity, dest);
	VectorCopy (dest, start);
	start[2] += pmove->movevars->stepsize + 1;
	trace = pmove->PM_PlayerTrace (start, dest, PM_NORMAL, -1 );
	if (!trace.startsolid && !trace.allsolid)	// FIXME: check steep slope?
	{	// walked up the step, so just keep result and exit
		VectorCopy (trace.endpos, pmove->origin);
		return;
	}
	
	// Try moving straight along out normal path.
	PM_FlyMove ();
}


/*
===================
PM_AirMove

===================
*/
void PM_AirMove (void)
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	float		ent_gravity;

	if (pmove->gravity)
		ent_gravity = pmove->gravity;
	else
		ent_gravity = 1.0;

	fmove = pmove->cmd.forwardmove;
	smove = pmove->cmd.sidemove;
	
	pmove->forward[2] = 0;
	pmove->right[2]   = 0;
	VectorNormalize (pmove->forward);
	VectorNormalize (pmove->right);

	for (i=0 ; i<2 ; i++)       
		wishvel[i] = pmove->forward[i]*fmove + pmove->right[i]*smove;
	wishvel[2] = 0;             

	VectorCopy (wishvel, wishdir);  
	wishspeed = VectorNormalize(wishdir);

//
// clamp to server defined max speed
//
	if (wishspeed > pmove->maxspeed)
	{
		VectorScale (wishvel, pmove->maxspeed/wishspeed, wishvel);
		wishspeed = pmove->maxspeed;
	}

	if ( pmove->onground != -1)
	{
		pmove->velocity[2] = 0;
		PM_Accelerate (wishdir, wishspeed, pmove->movevars->accelerate);
		pmove->velocity[2] -= ent_gravity * pmove->movevars->gravity * pmove->frametime;

#ifdef WOLF3DGAME	
		if (pmove->alwaysrun) //walking code here. angle movement far better.
		{
			if (pmove->cmd.buttons & IN_RUN)
			{
				pmove->velocity[0] *= 0.6; //0.6
				pmove->velocity[1] *= 0.6;
			}
		}
		else
		{
			if (pmove->cmd.buttons & IN_RUN) {}
			else
			{
				pmove->velocity[0] *= 0.6;
				pmove->velocity[1] *= 0.6;
			}
		}
#endif

		PM_GroundMove ();
	}
	else
	{	// not on ground, so little effect on velocity
		PM_AirAccelerate (wishdir, wishspeed, pmove->movevars->airaccelerate);

		// add gravity
		pmove->velocity[2] -= ent_gravity * pmove->movevars->gravity * pmove->frametime;

		PM_FlyMove ();
	}
}

qboolean PM_InWater( void )
{
	return ( pmove->waterlevel > 1 );
}

/*
=============
PM_CheckWater

Sets pmove->waterlevel and pmove->watertype values.
=============
*/
qboolean PM_CheckWater ()
{
	vec3_t	point;
	int		cont;

	// Pick a spot just above the players feet.
	point[0] = pmove->origin[0];
	point[1] = pmove->origin[1];
	point[2] = pmove->origin[2] + pmove->player_mins[pmove->usehull][2] + 1;
	
	// Assume that we are not in water at all.
	pmove->waterlevel = 0;
	pmove->watertype = CONTENTS_EMPTY;

	// Grab point contents.
	cont = pmove->PM_PointContents (point, NULL );
	// Are we under water? (not solid and not empty?)
	if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT )
	{
		// Set water type
		pmove->watertype = cont;

		// We are at least at level one
		pmove->waterlevel = 1;

		// Now check a point that is at the player hull midpoint.
		point[2] = pmove->origin[2] + (pmove->player_mins[pmove->usehull][2] + pmove->player_maxs[pmove->usehull][2]) * 0.5;
		cont = pmove->PM_PointContents (point, NULL );
		// If that point is also under water...
		if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT )
		{
			// Set a higher water level.
			pmove->waterlevel = 2;

			// Now check the eye position.  (view_ofs is relative to the origin)
			point[2] = pmove->origin[2] + pmove->view_ofs[2];

			cont = pmove->PM_PointContents (point, NULL );
			if (cont <= CONTENTS_WATER && cont > CONTENTS_TRANSLUCENT ) 
				pmove->waterlevel = 3;  // In over our eyes
		}
	}

	return pmove->waterlevel > 1;
}

/*
=============
PM_CatagorizePosition
=============
*/
void PM_CatagorizePosition (void)
{
	vec3_t		point;
	pmtrace_t		tr;

	pmove->alwaysrun = atoi(pmove->PM_Info_ValueForKey(pmove->physinfo, "arun")) == 1 ? true : false;

// if the player hull point one unit down is solid, the player
// is on ground

// see if standing on something solid	
	point[0] = pmove->origin[0];
	point[1] = pmove->origin[1];
	point[2] = pmove->origin[2] - 1;

	if (pmove->velocity[2] > 180)   // Shooting up really fast.  Definitely not on ground.
	{
		pmove->onground = -1;
	}
	else
	{
		// Try and move down.
		tr = pmove->PM_PlayerTrace (pmove->origin, point, PM_NORMAL, -1 );
		// If we hit a steep plane, we are not on ground
		if ( tr.plane.normal[2] < 0.7)
			pmove->onground = -1;	// too steep
		else
			pmove->onground = tr.ent;  // Otherwise, point to index of ent under us.

		// If we are on something...
		if (pmove->onground != -1)
		{
			pmove->waterjumptime = 0;
			if (!tr.startsolid && !tr.allsolid)
				VectorCopy (tr.endpos, pmove->origin);
		}

		// standing on an entity other than the world
		if (tr.ent > 0)
		{
			PM_AddToTouched(tr, pmove->velocity);
		}
	}

	//
	// get waterlevel
	//
	PM_CheckWater();
}

/*
=================
PM_GetRandomStuckOffsets

When a player is stuck, it's costly to try and unstick them
Grab a test offset for the player based on a passed in index
=================
*/
int PM_GetRandomStuckOffsets(int nIndex, int server, vec3_t offset)
{
 // Last time we did a full
	int idx;
	idx = rgStuckLast[nIndex][server]++;

	VectorCopy(rgv3tStuckTable[idx % 54], offset);

	return (idx % 54);
}

void PM_ResetStuckOffsets(int nIndex, int server)
{
	rgStuckLast[nIndex][server] = 0;
}

/*
=================
NudgePosition

If pmove->origin is in a solid position,
try nudging slightly on all axis to
allow for the cut precision of the net coordinates
=================
*/
#define PM_CHECKSTUCK_MINTIME 0.05  // Don't check again too quickly.

int PM_CheckStuck (void)
{
	vec3_t	base;
	vec3_t  offset;
	vec3_t  test;
	int     hitent;
	int		idx;
	float	fTime;
	int i;
	pmtrace_t traceresult;

	static float rgStuckCheckTime[MAX_CLIENTS][2]; // Last time we did a full

	// If position is okay, exit
	hitent = pmove->PM_TestPlayerPosition (pmove->origin, &traceresult );
	if (hitent == -1 )
	{
		PM_ResetStuckOffsets( pmove->player_index, pmove->server );
		return 0;
	}

	VectorCopy (pmove->origin, base);

	// 
	// Deal with precision error in network.
	// 
	if (!pmove->server)
	{
		// World or BSP model
		if ( ( hitent == 0 ) ||
			 ( pmove->physents[hitent].model != NULL ) )
		{
			int nReps = 0;
			PM_ResetStuckOffsets( pmove->player_index, pmove->server );
			do 
			{
				i = PM_GetRandomStuckOffsets(pmove->player_index, pmove->server, offset);

				VectorAdd(base, offset, test);
				if (pmove->PM_TestPlayerPosition (test, &traceresult ) == -1)
				{
					PM_ResetStuckOffsets( pmove->player_index, pmove->server );
		
					VectorCopy ( test, pmove->origin );
					return 0;
				}
				nReps++;
			} while (nReps < 54);
		}
	}

	// Only an issue on the client.

	if (pmove->server)
		idx = 0;
	else
		idx = 1;

	fTime = pmove->Sys_FloatTime();
	// Too soon?
	if (rgStuckCheckTime[pmove->player_index][idx] >= 
		( fTime - PM_CHECKSTUCK_MINTIME ) )
	{
		return 1;
	}
	rgStuckCheckTime[pmove->player_index][idx] = fTime;

	pmove->PM_StuckTouch( hitent, &traceresult );

	i = PM_GetRandomStuckOffsets(pmove->player_index, pmove->server, offset);

	VectorAdd(base, offset, test);
	if ( ( hitent = pmove->PM_TestPlayerPosition ( test, NULL ) ) == -1 )
	{
		//Con_DPrintf("Nudged\n");

		PM_ResetStuckOffsets( pmove->player_index, pmove->server );
		VectorCopy ( test, pmove->origin );

		return 0;
	}

	// If player is flailing while stuck in another player ( should never happen ), then see
	//  if we can't "unstick" them forceably.
	if ( pmove->cmd.buttons & ( IN_JUMP | IN_ATTACK ) && ( pmove->physents[ hitent ].player != 0 ) )
	{
		float x, y, z;
		float xystep = 8.0;
		float zstep = 18.0;
		float xyminmax = xystep;
		float zminmax = 4 * zstep;
		
		for ( z = 0; z <= zminmax; z += zstep )
		{
			for ( x = -xyminmax; x <= xyminmax; x += xystep )
			{
				for ( y = -xyminmax; y <= xyminmax; y += xystep )
				{
					VectorCopy( base, test );
					test[0] += x;
					test[1] += y;
					test[2] += z;

					if ( pmove->PM_TestPlayerPosition ( test, NULL ) == -1 )
					{
						VectorCopy( test, pmove->origin );
						return 0;
					}
				}
			}
		}
	}

	//VectorCopy (base, pmove->origin);

	return 1;
}

/*
===============
PM_SpectatorMove
===============
*/
void PM_SpectatorMove (void)
{
	float	speed, drop, friction, control, newspeed;
	//float   accel;
	float	currentspeed, addspeed, accelspeed;
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
	vec3_t		wishdir;
	float		wishspeed;
	// this routine keeps track of the spectators psoition
	// there a two different main move types : track player or moce freely (OBS_ROAMING)
	// doesn't need excate track position, only to generate PVS, so just copy
	// targets position and real view position is calculated on client (saves server CPU)
	
	if ( pmove->iuser1 == OBS_ROAMING)
	{

#ifdef CLIENT_DLL
		// jump only in roaming mode
		if ( iJumpSpectator )
		{
			VectorCopy( vJumpOrigin, pmove->origin );
			VectorCopy( vJumpAngles, pmove->angles );
			VectorCopy( vec3_origin, pmove->velocity );
			iJumpSpectator	= 0;
			return;
		}
		#endif
		// Move around in normal spectator method

		speed = Length (pmove->velocity);
		if (speed < 1)
		{
			VectorCopy (vec3_origin, pmove->velocity)
		}
		else
		{
			drop = 0;

			friction = pmove->movevars->friction*1.5;	// extra friction
			control = speed < pmove->movevars->stopspeed ? pmove->movevars->stopspeed : speed;
			drop += control*friction*pmove->frametime;

			// scale the velocity
			newspeed = speed - drop;
			if (newspeed < 0)
				newspeed = 0;
			newspeed /= speed;

			VectorScale (pmove->velocity, newspeed, pmove->velocity);
		}

		// accelerate
		fmove = pmove->cmd.forwardmove;
		smove = pmove->cmd.sidemove;
		
		VectorNormalize (pmove->forward);
		VectorNormalize (pmove->right);

		for (i=0 ; i<3 ; i++)
		{
			wishvel[i] = pmove->forward[i]*fmove + pmove->right[i]*smove;
		}
		wishvel[2] += pmove->cmd.upmove;

		VectorCopy (wishvel, wishdir);
		wishspeed = VectorNormalize(wishdir);

		//
		// clamp to server defined max speed
		//
		if (wishspeed > pmove->movevars->spectatormaxspeed)
		{
			VectorScale (wishvel, pmove->movevars->spectatormaxspeed/wishspeed, wishvel);
			wishspeed = pmove->movevars->spectatormaxspeed;
		}

		currentspeed = DotProduct(pmove->velocity, wishdir);
		addspeed = wishspeed - currentspeed;
		if (addspeed <= 0)
			return;

		accelspeed = pmove->movevars->accelerate*pmove->frametime*wishspeed;
		if (accelspeed > addspeed)
			accelspeed = addspeed;
		
		for (i=0 ; i<3 ; i++)
			pmove->velocity[i] += accelspeed*wishdir[i];	

		// move
		VectorMA (pmove->origin, pmove->frametime, pmove->velocity, pmove->origin);
	}
	else
	{
		// all other modes just track some kind of target, so spectator PVS = target PVS

		int target;

		// no valid target ?
		if ( pmove->iuser2 <= 0)
			return;

		// Find the client this player's targeting
		for (target = 0; target < pmove->numphysent; target++)
		{
			if ( pmove->physents[target].info == pmove->iuser2 )
				break;
		}

		if (target == pmove->numphysent)
			return;

		// use targets position as own origin for PVS
		VectorCopy( pmove->physents[target].angles, pmove->angles );
		VectorCopy( pmove->physents[target].origin, pmove->origin );

		// no velocity
		VectorCopy( vec3_origin, pmove->velocity );
	}
}

void PM_WaterJump (void)
{
	if ( pmove->waterjumptime > 2 )
	{
		pmove->waterjumptime = 2;
	}

	if ( !pmove->waterjumptime )
		return;

	pmove->waterjumptime -= pmove->cmd.msec;
	if ( pmove->waterjumptime < 0 ||
		 !pmove->waterlevel )
	{
		pmove->waterjumptime = 0;
		pmove->flags &= ~FL_WATERJUMP;
	}

	// g-cont. to differintiate with teleport_time
	if(pmove->flags & FL_WATERJUMP)
	{
		pmove->velocity[0] = pmove->movedir[0];
		pmove->velocity[1] = pmove->movedir[1];
	}
}

/*
============
PM_AddGravity

============
*/
void PM_AddGravity ()
{
	float	ent_gravity;

	if (pmove->gravity)
		ent_gravity = pmove->gravity;
	else
		ent_gravity = 1.0;

	// Add gravity incorrectly
	pmove->velocity[2] -= (ent_gravity * pmove->movevars->gravity * pmove->frametime );
	pmove->velocity[2] += pmove->basevelocity[2] * pmove->frametime;
	pmove->basevelocity[2] = 0;
	PM_CheckVelocity();
}
/*
============
PM_PushEntity

Does not change the entities velocity at all
============
*/
pmtrace_t PM_PushEntity (vec3_t push)
{
	pmtrace_t	trace;
	vec3_t	end;
		
	VectorAdd (pmove->origin, push, end);

	trace = pmove->PM_PlayerTrace (pmove->origin, end, PM_NORMAL, -1 );
	
	VectorCopy (trace.endpos, pmove->origin);

	// So we can run impact function afterwards.
	if (trace.fraction < 1.0 &&
		!trace.allsolid)
	{
		PM_AddToTouched(trace, pmove->velocity);
	}

	return trace;
}	

/*
============
PM_Physics_Toss()

Dead player flying through air., e.g.
============
*/
void PM_Physics_Toss()
{
	pmtrace_t trace;
	vec3_t	move;
	float	backoff;

	PM_CheckWater();

	if (pmove->velocity[2] > 0)
		pmove->onground = -1;

	// If on ground and not moving, return.
	if ( pmove->onground != -1 )
	{
		if (VectorCompare(pmove->basevelocity, vec3_origin) &&
		    VectorCompare(pmove->velocity, vec3_origin))
			return;
	}

	PM_CheckVelocity ();

// add gravity
	if ( pmove->movetype != MOVETYPE_FLY &&
		 pmove->movetype != MOVETYPE_BOUNCEMISSILE &&
		 pmove->movetype != MOVETYPE_FLYMISSILE )
		PM_AddGravity ();

// move origin
	// Base velocity is not properly accounted for since this entity will move again after the bounce without
	// taking it into account
	VectorAdd (pmove->velocity, pmove->basevelocity, pmove->velocity);
	
	PM_CheckVelocity();
	VectorScale (pmove->velocity, pmove->frametime, move);
	VectorSubtract (pmove->velocity, pmove->basevelocity, pmove->velocity);

	trace = PM_PushEntity (move);	// Should this clear basevelocity

	PM_CheckVelocity();

	if (trace.allsolid)
	{	
		// entity is trapped in another solid
		pmove->onground = trace.ent;
		VectorCopy (vec3_origin, pmove->velocity);
		return;
	}
	
	if (trace.fraction == 1)
	{
		PM_CheckWater();
		return;
	}


	if (pmove->movetype == MOVETYPE_BOUNCE)
		backoff = 2.0 - pmove->friction;
	else if (pmove->movetype == MOVETYPE_BOUNCEMISSILE)
		backoff = 2.0;
	else
		backoff = 1;

	PM_ClipVelocity (pmove->velocity, trace.plane.normal, pmove->velocity, backoff);

	// stop if on ground
	if (trace.plane.normal[2] > 0.7)
	{		
		float vel;
		vec3_t base;

		VectorClear( base );
		if (pmove->velocity[2] < pmove->movevars->gravity * pmove->frametime)
		{
			// we're rolling on the ground, add static friction.
			pmove->onground = trace.ent;
			pmove->velocity[2] = 0;
		}

		vel = DotProduct( pmove->velocity, pmove->velocity );

		// Con_DPrintf("%f %f: %.0f %.0f %.0f\n", vel, trace.fraction, ent->velocity[0], ent->velocity[1], ent->velocity[2] );

		if (vel < (30 * 30) || (pmove->movetype != MOVETYPE_BOUNCE && pmove->movetype != MOVETYPE_BOUNCEMISSILE))
		{
			pmove->onground = trace.ent;
			VectorCopy (vec3_origin, pmove->velocity);
		}
		else
		{
			VectorScale (pmove->velocity, (1.0 - trace.fraction) * pmove->frametime * 0.9, move);
			trace = PM_PushEntity (move);
		}
		VectorSubtract( pmove->velocity, base, pmove->velocity )
	}
	
// check for in water
	PM_CheckWater();
}

/*
====================
PM_NoClip

====================
*/
void PM_NoClip()
{
	int			i;
	vec3_t		wishvel;
	float		fmove, smove;
//	float		currentspeed, addspeed, accelspeed;

	// Copy movement amounts
	fmove = pmove->cmd.forwardmove;
	smove = pmove->cmd.sidemove;
	
	VectorNormalize ( pmove->forward ); 
	VectorNormalize ( pmove->right );

	for (i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
	{
		wishvel[i] = pmove->forward[i]*fmove + pmove->right[i]*smove;
	}
	wishvel[2] += pmove->cmd.upmove;

	VectorMA (pmove->origin, pmove->frametime, wishvel, pmove->origin);
	
	// Zero out the velocity so that we don't accumulate a huge downward velocity from
	//  gravity, etc.
	VectorClear( pmove->velocity );

}

/*
=============
PM_Jump
=============
*/
void PM_Jump (void)
{
	if (pmove->dead)
	{
		pmove->oldbuttons |= IN_JUMP ;	// don't jump again until released
		return;
	}

	// See if we are waterjumping.  If so, decrement count and return.
	if ( pmove->waterjumptime )
	{
		pmove->waterjumptime -= pmove->frametime;
		if (pmove->waterjumptime < 0)
			pmove->waterjumptime = 0;
		return;
	}

	// If we are in the water most of the way...
	if (pmove->waterlevel >= 2)
	{	// swimming, not jumping
		pmove->onground = -1;

		if (pmove->watertype == CONTENTS_WATER)    // We move up a certain amount
			pmove->velocity[2] = 100;
		else if (pmove->watertype == CONTENTS_SLIME)
			pmove->velocity[2] = 80;
		else  // LAVA
			pmove->velocity[2] = 50;

		// play swiming sound
		if ( pmove->flSwimTime <= 0 )
		{
			// Don't play sound again for 1 second
			pmove->flSwimTime = 1000;
		}

		return;
	}

 	if ( pmove->onground == -1 )
		return;		// in air, so no effect

	if ( pmove->oldbuttons & IN_JUMP )
		return;		// don't pogo stick

	// In the air now.
	pmove->onground = -1;

	pmove->PM_PlaySound( CHAN_BODY, "player/plyrjmp8.wav", 0.5, ATTN_NORM, 0, PITCH_NORM ); //magic nipples - jump sound here works 100% over player.cpp

	pmove->velocity[2] += 270;

	// Flag that we jumped.
	pmove->oldbuttons |= IN_JUMP;	// don't jump again until released
}

/*
=============
PM_CheckWaterJump
=============
*/
#define WJ_HEIGHT 8
void PM_CheckWaterJump (void)
{
	vec3_t	spot;
	int		cont;
	vec3_t	flatforward;

	if ( pmove->waterjumptime )
		return;

	// ZOID, don't hop out if we just jumped in
	if ( pmove->velocity[2] < -180 )
		return; // only hop out if we are moving up
	
	// see if near an edge
	flatforward[0] = pmove->forward[0];
	flatforward[1] = pmove->forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA ( pmove->origin, 24, flatforward, spot );
	spot[2] += WJ_HEIGHT;

	cont = pmove->PM_PointContents (spot, NULL );
	if (cont != CONTENTS_SOLID)
		return;

	spot[2] += 24;
	cont = pmove->PM_PointContents (spot, NULL );
	if (cont != CONTENTS_EMPTY)
		return;
	// jump out of water
	VectorScale (flatforward, 50, pmove->movedir);
	pmove->velocity[2] = 310;
	pmove->waterjumptime = 2;	// safety net
	pmove->oldbuttons |= IN_JUMP;	// don't jump again until released
	pmove->flags |= FL_WATERJUMP;
}

void PM_CheckFalling( void )
{
	if ( pmove->onground != -1 &&
		 !pmove->dead &&
		 pmove->flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD )
	{
		float fvol = 0.5;

		if ( pmove->waterlevel > 0 )
		{
		}
		else if ( pmove->flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED )
		{
			pmove->PM_PlaySound( CHAN_VOICE, "player/land2.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			fvol = 1.0;
		}
		else if ( pmove->flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED / 2 )
		{
			pmove->PM_PlaySound( CHAN_VOICE, "player/use1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			fvol = 1.0;
		}
		else if ( pmove->flFallVelocity < PLAYER_MIN_BOUNCE_SPEED )
		{
			fvol = 0;
		}

		if ( fvol > 0.0 )
		{
			// Play landing step right away
			pmove->flTimeStepSound = 0;

			// Knock the screen around a little bit, temporary effect
			pmove->punchangle[ 2 ] = pmove->flFallVelocity * 0.013;	// punch z axis

			if ( pmove->punchangle[ 0 ] > 8 )
			{
				pmove->punchangle[ 0 ] = 8;
			}
		}
	}

	if ( pmove->onground != -1 ) 
	{		
		pmove->flFallVelocity = 0;
	}
}

/*
===============
PM_CalcRoll

===============
*/
float PM_CalcRoll (vec3_t angles, vec3_t velocity, float rollangle, float rollspeed )
{
    float   sign;
    float   side;
    float   value;
	vec3_t  forward, right, up;
    
	AngleVectors (angles, forward, right, up);
    
	side = DotProduct (velocity, right);
    
	sign = side < 0 ? -1 : 1;
    
	side = fabs(side);
    
	value = rollangle;
    
	if (side < rollspeed)
	{
		side = side * value / rollspeed;
	}
    else
	{
		side = value;
	}
  
	return side * sign;
}

/*
=============
PM_DropPunchAngle

=============
*/
void PM_DropPunchAngle ( vec3_t punchangle )
{
	float	len;
	
	len = VectorNormalize ( punchangle );
	len -= (10.0 + len * 0.5) * pmove->frametime;
	len = max( len, 0.0 );
	VectorScale ( punchangle, len, punchangle);
}

/*
==============
PM_CheckParamters

==============
*/
void PM_CheckParamters( void )
{
	vec3_t	v_angle;

	if (  pmove->dead )
	{
		pmove->cmd.forwardmove = 0;
		pmove->cmd.sidemove    = 0;
		pmove->cmd.upmove      = 0;
	}


	PM_DropPunchAngle( pmove->punchangle );

	// Take angles from command.
	if ( !pmove->dead )
	{
		VectorCopy ( pmove->cmd.viewangles, v_angle );         
		VectorAdd( v_angle, pmove->punchangle, v_angle );

		// Set up view angles.
		pmove->angles[ROLL]	=	PM_CalcRoll ( v_angle, pmove->velocity, pmove->movevars->rollangle, pmove->movevars->rollspeed )*4;
		pmove->angles[PITCH] =	v_angle[PITCH];
		pmove->angles[YAW]   =	v_angle[YAW];
	}
	else
	{
		VectorCopy( pmove->oldangles, pmove->angles );
	}

	// Set dead player view_offset
	if ( pmove->dead )
	{
		pmove->view_ofs[2] = PM_DEAD_VIEWHEIGHT;
	}

	// Adjust client view angles to match values used on server.
	if (pmove->angles[YAW] > 180.0f)
	{
		pmove->angles[YAW] -= 360.0f;
	}

}

void PM_ReduceTimers( void )
{
	if ( pmove->flTimeStepSound > 0 )
	{
		pmove->flTimeStepSound -= pmove->cmd.msec;
		if ( pmove->flTimeStepSound < 0 )
		{
			pmove->flTimeStepSound = 0;
		}
	}
	if ( pmove->flSwimTime > 0 )
	{
		pmove->flSwimTime -= pmove->cmd.msec;
		if ( pmove->flSwimTime < 0 )
		{
			pmove->flSwimTime = 0;
		}
	}
}

/*
=============
PlayerMove

Returns with origin, angles, and velocity modified in place.

Numtouch and touchindex[] will be set if any of the physents
were contacted during the move.
=============
*/
void PM_PlayerMove ( qboolean server )
{
	// Are we running server code?
	pmove->server = server;                

	// Adjust speeds etc.
	PM_CheckParamters();

	// Assume we don't touch anything
	pmove->numtouch = 0;                    

	// # of msec to apply movement
	pmove->frametime = pmove->cmd.msec * 0.001;    

	PM_ReduceTimers();

	// Convert view angles to vectors
	AngleVectors (pmove->angles, pmove->forward, pmove->right, pmove->up);

	// PM_ShowClipBox();

	// Special handling for spectator and observers. (iuser1 is set if the player's in observer mode)
	if ( pmove->spectator || pmove->iuser1 > 0 )
	{
		PM_SpectatorMove();
		PM_CatagorizePosition();
		return;
	}

	// Always try and unstick us unless we are in NOCLIP mode
	if ( pmove->movetype != MOVETYPE_NOCLIP && pmove->movetype != MOVETYPE_NONE )
	{
		if ( PM_CheckStuck() )
			return;  // Can't move, we're stuck
	}

	// set onground, watertype, and waterlevel
	PM_CatagorizePosition();

	// Store off the starting water level
	pmove->oldwaterlevel = pmove->waterlevel;

	// If we are not on ground, store off how fast we are moving down
	if ( pmove->onground == -1 )
		pmove->flFallVelocity = -pmove->velocity[2];

	// Handle movement
	switch ( pmove->movetype )
	{
	default:
		pmove->Con_DPrintf("Bogus pmove player movetype %i on (%i) 0=cl 1=sv\n", pmove->movetype, pmove->server);
		break;

	case MOVETYPE_NONE:
		break;

	case MOVETYPE_NOCLIP:
		PM_NoClip();
		break;

	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
		PM_Physics_Toss();
		break;

	case MOVETYPE_FLY:
		if ( !PM_InWater() )
			PM_AddCorrectGravity();
	
		PM_CheckWater();

		//if ( pmove->cmd.buttons & IN_JUMP )
		//	PM_Jump ();
		//else
			//pmove->oldbuttons &= ~IN_JUMP;
		
		PM_AirMove();
		break;

	case MOVETYPE_WALK:
		// If we are leaping out of the water, just update the counters.
		if ( pmove->waterjumptime )
		{
			PM_WaterJump();
			PM_FlyMove();

			// Make sure waterlevel is set correctly
			PM_CheckWater();
			return;
		}

		if ( pmove->waterlevel == 2 )
			PM_CheckWaterJump();

		if (pmove->velocity[2] < 0 && pmove->waterjumptime)
			pmove->waterjumptime = 0;

		if (pmove->cmd.buttons & IN_JUMP)
			PM_Jump ();
		else
			pmove->oldbuttons &= ~IN_JUMP;

#ifndef WOLF3DGAME
		PM_Friction(); //WOLF 3D - NO FRICTION
#endif

		if (pmove->waterlevel >= 2)
			PM_WaterMove ();
		else
			PM_AirMove ();

		// See if we landed on the ground with enough force to play
		//  a landing sound.
		PM_CheckFalling();

		// set onground, watertype, and waterlevel for final spot
		PM_CatagorizePosition ();
		break;
	}

#ifndef WOLF3DGAME
	if (pmove->alwaysrun)
	{
		if ((pmove->onground != -1) && (pmove->cmd.buttons & IN_RUN))
		{
			VectorScale(pmove->velocity, 0.935, pmove->velocity);
		}
	}
	else
	{
		if (pmove->onground != -1)
		{
			if (pmove->cmd.buttons & IN_RUN) {}
			else
				VectorScale(pmove->velocity, 0.935, pmove->velocity);
		}
	}
#endif
}

void PM_CreateStuckTable( void )
{
	float x, y, z;
	int idx;
	int i;
	float zi[3];

	memset(rgv3tStuckTable, 0, 54 * sizeof(vec3_t));

	idx = 0;
	// Little Moves.
	x = y = 0;
	// Z moves
	for (z = -0.125 ; z <= 0.125 ; z += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	x = z = 0;
	// Y moves
	for (y = -0.125 ; y <= 0.125 ; y += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -0.125 ; x <= 0.125 ; x += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for ( x = - 0.125; x <= 0.125; x += 0.250 )
	{
		for ( y = - 0.125; y <= 0.125; y += 0.250 )
		{
			for ( z = - 0.125; z <= 0.125; z += 0.250 )
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}

	// Big Moves.
	x = y = 0;
	zi[0] = 0.0f;
	zi[1] = 1.0f;
	zi[2] = 6.0f;

	for (i = 0; i < 3; i++)
	{
		// Z moves
		z = zi[i];
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	x = z = 0;

	// Y moves
	for (y = -2.0f ; y <= 2.0f ; y += 2.0)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -2.0f ; x <= 2.0f ; x += 2.0f)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for (i = 0 ; i < 3; i++)
	{
		z = zi[i];
		
		for (x = -2.0f ; x <= 2.0f ; x += 2.0f)
		{
			for (y = -2.0f ; y <= 2.0f ; y += 2.0)
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}
}

/*
This modume implements the shared player physics code between any particular game and 
the engine.  The same PM_Move routine is built into the game .dll and the client .dll and is
invoked by each side as appropriate.  There should be no distinction, internally, between server
and client.  This will ensure that prediction behaves appropriately.
*/

void PM_Move ( struct playermove_s *ppmove, int server )
{
	assert( pm_shared_initialized );

	pmove = ppmove;

//	pmove->Con_Printf( "PM_Move: %g, frametime %g, onground %i\n", pmove->time, pmove->frametime, pmove->onground );
	
	PM_PlayerMove( ( server != 0 ) ? true : false );

	if ( pmove->onground != -1 )
	{
		pmove->flags |= FL_ONGROUND;
	}
	else
	{
		pmove->flags &= ~FL_ONGROUND;
	}

	// In single player, reset friction after each movement to FrictionModifier Triggers work still.
	if ( !pmove->multiplayer && ( pmove->movetype == MOVETYPE_WALK  ) )
	{
		pmove->friction = 1.0f;
	}
}

int PM_GetVisEntInfo( int ent )
{
	if ( ent >= 0 && ent <= pmove->numvisent )
	{
		return pmove->visents[ ent ].info;
	}
	return -1;
}

int PM_GetPhysEntInfo( int ent )
{
	if ( ent >= 0 && ent <= pmove->numphysent)
	{
		return pmove->physents[ ent ].info;
	}
	return -1;
}

void PM_Init( struct playermove_s *ppmove )
{
	assert( !pm_shared_initialized );

	pmove = ppmove;

	PM_CreateStuckTable();

	pm_shared_initialized = 1;
}
