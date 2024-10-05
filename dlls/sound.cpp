//=========================================================
// sound.cpp 
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"

/*QUAKED ambient_suck_wind (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
class CAmbientSuckWind : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_SOUND( "ambience/suck1.wav" );
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/suck1.wav", 1, ATTN_STATIC, SND_SPAWNING, 100 );
	}
	void Spawn( void ) { Precache(); }
};

LINK_ENTITY_TO_CLASS( ambient_suck_wind, CAmbientSuckWind );

/*QUAKED ambient_drone (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
class CAmbientDrone : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_SOUND( "ambience/drone6.wav" );
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/drone6.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
	}
	void Spawn( void ) { Precache(); }
};

LINK_ENTITY_TO_CLASS( ambient_drone, CAmbientDrone );

/*QUAKED ambient_flouro_buzz (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
class CAmbientFluoroBuzz : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_SOUND( "ambience/buzz1.wav" );
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/buzz1.wav", 1, ATTN_STATIC, SND_SPAWNING, 100 );
	}
	void Spawn( void ) { Precache(); }
};

LINK_ENTITY_TO_CLASS( ambient_flouro_buzz, CAmbientFluoroBuzz );

/*QUAKED ambient_drip (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
class CAmbientDrip : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_SOUND( "ambience/drip1.wav" );
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/drip1.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
	}
	void Spawn( void ) { Precache(); }
};

LINK_ENTITY_TO_CLASS( ambient_drip, CAmbientDrip );

/*QUAKED ambient_comp_hum (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
class CAmbientCompHum : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_SOUND( "ambience/comp1.wav" );
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/comp1.wav", 1, ATTN_STATIC, SND_SPAWNING, 100 );
	}
	void Spawn( void ) { Precache(); }
};

LINK_ENTITY_TO_CLASS( ambient_comp_hum, CAmbientCompHum );

/*QUAKED ambient_thunder (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
a random strike thunder sound
*/
class CAmbientThunder : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_SOUND( "ambience/thunder1.wav" );
	}

	void Spawn( void )
	{
		Precache();
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 5.0, 15.0 );
	}

	void Think( void )
	{
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/thunder1.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 10.0, 30.0 ); // next time is 10 - 30 seconds
	}
};

LINK_ENTITY_TO_CLASS( ambient_thunder, CAmbientThunder );

/*QUAKED ambient_light_buzz (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
class CAmbientLightBuzz : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_SOUND( "ambience/fl_hum1.wav" );
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/fl_hum1.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
	}
	void Spawn( void ) { Precache(); }
};

LINK_ENTITY_TO_CLASS( ambient_light_buzz, CAmbientLightBuzz );

/*QUAKED ambient_swamp1 (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
class CAmbientSwamp1 : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_SOUND( "ambience/swamp1.wav" );
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/swamp1.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
	}
	void Spawn( void ) { Precache(); }
};

LINK_ENTITY_TO_CLASS( ambient_swamp1, CAmbientSwamp1 );

/*QUAKED ambient_swamp2 (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
class CAmbientSwamp2 : public CBaseEntity
{
public:
	void Precache( void )
	{
		PRECACHE_SOUND( "ambience/swamp2.wav" );
		UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/swamp2.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
	}
	void Spawn( void ) { Precache(); }
};

LINK_ENTITY_TO_CLASS( ambient_swamp2, CAmbientSwamp2 );