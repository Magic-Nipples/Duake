/*

===== combat.cpp ========================================================

  functions dealing with damage infliction & death

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "weapons.h"
#include "monster.h"
#include "skill.h"

extern DLL_GLOBAL Vector	g_vecAttackDir;

extern entvars_t *g_pevLastInflictor;
extern int gmsgTempEntity;

LINK_ENTITY_TO_CLASS( gib, CGib );
//
// fade out - slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER! 
// SET A FUTURE THINK AND A RENDERMODE!!
void CBaseEntity :: SUB_StartFadeOut ( void )
{
	if (pev->rendermode == kRenderNormal)
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	pev->solid = SOLID_NOT;
	pev->avelocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink ( &CBaseEntity::SUB_FadeOut );
}

void CBaseEntity :: SUB_FadeOut ( void  )
{
	if ( pev->renderamt > 7 )
	{
		pev->renderamt -= 7;
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else 
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.2;
		SetThink ( &CBaseEntity::SUB_Remove );
	}
}

//=========================================================
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop 
// bouncing to emit their scent. That's what this function
// does.
//=========================================================
void CGib :: WaitTillLand ( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	if ( pev->velocity == g_vecZero )
	{
		SetThink (&CGib::SUB_StartFadeOut);
		pev->nextthink = gpGlobals->time + m_lifeTime;
	}
	else
	{
		// wait and check again in another half second.
		pev->nextthink = gpGlobals->time + 0.5;
	}
}

//
// Gib bounces on the ground or wall, sponges some blood down, too!
//
void CGib :: BounceGibTouch ( CBaseEntity *pOther )
{
	if (pev->flags & FL_ONGROUND)
	{
		pev->velocity = pev->velocity * 0.9;
		pev->angles.x = 0;
		pev->angles.z = 0;
		pev->avelocity.x = 0;
		pev->avelocity.z = 0;
	}
	else
	{
		Vector vecSpot;
		TraceResult tr;

		if ( m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED )
		{
			vecSpot = pev->origin + Vector ( 0 , 0 , 8 );//move up a bit, and trace down.
			UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -24 ),  ignore_monsters, ENT(pev), & tr);

			UTIL_BloodDecalTrace( &tr, m_bloodColor );

			m_cBloodDecals--; 
		}
	}
}

//
// Throw a chunk
//
void CGib :: Spawn( const char *szGibModel )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55; // deading the bounce a bit
	
	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_TRIGGER; //fixes gibs getting stuck on each other | SOLID_SLIDEBOX
	pev->classname = MAKE_STRING("gib");

	pev->avelocity.x = RANDOM_FLOAT( 0, 600 );
	pev->avelocity.y = RANDOM_FLOAT( 0, 600 );
	pev->avelocity.z = RANDOM_FLOAT( 0, 600 );

	SET_MODEL(ENT(pev), szGibModel);

	m_bloodColor = BLOOD_COLOR_RED;
	pev->nextthink = gpGlobals->time + 4;
	m_lifeTime = 25;
	SetThink ( &CGib::WaitTillLand );
	SetTouch ( &CGib::BounceGibTouch );

	m_cBloodDecals = 5;// how many blood decals this gib can place (1 per bounce until none remain). 
}

Vector CGib :: VelocityForDamage( float flDamage )
{
	Vector vecVelocity;

	vecVelocity.x = RANDOM_FLOAT( -100, 100 );
	vecVelocity.y = RANDOM_FLOAT( 100, 100 );
	vecVelocity.z = RANDOM_FLOAT( 200, 300 );

	//ALERT(at_console, "%0.2f\n", flDamage);

	if (flDamage > -110) //-50
		vecVelocity *= 0.7;
	else if (flDamage > -260) //200
		vecVelocity *= 2;
	else
		vecVelocity *= 2.5;

	return vecVelocity;
}

void CGib :: ThrowHead( const char *szGibName, entvars_t *pevVictim )
{
	CGib *pGib = GetClassPtr( (CGib *)NULL );
	CQuakeMonster *pMonster = CBaseEntity :: Instance( pevVictim )->GetMonster();

	pGib->Spawn( szGibName );
	pGib->SetThink( NULL );	// stay on ground

	if( pMonster != NULL )
		pGib->m_bloodColor = pMonster->BloodColor();

	// spawn the gib somewhere in the monster's bounding volume
	pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * (RANDOM_FLOAT ( 0 , 1 ) );
	pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * (RANDOM_FLOAT ( 0 , 1 ) );
	pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * (RANDOM_FLOAT ( 0 , 1 ) ) + 1;
	pGib->pev->velocity = pGib->VelocityForDamage( pevVictim->health );
	pGib->pev->avelocity.x = pGib->pev->avelocity.z = 0; // yaw only

	UTIL_SetSize(pGib->pev, g_vecZero, g_vecZero );
}

void CGib :: ThrowGib( const char *szGibName, entvars_t *pevVictim )
{
	CQuakeMonster *pMonster = CBaseEntity :: Instance( pevVictim )->GetMonster();

	// g-cont. produce more gibs than one
	for( int i = 0; i < RANDOM_LONG( 1, 3 ); i++ )
	{
		CGib *pGib = GetClassPtr( (CGib *)NULL );

		pGib->Spawn( szGibName );
		pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * (RANDOM_FLOAT ( 0 , 1 ) );
		pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * (RANDOM_FLOAT ( 0 , 1 ) );
		pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * (RANDOM_FLOAT ( 0 , 1 ) ) + 1;
		pGib->pev->velocity = pGib->VelocityForDamage( pevVictim->health );

		if( pMonster != NULL )
			pGib->m_bloodColor = pMonster->BloodColor();

		UTIL_SetSize(pGib->pev, g_vecZero, g_vecZero );
	}
}

/*
================
TraceAttack
================
*/
void CBaseEntity::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4;

	if ( pev->takedamage )
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );

		int blood = BloodColor();
		
		if ( blood != DONT_BLEED )
		{
			//SpawnBlood(vecOrigin, blood, flDamage);// a little surface blood.
			//TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
			UTIL_ParticleEffect( vecOrigin, g_vecAttackDir, blood, flDamage * 2 ); //WOLF 3D - 32 for wolf palette
		}
		else
		{
			// g-cont. for doors and buttons
			// this looks silly i knew but we need to make difference between secret door and world
			UTIL_ParticleEffect( vecOrigin, g_vecAttackDir, 0, flDamage * 2 );
		}
	}
	else if(( bitsDamageType & DMG_BULLET ) && !IsSkySurface( this, ptr->vecEndPos, vecDir ))
	{
		MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
			WRITE_BYTE( TE_GUNSHOT );
			WRITE_COORD( vecOrigin.x );
			WRITE_COORD( vecOrigin.y );
			WRITE_COORD( vecOrigin.z );
		MESSAGE_END();
	}
}

void CBaseEntity :: TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if (BloodColor() == DONT_BLEED)
		return;
	
	if (flDamage == 0)
		return;

	if (! (bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		return;
	
	// make blood decal on the wall! 
	TraceResult Bloodtr;
	Vector vecTraceDir; 
	float flNoise;
	int cCount;
	int i;

	if (flDamage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (flDamage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	for ( i = 0 ; i < cCount ; i++ )
	{
		vecTraceDir = vecDir * -1;// trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172, ignore_monsters, ENT(pev), &Bloodtr);

		if ( Bloodtr.flFraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}