/*

===== lights.cpp ========================================================

  spawn and think functions for editor-placed lights

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

extern DLL_GLOBAL BOOL		g_fXashEngine;

#define SF_LIGHT_START_OFF		1

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) LIGHT_START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
*/
class CLight : public CPointEntity
{
public:
	virtual void	KeyValue( KeyValueData* pkvd ); 
	virtual void	Spawn( void );
	void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	
	static TYPEDESCRIPTION m_SaveData[];

	int		m_iStyle;
};
LINK_ENTITY_TO_CLASS(light, CLight);

TYPEDESCRIPTION	CLight::m_SaveData[] = 
{
	DEFINE_FIELD( CLight, m_iStyle, FIELD_INTEGER ),
}; IMPLEMENT_SAVERESTORE( CLight, CPointEntity );

void CLight :: KeyValue( KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "style"))
	{
		m_iStyle = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CPointEntity::KeyValue( pkvd );
	}
}

void CLight :: Spawn( void )
{
	if (FStringNull(pev->targetname))
	{       // inert light
		REMOVE_ENTITY(ENT(pev));
		return;
	}
	
	if (m_iStyle >= 32)
	{
		if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
			LIGHT_STYLE(m_iStyle, "a");
		else
			LIGHT_STYLE(m_iStyle, "m");
	}
}


void CLight :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_iStyle >= 32)
	{
		if ( !ShouldToggle( useType, !FBitSet(pev->spawnflags, SF_LIGHT_START_OFF) ) )
			return;

		if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
		{
			LIGHT_STYLE(m_iStyle, "m");
			ClearBits(pev->spawnflags, SF_LIGHT_START_OFF);
		}
		else
		{
			LIGHT_STYLE(m_iStyle, "a");
			SetBits(pev->spawnflags, SF_LIGHT_START_OFF);
		}
	}
}

/*QUAKED light_fluoro (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
Makes steady fluorescent humming sound
*/
class CLightFluoro : public CLight
{
public:
	void	Precache( void ); 
	void	Spawn( void );
};

LINK_ENTITY_TO_CLASS( light_fluoro, CLightFluoro );

void CLightFluoro :: Precache( void )
{
	PRECACHE_SOUND( "ambience/fl_hum1.wav" );

	UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/fl_hum1.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
}

void CLightFluoro :: Spawn( void )
{
	Precache ();

	if (m_iStyle >= 32)
	{
		if (FBitSet(pev->spawnflags, SF_LIGHT_START_OFF))
			LIGHT_STYLE(m_iStyle, "a");
		else
			LIGHT_STYLE(m_iStyle, "m");
	}
}

/*QUAKED light_fluorospark (0 1 0) (-8 -8 -8) (8 8 8)
Non-displayed light.
Default light value is 300
Default style is 10
Makes sparking, broken fluorescent sound
*/
class CLightFluoroSpark : public CLight
{
public:
	void	Precache( void ); 
	void	Spawn( void );
};

LINK_ENTITY_TO_CLASS( light_fluorospark, CLightFluoroSpark );

void CLightFluoroSpark :: Precache( void )
{
	PRECACHE_SOUND( "ambience/buzz1.wav" );

	UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/buzz1.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
}

void CLightFluoroSpark :: Spawn( void )
{
	Precache ();

	if (!m_iStyle)
		m_iStyle = 10;
}

/*QUAKED light_globe (0 1 0) (-8 -8 -8) (8 8 8)
Sphere globe light.
Default light value is 300
Default style is 0
*/
class CLightGlobe : public CPointEntity
{
public:
	void	Precache( void ); 
	void	Spawn( void );
};

LINK_ENTITY_TO_CLASS( light_globe, CLightGlobe );

void CLightGlobe :: Precache( void )
{
	PRECACHE_MODEL( "sprites/s_light.spr" );
}

void CLightGlobe :: Spawn( void )
{
	Precache ();

	if( g_fXashEngine )
		pev->effects = EF_FULLBRIGHT; // NOTE: this has effect only in Xash3D

	SET_MODEL( ENT(pev), "sprites/s_light.spr" );

	// set this to allow support HD-textures replacement
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;
}

class CLightTorch : public CPointEntity
{
public:
	void	Precache( void ); 
	void	Spawn( void );
};

LINK_ENTITY_TO_CLASS( light_torch_small_walltorch, CLightTorch );

void CLightTorch :: Precache( void )
{
	PRECACHE_MODEL( "progs/flame.mdl" );
	PRECACHE_SOUND( "ambience/fire1.wav" );

	UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/fire1.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
}

void CLightTorch :: Spawn( void )
{
	Precache ();

	SET_MODEL( ENT(pev), "progs/flame.mdl" );
}


class CLightFlame : public CPointEntity
{
public:
	void	Precache( void ); 
	void	Spawn( void );
	void	Think( void );
};

LINK_ENTITY_TO_CLASS( light_flame_large_yellow, CLightFlame );
LINK_ENTITY_TO_CLASS( light_flame_small_yellow, CLightFlame );
LINK_ENTITY_TO_CLASS( light_flame_small_white, CLightFlame );

void CLightFlame :: Precache( void )
{
	PRECACHE_SOUND( "ambience/fire1.wav" );
	PRECACHE_MODEL("progs/flame2.mdl");

	UTIL_EmitAmbientSound( ENT(pev), pev->origin, "ambience/fire1.wav", 0.5, ATTN_STATIC, SND_SPAWNING, 100 );
}

void CLightFlame :: Spawn( void )
{
	Precache ();

	SET_MODEL(ENT(pev), "progs/flame2.mdl");

	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.0f, 0.2f );

	if (FClassnameIs(pev, "light_flame_large_yellow"))
		pev->frame = 1;
	else
		pev->frame = 0;
}

void CLightFlame :: Think( void )
{
	TraceResult tr;

	// try to link flame with moving brush
	UTIL_TraceLine( pev->origin + Vector( 0, 0, 32 ), pev->origin + Vector( 0, 0, -32 ), ignore_monsters, ENT( pev ), &tr);

	// g-cont. e1m5 have a secret button with flame at the top.
	if( g_fXashEngine && tr.flFraction != 1.0 && !FNullEnt( tr.pHit ))
	{
		ALERT( at_aiconsole, "%s linked with %s (%s)\n", STRING( pev->classname ),
		STRING( VARS( tr.pHit )->classname ), STRING( VARS( tr.pHit )->targetname )); 
		pev->movetype = MOVETYPE_COMPOUND;	// set movewith type
		pev->aiment = tr.pHit;		// set parent
	}
}