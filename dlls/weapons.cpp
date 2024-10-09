/*

===== weapons.cpp ========================================================

  Quake weaponry

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "decals.h"
#include "items.h"
#include "skill.h"

extern DLL_GLOBAL BOOL		g_fXashEngine;

DLL_GLOBAL short	g_sModelIndexBubbles;// holds the index for the bubbles model
//DLL_GLOBAL short	g_sModelIndexBloodDrop;// holds the sprite index for the initial blood
//DLL_GLOBAL short	g_sModelIndexBloodSpray;// holds the sprite index for splattered blood

/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

MULTIDAMAGE gMultiDamage;

//
// ClearMultiDamage - resets the global multi damage accumulator
//
void ClearMultiDamage(void)
{
	gMultiDamage.pEntity = NULL;
	gMultiDamage.amount	= 0;
}

//
// ApplyMultiDamage - inflicts contents of global multi damage register on gMultiDamage.pEntity
//
void ApplyMultiDamage(entvars_t *pevInflictor, entvars_t *pevAttacker )
{
	if ( !gMultiDamage.pEntity )
		return;

	if( gMultiDamage.pEntity->pev->takedamage )
		gMultiDamage.pEntity->TakeDamage(pevInflictor, pevAttacker, gMultiDamage.amount, DMG_GENERIC );
}

void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType)
{
	if ( !pEntity )
		return;

	if ( pEntity != gMultiDamage.pEntity )
	{
		ApplyMultiDamage(pevInflictor,pevInflictor); // UNDONE: wrong attacker!
		gMultiDamage.pEntity = pEntity;
		gMultiDamage.amount	= flDamage;
	}

	gMultiDamage.amount += flDamage;
}

BOOL IsSkySurface( CBaseEntity *pEnt, const Vector &point, const Vector &vecDir )
{
	Vector vecSrc = point + vecDir * -2.0f;
	Vector vecEnd = point + vecDir * 2.0f;

	const char *pTex = TRACE_TEXTURE( pEnt->edict(), vecSrc, vecEnd );

	if( pTex != NULL && !_strnicmp( pTex, "sky", 3 ))
		return TRUE;
	return FALSE;
}

LINK_ENTITY_TO_CLASS( spike, CNail );

//=========================================================
CNail *CNail::CreateNail( Vector vecOrigin, Vector vecDir, CBaseEntity *pOwner )
{
	CNail *pNail = GetClassPtr((CNail *)NULL );

	UTIL_SetOrigin( pNail->pev, vecOrigin );

	pNail->pev->velocity = vecDir * 1000;
	pNail->pev->angles = UTIL_VecToAngles( vecDir );
	pNail->pev->owner = pOwner->edict();
	pNail->Spawn();
	pNail->pev->classname = MAKE_STRING( "spike" );
	pNail->pev->animtime = gpGlobals->time;
	pNail->pev->framerate = 1.0;

	return pNail;
}

CNail *CNail::CreateSuperNail( Vector vecOrigin, Vector vecDir, CBaseEntity *pOwner )
{
	CNail *pNail = CreateNail( vecOrigin, vecDir, pOwner );

	// super nails simply do more damage
	pNail->pev->dmg = 18;
	pNail->pev->skin = 1;
	return pNail;
}

CNail *CNail::CreateKnightSpike( Vector vecOrigin, Vector vecDir, CBaseEntity *pOwner )
{
	CNail *pNail = CreateNail( vecOrigin, vecDir, pOwner );

	SET_MODEL( ENT(pNail->pev), "models/k_spike.mdl" );
	pNail->pev->velocity = vecDir * 300; // knightspikes goes slow
	pNail->pev->effects |= EF_FULLBRIGHT;

	return pNail;
}

//=========================================================
void CNail::Spawn( void )
{
	Precache();

	// Setup
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;
	
	// Safety removal
	pev->nextthink = gpGlobals->time + 6;
	SetThink( &CNail::SUB_Remove );
	
	// Touch
	SetTouch( &CNail::NailTouch );

	// Model
	SET_MODEL( ENT(pev), "progs/spike.mdl" );
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	// Damage
	pev->dmg = 9;
}

//=========================================================
void CNail::NailTouch( CBaseEntity *pOther )
{
	if (pOther->pev->solid == SOLID_TRIGGER)
		return;

	TraceResult tr = UTIL_GetGlobalTrace();

	// Remove if we've hit skybrush
	if ( IsSkySurface( pOther, tr.vecEndPos, pev->velocity.Normalize( ) ) )
	{
		UTIL_Remove( this );
		return;
	}

	// Hit something that bleeds
	if (pOther->pev->takedamage)
	{
		CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
		if( !pOwner ) pOwner = gpWorld;

		if ( g_pGameRules->PlayerRelationship( pOther, pOwner ) != GR_TEAMMATE )
			SpawnBlood( pev->origin, pOther->BloodColor(), pev->dmg );

		if( !FNullEnt( pOwner ))
			pOther->TakeDamage( pev, pOwner->pev, pev->dmg, DMG_GENERIC );
		else 
			pOther->TakeDamage( pev, pev, pev->dmg, DMG_GENERIC );
	}
	else
	{
		if ( pOther->pev->solid == SOLID_BSP || pOther->pev->movetype == MOVETYPE_PUSHSTEP )
		{
			Vector vecDir = pev->velocity.Normalize( );

			CBaseEntity *pEntity = NULL;
			Vector point = pev->origin - vecDir * 3;

			MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
				if( FStrEq( STRING (pev->model), "models/w_spike.mdl" ))
					WRITE_BYTE( TE_WIZSPIKE );
				else if( FStrEq( STRING (pev->model), "models/k_spike.mdl" ))
					WRITE_BYTE( TE_KNIGHTSPIKE );
				else
					WRITE_BYTE( pev->skin ? TE_SUPERSPIKE : TE_SPIKE );

				WRITE_COORD( point.x );
				WRITE_COORD( point.y );
				WRITE_COORD( point.z );
			MESSAGE_END();

			if ( strcmp( "progs/spike.mdl", STRING(pev->model)) )
			{
				// wizard and knight spikes aren't stuck in the walls
				UTIL_Remove( this );
				return;
			}

			// too many overlapped spikes looks ugly
			while ((pEntity = UTIL_FindEntityInSphere( pEntity, point, 1 )) != NULL)
			{
				if( FClassnameIs( pEntity->pev, "spike" ) && pEntity != this )
				{
					UTIL_Remove( this );
					return;
				}
                              }

			UTIL_SetOrigin( pev, point );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->velocity = Vector( 0, 0, 0 );
			pev->angles.z = RANDOM_LONG(0, 360);

			if (g_fXashEngine)
			{
				// g-cont. Setup movewith feature
				pev->movetype = MOVETYPE_COMPOUND;	// set movewith type
				pev->aiment = ENT( pOther->pev );	// set parent
			}

			SetTouch( NULL );
			return;	// stay in brush
		}
	}

	UTIL_Remove( this );
}

void CNail::ExplodeTouch( CBaseEntity *pOther )
{
	if( pOther->edict() == pev->owner )
		return; // don't explode on owner

	TraceResult tr = UTIL_GetGlobalTrace();

	// Remove if we've hit skybrush
	if ( IsSkySurface( pOther, tr.vecEndPos, pev->velocity.Normalize( ) ) )
	{
		UTIL_Remove( this );
		return;
	}

	UTIL_ScreenShake( pev->origin, 20.0f, 4.0f, 0.7f, 350.0f );

	float dmg = 100 + RANDOM_FLOAT( 0.0f, 20.0f );
	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if( !pOwner ) pOwner = gpWorld;

	if( pOther->pev->health && pOther->pev->takedamage )
	{
		if( FClassnameIs( pOther->pev, "monster_shambler" ))
			dmg *= 0.5f; // mostly immune
		pOther->TakeDamage( pev, pOwner->pev, dmg, DMG_BLAST );
	}

	Q_RadiusDamage( this, pOwner, 120, pOther );

	pev->origin = pev->origin - 8 * pev->velocity.Normalize();

	MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();

	UTIL_DecalTrace( &tr, DECAL_SCORCH1 + RANDOM_LONG( 0, 1 ));

	UTIL_Remove( this );
	SetTouch( NULL );
}

LINK_ENTITY_TO_CLASS( laser, CLaser );

//=========================================================
CLaser *CLaser::LaunchLaser( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner )
{
	CLaser *pLaser = GetClassPtr((CLaser *)NULL );

	UTIL_SetOrigin( pLaser->pev, vecOrigin );

	pLaser->pev->velocity = vecAngles * 600;
	pLaser->pev->angles = UTIL_VecToAngles( vecAngles );
	pLaser->pev->owner = pOwner->edict();
	pLaser->Spawn();
	pLaser->pev->classname = MAKE_STRING( "laser" );

	return pLaser;
}

//=========================================================
void CLaser::Spawn( void )
{
	Precache();

	// Setup
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->effects = EF_DIMLIGHT;
	
	// Safety removal
	pev->nextthink = gpGlobals->time + 5;
	SetThink( &CLaser::SUB_Remove );
	
	// Touch
	SetTouch( &CLaser::LaserTouch );

	// Model
	SET_MODEL( ENT(pev), "progs/laser.mdl" );
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	// Damage
	pev->dmg = 9;
}

//=========================================================
void CLaser::LaserTouch( CBaseEntity *pOther )
{
	if (pOther->pev->solid == SOLID_TRIGGER)
		return;

	TraceResult tr = UTIL_GetGlobalTrace();

	// Remove if we've hit skybrush
	if ( IsSkySurface( pOther, tr.vecEndPos, pev->velocity.Normalize( ) ) )
	{
		UTIL_Remove( this );
		return;
	}

	EMIT_SOUND(ENT(pOther->pev), CHAN_WEAPON, "enforcer/enfstop.wav", 1, ATTN_NORM);

	// Hit something that bleeds
	if (pOther->pev->takedamage)
	{
		CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
		if( !pOwner ) pOwner = gpWorld;

		if ( g_pGameRules->PlayerRelationship( pOther, pOwner ) != GR_TEAMMATE )
			SpawnBlood( pev->origin, pOther->BloodColor(), pev->dmg );

		pOther->TakeDamage( pev, pOwner->pev, pev->dmg, DMG_GENERIC );
	}
	else
	{
		if ( pOther->pev->solid == SOLID_BSP || pOther->pev->movetype == MOVETYPE_PUSHSTEP )
		{
			Vector vecDir = pev->velocity.Normalize( );
			Vector point = pev->origin - vecDir * 8;

			MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
				WRITE_BYTE( TE_GUNSHOT );
				WRITE_COORD( point.x );
				WRITE_COORD( point.y );
				WRITE_COORD( point.z );
			MESSAGE_END();
		}
	}

	UTIL_Remove( this );
}

LINK_ENTITY_TO_CLASS(grenade, CRocket);
LINK_ENTITY_TO_CLASS(rocket, CRocket);
LINK_ENTITY_TO_CLASS(plasma, CRocket);

//=========================================================
CRocket *CRocket::CreateRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner )
{
	CRocket *pRocket = GetClassPtr( (CRocket *)NULL );
	
	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	SET_MODEL(ENT(pRocket->pev), "models/missile.mdl");
	pRocket->Spawn();
	pRocket->pev->classname = MAKE_STRING("missile");
	pRocket->pev->owner = pOwner->edict();

	// Setup
	pRocket->pev->movetype = MOVETYPE_FLYMISSILE;
	pRocket->pev->solid = SOLID_BBOX;
		
	// Velocity
	pRocket->pev->velocity = vecAngles * 1000;
	pRocket->pev->angles = UTIL_VecToAngles( vecAngles );
	
	// Touch
	pRocket->SetTouch( &CRocket::RocketTouch );

	pRocket->pev->nextthink = gpGlobals->time + 5;
	pRocket->SetThink(&CRocket::SUB_Remove );

	return pRocket;
}

//=========================================================
CRocket *CRocket::CreateGrenade( Vector vecOrigin, Vector vecVelocity, CBaseEntity *pOwner )
{
	CRocket *pRocket = GetClassPtr( (CRocket *)NULL );

	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	SET_MODEL(ENT(pRocket->pev), "models/grenade.mdl");
	pRocket->Spawn();
	pRocket->pev->classname = MAKE_STRING("grenade");
	pRocket->pev->owner = pOwner->edict();

	// Setup
	pRocket->pev->movetype = MOVETYPE_BOUNCE;
	pRocket->pev->solid = SOLID_BBOX;

	pRocket->pev->avelocity = Vector(300,300,300);
	
	// Velocity
	pRocket->pev->velocity = vecVelocity;
	pRocket->pev->angles = UTIL_VecToAngles(vecVelocity);
	pRocket->pev->friction = 0.5;

	// Touch
	pRocket->SetTouch( &CRocket::GrenadeTouch );

	if (FClassnameIs( pOwner->pev, "monster_ogre"))
		pRocket->pev->dmg = 40.0f;
	else pRocket->pev->dmg = 120.0f;

	// set newmis duration
	if ( gpGlobals->deathmatch == 4 )
	{
		pRocket->m_flAttackFinished = gpGlobals->time + 1.1;	// What's this used for?
		if (pOwner)
			pOwner->TakeDamage( pOwner->pev, pOwner->pev, 10, DMG_GENERIC );
	}

	pRocket->pev->nextthink = gpGlobals->time + 2.5;
	pRocket->SetThink( &CRocket::GrenadeExplode );

	return pRocket;
}

//=========================================================
CRocket* CRocket::CreatePlasma(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner)
{
	CRocket* pPlasma = GetClassPtr((CRocket*)NULL);

	UTIL_SetOrigin(pPlasma->pev, vecOrigin);
	SET_MODEL(ENT(pPlasma->pev), "sprites/weapons/plasma.spr");
	pPlasma->Spawn();
	pPlasma->pev->classname = MAKE_STRING("plasma");
	pPlasma->pev->owner = pOwner->edict();
	pPlasma->pev->iuser1 = 0;

	pPlasma->pev->frame = RANDOM_LONG(0,1);
	pPlasma->pev->effects |= EF_FULLBRIGHT;

	// Setup
	pPlasma->pev->movetype = MOVETYPE_FLYMISSILE;
	pPlasma->pev->solid = SOLID_BBOX;

	// Velocity
	pPlasma->pev->velocity = vecAngles * 1000;
	pPlasma->pev->angles = UTIL_VecToAngles(vecAngles);

	// Touch
	pPlasma->SetTouch(&CRocket::PlasmaTouch);

	// Safety Remove
	pPlasma->pev->nextthink = gpGlobals->time + 0.1;
	pPlasma->SetThink(&CRocket::PlasmaThink);

	pPlasma->pev->effects |= EF_BRIGHTLIGHT;

	return pPlasma;
}

void CRocket::PlasmaTouch(CBaseEntity* pOther)
{
	TraceResult tr = UTIL_GetGlobalTrace();

	// Remove if we've hit skybrush
	if (IsSkySurface(pOther, tr.vecEndPos, pev->velocity.Normalize()))
	{
		UTIL_Remove(this);
		return;
	}

	if (FClassnameIs(pOther->pev, "plasma"))
		return;

	// Do touch damage
	float flDmg = RANDOM_FLOAT(5, 40);
	CBaseEntity* pOwner = CBaseEntity::Instance(pev->owner);
	if (!pOwner) pOwner = gpWorld;

	if (pOther->pev->health && pOther->pev->takedamage)
		pOther->TakeDamage(pev, pOwner->pev, flDmg, DMG_GENERIC);

	UTIL_DecalTrace(&tr, DECAL_SMORCH1 + RANDOM_LONG(0, 1));

	pev->origin = pev->origin + pev->velocity * -0.025;

	pev->solid = SOLID_NOT;
	pev->iuser1 = 1;
	pev->frame = 2;
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CRocket::PlasmaThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if (pev->iuser1 == 0)
	{
		if (pev->frame == 0)
			pev->frame = 1;
		else
			pev->frame = 0;
	}
	else
	{
		if (pev->frame != MODEL_FRAMES(pev->modelindex) - 1)
			pev->frame++;
		else
			UTIL_Remove(this);
	}
}

CRocket* CRocket::CreateBFGBall(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner)
{
	CRocket* pBall = GetClassPtr((CRocket*)NULL);

	UTIL_SetOrigin(pBall->pev, vecOrigin);
	SET_MODEL(ENT(pBall->pev), "sprites/weapons/bfgball.spr");
	pBall->Spawn();
	pBall->pev->classname = MAKE_STRING("bfgball");
	pBall->pev->owner = pOwner->edict();
	pBall->pev->iuser1 = 0;

	pBall->pev->frame = RANDOM_LONG(0, 1);
	pBall->pev->effects |= EF_FULLBRIGHT;

	// Setup
	pBall->pev->movetype = MOVETYPE_FLYMISSILE;
	pBall->pev->solid = SOLID_BBOX;

	// Velocity
	pBall->pev->velocity = vecAngles * 610;
	pBall->pev->angles = UTIL_VecToAngles(vecAngles);

	pBall->SetTouch(&CRocket::BFGBallTouch);
	pBall->pev->nextthink = gpGlobals->time + 0.1;
	pBall->SetThink(&CRocket::BFGBallThink);

	pBall->pev->effects |= EF_LIGHT;

	return pBall;
}

void CRocket::BFGBallTouch(CBaseEntity* pOther)
{
	TraceResult tr = UTIL_GetGlobalTrace();

	// Remove if we've hit skybrush
	if (IsSkySurface(pOther, tr.vecEndPos, pev->velocity.Normalize()))
	{
		UTIL_Remove(this);
		return;
	}

	if (FClassnameIs(pOther->pev, "bfgball"))
		return;

	// Do touch damage
	float flDmg = (RANDOM_LONG(1, 8) * 100);
	CBaseEntity* pOwner = CBaseEntity::Instance(pev->owner);
	if (!pOwner) pOwner = gpWorld;

	if (pOther->pev->health && pOther->pev->takedamage)
		pOther->TakeDamage(pev, pOwner->pev, flDmg, DMG_GENERIC);

	pev->origin = pev->origin + pev->velocity * -0.025;

	pev->solid = SOLID_NOT;
	pev->iuser1 = 1;
	pev->frame = 2;
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.1f;

	EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/bfg_hit.wav", 1, ATTN_NORM);
}

void CRocket::BFGBallThink(void)
{
	if(pev->iuser1 != 2)
		pev->nextthink = gpGlobals->time + 0.1f;
	else
		pev->nextthink = gpGlobals->time + 0.15f;

	if(pev->frame == 3)
		BFGTracer(pev->angles);

	if (pev->iuser1 == 0)
	{
		if (pev->frame == 0)
			pev->frame = 1;
		else
			pev->frame = 0;
	}
	else if (pev->iuser1 == 1)
	{
		pev->frame = 1;
		pev->iuser1 = 2;
	}
	else
	{
		if (pev->frame != MODEL_FRAMES(pev->modelindex) - 1)
			pev->frame++;
		else
			UTIL_Remove(this);
	}
}

CRocket* CRocket::CreateBFGImpact(Vector vecOrigin)
{
	CRocket* pHit = GetClassPtr((CRocket*)NULL);

	UTIL_SetOrigin(pHit->pev, vecOrigin);
	SET_MODEL(ENT(pHit->pev), "sprites/weapons/bfghit.spr");
	pHit->Spawn();
	pHit->pev->effects |= EF_FULLBRIGHT;
	pHit->pev->renderfx = kRenderFxNoDissipation;

	pHit->pev->movetype = MOVETYPE_NONE;
	pHit->pev->solid = SOLID_NOT;

	pHit->pev->nextthink = gpGlobals->time + 0.15;
	pHit->SetThink(&CRocket::BFGImpactThink);

	//pHit->pev->effects |= EF_LIGHT;

	return pHit;
}

void CRocket::BFGImpactThink(void)
{
	pev->nextthink = gpGlobals->time + 0.15f;

	if (pev->frame != MODEL_FRAMES(pev->modelindex) - 1)
		pev->frame++;
	else
		UTIL_Remove(this);
}

void CRocket::autoaimTracer(Vector origin, Vector vForward, Vector vRight, Vector vUp)
{
	Vector vec2LOS;
	int damage;
	CBaseEntity* pEntity = NULL;
	CBaseEntity* pEhit = NULL;
	float invertcone, inrightcone, inforwardcone;
	TraceResult	tr;
	edict_t* pEdict = g_engfuncs.pfnPEntityOfEntIndex(1);
	float cone = -0.7;

	for (int i = 1; i < gpGlobals->maxEntities; i++, pEdict++)
	{
		if (pEdict->free) //not in use
			continue;

		if (pEdict == edict())
			continue;

		pEntity = Instance(pEdict);

		if (pEntity == NULL)
			continue;

		if (pEntity->IsPlayer())
			continue;

		if (!pEntity->IsAlive())
			continue;

		if (pEntity->pev->health <= 0)
			continue;

		if (pEntity->pev->flags & FL_MONSTER || pEntity->pev->movetype == MOVETYPE_PUSHSTEP) //monster probably or explosive<--pushstep
		{
			if (pEdict->v.takedamage == DAMAGE_NO)
				continue;

			vec2LOS = (origin - pEntity->Center());
			vec2LOS = vec2LOS.Normalize();

			inforwardcone = DotProduct(vec2LOS, vForward);
			inrightcone = DotProduct(vec2LOS, vRight);
			invertcone = DotProduct(vec2LOS, vUp);

			if ((inforwardcone < cone) && (inrightcone <= 0.12 && inrightcone >= -0.12) && (invertcone <= 0.52 && invertcone >= -0.52))
			{
				UTIL_TraceLine(origin, pEntity->Center(), ignore_monsters, edict(), &tr);

				if (tr.flFraction == 1.0)
				{
					//ALERT(at_console, "fwrd dot: %.2f | rght dot: %.2f |up dot: %.2f\n", inforwardcone, inrightcone, invertcone);
					ALERT(at_console, "hit monster: %s\n", STRING(pEntity->pev->classname));
					cone = inforwardcone;

					damage = RANDOM_LONG(50, 100);
					if (pEntity->pev->health && pEntity->pev->takedamage)
						pEntity->TakeDamage(pev, pev, damage, DMG_ALWAYSGIB);

					CRocket* pHit = CRocket::CreateBFGImpact(pEntity->Center() + Vector(0, 0, -4));
				}
				else if (pEntity->IsBSPModel() && tr.flFraction != 1.0)
				{
					pEhit = CBaseEntity::Instance(tr.pHit);

					if (pEhit == pEntity)
					{
						//ALERT(at_console, "hit explosive\n");
						cone = inforwardcone;

						damage = RANDOM_LONG(50, 100);
						if (pEntity->pev->health && pEntity->pev->takedamage)
							pEntity->TakeDamage(pev, pev, damage, DMG_GENERIC);

						CRocket* pHit = CRocket::CreateBFGImpact(pEntity->Center() + Vector(0, 0, -4));
					}
				}
			}
		}
	}
}

void CRocket::BFGTracer(Vector Angles)
{
	Vector rforward, rright, rup;
	CBaseEntity* pPlayer = UTIL_PlayerByIndex(1);

	if (!pPlayer)
		return;

	Vector whateven = Angles;//pPlayer->pev->v_angle;
	whateven.x = 0; // level out tracers;
	whateven.y -= 47;

	for (int i = 0; i < 40; i++)
	{
		gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
		UTIL_MakeVectorsPrivate(whateven, rforward, rright, rup);
		autoaimTracer(pPlayer->pev->origin, rforward, rright, rup) ;
		//UTIL_LineTest(pPlayer->pev->origin, pPlayer->pev->origin + autoaimTracer(pPlayer->pev->origin, rforward, rright, rup) * 512, 255, 1, 1, 5);
		
		whateven.y += 2.35;
	}
}

//=========================================================
void CRocket::Spawn( void )
{
	Precache();

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );
}

//=========================================================
void CRocket::RocketTouch ( CBaseEntity *pOther )
{
	TraceResult tr = UTIL_GetGlobalTrace();

	// Remove if we've hit skybrush
	if ( IsSkySurface( pOther, tr.vecEndPos, pev->velocity.Normalize( ) ) )
	{
		UTIL_Remove( this );
		return;
	}

	// Do touch damage
	float flDmg = RANDOM_FLOAT( 100, 120 );
	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if( !pOwner ) pOwner = gpWorld;

	if (pOther->pev->health && pOther->pev->takedamage)
	{
		if (FClassnameIs( pOther->pev, "monster_shambler"))
			flDmg *= 0.5f; // mostly immune

		pOther->TakeDamage( pev, pOwner->pev, flDmg, DMG_GENERIC );
	}

	// Don't do radius damage to the other, because all the damage was done in the impact
	Q_RadiusDamage(this, pOwner, 120, pOther);

	UTIL_DecalTrace(&tr, DECAL_SCORCH1 + RANDOM_LONG(0, 1));

	// Finish and remove
	Explode();
}

//=========================================================
void CRocket::GrenadeTouch( CBaseEntity *pOther )
{
	if (pOther->pev->takedamage == DAMAGE_AIM)
	{
		GrenadeExplode();
		return;
	}

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/bounce.wav", 1, ATTN_NORM);

	if (pev->velocity == g_vecZero)
		pev->avelocity = g_vecZero;
}

//=========================================================
void CRocket::GrenadeExplode()
{
	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if( !pOwner ) pOwner = gpWorld;

	Q_RadiusDamage(this, pOwner, pev->dmg, NULL);

	TraceResult tr;
	UTIL_TraceLine(pev->origin + Vector(0,0,16), pev->origin + Vector(0, 0, -16), ignore_monsters, ignore_glass, ENT(pev), &tr);
	UTIL_DecalTrace(&tr, DECAL_SCORCH1 + RANDOM_LONG(0, 1));

	// Finish and remove
	Explode();
}

//=========================================================
void CRocket::Explode()
{
	pev->origin = pev->origin - 8 * pev->velocity.Normalize();

	UTIL_ScreenShake( pev->origin, 16.0f, 2.0f, 0.5f, 250.0f );

	MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();

	UTIL_Remove( this );
}

LINK_ENTITY_TO_CLASS( zombie_missile, CZombieMissile );

CZombieMissile *CZombieMissile :: CreateMissile( Vector vecOrigin, Vector vecOffset, Vector vecAngles, CBaseEntity *pOwner )
{
	if( !pOwner || pOwner->m_hEnemy == NULL )
		return NULL;

	UTIL_MakeVectors( vecAngles );

	// calc org
	Vector org = vecOrigin + vecOffset.x * gpGlobals->v_forward + vecOffset.y * gpGlobals->v_right + (vecOffset.z - 24) * gpGlobals->v_up;

	CZombieMissile *pMeat = GetClassPtr((CZombieMissile *)NULL );

	UTIL_SetOrigin( pMeat->pev, org );

	pMeat->pev->classname = MAKE_STRING( "zombie_missile" );
	pMeat->pev->velocity = (pOwner->m_hEnemy->pev->origin - org).Normalize() * 600;
	pMeat->pev->velocity.z = 200;
	pMeat->pev->avelocity = Vector( 3000, 1000, 2000 );
	pMeat->pev->owner = pOwner->edict();
	pMeat->pev->solid = SOLID_BBOX;
	pMeat->Spawn();
	pMeat->SetTouch(&CZombieMissile::MeatTouch );

	// Safety removal
	pMeat->pev->nextthink = gpGlobals->time + 2.5f;
	pMeat->SetThink( &CZombieMissile::SUB_Remove );

	// done
	return pMeat;
}

CZombieMissile *CZombieMissile :: CreateSpray( Vector vecOrigin, Vector vecVelocity )
{
	CZombieMissile *pMeat = GetClassPtr((CZombieMissile *)NULL );

	UTIL_SetOrigin( pMeat->pev, vecOrigin );
	pMeat->pev->classname = MAKE_STRING( "zombie_missile" );
	pMeat->pev->velocity = vecVelocity;
	pMeat->pev->velocity.z += 250 + RANDOM_FLOAT( 0.0f, 50.0f );
	pMeat->pev->avelocity = Vector( 3000, 1000, 2000 );
	pMeat->pev->solid = SOLID_NOT;
	pMeat->Spawn();

	// Safety removal
	pMeat->pev->nextthink = gpGlobals->time + 1.0f;
	pMeat->SetThink( &CZombieMissile::SUB_Remove );

	// done
	return pMeat;
}

void CZombieMissile :: Spawn( void )
{
	Precache();

	// Setup
	pev->movetype = MOVETYPE_BOUNCE;
	pev->dmg = 10;

	SET_MODEL( ENT(pev), "progs/zom_gib.mdl" );
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );
}

void CZombieMissile :: Precache( void )
{
	PRECACHE_MODEL( "progs/zom_gib.mdl" );
	PRECACHE_SOUND( "zombie/z_hit.wav" );
	PRECACHE_SOUND( "zombie/z_miss.wav" );
}

void CZombieMissile :: MeatTouch( CBaseEntity *pOther )
{
	if( pOther->edict() == pev->owner )
		return; // don't explode on owner

	if( pOther->pev->takedamage )
	{
		CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);

		if( !FNullEnt( pOwner ))
			pOther->TakeDamage( pev, pOwner->pev, pev->dmg, DMG_GENERIC );
		else 
			pOther->TakeDamage( pev, pev, pev->dmg, DMG_GENERIC );

		EMIT_SOUND( edict(), CHAN_WEAPON, "zombie/z_hit.wav", 1.0, ATTN_NORM );
		UTIL_Remove( this );
		return;
	}

	EMIT_SOUND( edict(), CHAN_WEAPON, "zombie/z_miss.wav", 1.0, ATTN_NORM );	// bounce sound

	TraceResult tr = UTIL_GetGlobalTrace();
	UTIL_DecalTrace( &tr, DECAL_BLOOD1 + RANDOM_LONG( 0, 5 ));

	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;

	SetTouch( NULL );
	UTIL_Remove( this );
}

LINK_ENTITY_TO_CLASS( shal_missile, CShalMissile );

CShalMissile *CShalMissile :: CreateMissile( Vector vecOrigin, Vector vecVelocity )
{
	CShalMissile *pMiss = GetClassPtr((CShalMissile *)NULL );

	UTIL_SetOrigin( pMiss->pev, vecOrigin );
	pMiss->pev->classname = MAKE_STRING( "shal_missile" );
	pMiss->pev->avelocity = Vector( 300, 300, 300 );
	pMiss->pev->velocity = vecVelocity;
	pMiss->Spawn();

	// done
	return pMiss;
}

void CShalMissile :: Spawn( void )
{
	Precache();

	// Setup
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLYMISSILE;

	SET_MODEL( ENT(pev), "progs/v_spike.mdl" );
	UTIL_SetSize(pev, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ));	// allow to explode with himself
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CShalMissile::ShalTouch );
	SetThink( &CShalMissile::ShalHome );
}

void CShalMissile :: Precache( void )
{
	PRECACHE_MODEL( "progs/v_spike.mdl" );
}

void CShalMissile :: ShalHome( void )
{
	if( !pev->enemy || pev->enemy->v.health <= 0.0f )
	{
		CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
		if( pOwner ) pOwner->pev->impulse--; // decrease missiles		
		REMOVE_ENTITY( edict() );
		return;
	}

	CBaseEntity *pEnemy = CBaseEntity::Instance(pev->enemy);
	Vector vecTmp = pEnemy->pev->origin + Vector( 0, 0, 10 );
	Vector vecDir = ( vecTmp - pev->origin ).Normalize();

	if( g_iSkillLevel == SKILL_NIGHTMARE )
		pev->velocity = vecDir * 350.0f;
	else pev->velocity = vecDir * 250.0f;

	pev->nextthink = gpGlobals->time + 0.2f;
}

void CShalMissile :: ShalTouch( CBaseEntity *pOther )
{
	if( pOther->edict() == pev->owner )
		return; // don't explode on owner

	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);

	if( !pOwner ) pOwner = gpWorld; // shalrath is gibbed
	else pOwner->pev->impulse--; // decrease missiles

	if( FClassnameIs( pOther->pev, "monster_zombie" ))
		pOther->TakeDamage( pev, pev, 110, DMG_GENERIC );
	Q_RadiusDamage( this, pOwner, 40, gpWorld );

	MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
	MESSAGE_END();

	pev->velocity = g_vecZero;
	SetThink( NULL );
	SetTouch( NULL );

	UTIL_Remove( this );
}

// Plays quad sound if needed
void CBasePlayer::SuperDamageSound( void )
{
	if ( m_iItems & IT_QUAD )
	{
		if ( m_flSuperDamageSound < gpGlobals->time)
		{
			EMIT_SOUND( edict(), CHAN_ITEM, "items/damage3.wav", 1.0, ATTN_NORM );
			m_flSuperDamageSound = gpGlobals->time + 1;
		}
	}
}

//================================================================================================
// WEAPON SELECTION
//================================================================================================
// Return the ID of the best weapon being carried by the player
int CBasePlayer::W_BestWeapon()
{
	if (ammo_cells >= 40 && (m_iItems & IT_LIGHTNING) )
		return IT_LIGHTNING;
	else if (ammo_cells >= 1 && (m_iItems & IT_GRENADE_LAUNCHER))
		return IT_GRENADE_LAUNCHER;
	else if (ammo_rockets >= 1 && (m_iItems & IT_ROCKET_LAUNCHER))
		return IT_ROCKET_LAUNCHER;
	else if(ammo_nails >= 2 && (m_iItems & IT_SUPER_NAILGUN) )
		return IT_SUPER_NAILGUN;
	else if(ammo_shells >= 2 && (m_iItems & IT_SUPER_SHOTGUN) )
		return IT_SUPER_SHOTGUN;
	else if(ammo_nails >= 1 && (m_iItems & IT_NAILGUN) )
		return IT_NAILGUN;
	else if(ammo_shells >= 1 && (m_iItems & IT_SHOTGUN)  )
		return IT_SHOTGUN;
	else if (ammo_nails >= 1 && (m_iItems & IT_PISTOL))
		return IT_PISTOL;
	else if (m_iItems & IT_CHAINSAW)
		return IT_CHAINSAW;
		
	return IT_AXE;
}

// Weapon setup after weapon switch
void CBasePlayer::W_SetCurrentAmmo( int sendanim /* = 1 */ )
{
	m_iItems &= ~(IT_SHELLS | IT_NAILS | IT_ROCKETS | IT_CELLS);
	int	iszViewModel = 0;
	char *viewmodel = "";
	int iszWeaponModel = 0;
	char *szAnimExt;
	
	viewmodel = "models/null.mdl";
	iszViewModel = MAKE_STRING(viewmodel);
	szAnimExt = "shotgun";
	iszWeaponModel = MAKE_STRING("models/null.mdl");

	// Find out what weapon the player's using
	if ((m_iWeapon == IT_AXE) || (m_iWeapon == IT_CHAINSAW))
	{
		m_pCurrentAmmo = NULL;
	}
	else if ((m_iWeapon == IT_PISTOL) || (m_iWeapon == IT_NAILGUN) || (m_iWeapon == IT_SUPER_NAILGUN))
	{
		m_pCurrentAmmo = &ammo_nails;
		m_iItems |= IT_NAILS;
	}
	else if ((m_iWeapon == IT_SHOTGUN) || (m_iWeapon == IT_SUPER_SHOTGUN))
	{
		m_pCurrentAmmo = &ammo_shells;
		m_iItems |= IT_SHELLS;
	}
	else if ((m_iWeapon == IT_GRENADE_LAUNCHER) || (m_iWeapon == IT_LIGHTNING))
	{
		m_pCurrentAmmo = &ammo_cells;
		m_iItems |= IT_CELLS;
	}
	else if (m_iWeapon == IT_ROCKET_LAUNCHER)
	{
		m_pCurrentAmmo = &ammo_rockets;
		m_iItems |= IT_ROCKETS;
	}
	else
	{
		m_pCurrentAmmo = NULL;
	}

	pev->viewmodel = iszViewModel;
	pev->weaponmodel = iszWeaponModel;
	strcpy( m_szAnimExtention, szAnimExt );
}

void CBasePlayer::CheckAmmo( void )
{
	int backpack = 1;

	if (m_iItems & IT_BACKPACK)
		backpack = 2;

	if (ammo_shells > IT_MAX_SHELLS * backpack)
		ammo_shells = IT_MAX_SHELLS * backpack;
	if (ammo_nails > IT_MAX_NAILS * backpack)
		ammo_nails = IT_MAX_NAILS * backpack;
	if (ammo_rockets > IT_MAX_ROCKETS * backpack)
		ammo_rockets = IT_MAX_ROCKETS * backpack;
	if (ammo_cells > IT_MAX_CELLS * backpack)
		ammo_cells = IT_MAX_CELLS * backpack;
}

// Return TRUE if the weapon still has ammo
BOOL CBasePlayer::W_CheckNoAmmo( void )
{
	if (m_pCurrentAmmo && *m_pCurrentAmmo > 0)
		return TRUE;

	if (m_iWeapon == IT_AXE || m_iWeapon == IT_CHAINSAW)
		return TRUE;

	m_iCueWeapon = W_BestWeapon();
	//W_SetCurrentAmmo();
	return FALSE;
}

// Change to the specified weapon
void CBasePlayer::W_ChangeWeapon( int iWeaponNumber )
{
	int iWeapon = 0;
	BOOL bHaveAmmo = TRUE;

	//if (m_flNextAttack >= UTIL_WeaponTimeBase())
		//return;

	//if (m_iCurFrame != 0)
		//return;

	if (m_iCueWeapon != m_iWeapon)
		return;

	if (HOLSTER_SERVER_IN >= HOLSTER_STATE_START && HOLSTER_SERVER_IN < HOLSTER_STATE_DONE) // DOOM - holstering
		return;
	
	if (iWeaponNumber == 1)
	{
		//iWeapon = IT_AXE;
		if (m_iItems & IT_CHAINSAW)
		{
			if (m_iWeapon == IT_CHAINSAW)
				iWeapon = IT_AXE;
			else if (m_iWeapon != IT_CHAINSAW)
				iWeapon = IT_CHAINSAW;
		}
		else
		{
			iWeapon = IT_AXE;
		}
	}
	else if (iWeaponNumber == 2)
	{
		iWeapon = IT_PISTOL;
		if (ammo_nails < 1)
			bHaveAmmo = FALSE;
	}
	else if (iWeaponNumber == 3)
	{
		//go to super shotgun first like doom
		if (ammo_shells >= 2 && (m_iItems & IT_SUPER_SHOTGUN))
		{
			if (m_iWeapon == IT_SUPER_SHOTGUN)
			{
				iWeapon = IT_SHOTGUN;
				if (ammo_shells < 1)
					bHaveAmmo = FALSE;
			}
			else if (m_iWeapon != IT_SUPER_SHOTGUN)
			{
				iWeapon = IT_SUPER_SHOTGUN;
				if (ammo_shells < 2)
					bHaveAmmo = FALSE;
			}
		}
		else
		{
			iWeapon = IT_SHOTGUN;
			if (ammo_shells < 1)
				bHaveAmmo = FALSE;
		}
	}
	else if (iWeaponNumber == 4)
	{
		//going to do the same as super shotgun here for chaingun
		if (ammo_nails >= 2 && (m_iItems & IT_SUPER_NAILGUN))
		{
			if (m_iWeapon == IT_SUPER_NAILGUN)
			{
				iWeapon = IT_NAILGUN;
				if (ammo_nails < 1)
					bHaveAmmo = FALSE;
			}
			else if (m_iWeapon != IT_SUPER_NAILGUN)
			{
				iWeapon = IT_SUPER_NAILGUN;
				if (ammo_nails < 2)
					bHaveAmmo = FALSE;
			}
		}
		else
		{
			iWeapon = IT_NAILGUN;
			if (ammo_nails < 1)
				bHaveAmmo = FALSE;
		}
	}               
	else if (iWeaponNumber == 5)
	{
		iWeapon = IT_ROCKET_LAUNCHER;
		if (ammo_rockets < 1)
			bHaveAmmo = FALSE;
	}
	else if (iWeaponNumber == 6)
	{
		iWeapon = IT_GRENADE_LAUNCHER;
		if (ammo_cells < 1)
			bHaveAmmo = FALSE;
	}
	else if (iWeaponNumber == 7)
	{
		iWeapon = IT_LIGHTNING;
		if (ammo_cells < 40)
			bHaveAmmo = FALSE;
	}

	// Have the weapon?
	if ( !(m_iItems & iWeapon) )
	{       
		m_sMessage = MAKE_STRING("no weapon!"); //CLIENT_PRINTF( edict(), print_console, "no weapon.\n" );
		return;
	}
	
	// Have ammo for it?
	if ( !bHaveAmmo )
	{
		m_sMessage = MAKE_STRING("not enough ammo!"); //CLIENT_PRINTF( edict(), print_console, "not enough ammo.\n" );
		return;
	}

	// Set weapon, update ammo
	m_iCueWeapon = iWeapon;
	//W_SetCurrentAmmo();
}


//================================================================================================
// WEAPON FUNCTIONS
//================================================================================================
// Returns true if the inflictor can directly damage the target.  Used for explosions and melee attacks.
float Q_CanDamage(CBaseEntity *pTarget, CBaseEntity *pInflictor) 
{
	TraceResult trace;

	// bmodels need special checking because their origin is 0,0,0
	if (pTarget->pev->movetype == MOVETYPE_PUSH)
	{
		UTIL_TraceLine( pInflictor->pev->origin, 0.5 * (pTarget->pev->absmin + pTarget->pev->absmax), ignore_monsters, NULL, &trace );
		if (trace.flFraction == 1)
			return TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(trace.pHit);
		if (pEntity == pTarget)
			return TRUE;
		return FALSE;
	}
	
	UTIL_TraceLine( pInflictor->pev->origin, pTarget->pev->origin, ignore_monsters, NULL, &trace );
	if (trace.flFraction == 1)
		return TRUE;
	UTIL_TraceLine( pInflictor->pev->origin, pTarget->pev->origin + Vector(15,15,0), ignore_monsters, NULL, &trace );
	if (trace.flFraction == 1)
		return TRUE;
	UTIL_TraceLine( pInflictor->pev->origin, pTarget->pev->origin + Vector(-15,-15,0), ignore_monsters, NULL, &trace );
	if (trace.flFraction == 1)
		return TRUE;
	UTIL_TraceLine( pInflictor->pev->origin, pTarget->pev->origin + Vector(-15,15,0), ignore_monsters, NULL, &trace );
	if (trace.flFraction == 1)
		return TRUE;
	UTIL_TraceLine( pInflictor->pev->origin, pTarget->pev->origin + Vector(15,-15,0), ignore_monsters, NULL, &trace );
	if (trace.flFraction == 1)
		return TRUE;

	return FALSE;
}

// Quake Bullet firing
void CBasePlayer::FireBullets( entvars_t *pev, int iShots, Vector vecDir, Vector vecSpread, int iFlags )
{
	TraceResult trace;
	UTIL_MakeVectors(pev->v_angle);
	int damage;

	Vector vecSrc = pev->origin + (gpGlobals->v_forward * 10);
	vecSrc.z = pev->absmin.z + (pev->size.z * 0.8); //0.7
	ClearMultiDamage();

	while ( iShots > 0 )
	{
		Vector vecPath = vecDir + ( RANDOM_FLOAT( -1, 1 ) * vecSpread.x * gpGlobals->v_right ) + ( RANDOM_FLOAT( -1, 1 ) * vecSpread.y * gpGlobals->v_up );
		Vector vecEnd = vecSrc + ( vecPath * 2048 );

		//if( g_iSkillLevel == SKILL_EASY )
			gpGlobals->trace_flags = FTRACE_SIMPLEBOX; // ignore hitboxes for easy skills //DOOM - quake doesnt have hitboxes to trace?

		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &trace );

		if (trace.flFraction != 1.0)
		{
			if (iFlags & BULLET_FLAG_NPC)
				damage = RANDOM_LONG(3, 10);
			else
				damage = RANDOM_LONG(4, 10);

			CBaseEntity *pEntity = CBaseEntity::Instance( trace.pHit );

			if (FClassnameIs(pEntity->pev, "monster_zombie"))
			{
				if (iFlags & BULLET_FLAG_SHOTGUN)
					damage = 4;
				else if (iFlags & BULLET_FLAG_SUPERSHOTGUN)
					damage = 10;
			}

			UTIL_DecalTrace(&trace, DECAL_GUNSHOT1 + RANDOM_LONG(0, 1));

			pEntity->TraceAttack( pev, damage, vecPath, &trace, DMG_BULLET );
		}

		iShots--;
	}

	ApplyMultiDamage( pev, pev );
}

// Quake Radius damage
void Q_RadiusDamage( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, CBaseEntity *pIgnore )
{
	CBaseEntity *pEnt = NULL;

	while ( (pEnt = UTIL_FindEntityInSphere( pEnt, pInflictor->pev->origin, flDamage+40 )) != NULL )
	{
		if (pEnt != pIgnore)
		{
			if (pEnt->pev->takedamage)
			{
				Vector vecOrg = pEnt->pev->origin + ((pEnt->pev->mins + pEnt->pev->maxs) * 0.5);
				float flPoints = 0.5 * (pInflictor->pev->origin - vecOrg).Length();
				if (flPoints < 0)
					flPoints = 0;
				flPoints = flDamage - flPoints;
				
				if (pEnt == pAttacker)
					flPoints = flPoints * 0.5;
				if (flPoints > 0)
				{
					if ( Q_CanDamage( pEnt, pInflictor ) )
					{
						// shambler takes half damage from all explosions
						if (FClassnameIs( pEnt->pev, "monster_shambler"))
							pEnt->TakeDamage( pInflictor->pev, pAttacker->pev, flPoints * 0.5f, DMG_GENERIC );
						else
							pEnt->TakeDamage( pInflictor->pev, pAttacker->pev, flPoints, DMG_GENERIC );
					}
				}
			}
		}
	}
}

// Lightning hit a target
void LightningHit(CBaseEntity *pTarget, CBaseEntity *pAttacker, Vector vecHitPos, float flDamage, TraceResult *ptr, Vector vecDir ) 
{
	SpawnBlood( vecHitPos, BLOOD_COLOR_RED, flDamage * 1.5 );

	if ( g_pGameRules->PlayerRelationship( pTarget, pAttacker ) != GR_TEAMMATE )
	{
		pTarget->TakeDamage( pAttacker->pev, pAttacker->pev, flDamage, DMG_GENERIC );
		pTarget->TraceBleed( flDamage, vecDir, ptr, DMG_BULLET ); // have to use DMG_BULLET or it wont spawn.
	}

	UTIL_ScreenShake(pTarget->pev->origin, 2.0f, 2.0f, 0.5f, 128.0f);
}

void SpawnMeatSpray( Vector vecOrigin, Vector vecVelocity )
{
	CZombieMissile :: CreateSpray( vecOrigin, vecVelocity );
}

/*
================
SpawnBlood
================
*/
void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage)
{
	UTIL_BloodDrips( vecSpot, g_vecAttackDir * 5, bloodColor, (int)flDamage );
}

//
// EjectBrass - tosses a brass shell from passed origin at passed velocity
//
void EjectBrass ( const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE( TE_MODEL);
		WRITE_COORD( vecOrigin.x);
		WRITE_COORD( vecOrigin.y);
		WRITE_COORD( vecOrigin.z);
		WRITE_COORD( vecVelocity.x);
		WRITE_COORD( vecVelocity.y);
		WRITE_COORD( vecVelocity.z);
		WRITE_ANGLE( rotation );
		WRITE_SHORT( model );
		WRITE_BYTE ( soundtype);
		WRITE_BYTE ( 25 );// 2.5 seconds
	MESSAGE_END();
}

// Lightning Damage
void CBasePlayer::LightningDamage( Vector p1, Vector p2, CBaseEntity *pAttacker, float flDamage, Vector vecDir)
{
	TraceResult trace;
	Vector vecThru = (p2 - p1).Normalize();
	vecThru.x = 0 - vecThru.y;
	vecThru.y = vecThru.x;
	vecThru.z = 0;
	vecThru = vecThru * 16;

	CBaseEntity *pEntity1 = NULL;
	CBaseEntity *pEntity2 = NULL;

	gpGlobals->trace_flags = FTRACE_SIMPLEBOX; // ignore hitboxes for now

	// Hit first target?
	UTIL_TraceLine( p1, p2, dont_ignore_monsters, ENT(pAttacker->pev), &trace );
	CBaseEntity *pEntity = CBaseEntity::Instance(trace.pHit);
	if (pEntity && pEntity->pev->takedamage)
	{
		LightningHit(pEntity, pAttacker, trace.vecEndPos, flDamage, &trace, vecDir );
	}
	pEntity1 = pEntity;

	gpGlobals->trace_flags = FTRACE_SIMPLEBOX; // ignore hitboxes for now

	// Hit second target?
	UTIL_TraceLine( p1 + vecThru, p2 + vecThru, dont_ignore_monsters, ENT(pAttacker->pev), &trace );
	pEntity = CBaseEntity::Instance(trace.pHit);
	if (pEntity && pEntity != pEntity1 && pEntity->pev->takedamage)
	{
		LightningHit(pEntity, pAttacker, trace.vecEndPos, flDamage, &trace, vecDir );
	}
	pEntity2 = pEntity;

	gpGlobals->trace_flags = FTRACE_SIMPLEBOX; // ignore hitboxes for now

	// Hit third target?
	UTIL_TraceLine( p1 - vecThru, p2 - vecThru, dont_ignore_monsters, ENT(pAttacker->pev), &trace );
	pEntity = CBaseEntity::Instance(trace.pHit);
	if (pEntity && pEntity != pEntity1 && pEntity != pEntity2 && pEntity->pev->takedamage)
	{
		LightningHit(pEntity, pAttacker, trace.vecEndPos, flDamage, &trace, vecDir );
	}
}

//================================================================================================
// WEAPON FIRING
//================================================================================================
// fists & chainsaw
void CBasePlayer::W_FireAxe( void )
{
	TraceResult trace;
	Vector vecSrc = pev->origin + Vector(0, 0, 16);

	// Swing forward 64 units
	UTIL_MakeVectors(pev->v_angle);
	UTIL_TraceLine( vecSrc, vecSrc + (gpGlobals->v_forward * 64), dont_ignore_monsters, ENT(pev), &trace );

	if (trace.flFraction == 1.0)
	{
		if (m_iWeapon == IT_CHAINSAW)
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/chainsaw_rev.wav", 1, ATTN_NORM);
		else
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/ax1.wav", 1, ATTN_NORM);

		return;
	}	

	// we make sound, so show hostile
	m_flShowHostile = gpGlobals->time + 1.0f;

	Vector vecOrg = trace.vecEndPos - gpGlobals->v_forward * 4;

	CBaseEntity *pEntity = CBaseEntity::Instance(trace.pHit);
	if (pEntity && pEntity->pev->takedamage)
	{
		int iDmg = 5 * RANDOM_LONG(1, 7);//RANDOM_LONG(3,20);

		if (m_iWeapon == IT_CHAINSAW)
			iDmg = 5 * RANDOM_LONG(1, 4);/*RANDOM_LONG(5, 20)*/;

		if (FClassnameIs(pEntity->pev, "monster_zombie") && m_iWeapon == IT_CHAINSAW)
			pEntity->TakeDamage(pev, pev, (5 + (75 * RANDOM_LONG(0,1))), DMG_ALWAYSGIB);
		else
			pEntity->TakeDamage( pev, pev, iDmg, DMG_GENERIC );

		if (m_iWeapon == IT_CHAINSAW)
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/chainsaw_hit.wav", 1, ATTN_NORM);
			UTIL_ScreenShake(pev->origin, 2.0f, 2.0f, 0.5f, 128.0f);

			//make player face attack direction and pull them into enemy
			if (!pEntity->IsBSPModel())
			{
				Vector facedir = (pEntity->pev->origin - pev->origin).Normalize();
				Vector facediryaw = UTIL_VecToAngles(facedir);

				facediryaw.x = pev->v_angle.x;
				pev->fixangle = 1;
				pev->angles.y = facediryaw.y + (RANDOM_LONG(-5, 5));
				pev->angles.x = facediryaw.x;

				UTIL_MakeVectors(pev->angles);
				pev->velocity = gpGlobals->v_forward * 200;
			}
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/punch.wav", 1, ATTN_NORM);
		}

		if (g_pGameRules->PlayerRelationship(this, pEntity) != GR_TEAMMATE)
		{
			SpawnBlood(vecOrg + Vector(0,0,2), (BLOOD_COLOR_RED - 2), iDmg * 4); // Make a lot of Blood!
			UTIL_ParticleEffect(vecOrg + Vector(0, 0, 2), g_vecAttackDir, BLOOD_COLOR_RED, iDmg * 4);
		}
	}
	else //hit world
	{
		if (m_iWeapon == IT_CHAINSAW)
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/chainsaw_hit.wav", 1, ATTN_NORM);
			UTIL_ScreenShake(pev->origin, 2.0f, 2.0f, 0.5f, 128.0f);
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/punch.wav", 1, ATTN_NORM);
		}

		MESSAGE_BEGIN( MSG_BROADCAST, gmsgTempEntity );
			WRITE_BYTE( TE_GUNSHOT );
			WRITE_COORD( vecOrg.x );
			WRITE_COORD( vecOrg.y );
			WRITE_COORD( vecOrg.z );
		MESSAGE_END();
	}
}

//pistol firing
void CBasePlayer::W_FirePistol(BOOL refire)
{
	if (gpGlobals->deathmatch != 4)
		*m_pCurrentAmmo -= 1;

	if (*m_pCurrentAmmo <= 0)
		m_iCueWeapon = W_BestWeapon();

	EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/pistol_fire.wav", 1, ATTN_NORM);
	UTIL_MakeVectors(pev->v_angle);

	GetAutoAimVector();

	if(refire)
		FireBullets(pev, 1, m_vAutoAim, Vector(0.045, 0.045, 0), 0);
	else
		FireBullets(pev, 1, m_vAutoAim, Vector(0, 0, 0), 0);
}

void CBasePlayer::W_FireShotgun( void )
{
	if (gpGlobals->deathmatch != 4 )
		*m_pCurrentAmmo -= 1;

	if (*m_pCurrentAmmo <= 0)
		m_iCueWeapon = W_BestWeapon();

	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/shotgun_fire.wav", 1, ATTN_NORM );
	UTIL_MakeVectors( pev->v_angle );

	GetAutoAimVector();

	FireBullets( pev, 7, m_vAutoAim, Vector(0.08, 0.03, 0), BULLET_FLAG_SHOTGUN);
}

void CBasePlayer::W_FireSuperShotgun( void )
{
	if (*m_pCurrentAmmo == 1)
	{
		m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
		m_iCurFrame = 0;
		m_iCueWeapon = W_BestWeapon();
		return;
	}

	if (gpGlobals->deathmatch != 4 )
		*m_pCurrentAmmo -= 2;

	if (*m_pCurrentAmmo <= 1)
		m_iCueWeapon = W_BestWeapon();

	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/supershot_fire.wav", 1, ATTN_NORM );
	UTIL_MakeVectors( pev->v_angle );

	GetAutoAimVector();

	FireBullets( pev, 20, m_vAutoAim, Vector(0.14, 0.08, 0), BULLET_FLAG_SUPERSHOTGUN);
}

void CBasePlayer::W_FireRocket( void )
{
	if (gpGlobals->deathmatch != 4 )
		*m_pCurrentAmmo -= 1;

	if (*m_pCurrentAmmo <= 0)
		m_iCueWeapon = W_BestWeapon();

	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/rocket_launch.wav", 1, ATTN_NORM );

	// Create the rocket
	UTIL_MakeVectors( pev->v_angle );
	Vector vecOrg = pev->origin + (gpGlobals->v_forward * 8) + Vector(0,0,16);

	GetAutoAimVector();

	CRocket *pRocket = CRocket::CreateRocket( vecOrg, m_vAutoAim, this );
}

// Grenade launcher UNUSED
void CBasePlayer::W_FireGrenade( void )
{       
	if (gpGlobals->deathmatch != 4 )
		*m_pCurrentAmmo -= 1;

	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/grenade.wav", 1, ATTN_NORM );

	// Get initial velocity
	UTIL_MakeVectors( pev->v_angle );
	Vector vecVelocity;
	if ( pev->v_angle.x )
	{
		vecVelocity = gpGlobals->v_forward * 600 + gpGlobals->v_up * 200 + RANDOM_FLOAT(-1,1) * gpGlobals->v_right * 10 + RANDOM_FLOAT(-1,1) * gpGlobals->v_up * 10;
	}
	else
	{
		GET_AIM_VECTOR( edict(), 1000.0f, vecVelocity );
		vecVelocity = vecVelocity * 600;
		vecVelocity.z = 200;
	}

	// Create the grenade
	CRocket *pRocket = CRocket::CreateGrenade( pev->origin, vecVelocity, this );
}

void CBasePlayer::W_FirePlasma(void)
{
	if (gpGlobals->deathmatch != 4)
		*m_pCurrentAmmo -= 1;

	if (*m_pCurrentAmmo <= 0)
		m_iCueWeapon = W_BestWeapon();

	UTIL_MakeVectors(pev->v_angle);
	Vector vecOrg = pev->origin + (gpGlobals->v_forward * 8) + Vector(0, 0, 16);

	GetAutoAimVector();

	CRocket* pPlasma = CRocket::CreatePlasma(vecOrg, m_vAutoAim, this);
}

void CBasePlayer::W_FireBFG(void)
{
	*m_pCurrentAmmo -= 40;

	if (*m_pCurrentAmmo <= 0)
		m_iCueWeapon = W_BestWeapon();

	ALERT(at_console, "fired bfg\n");

	UTIL_MakeVectors(pev->v_angle);
	Vector vecOrg = pev->origin + (gpGlobals->v_forward * 8) + Vector(0, 0, 16);

	GetAutoAimVector();

	CRocket* pBall = CRocket::CreateBFGBall(vecOrg, m_vAutoAim, this);
}

void CBasePlayer::W_FireRifle( void )
{
	if (gpGlobals->deathmatch != 4)
		*m_pCurrentAmmo -= 1;

	if(*m_pCurrentAmmo <= 0)
		m_iCueWeapon = W_BestWeapon();

	if (m_iWeapon == IT_SUPER_NAILGUN)
		EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/chaingun_fire.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/pistol_fire.wav", 1, ATTN_NORM);

	GetAutoAimVector();
	FireBullets(pev, 1, m_vAutoAim, Vector(0.025, 0.025, 0), 0);
}

//===============================================================================
// PLAYER WEAPON USE
//===============================================================================
void CBasePlayer::W_Attack( void )
{
	BOOL refire;

	if (m_iCueWeapon != m_iWeapon)
		return;

	// Out of ammo?
	if ( !W_CheckNoAmmo() )
		return;

	if (HOLSTER_SERVER_IN >= HOLSTER_STATE_START && HOLSTER_SERVER_IN < HOLSTER_STATE_DONE) //if (m_iWeapon != m_iClientWeapon || (GET_HOLSTER != 0)) //DOOM - holstering
		return;

	if (m_iWeapon == IT_PISTOL)
	{
		if (m_iCurFrame <= 0)
		{
			m_iCurFrame = 2;
			m_flNextFrameTime = gpGlobals->time + 0.17;
			refire = FALSE;
		}
		else
		{
			m_iCurFrame = 1;
			m_flNextFrameTime = gpGlobals->time + 0.1;
			refire = TRUE;
		}
	}
	else if (m_iWeapon == IT_GRENADE_LAUNCHER)
	{
		if (m_iCurFrame == 0 || m_iCurFrame == 3)
		{
			m_iCurFrame = 1;
			m_flNextFrameTime = gpGlobals->time + 0.05;
		}
	}
	else if (m_iWeapon == IT_LIGHTNING)
	{
		if (*m_pCurrentAmmo < 40)
		{
			m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
			m_iCurFrame = 0;
			m_iCueWeapon = W_BestWeapon();
			return;
		}

		m_iCurFrame = 1;
		m_flNextFrameTime = gpGlobals->time + 0.4;
		m_flNextAttack = UTIL_WeaponTimeBase() + 1.1;

		EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/bfg_fire.wav", 1, ATTN_NORM);
	}
	else
	{
		m_iCurFrame = 1;
		m_flNextFrameTime = gpGlobals->time + 0.1;
	}

	// g-cont. don't update shaft animation it's looped
	if ( m_iWeapon != IT_LIGHTNING || pev->weaponanim != 1 )
		SendWeaponAnim( 1 );

	// g-cont. no react for monsters when player fire axe
	if (m_iWeapon != IT_AXE)
		m_flShowHostile = gpGlobals->time + 1.0f;

	if (m_iWeapon == IT_AXE)
	{
		m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

		W_FireAxe();
	}
	else if (m_iWeapon == IT_CHAINSAW)
	{
		m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
		W_FireAxe();
	}
	else if (m_iWeapon == IT_PISTOL)
	{
		m_flNextAttack = UTIL_WeaponTimeBase() + 0.4;

		W_FirePistol(refire);
	}
	else if (m_iWeapon == IT_SHOTGUN)
	{
		m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;

		W_FireShotgun();
	}
	else if (m_iWeapon == IT_SUPER_SHOTGUN)
	{
		m_flNextAttack = UTIL_WeaponTimeBase() + 1.5;

		W_FireSuperShotgun();
	}
	else if (m_iWeapon == IT_NAILGUN)
	{	
		m_flNextAttack = UTIL_WeaponTimeBase() + 0.175; //0.1
	
		W_FireRifle();
	}
	else if (m_iWeapon == IT_ROCKET_LAUNCHER)
	{
		m_flNextAttack = UTIL_WeaponTimeBase() + 0.55;
	}
	else if (m_iWeapon == IT_GRENADE_LAUNCHER)
	{
		m_flNextAttack = UTIL_WeaponTimeBase() + 0.075;

		if (!m_bPlasmaFire)
		{
			EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/plasma_fire.wav", 1, ATTN_NORM);
			m_bPlasmaFire = TRUE;
		}

		EMIT_SOUND(edict(), CHAN_ITEM, "weapons/plasma_stop.wav", 0.5, ATTN_NORM);

		W_FirePlasma();//W_FireGrenade();
	}

	m_flAttackFinished = m_flNextAttack; //for ai slide dodge
	// Make player attack
	if ( pev->health >= 0 )
		SetAnimation( PLAYER_ATTACK1 );
}
