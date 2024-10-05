/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#ifndef ITEMS_H
#define ITEMS_H

class CItem : public CBaseEntity
{
public:
	void EXPORT 	PlaceItem( void );
	void EXPORT 	AnimateSprite(void);
	void		StartItem( void );
	CBaseEntity*	Respawn( void );
	void		SetObjectCollisionBox( void );
	void	EXPORT ItemTouch( CBaseEntity *pOther );
	void	EXPORT Materialize( void );
	virtual BOOL MyTouch( CBasePlayer *pPlayer ) { return FALSE; };
};

class CItemAmmo : public CItem
{
public:
	void Precache(void);
	void Spawn(void);
	BOOL EXPORT MyTouch(CBasePlayer* pPlayer);

	static CItemAmmo* DropAmmo(CBaseEntity* pVictim, int ammo, BOOL big);
};

class CItemWeapon : public CItem
{
public:
	void Precache(void);
	void Spawn(void);
	BOOL MyTouch(CBasePlayer* pPlayer);
	static CItemWeapon* DropWeapon(CBaseEntity* pVictim, int weapon);
};

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo. 
//=========================================================
class CWeaponBox : public CBaseEntity
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	static CWeaponBox *DropBackpack( CBaseEntity *pVictim, int weapon );
	void SetObjectCollisionBox( void );
};

#endif // ITEMS_H
