/*

===== bmodels.cpp ========================================================

  spawn, think, and use functions for entities that use brush models

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "doors.h"

//
// BModelOrigin - calculates origin of a bmodel from absmin/size because all bmodel origins are 0 0 0
//
Vector VecBModelOrigin( entvars_t* pevBModel )
{
	return pevBModel->absmin + ( pevBModel->size * 0.5 );
}

// =================== FUNC_WALL ==============================================

/*QUAKED func_wall (0 .5 .8) ?
This is just a solid wall if not inhibited
*/
class CFuncWall : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( func_wall, CFuncWall );

void CFuncWall :: Spawn( void )
{
	pev->angles	= g_vecZero;
	pev->movetype	= MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	pev->solid	= SOLID_BSP;

	SET_MODEL( ENT(pev), STRING(pev->model) );
	
	// If it can't move/go away, it's really part of the world
	pev->flags |= FL_WORLDBRUSH;
}

void CFuncWall :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( ShouldToggle( useType, (int)( pev->frame )) )
		pev->frame = 1 - pev->frame;
}

/*QUAKED func_illusionary (0 .5 .8) ?
A simple entity that looks solid but lets you walk through it.
*/
class CFuncIllusionary : public CBaseToggle 
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( func_illusionary, CFuncIllusionary );

void CFuncIllusionary :: Spawn( void )
{
	pev->angles = g_vecZero;
	pev->movetype = MOVETYPE_NONE;  
	pev->solid = SOLID_NOT;// always solid_not 
	SET_MODEL( ENT(pev), STRING(pev->model) );
	
	// I'd rather eat the network bandwidth of this than figure out how to save/restore
	// these entities after they have been moved to the client, or respawn them ala Quake
	// Perhaps we can do this in deathmatch only.
	//	MAKE_STATIC(ENT(pev));
}

/*QUAKED func_episodegate (0 .5 .8) ? E1 E2 E3 E4
This bmodel will appear if the episode has allready been completed, so players can't reenter it.
*/
class CFuncEpisodeGate : public CBaseToggle 
{
public:
	void Spawn( void );
	void EXPORT StartThink(void);
};

LINK_ENTITY_TO_CLASS( func_episodegate, CFuncEpisodeGate );

void CFuncEpisodeGate :: Spawn( void )
{
	pev->angles = g_vecZero;
	pev->movetype = MOVETYPE_NONE;	// so it doesn't get pushed by anything  
	pev->solid = SOLID_BBOX;// always solid_not 
	SET_MODEL( ENT(pev), STRING(pev->model) );

	// If it can't move/go away, it's really part of the world
	pev->flags |= FL_WORLDBRUSH;

	pev->effects |= EF_NODRAW;

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CFuncEpisodeGate::StartThink);

	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;
}

extern int	gmsgTempEntity;

void CFuncEpisodeGate::StartThink(void)
{
	Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

	if (!((int)gpWorld->serverflags & pev->spawnflags))
	{
		pev->nextthink = -1;
		REMOVE_ENTITY(ENT(pev));
		return;
	}
	else
	{
		pev->effects &= ~EF_NODRAW;

		if (pev->renderamt >= 255)
		{
			pev->renderamt = 255;
			pev->nextthink = -1;
			return;
		}
		else
		{
			pev->renderamt++;
		}
		pev->nextthink = gpGlobals->time;
	}
}

/*QUAKED func_bossgate (0 .5 .8) ?
This bmodel appears unless players have all of the episode sigils.
*/
class CFuncBossGate : public CBaseToggle 
{
public:
	void Spawn( void );
	void EXPORT StartThink(void);
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( func_bossgate, CFuncBossGate );

void CFuncBossGate :: Spawn( void )
{
	/*if (((int)gpWorld->serverflags & 15) == 15)
	{
		REMOVE_ENTITY( ENT( pev )); // all episodes completed
		return;
	}*/

	pev->angles = g_vecZero;
	//pev->movetype = MOVETYPE_PUSH;	// so it doesn't get pushed by anything  
	//pev->solid = SOLID_BSP;// always solid_not 
	pev->movetype = MOVETYPE_NONE;	// so it doesn't get pushed by anything  
	pev->solid = SOLID_BBOX;// always solid_not 
	SET_MODEL( ENT(pev), STRING(pev->model) );

	// If it can't move/go away, it's really part of the world
	pev->flags |= FL_WORLDBRUSH;

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CFuncBossGate::StartThink);
}

void CFuncBossGate::StartThink(void)
{
	pev->nextthink = -1;

	if (((int)gpWorld->serverflags & 15) == 15)
	{
		REMOVE_ENTITY(ENT(pev)); // all episodes completed
		return;
	}
}

void CFuncBossGate :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( ShouldToggle( useType, (int)( pev->frame )) )
		pev->frame = 1 - pev->frame;
}