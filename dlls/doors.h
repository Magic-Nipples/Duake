
// doors
#ifndef DOORS_H
#define DOORS_H


#define SF_DOOR_START_OPEN 		1

#define SF_DOOR_DONT_LINK		4
#define SF_DOOR_GOLD_KEY		8
#define SF_DOOR_SILVER_KEY		16
#define SF_DOOR_RED_KEY			64
#define SF_DOOR_TOGGLE			32
#define SF_DOOR_USE_ONLY		256
#define SF_DOOR_SHADOW_ON		128

// secret doors
#define SF_SECRET_OPEN_ONCE		1	// stays open
#define SF_SECRET_1ST_LEFT		2	// 1st move is left of arrow
#define SF_SECRET_1ST_DOWN		4	// 1st move is down from arrow
#define SF_SECRET_NO_SHOOT		8	// only opened by trigger
#define SF_SECRET_YES_SHOOT		16	// shootable even if targeted

class CBaseDoor : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	virtual void KeyValue( KeyValueData *pkvd );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void Blocked( CBaseEntity *pOther );

	int BloodColor( void ) { return pev->takedamage ? BLOOD_COLOR_RED : DONT_BLEED; }
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	virtual void Killed( entvars_t *pevAttacker, int iGib );

	// used to selectivly override defaults
	void EXPORT DoorTouch( CBaseEntity *pOther );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	// local functions
	void DoorActivate( void );
	void EXPORT DoorGoUp( void );
	void EXPORT DoorGoDown( void );
	void EXPORT DoorHitTop( void );
	void EXPORT DoorHitBottom( void );
	void EXPORT LinkDoors( void );

	int lightstyle;
	BOOL lightstyletoggle;

	CBaseDoor	*m_pOwner;
	CBaseDoor	*m_pNextDoor;	// g-cont. i'm living next door to Alice :-)
};

#endif //DOORS_H