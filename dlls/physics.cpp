/*

===== physics.cpp ========================================================

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
#include "physcallback.h"
#include "com_model.h"
#include "features.h"

extern DLL_GLOBAL BOOL		g_fXashEngine;

//
// Xash3D physics interface
//
#define IS_NAN(x)		(((*(int *)&x) & (255<<23)) == (255<<23))
#define STOP_EPSILON	0.1f
#define MAX_CLIP_PLANES	5

//
// Xash3D physics interface
//
unsigned int EngineSetFeatures( void )
{
	return ENGINE_QUAKE_COMPATIBLE;
}

//
// attempt to create custom entity when default method is failed
// 0 - attempt to create, -1 - reject to create
//
int DispatchCreateEntity( edict_t *pent, const char *szName )
{
	return -1;
}

/*
===============
UTIL_DropToFloor

===============
*/
int UTIL_DropToFloor( CBaseEntity *pEntity )
{
	Vector end;
	TraceResult trace;

	end = pEntity->pev->origin;
	end.z -= 256;

	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	TRACE_MONSTER_HULL( ENT( pEntity->pev ), pEntity->pev->origin, end, dont_ignore_monsters, ENT( pEntity->pev ), &trace ); 

	if( trace.fAllSolid )
		return -1;

	if( trace.flFraction == 1.0f )
		return 0;

	if (g_fXashEngine && g_physfuncs.pfnLinkEdict != NULL)
	{
		pEntity->pev->origin = trace.vecEndPos;
		LINK_ENTITY( ENT( pEntity->pev ), FALSE );
	}
	else
	{
		UTIL_SetOrigin( pEntity->pev, trace.vecEndPos );
	}

	pEntity->pev->flags |= FL_ONGROUND;
	pEntity->pev->groundentity = trace.pHit;

	return 1;
}

/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
BOOL SV_MoveStep( CBaseEntity *pEntity, const Vector &vecMove, BOOL relink )
{
	float		dz;
	TraceResult	trace;

	// try the move	
	Vector oldorg = pEntity->pev->origin;
	Vector neworg = pEntity->pev->origin + vecMove;

	// flying monsters don't step up
	if( pEntity->pev->flags & ( FL_SWIM|FL_FLY ))
	{
		// try one move with vertical motion, then one without
		for( int i = 0; i < 2; i++ )
		{
			CBaseEntity *pEnemy = pEntity->m_hEnemy;
			neworg = pEntity->pev->origin + vecMove;

			if( i == 0 && pEnemy != NULL )
			{
				dz = pEntity->pev->origin.z - pEnemy->pev->origin.z;

				if( dz > 40 )
					neworg.z -= 8;
				if( dz < 30 )
					neworg.z += 8;
			}

			gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
			TRACE_MONSTER_HULL( pEntity->edict(), pEntity->pev->origin, neworg, dont_ignore_monsters, pEntity->edict(), &trace ); 
	
			if (trace.fAllSolid) //magic nipples - fish could go into walls via slopes
				return FALSE;

			if( trace.flFraction == 1.0f )
			{
				float orgz;
				if ((pEntity->pev->flags & FL_SWIM) && POINT_CONTENTS(trace.vecEndPos) == CONTENTS_EMPTY)
					orgz = pEntity->pev->origin.z; //return FALSE; // swim monster left water			
	
				pEntity->pev->origin = trace.vecEndPos;

				if( relink )
					UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );

				if ((pEntity->pev->flags & FL_SWIM) && POINT_CONTENTS(trace.vecEndPos) == CONTENTS_EMPTY)
					pEntity->pev->origin.z = orgz;
				return TRUE;
			}
			
			if( !pEnemy )
				break;
		}
		
		return FALSE;
	}

	float stepSize = CVAR_GET_FLOAT( "sv_stepsize" );

	// push down from a step height above the wished position
	neworg.z += stepSize;
	Vector end = neworg;
	end.z -= stepSize * 2;

	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	TRACE_MONSTER_HULL( pEntity->edict(), neworg, end, dont_ignore_monsters, pEntity->edict(), &trace ); 

	if( trace.fAllSolid )
		return FALSE;

	if( trace.fStartSolid )
	{
		neworg.z -= stepSize;
		gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
		TRACE_MONSTER_HULL( pEntity->edict(), neworg, end, dont_ignore_monsters, pEntity->edict(), &trace ); 

		if( trace.fAllSolid || trace.fStartSolid )
			return FALSE;
	}

	if( trace.flFraction == 1.0f )
	{
		// if monster had the ground pulled out, go ahead and fall
		if( pEntity->pev->flags & FL_PARTIALGROUND )
		{
			pEntity->pev->origin += vecMove;

			if( relink )
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );

			pEntity->pev->flags &= ~FL_ONGROUND;
			return TRUE;
		}
	
		return FALSE; // walked off an edge
	}

	// check point traces down for dangling corners
	pEntity->pev->origin = trace.vecEndPos;

	if( !ENT_IS_ON_FLOOR( pEntity->edict() ))	
	{
		if( pEntity->pev->flags & FL_PARTIALGROUND )
		{	
			// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if( relink )
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );
			return TRUE;
		}

		pEntity->pev->origin = oldorg;
		return FALSE;
	}

	if( pEntity->pev->flags & FL_PARTIALGROUND )
		pEntity->pev->flags &= ~FL_PARTIALGROUND;

	pEntity->pev->groundentity = trace.pHit;

	// the move is ok
	if( relink )
		UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );

	return TRUE;
}

/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
BOOL SV_StepDirection( CBaseEntity *pEntity, float flYaw, float flDist )
{
	Vector	move, oldorigin;
	float	delta;
	
	pEntity->pev->ideal_yaw = flYaw;
	CHANGE_YAW( pEntity->edict() );
	
	flYaw = flYaw * M_PI * 2 / 360;
	move.x = cos( flYaw ) * flDist;
	move.y = sin( flYaw ) * flDist;
	move.z = 0.0f;

	oldorigin = pEntity->pev->origin;

	if( SV_MoveStep( pEntity, move, false ))
	{
		delta = pEntity->pev->angles.y - pEntity->pev->ideal_yaw;

		if( delta > 45 && delta < 315 )
		{	
			// not turned far enough, so don't take the step
			pEntity->pev->origin = oldorigin;
		}
                    
		UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );
		return TRUE;
	}

	UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );
		
	return FALSE;
}

/*
================
SV_NewChaseDir

================
*/
void SV_NewChaseDir( CBaseEntity *pActor, CBaseEntity *pEnemy, float flDist )
{
	float	deltax, deltay;
	float	tdir, olddir, turnaround;
	Vector	d;

	olddir = UTIL_AngleMod( (int)(pActor->pev->ideal_yaw / 45 ) * 45 );
	turnaround = UTIL_AngleMod( olddir - 180 );

	deltax = pEnemy->pev->origin.x - pActor->pev->origin.x;
	deltay = pEnemy->pev->origin.y - pActor->pev->origin.y;

	if( deltax > 10 )
		d[1] = 0;
	else if( deltax < -10 )
		d[1] = 180;
	else
		d[1] = -1;

	if( deltay < -10 )
		d[2] = 270;
	else if( deltay > 10 )
		d[2] = 90;
	else
		d[2] = -1;

	// try direct route
	if( d[1] != -1 && d[2] != -1 )
	{
		if( d[1] == 0 )
			tdir = d[2] == 90 ? 45 : 315;
		else
			tdir = d[2] == 90 ? 135 : 215;
			
		if( tdir != turnaround && SV_StepDirection( pActor, tdir, flDist ))
			return;
	}

	// try other directions
	if((( rand() & 3 ) & 1 ) || fabs( deltay ) > fabs( deltax ))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if( d[1] != -1 && d[1] != turnaround && SV_StepDirection( pActor, d[1], flDist ))
		return;

	if( d[2] != -1 && d[2] != turnaround && SV_StepDirection( pActor, d[2], flDist ))
		return;

	// there is no direct path to the player, so pick another direction
	if( olddir != -1 && SV_StepDirection( pActor, olddir, flDist ))
		return;

	if( rand() & 1 ) // randomly determine direction of search
	{
		for( tdir = 0; tdir <= 315; tdir += 45 )
		{
			if( tdir != turnaround && SV_StepDirection( pActor, tdir, flDist ))
				return;
		}
	}
	else
	{
		for( tdir = 315; tdir >= 0; tdir -= 45 )
		{
			if( tdir != turnaround && SV_StepDirection( pActor, tdir, flDist ))
				return;
		}
	}

	if( turnaround != -1 && SV_StepDirection( pActor, turnaround, flDist ))
		return;

	pActor->pev->ideal_yaw = olddir; // can't move

	// if a bridge was pulled out from underneath a monster, it may not have
	// a valid standing position at all
	if( !ENT_IS_ON_FLOOR( pActor->edict() ))
		pActor->pev->flags |= FL_PARTIALGROUND;
}

/*
======================
SV_CloseEnough

======================
*/
BOOL SV_CloseEnough( CBaseEntity *pEntity, CBaseEntity *pGoal, float flDist )
{
	for( int i = 0; i < 3; i++ )
	{
		if( pGoal->pev->absmin[i] > pEntity->pev->absmax[i] + flDist )
			return FALSE;
		if( pGoal->pev->absmax[i] < pEntity->pev->absmin[i] - flDist )
			return FALSE;
	}
	return TRUE;
}

/*
======================
SV_MoveToGoal

======================
*/
int SV_MoveToGoal( CBaseEntity *pEntity, float flDist )
{
	if(!( pEntity->pev->flags & (FL_ONGROUND|FL_FLY|FL_SWIM )))
		return 0;

	CBaseEntity *pGoal = pEntity->m_pGoalEnt;
	CBaseEntity *pEnemy = pEntity->m_hEnemy;

	if( !pGoal )
		return 0;

	if( pEnemy != NULL && SV_CloseEnough( pEntity, pGoal, flDist ))
		return 0;

	// bump around...
	if(( rand() & 3 ) == 1 || !SV_StepDirection( pEntity, pEntity->pev->ideal_yaw, flDist ))
		SV_NewChaseDir( pEntity, pGoal, flDist );

	return 1;
}

/*
======================
SV_WalkMove

======================
*/
int SV_WalkMove( CBaseEntity *pEntity, float flYaw, float flDist )
{
	Vector	move;

	if(!( pEntity->pev->flags & ( FL_FLY|FL_SWIM|FL_ONGROUND )))
		return 0;

	flYaw = flYaw * M_PI * 2 / 360;
	move.x = cos( flYaw ) * flDist;
	move.y = sin( flYaw ) * flDist;
	move.z = 0.0f;

	return SV_MoveStep( pEntity, move, true );
}


/*
==================
SV_CopyTraceToGlobal

share local trace with global variables
==================
*/
void SV_CopyTraceToGlobal( TraceResult *trace )
{
	gpGlobals->trace_allsolid = trace->fAllSolid;
	gpGlobals->trace_startsolid = trace->fStartSolid;
	gpGlobals->trace_fraction = trace->flFraction;
	gpGlobals->trace_plane_dist = trace->flPlaneDist;
	gpGlobals->trace_flags = 0;
	gpGlobals->trace_inopen = trace->fInOpen;
	gpGlobals->trace_inwater = trace->fInWater;
	gpGlobals->trace_endpos = trace->vecEndPos;
	gpGlobals->trace_plane_normal = trace->vecPlaneNormal;
	gpGlobals->trace_hitgroup = trace->iHitgroup;

	if( !FNullEnt( trace->pHit ))
		gpGlobals->trace_ent = trace->pHit;
	else gpGlobals->trace_ent = ENT(0); // world
}

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
BOOL SV_RunThink( CBaseEntity *pEntity )
{
	edict_t	*edict = pEntity->edict();
	float	thinktime;

	thinktime = pEntity->pev->nextthink;

	if (thinktime <= 0 || thinktime > PHYSICS_TIME() + gpGlobals->frametime)
		return true;
		
	if (thinktime < PHYSICS_TIME())
		thinktime = PHYSICS_TIME();	// don't let things stay in the past.
					// it is possible to start that way
					// by a trigger with a local time.
	pEntity->pev->nextthink = 0;

	gpGlobals->time = thinktime;	// ouch!!!
	DispatchThink( edict );

	return !edict->free;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact( CBaseEntity *pEntity1, TraceResult *trace )
{
	CBaseEntity *pEntity2 = CBaseEntity::Instance( trace->pHit );

	gpGlobals->time = PHYSICS_TIME();

	if(( pEntity1->pev->flags|pEntity2->pev->flags ) & FL_SPECTATOR )
		return;

	if( pEntity1->pev->solid != SOLID_NOT )
	{
		SV_CopyTraceToGlobal( trace );
		DispatchTouch( ENT(pEntity1->pev), ENT(pEntity2->pev));
	}

	if( pEntity2->pev->solid != SOLID_NOT )
	{
		SV_CopyTraceToGlobal( trace );
		DispatchTouch( ENT(pEntity2->pev), ENT(pEntity1->pev));
	}
}

/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity (CBaseEntity *pEntity)
{
	float	maxVelocity;

	maxVelocity = CVAR_GET_FLOAT( "sv_maxvelocity" );

	// bound velocity
	for( int i = 0; i < 3; i++ )
	{
		if (IS_NAN(pEntity->pev->velocity[i]))
		{
			ALERT( at_console, "Got a NaN velocity on %s\n", STRING( pEntity->pev->classname ));
			pEntity->pev->velocity[i] = 0;
		}
		if (IS_NAN(pEntity->pev->origin[i]))
		{
			ALERT( at_console, "Got a NaN origin on %s\n", STRING( pEntity->pev->classname ));
			pEntity->pev->origin[i] = 0;
		}
		if (pEntity->pev->velocity[i] > maxVelocity)
			pEntity->pev->velocity[i] = maxVelocity;
		else if (pEntity->pev->velocity[i] < -maxVelocity)
			pEntity->pev->velocity[i] = -maxVelocity;
	}
}

/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
int SV_ClipVelocity( Vector in, Vector normal, Vector &out, float overbounce )
{
	float	backoff;
	float	change;
	int	blocked = 0;
	
	if (normal.z > 0) blocked |= 1;	// floor
	if (!normal.z) blocked |= 2;		// step
	
	backoff = DotProduct( in, normal ) * overbounce;

	for( int i = 0; i < 3; i++ )
	{
		change = normal[i] * backoff;
		out[i] = in[i] - change;

		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
	}
	
	return blocked;
}

/*
============
SV_AddGravity

============
*/
void SV_AddGravity (CBaseEntity *pEntity)
{
	float	ent_gravity, gravity;

	gravity = CVAR_GET_FLOAT ("sv_gravity");

	if (pEntity->pev->gravity)
		ent_gravity = pEntity->pev->gravity;
	else
		ent_gravity = 1.0;

	pEntity->pev->velocity[2] -= ent_gravity * gravity * gpGlobals->frametime;
}

/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
TraceResult SV_PushEntity (CBaseEntity *pEntity, Vector push)
{
	TraceResult trace;
	Vector end;

	end = pEntity->pev->origin + push;		

	gpGlobals->trace_flags = FTRACE_SIMPLEBOX; // ignore hitboxes

	if (pEntity->pev->movetype == MOVETYPE_FLYMISSILE)
		TRACE_MONSTER_HULL( ENT( pEntity->pev ), pEntity->pev->origin, end, missile, ENT( pEntity->pev ), &trace ); 
	else if (pEntity->pev->solid == SOLID_TRIGGER || pEntity->pev->solid == SOLID_NOT)
		// only clip against bmodels
		TRACE_MONSTER_HULL( ENT( pEntity->pev ), pEntity->pev->origin, end, ignore_monsters, ENT( pEntity->pev ), &trace ); 
	else
		TRACE_MONSTER_HULL( ENT( pEntity->pev ), pEntity->pev->origin, end, dont_ignore_monsters, ENT( pEntity->pev ), &trace ); 
	
	pEntity->pev->origin = trace.vecEndPos;
	LINK_ENTITY( ENT( pEntity->pev ), true );

	if (trace.pHit)
		SV_Impact (pEntity, &trace);		

	return trace;
}

/*
=============
SV_CheckWaterTransition

=============
*/
void SV_CheckWaterTransition( CBaseEntity *pEntity )
{
	int cont = UTIL_PointContents(pEntity->pev->origin);

	if (!pEntity->pev->watertype)
	{
		// just spawned here
		pEntity->pev->watertype = cont;
		pEntity->pev->waterlevel = 1;
		return;
	}
	
	if (cont <= CONTENTS_WATER)
	{
		if (pEntity->pev->watertype == CONTENTS_EMPTY)
		{	
			// just crossed into water
			EMIT_SOUND( pEntity->edict(), CHAN_AUTO, "misc/h2ohit1.wav", 1, ATTN_NORM );
		}		
		pEntity->pev->watertype = cont;
		pEntity->pev->waterlevel = 1;
	}
	else
	{
		if (pEntity->pev->watertype != CONTENTS_EMPTY)
		{	
			// just crossed into water
			EMIT_SOUND( pEntity->edict(), CHAN_AUTO, "misc/h2ohit1.wav", 1, ATTN_NORM );
		}		
		pEntity->pev->watertype = CONTENTS_EMPTY;
		pEntity->pev->waterlevel = cont;
	}
}

/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
int SV_FlyMove( CBaseEntity *pEntity, float time, TraceResult *steptrace )
{
	int	numplanes;
	Vector	planes[MAX_CLIP_PLANES];
	Vector	primal_velocity, original_velocity, new_velocity;
	edict_t	*edict = pEntity->edict();
	TraceResult trace;
	float	time_left;
	int	blocked;
	int	i, j;

	blocked = 0;
	numplanes = 0;
	original_velocity = primal_velocity = pEntity->pev->velocity;
	
	time_left = time;

	for( int bumpcount = 0; bumpcount < 4; bumpcount++ )
	{
		if (pEntity->pev->velocity == g_vecZero)
			break;

		Vector end = pEntity->pev->origin + time_left * pEntity->pev->velocity;

		gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
		TRACE_MONSTER_HULL( pEntity->edict(), pEntity->pev->origin, end, dont_ignore_monsters, pEntity->edict(), &trace );

		if (trace.fAllSolid)
		{	
			// entity is trapped in another solid
			pEntity->pev->velocity = g_vecZero;
			return 3;
		}

		if (trace.flFraction > 0.0f)
		{	
			// actually covered some distance
			pEntity->pev->origin = trace.vecEndPos;
			original_velocity = pEntity->pev->velocity;
			numplanes = 0;
		}

		if (trace.flFraction == 1.0f)
			 break; // moved the entire distance

		if( !trace.pHit )
		{
			ALERT( at_error, "SV_FlyMove: trace.pHit == NULL\n" );
			pEntity->pev->velocity = g_vecZero;
			return 3;
		}

		if (trace.vecPlaneNormal.z > 0.7f)
		{
			blocked |= 1; // floor
			if (trace.pHit->v.solid == SOLID_BSP)
			{
				pEntity->pev->flags |= FL_ONGROUND;
				pEntity->pev->groundentity = trace.pHit;
			}
		}

		if (!trace.vecPlaneNormal.z)
		{
			blocked |= 2; // step
			if (steptrace)
				*steptrace = trace;	// save for player extrafriction
		}

		// run the impact function
		SV_Impact (pEntity, &trace);

		// break if removed by the impact function
		if( edict->free || pEntity->pev->flags & FL_KILLME )
			break;
		
		time_left -= time_left * trace.flFraction;
		
		// cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	
			// this shouldn't really happen
			pEntity->pev->velocity = g_vecZero;
			return 3;
		}

		planes[numplanes] = trace.vecPlaneNormal;
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		for (i = 0; i < numplanes; i++)
		{
			SV_ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
			for (j = 0; j < numplanes; j++)
			{
				if( j != i )
				{
					if( DotProduct( new_velocity, planes[j] ) < 0.0f )
						break; // not ok
				}
			}

			if (j == numplanes)
				break;
		}
		
		if (i != numplanes)
		{	
			// go along this plane
			pEntity->pev->velocity = new_velocity;
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
				pEntity->pev->velocity = g_vecZero;
				return 7;
			}

			Vector dir = CrossProduct (planes[0], planes[1]);
			float d = DotProduct (dir, pEntity->pev->velocity);
			pEntity->pev->velocity = dir * d;
		}

		// if original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		if (DotProduct (pEntity->pev->velocity, primal_velocity) <= 0.0f)
		{
			pEntity->pev->velocity = g_vecZero;
			return blocked;
		}
	}
	return blocked;
}

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
=============
*/
void SV_Physics_Step( CBaseEntity *pEntity )
{
	edict_t *edict = pEntity->edict();
	BOOL hitsound;

	// freefall if not onground
	if ( !FBitSet( pEntity->pev->flags, ( FL_ONGROUND|FL_FLY|FL_SWIM )))
	{
		float gravity = CVAR_GET_FLOAT ("sv_gravity");
		if (pEntity->pev->velocity.z < gravity * -0.1f)
			hitsound = true;
		else
			hitsound = false;

		SV_AddGravity( pEntity );
		SV_CheckVelocity( pEntity );
		SV_FlyMove( pEntity, gpGlobals->frametime, NULL );
		if (edict->free) return; // removed by impact function

		LINK_ENTITY( ENT( pEntity->pev ), true );

		if (FBitSet( pEntity->pev->flags, FL_ONGROUND ))	// just hit ground
		{
			if( hitsound )
				EMIT_SOUND(ENT(pEntity->pev), CHAN_AUTO, "demon/dland2.wav", 1, ATTN_NORM);
		}
	}

	// regular thinking
	if( !SV_RunThink( pEntity ))
		return;
	
	SV_CheckWaterTransition( pEntity );
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
g-cont. Override physics toss with quake code
=============
*/
void SV_Physics_Toss( CBaseEntity *pEntity )
{
	edict_t *edict = pEntity->edict();

	// regular thinking
	if (!SV_RunThink( pEntity ))
		return;

	// if onground, return without moving
	if (FBitSet( pEntity->pev->flags, FL_ONGROUND ))
		return;

	SV_CheckVelocity (pEntity);

	// add gravity
	if (pEntity->pev->movetype != MOVETYPE_FLY && pEntity->pev->movetype != MOVETYPE_FLYMISSILE)
		SV_AddGravity (pEntity);

	// move angles
	pEntity->pev->angles = pEntity->pev->angles + pEntity->pev->avelocity * gpGlobals->frametime;

	TraceResult trace;
	Vector move;

	move = pEntity->pev->velocity * gpGlobals->frametime;
	trace = SV_PushEntity (pEntity, move);

	if (trace.flFraction == 1)
		return;
	if (edict->free)
		return;

	float backoff;
	
	if (pEntity->pev->movetype == MOVETYPE_BOUNCE)
		backoff = 1.5;
	else
		backoff = 1;

	SV_ClipVelocity(pEntity->pev->velocity, trace.vecPlaneNormal, pEntity->pev->velocity, backoff);

	// stop if on ground
	if (trace.vecPlaneNormal[2] > 0.7)
	{		
		if (pEntity->pev->velocity[2] < 60 || pEntity->pev->movetype != MOVETYPE_BOUNCE)
		{
			pEntity->pev->flags |= FL_ONGROUND;
			pEntity->pev->groundentity = trace.pHit;
			pEntity->pev->velocity = g_vecZero;
			pEntity->pev->avelocity = g_vecZero;
		}
	}
	
	// check for in water
	SV_CheckWaterTransition (pEntity);
}

void SV_RetouchEntity( CBaseEntity *pEntity )
{
	if( gpGlobals->force_retouch != 0.0f )
	{
		// force retouch even for stationary
		LINK_ENTITY( pEntity->edict(), true );
	}
}

//
// assume pEntity is valid
//
int RunPhysicsFrame( CBaseEntity *pEntity )
{
	// NOTE: at this point pEntity assume to be valid
	switch( pEntity->pev->movetype )
	{
	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
	case MOVETYPE_FLY:
	case MOVETYPE_FLYMISSILE:
		SV_RetouchEntity (pEntity);
		SV_Physics_Toss (pEntity);
		return 1;	// overrided
	case MOVETYPE_STEP:
		SV_RetouchEntity (pEntity);
		SV_Physics_Step (pEntity);
		return 1;
	}

	return 0;	// other movetypes uses built-in engine physic
}

//
// run custom physics for each entity
// return 0 to use built-in engine physic
//
int DispatchPhysicsEntity( edict_t *pEdict )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pEdict);

	if( !pEntity )
		return 0;	// not initialized

	if( RunPhysicsFrame( pEntity ))
	{
		if( !pEdict->free && pEntity->pev->flags & FL_KILLME )
			REMOVE_ENTITY( pEntity->edict() );
		return 1;
	}

	return 0;	// other movetypes uses built-in engine physic
}

//
// Quake uses simple trigger touch that approached to bbox
//
int DispatchTriggerTouch( edict_t *pent, edict_t *trigger )
{
	if( trigger == pent || trigger->v.solid != SOLID_TRIGGER ) // disabled ?
		return 0;

	if( pent->v.absmin[0] > trigger->v.absmax[0]
	|| pent->v.absmin[1] > trigger->v.absmax[1]
	|| pent->v.absmin[2] > trigger->v.absmax[2]
	|| pent->v.absmax[0] < trigger->v.absmin[0]
	|| pent->v.absmax[1] < trigger->v.absmin[1]
	|| pent->v.absmax[2] < trigger->v.absmin[2] )
		return 0;

	return 1;
}

//
// Quake hull select order
//
void *DispatchHullForBsp( edict_t *ent, const float *fmins, const float *fmaxs, float *foffset )
{
	hull_t	*hull;
	model_t	*model;
	Vector	mins = Vector( (float *)fmins );
	Vector	maxs = Vector( (float *)fmaxs );
	Vector	size, offset;

	// decide which clipping hull to use, based on the size
	model = (model_t *)MODEL_HANDLE( ent->v.modelindex );

	if( !model || model->type != mod_brush )
		HOST_ERROR( "Entity %i SOLID_BSP with a non bsp model %i\n", ENTINDEX( ent ), (model) ? model->type : mod_bad );

	size = maxs - mins;

	if( size[0] < 3.0f )
		hull = &model->hulls[0];
	else if( size[0] <= 32.0f )
		hull = &model->hulls[1];
	else hull = &model->hulls[2];

	offset = hull->clip_mins - mins;
	offset += ent->v.origin;

	offset.CopyToArray( foffset );

	return hull;
}
	
static physics_interface_t gPhysicsInterface = 
{
	SV_PHYSICS_INTERFACE_VERSION,
	DispatchCreateEntity,
	DispatchPhysicsEntity,
	NULL,			// SV_LoadEntities
	NULL,			// SV_UpdatePlayerBaseVelocity
	NULL,			// SV_AllowSaveGame
	DispatchTriggerTouch,	
	EngineSetFeatures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	DispatchHullForBsp,
};

int Server_GetPhysicsInterface( int iVersion, server_physics_api_t *pfuncsFromEngine, physics_interface_t *pFunctionTable )
{
	if ( !pFunctionTable || !pfuncsFromEngine || iVersion != SV_PHYSICS_INTERFACE_VERSION )
	{
		return FALSE;
	}

	size_t iExportSize = sizeof( server_physics_api_t );
	size_t iImportSize = sizeof( physics_interface_t );

	// NOTE: the very old versions NOT have information about current build in any case
	if( g_iXashEngineBuildNumber <= 1910 )
	{
		if( g_fXashEngine )
			ALERT( at_console, "old version of Xash3D was detected. Engine features was disabled.\n" );

		// interface sizes for build 1905 and older
		iExportSize = 28;
		iImportSize = 24;
	}

	if( g_iXashEngineBuildNumber <= 3700 )
		iImportSize -= 12;

	// copy new physics interface
	memcpy( &g_physfuncs, pfuncsFromEngine, iExportSize );

	// fill engine callbacks
	memcpy( pFunctionTable, &gPhysicsInterface, iImportSize );

	return TRUE;
}