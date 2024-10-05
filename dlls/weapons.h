
#ifndef WEAPONS_H
#define WEAPONS_H

extern DLL_GLOBAL short g_sModelIndexBubbles;// holds the index for the bubbles model
//extern DLL_GLOBAL short g_sModelIndexBloodDrop;// holds the sprite index for blood drops
//extern DLL_GLOBAL short g_sModelIndexBloodSpray;// holds the sprite index for blood spray (bigger)

extern void ClearMultiDamage(void);
extern void ApplyMultiDamage(entvars_t* pevInflictor, entvars_t* pevAttacker );
extern void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType);

extern void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern void Q_RadiusDamage( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, CBaseEntity *pIgnore );
extern void EjectBrass ( const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype );

extern float Q_CanDamage(CBaseEntity *pTarget, CBaseEntity *pInflictor);
extern void SpawnMeatSpray( Vector vecOrigin, Vector vecVelocity );

extern BOOL IsSkySurface( CBaseEntity *pEnt, const Vector &point, const Vector &vecDir );

typedef struct 
{
	CBaseEntity	*pEntity;
	float		amount;
} MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;

class CRocket : public CBaseEntity
{
public:
	void Spawn( void );
	void Explode( void );

	// Rocket funcs
	static CRocket *CreateRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner );
	void EXPORT RocketTouch( CBaseEntity *pOther );

	// Grenade funcs
	static CRocket *CreateGrenade( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner );
	void EXPORT GrenadeTouch( CBaseEntity *pOther );
	void EXPORT GrenadeExplode( void );

	// Plasma funcs //DOOM
	static CRocket* CreatePlasma(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner);
	void EXPORT PlasmaTouch(CBaseEntity* pOther);
	void EXPORT PlasmaThink(void);

	static CRocket* CreateBFGBall(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner);
	void EXPORT BFGBallTouch(CBaseEntity* pOther);
	void EXPORT BFGBallThink(void);
	void BFGTracer(void);
	static CRocket* CreateBFGImpact(Vector vecOrigin);
	void EXPORT BFGImpactThink(void);

	void autoaimTracer(Vector origin, Vector vForward, Vector vRight, Vector vUp);



	int	m_iTrail;
};

class CNail : public CBaseEntity
{
public:
	void Spawn( void );
	static  CNail *CreateNail( Vector vecOrigin, Vector vecDir, CBaseEntity *pOwner );
	static  CNail *CreateSuperNail( Vector vecOrigin, Vector vecDir, CBaseEntity *pOwner );
	static  CNail *CreateKnightSpike( Vector vecOrigin, Vector vecDir, CBaseEntity *pOwner );
	void EXPORT NailTouch( CBaseEntity *pOther );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );
};

class CLaser : public CBaseEntity
{
public:
	void Spawn( void );
	static  CLaser *LaunchLaser( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner );
	void EXPORT LaserTouch( CBaseEntity *pOther );
};

class CZombieMissile : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	void EXPORT MeatTouch( CBaseEntity *pOther );
public:
	static CZombieMissile *CreateMissile( Vector vecOrigin, Vector vecOffset, Vector vecAngles, CBaseEntity *pOwner );
	static CZombieMissile *CreateSpray( Vector vecOrigin, Vector vecVelocity );
};

class CShalMissile : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	void EXPORT ShalTouch( CBaseEntity *pOther );
	void EXPORT ShalHome( void );
public:
	static CShalMissile *CreateMissile( Vector vecOrigin, Vector vecVelocity );
};

#endif // WEAPONS_H