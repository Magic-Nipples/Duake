/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "shake.h"

extern int gmsgItemPickup;
extern DLL_GLOBAL int		g_iWorldType;
extern DLL_GLOBAL BOOL		g_fXashEngine;

void CItem::StartItem( void )
{
	SetThink( &CItem::PlaceItem );
	pev->nextthink = gpGlobals->time + 0.2;	// items start after other solids
}

void CItem::PlaceItem( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	pev->origin.z += 6;	// quake code

	SetTouch( &CItem::ItemTouch );

	if (UTIL_DropToFloor(this) == 0)
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
		return;
	}

	// g-cont: e3m2 has key on a moving platform. link them together to prevent loosing key
	if( g_fXashEngine && !FNullEnt( pev->groundentity ) && VARS( pev->groundentity )->movetype == MOVETYPE_PUSH )
	{ 
		edict_t	*gnd = pev->groundentity;

		ALERT( at_aiconsole, "%s linked with %s (%s)\n", STRING( pev->classname ),
		STRING( VARS( gnd )->classname ), STRING( VARS( gnd )->targetname )); 
		pev->movetype = MOVETYPE_COMPOUND;	// set movewith type
		pev->aiment = pev->groundentity;	// set parent
	}

	// sprite stuff here
	if (pev->flags & SF_SPRITE_ANIMATED)
	{
		SetThink(&CItem::AnimateSprite);
		pev->nextthink = gpGlobals->time + 0.01f;
	}
}

void CItem::AnimateSprite(void)
{
	int m_maxFrame;

	if (FClassnameIs(pev, "item_key1") || FClassnameIs(pev, "item_key2") || FClassnameIs(pev, "item_key3"))
	{
		if (g_iWorldType == WORLDTYPE_PRESENT)
		{
			if (pev->frame == 0)
				pev->frame = 1;
			else if (pev->frame == 1)
				pev->frame = 0;
		}
		else
		{
			if (pev->frame == 2)
				pev->frame = 3;
			else if (pev->frame == 3)
				pev->frame = 2;
		}
		pev->nextthink = gpGlobals->time + 0.2;
	}

	if (FClassnameIs(pev, "item_health_bonus") || FClassnameIs(pev, "item_armor_bonus"))
	{
		m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;

		if (pev->iuser1 == 0)
		{
			if (pev->frame >= m_maxFrame - 1)
				pev->iuser1 = 1;

			pev->frame++;
		}
		else if (pev->iuser1 <= 1)
		{
			if (pev->frame <= 1)
				pev->iuser1 = 0;

			pev->frame--;
		}
		pev->nextthink = gpGlobals->time + 0.2;
	}

	if (FClassnameIs(pev, "item_health"))
	{
		m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;

		pev->frame++;

		if (pev->frame > m_maxFrame)
			pev->frame = 0;

		pev->nextthink = gpGlobals->time + 0.2;
	}
	if (FClassnameIs(pev, "item_armor1"))
	{
		if (pev->frame == 0)
			pev->frame = 1;
		else if (pev->frame == 1)
			pev->frame = 0;

		pev->nextthink = gpGlobals->time + 0.2;
	}
	if (FClassnameIs(pev, "item_armor2"))
	{
		if (pev->frame == 2)
			pev->frame = 3;
		else if (pev->frame == 3)
			pev->frame = 2;

		pev->nextthink = gpGlobals->time + 0.2;
	}
	if (FClassnameIs(pev, "item_armorInv"))
	{
		if (pev->frame == 4)
			pev->frame = 5;
		else if (pev->frame == 5)
			pev->frame = 4;

		pev->nextthink = gpGlobals->time + 0.2;
	}
}

void CItem::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + pev->mins;
	pev->absmax = pev->origin + pev->maxs;

	// to make items easier to pick up and allow them to be grabbed off
	// of shelves, the abs sizes are expanded
	pev->absmin.x -= 15;
	pev->absmin.y -= 15;
	pev->absmax.x += 15;
	pev->absmax.y += 15;
}

void CItem::ItemTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if ( !g_pGameRules->CanHaveItem( pPlayer, this ))
		return; // no? Ignore the touch.

	if (MyTouch( pPlayer ))
	{
		// health touch sound
		EMIT_SOUND( pPlayer->edict(), CHAN_AUTO, STRING( pev->noise ), 1, ATTN_NORM );

		// send bonus flash (same as command "bf\n")
		BONUS_FLASH( pPlayer->edict() );

		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );
		
		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
			Respawn(); 
		else
			UTIL_Remove( this );
	}
}

CBaseEntity* CItem::Respawn( void )
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, g_pGameRules->VecItemRespawnSpot( this ) );// blip to whereever you should respawn.

	SetThink ( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this ); 
	return this;
}

void CItem::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "items/itembk2.wav", 1, ATTN_NORM );
		pev->effects &= ~EF_NODRAW;
	}

	SetTouch( &CItem::ItemTouch );
}


#define SF_HEALTH_ROTTEN 	1
#define SF_HEALTH_MEGA	2

class CItemHealth : public CItem
{
	void Spawn( void )
	{ 
		Precache( );

		if (FClassnameIs(pev, "item_health_bonus"))
		{
			SET_MODEL(ENT(pev), "sprites/items/bonus_health.spr");//SET_MODEL(ENT(pev), "progs/b_bh100.bsp" );
			pev->noise = MAKE_STRING("items/pickup_item.wav");
			pev->health = 1;
			pev->effects |= EF_FULLBRIGHT;
			pev->flags |= SF_SPRITE_ANIMATED;
		}
		else
		{
			if (pev->spawnflags & SF_HEALTH_ROTTEN)
			{
				SET_MODEL(ENT(pev), "sprites/items/health-misc.spr");//SET_MODEL(ENT(pev), "progs/b_bh10.bsp" );
				pev->noise = MAKE_STRING("items/pickup_item.wav");
				pev->health = 10;
				pev->frame = 2;
			}
			else if (pev->spawnflags & SF_HEALTH_MEGA)
			{
				SET_MODEL(ENT(pev), "sprites/items/soulsphere.spr");//SET_MODEL(ENT(pev), "progs/b_bh100.bsp" );
				pev->noise = MAKE_STRING("items/pickup_powerup.wav");
				pev->health = 100;
				pev->effects |= EF_FULLBRIGHT;
				pev->flags |= SF_SPRITE_ANIMATED;

				gpWorld->total_items++;
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/items/health-misc.spr");//SET_MODEL(ENT(pev), "progs/b_bh25.bsp" );
				pev->noise = MAKE_STRING("items/pickup_item.wav");
				pev->health = 25;
				pev->frame = 3;
			}
		}

		UTIL_SetSize(pev, Vector(-1, -1, 0), Vector(1, 1, 48));//UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 32, 32, 56 ));
		StartItem ();
	}

	void Precache( void )
	{
		if (pev->spawnflags & SF_HEALTH_MEGA)
		{
			PRECACHE_MODEL("sprites/items/soulsphere.spr");//PRECACHE_MODEL("progs/b_bh100.bsp");
			PRECACHE_SOUND("items/pickup_powerup.wav");
			pev->noise = MAKE_STRING("items/pickup_powerup.wav");
		}

		if (FClassnameIs(pev, "item_health_bonus"))
			PRECACHE_MODEL("sprites/items/bonus_health.spr");
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if (FClassnameIs(pev, "item_health_bonus"))
		{
			if (pPlayer->pev->health < 200)
				pPlayer->pev->health++;
		}
		else if (pev->spawnflags & SF_HEALTH_MEGA)
		{
			if ( pPlayer->pev->health >= 200 )
				return FALSE;

			// allow to override max_health here!
			if (!pPlayer->TakeHealth( pev->health, DMG_GENERIC, TRUE ))
				return FALSE;

			// has megahealth!
			pPlayer->m_iItems |= IT_SUPERHEALTH;
			//pPlayer->m_flCheckHealthTime = gpGlobals->time + 5;

			if (pPlayer->pev->health > 200)
				pPlayer->pev->health = 200;

			gpWorld->found_items++;
			MESSAGE_BEGIN(MSG_ALL, gmsgFoundItem);
			MESSAGE_END();
		}
		else
		{
			if (!pPlayer->TakeHealth( pev->health, DMG_GENERIC ))
				return FALSE;
		}

		//CLIENT_PRINTF( pPlayer->edict(), print_console, UTIL_VarArgs( "You receive %.f health\n", pev->health ));
		if (FClassnameIs(pev, "item_health_bonus"))
			pPlayer->m_sMessage = MAKE_STRING("Picked up a health bonus.");
		else if (pev->spawnflags & SF_HEALTH_MEGA)
			pPlayer->m_sMessage = MAKE_STRING("Picked up the Soul Sphere!");
		else if (pev->spawnflags & SF_HEALTH_ROTTEN)
			pPlayer->m_sMessage = MAKE_STRING("Picked up a stimpack.");
		else
			pPlayer->m_sMessage = MAKE_STRING("Picked up a medkit.");

		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_health, CItemHealth);
LINK_ENTITY_TO_CLASS(item_health_bonus, CItemHealth);


class CItemBackpack : public CItem
{
	void Spawn(void)
	{
		SET_MODEL(ENT(pev), "sprites/items/health-misc.spr");
		pev->noise = MAKE_STRING("items/pickup_item.wav");
		pev->frame = 0;
		gpWorld->total_items++;

		UTIL_SetSize(pev, Vector(-1, -1, 0), Vector(1, 1, 48));
		StartItem();
	}

	BOOL MyTouch(CBasePlayer* pPlayer)
	{
		int multiplyer = 1;

		pPlayer->m_iItems |= IT_BACKPACK;

		if (g_iSkillLevel == SKILL_EASY || g_iSkillLevel == SKILL_NIGHTMARE)
			multiplyer = 2;

		if (pPlayer->ammo_shells < IT_MAX_SHELLS * 2) //100
			pPlayer->ammo_shells += (4 * multiplyer);

		if (pPlayer->ammo_nails < IT_MAX_NAILS * 2) //200
			pPlayer->ammo_nails += (10 * multiplyer);

		if (pPlayer->ammo_rockets < IT_MAX_ROCKETS * 2) //100
			pPlayer->ammo_rockets += (1 * multiplyer);

		if (pPlayer->ammo_cells < IT_MAX_CELLS * 2) //200
			pPlayer->ammo_cells += (20 * multiplyer);

		//cap backpack contents
		if (pPlayer->ammo_shells > IT_MAX_SHELLS * 2)
			pPlayer->ammo_shells = (IT_MAX_SHELLS * 2);

		if (pPlayer->ammo_nails > IT_MAX_NAILS * 2)
			pPlayer->ammo_nails = (IT_MAX_NAILS * 2);

		if (pPlayer->ammo_rockets > IT_MAX_ROCKETS * 2)
			pPlayer->ammo_rockets = (IT_MAX_ROCKETS * 2);

		if (pPlayer->ammo_cells > IT_MAX_CELLS * 2)
			pPlayer->ammo_cells = (IT_MAX_CELLS * 2);

		pPlayer->m_sMessage = MAKE_STRING("Picked up a backpack full of ammo!");

		gpWorld->found_items++;
		MESSAGE_BEGIN(MSG_ALL, gmsgFoundItem);
		MESSAGE_END();

		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_backpack, CItemBackpack);

/*QUAKED item_sigil (0 .5 .8) (-16 -16 -24) (16 16 32) E1 E2 E3 E4
End of level sigil, pick up to end episode and return to jrstart.
*/
class CItemSigil : public CItem
{
	void Spawn( void )
	{ 
		Precache( );

		if (pev->spawnflags & 1)
			SET_MODEL(ENT(pev), "progs/end1.mdl" );
		else if (pev->spawnflags & 2)
			SET_MODEL(ENT(pev), "progs/end2.mdl" );
		else if (pev->spawnflags & 4)
			SET_MODEL(ENT(pev), "progs/end3.mdl" );
		else if (pev->spawnflags & 8)
			SET_MODEL(ENT(pev), "progs/end4.mdl" );

		SetBits(pev->effects, EF_FULLBRIGHT);
		pev->noise = MAKE_STRING( "misc/runekey.wav" );
		gpWorld->total_items++;
		UTIL_SetSize(pev, Vector(-1, -1, -24), Vector(1, 1, 32));//UTIL_SetSize( pev, Vector( -16, -16, -24 ), Vector( 16, 16, 32 ));
		StartItem ();
	}

	void Precache( void )
	{
		PRECACHE_SOUND( "misc/runekey.wav" );

		if (pev->spawnflags & 1)
			PRECACHE_MODEL( "progs/end1.mdl" );
		else if (pev->spawnflags & 2)
			PRECACHE_MODEL( "progs/end2.mdl" );
		else if (pev->spawnflags & 4)
			PRECACHE_MODEL( "progs/end3.mdl" );
		else if (pev->spawnflags & 8)
			PRECACHE_MODEL( "progs/end4.mdl" );
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		CenterPrint( pPlayer->pev, "You got the rune!" );
		gpWorld->serverflags |= (pev->spawnflags & 15);
		pPlayer->m_iItems |= (gpWorld->serverflags << 28); // store runes as high bits

		g_StoreRune |= (pev->spawnflags & 15);

		gpWorld->found_items++;
		MESSAGE_BEGIN(MSG_ALL, gmsgFoundItem);
		MESSAGE_END();

		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_sigil, CItemSigil );

class CItemKey : public CItem
{
	void Spawn( void )
	{ 
		Precache( );

		pev->effects |= EF_FULLBRIGHT;

		if (FClassnameIs(pev, "item_key1"))
			pev->team = IT_KEY1;
		else if (FClassnameIs(pev, "item_key2"))
			pev->team = IT_KEY2;
		else
			pev->team = IT_KEY3;

		pev->noise = MAKE_STRING("items/pickup_item.wav");

		switch( g_iWorldType )
		{
		case WORLDTYPE_MEDIEVAL:
		case WORLDTYPE_RUNIC:		
			if (FClassnameIs(pev, "item_key1"))
			{
				SET_MODEL(ENT(pev), "sprites/items/keyblue.spr");
				pev->netname = MAKE_STRING("blue skull key.");
			}
			else if (FClassnameIs(pev, "item_key2"))
			{
				SET_MODEL(ENT(pev), "sprites/items/keyyellow.spr");
				pev->netname = MAKE_STRING("yellow skull key.");
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/items/keyred.spr");
				pev->netname = MAKE_STRING("red skull key.");
			}
			pev->frame = 2;
			break;

		case WORLDTYPE_PRESENT:
			if (FClassnameIs(pev, "item_key1"))
			{
				SET_MODEL(ENT(pev), "sprites/items/keyblue.spr");
				pev->netname = MAKE_STRING("blue keycard.");
			}
			else if (FClassnameIs(pev, "item_key2"))
			{
				SET_MODEL(ENT(pev), "sprites/items/keyyellow.spr");
				pev->netname = MAKE_STRING("yellow keycard.");
			}
			else
			{
				SET_MODEL(ENT(pev), "sprites/items/keyred.spr");
				pev->netname = MAKE_STRING("red keycard.");
			}
			pev->frame = 0;
			break;
		}
		//UTIL_SetSize(pev, Vector(-16, -16, -24), Vector(16, 16, 32));
		UTIL_SetSize(pev, Vector(-1, -1, 0), Vector(1, 1, 48)); //fixes in wall issue?
		StartItem ();

		pev->flags |= SF_SPRITE_ANIMATED;
	}

	void Precache( void )
	{
		if (FClassnameIs(pev, "item_key1"))
			PRECACHE_MODEL("sprites/items/keyblue.spr");
		else if (FClassnameIs(pev, "item_key2"))
			PRECACHE_MODEL("sprites/items/keyyellow.spr");
		else
			PRECACHE_MODEL("sprites/items/keyred.spr");

		PRECACHE_MODEL( (char*)STRING( pev->model ));

		switch( g_iWorldType )
		{
		case WORLDTYPE_MEDIEVAL:
			PRECACHE_SOUND( "misc/medkey.wav" );
			pev->noise = MAKE_STRING( "misc/medkey.wav" );
			break;
		case WORLDTYPE_RUNIC:
			PRECACHE_SOUND( "misc/runekey.wav" );
			pev->noise = MAKE_STRING( "misc/runekey.wav" );
			break;
		case WORLDTYPE_PRESENT:
			PRECACHE_SOUND( "misc/basekey.wav" );
			pev->noise = MAKE_STRING( "misc/basekey.wav" );
			break;
		}
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if (pPlayer->m_iItems & pev->team)
			return FALSE;

		//CLIENT_PRINTF( pPlayer->edict(), print_console, UTIL_VarArgs( "You got the %s\n", STRING( pev->netname )));
		pPlayer->m_sMessage = MAKE_STRING(UTIL_VarArgs("Picked up the %s\n", STRING(pev->netname)));

		pPlayer->m_iItems |= pev->team;

		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_key1, CItemKey);
LINK_ENTITY_TO_CLASS(item_key2, CItemKey);
LINK_ENTITY_TO_CLASS(item_key3, CItemKey);


class CItemArmor : public CItem
{
	void Precache( void )
	{
		if (FClassnameIs(pev, "item_armor_bonus"))
			PRECACHE_MODEL("sprites/items/bonus_armor.spr");
		else
			PRECACHE_MODEL("sprites/items/armor.spr");
	}

	void Spawn( void )
	{ 
		Precache( );

		if (FClassnameIs(pev, "item_armor_bonus"))
		{
			SET_MODEL(ENT(pev), "sprites/items/bonus_armor.spr");
		}
		else
		{
			SET_MODEL(ENT(pev), "sprites/items/armor.spr");

			if (FClassnameIs(pev, "item_armor2"))
			{
				pev->frame = 2;
				gpWorld->total_items++;
			}
			else if (FClassnameIs(pev, "item_armorInv"))
			{
				pev->frame = 4;
				gpWorld->total_items++;
			}
		}

		pev->noise = MAKE_STRING("items/pickup_item.wav");
		UTIL_SetSize(pev, Vector(-1, -1, 0), Vector(1, 1, 48));//UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 56 ));

		pev->flags |= SF_SPRITE_ANIMATED;
		pev->effects |= EF_FULLBRIGHT;

		StartItem ();
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		float	type, value;
		int	bit;

		if (FClassnameIs( pev, "item_armor1" ))
		{
			type = 0.3;
			value = 100;
			bit = IT_ARMOR1;

			if(pPlayer->pev->armorvalue >= value)
				return FALSE;
		}
		if (FClassnameIs( pev, "item_armor2" ))
		{
			type = 0.6;
			value = 200;
			bit = IT_ARMOR2;

			if (pPlayer->pev->armorvalue > value)
				return FALSE;
		}
		if (FClassnameIs( pev, "item_armorInv" ))
		{
			type = 0.8;
			value = 250;
			bit = IT_ARMOR3;
		}

		if (FClassnameIs(pev, "item_armor_bonus"))
		{
			pPlayer->m_sMessage = MAKE_STRING("Picked up a armor bonus.");

			if (pPlayer->pev->armorvalue <= 0)
				pPlayer->pev->armortype = 0.3;

			if (pPlayer->pev->armorvalue < 250)
				pPlayer->pev->armorvalue++;

			return TRUE;
		}

		//CLIENT_PRINTF( pPlayer->edict(), print_console, "You got armor\n" );
		if (FClassnameIs(pev, "item_armor1"))
			pPlayer->m_sMessage = MAKE_STRING("Picked up armor.");
		else if (FClassnameIs(pev, "item_armor2"))
			pPlayer->m_sMessage = MAKE_STRING("Picked up the Mega Armor!");
		else if (FClassnameIs(pev, "item_armorInv"))
			pPlayer->m_sMessage = MAKE_STRING("Picked up the Ultra Armor!");

		pPlayer->pev->armortype = type;
		pPlayer->pev->armorvalue = value;

		pPlayer->m_iItems &= ~(IT_ARMOR1|IT_ARMOR2|IT_ARMOR3);
		pPlayer->m_iItems |= bit;

		if (FClassnameIs(pev, "item_armor2") || FClassnameIs(pev, "item_armorInv"))
		{
			gpWorld->found_items++;
			MESSAGE_BEGIN(MSG_ALL, gmsgFoundItem);
			MESSAGE_END();
		}

		return TRUE;		
	}
};

LINK_ENTITY_TO_CLASS(item_armor1, CItemArmor);
LINK_ENTITY_TO_CLASS(item_armor2, CItemArmor);
LINK_ENTITY_TO_CLASS(item_armorInv, CItemArmor);
LINK_ENTITY_TO_CLASS(item_armor_bonus, CItemArmor);


class CItemArtifact : public CItem
{
	void Precache( void )
	{
		if (FClassnameIs( pev, "item_artifact_invulnerability" ))
		{
			PRECACHE_MODEL ("progs/invulner.mdl");
			PRECACHE_SOUND( "items/protect.wav" );
			PRECACHE_SOUND( "items/protect2.wav" );
			PRECACHE_SOUND( "items/protect3.wav" );
		}
		else if (FClassnameIs( pev, "item_artifact_envirosuit" ))
		{
			PRECACHE_MODEL("sprites/items/wsuit.spr");//PRECACHE_MODEL ("progs/suit.mdl");
			PRECACHE_SOUND( "items/suit.wav" );
			PRECACHE_SOUND( "items/suit2.wav" );
		}
		else if (FClassnameIs( pev, "item_artifact_invisibility" ))
		{
			PRECACHE_MODEL ("progs/invisibl.mdl");
			PRECACHE_SOUND( "items/inv1.wav" );
			PRECACHE_SOUND( "items/inv2.wav" );
			PRECACHE_SOUND( "items/inv3.wav" );
		}
		else if (FClassnameIs( pev, "item_artifact_super_damage" ))
		{
			PRECACHE_MODEL ("progs/quaddama.mdl");
			PRECACHE_SOUND( "items/damage.wav" );
			PRECACHE_SOUND( "items/damage2.wav" );
			PRECACHE_SOUND( "items/damage3.wav" );
		}
	}

	void Spawn( void )
	{ 
		Precache( );

		if (FClassnameIs( pev, "item_artifact_invulnerability" ))
		{
			SET_MODEL(ENT(pev), "progs/invulner.mdl");
			pev->noise = MAKE_STRING( "items/protect.wav" );
			pev->netname = MAKE_STRING("Pentagram of Protection!");
			pev->team = IT_INVULNERABILITY;
			pev->effects |= EF_FULLBRIGHT;
		}
		else if (FClassnameIs( pev, "item_artifact_envirosuit" ))
		{
			SET_MODEL(ENT(pev), "sprites/items/wsuit.spr");
			pev->noise = MAKE_STRING( "items/suit.wav" );
			pev->netname = MAKE_STRING("Biosuit!");
			pev->team = IT_SUIT;
		}
		else if (FClassnameIs( pev, "item_artifact_invisibility" ))
		{
			SET_MODEL(ENT(pev), "progs/invisibl.mdl");
			pev->noise = MAKE_STRING( "items/inv1.wav" );
			pev->netname = MAKE_STRING("Ring of Shadows!");
			pev->team = IT_INVISIBILITY;
			pev->effects |= EF_FULLBRIGHT;
		}
		else if (FClassnameIs( pev, "item_artifact_super_damage" ))
		{
			SET_MODEL(ENT(pev), "progs/quaddama.mdl");
			pev->noise = MAKE_STRING( "items/damage.wav" );
			pev->netname = MAKE_STRING("Quad Damage!");
			pev->team = IT_QUAD;
			pev->effects |= EF_FULLBRIGHT;
		}

		gpWorld->total_items++;

		if (FClassnameIs(pev, "item_artifact_envirosuit"))
			UTIL_SetSize(pev, Vector(-1, -1, 0), Vector(1, 1, 48));
		else
			UTIL_SetSize(pev, Vector(-1, -1, -24), Vector(1, 1, 24));//UTIL_SetSize( pev, Vector( -16, -16, -24 ), Vector( 16, 16, 32 ));

		StartItem ();
	}

	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		// do the apropriate action
		if (FClassnameIs( pev, "item_artifact_envirosuit" ))
		{
			pPlayer->m_flRadSuitTime = 1;
			pPlayer->m_flRadSuitFinished = gpGlobals->time + 30;
		}
	
		if (FClassnameIs( pev, "item_artifact_invulnerability" ))
		{
			pPlayer->m_flInvincibleTime = 1;
			pPlayer->m_flInvincibleFinished = gpGlobals->time + 30;
		}

		if (FClassnameIs( pev, "item_artifact_invisibility" ))
		{
			pPlayer->m_flInvisibleTime = 1;
			pPlayer->m_flInvisibleFinished = gpGlobals->time + 30;
		}

		if (FClassnameIs( pev, "item_artifact_super_damage" ))
		{
			pPlayer->m_flSuperDamageTime = 1;
			pPlayer->m_flSuperDamageFinished = gpGlobals->time + 30;
		}

		//CLIENT_PRINTF( pPlayer->edict(), print_console, UTIL_VarArgs( "You got the %s\n", STRING( pev->netname )));
		pPlayer->m_sMessage = MAKE_STRING(UTIL_VarArgs("Picked up the %s\n", STRING(pev->netname)));

		pPlayer->m_iItems |= pev->team;

		gpWorld->found_items++;
		MESSAGE_BEGIN(MSG_ALL, gmsgFoundItem);
		MESSAGE_END();

		return TRUE;		
	}
};

LINK_ENTITY_TO_CLASS(item_artifact_invulnerability, CItemArtifact);
LINK_ENTITY_TO_CLASS(item_artifact_envirosuit, CItemArtifact);
LINK_ENTITY_TO_CLASS(item_artifact_invisibility, CItemArtifact);
LINK_ENTITY_TO_CLASS(item_artifact_super_damage, CItemArtifact);


#define SF_WEAPON_BIG2 	1

void CItemAmmo::Precache(void)
{
	if (FClassnameIs(pev, "item_shells") || FClassnameIs(pev, "item_boxofshells"))
		PRECACHE_MODEL("sprites/items/wshells.spr");
	else if (FClassnameIs(pev, "item_spikes") || FClassnameIs(pev, "item_clip") || FClassnameIs(pev, "item_boxofbullets"))
		PRECACHE_MODEL("sprites/items/wclip.spr");
	else if (FClassnameIs(pev, "item_rockets"))
		PRECACHE_MODEL("sprites/items/wrocket.spr");
	else if (FClassnameIs(pev, "item_cells"))
		PRECACHE_MODEL("sprites/items/wcell.spr");
}

void CItemAmmo::Spawn(void)
{
	// support for item_weapon
	if (FClassnameIs(pev, "item_weapon"))
	{
		if (pev->spawnflags & 1)
			pev->classname = MAKE_STRING("item_shells");
		if (pev->spawnflags & 2)
			pev->classname = MAKE_STRING("item_rockets");
		if (pev->spawnflags & 4)
			pev->classname = MAKE_STRING("item_spikes");
		if (pev->spawnflags & 8)
			pev->spawnflags = SF_WEAPON_BIG2;
		else pev->spawnflags = 0;
	}

	Precache();

	if (FClassnameIs(pev, "item_shells") || FClassnameIs(pev, "item_boxofshells"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wshells.spr");
		if (pev->spawnflags & SF_WEAPON_BIG2 || FClassnameIs(pev, "item_boxofshells"))
		{
			pev->frame = 1;
			pev->armorvalue = 20;
			pev->netname = MAKE_STRING("a box of shells.");
		}
		else
		{
			pev->frame = 0;
			if (g_iSkillLevel == SKILL_EASY || g_iSkillLevel == SKILL_NIGHTMARE)
				pev->armorvalue = 8;
			else
				pev->armorvalue = 4;

			pev->netname = MAKE_STRING("some shells.");
		}
		pev->team = (IT_SHOTGUN | IT_SUPER_SHOTGUN);
	}
	else if (FClassnameIs(pev, "item_spikes") || FClassnameIs(pev, "item_clip") || FClassnameIs(pev, "item_boxofbullets"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wclip.spr");
		if (pev->spawnflags & SF_WEAPON_BIG2 || FClassnameIs(pev, "item_boxofbullets"))
		{
			pev->frame = 1;
			pev->armorvalue = 50;
			pev->netname = MAKE_STRING("a box of bullets.");
		}
		else
		{
			pev->frame = 0;
			if (g_iSkillLevel == SKILL_EASY || g_iSkillLevel == SKILL_NIGHTMARE)
				pev->armorvalue = 20;
			else
				pev->armorvalue = 10;

			pev->netname = MAKE_STRING("a clip.");
		}
		pev->team = (IT_NAILGUN | IT_SUPER_NAILGUN);
	}
	else if (FClassnameIs(pev, "item_rockets"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wrocket.spr");
		if (pev->spawnflags & SF_WEAPON_BIG2)
		{
			pev->frame = 1;
			if (g_iSkillLevel == SKILL_EASY || g_iSkillLevel == SKILL_NIGHTMARE)
				pev->armorvalue = 10;
			else
				pev->armorvalue = 5;

			pev->netname = MAKE_STRING("a box of rockets.");
		}
		else
		{
			pev->frame = 0;
			if (g_iSkillLevel == SKILL_EASY || g_iSkillLevel == SKILL_NIGHTMARE)
			{
				pev->armorvalue = 2;
				pev->netname = MAKE_STRING("2 rockets.");
			}
			else
			{
				pev->armorvalue = 1;
				pev->netname = MAKE_STRING("1 rocket");
			}
		}
		pev->team = (IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER);
	}
	else if (FClassnameIs(pev, "item_cells"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wcell.spr");
		if (pev->spawnflags & SF_WEAPON_BIG2)
		{
			pev->frame = 1;
			if (g_iSkillLevel == SKILL_EASY || g_iSkillLevel == SKILL_NIGHTMARE)
				pev->armorvalue = 200;
			else
				pev->armorvalue = 100;

			pev->netname = MAKE_STRING("a energy cell pack.");
		}
		else
		{
			pev->frame = 0;
			if (g_iSkillLevel == SKILL_EASY || g_iSkillLevel == SKILL_NIGHTMARE)
				pev->armorvalue = 40;
			else
				pev->armorvalue = 20;

			pev->netname = MAKE_STRING("energy cells.");
		}
		pev->team = IT_LIGHTNING;
	}
	pev->noise = MAKE_STRING("items/pickup_item.wav");

	UTIL_SetSize(pev, g_vecZero, g_vecZero); //UTIL_SetSize(pev, Vector(-1, -1, 0), Vector(1, 1, 48));

	if(pev->frags == 1)
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->solid = SOLID_TRIGGER;
		SetTouch(&CItem::ItemTouch);
	}
	else
	{
		StartItem();
	}
}

BOOL CItemAmmo::MyTouch(CBasePlayer* pPlayer)
{
	int best = pPlayer->W_BestWeapon();

	int backpack = 1;
	if (pPlayer->m_iItems & IT_BACKPACK)
		backpack = 2;

	// do the apropriate action
	if (pev->team & (IT_SHOTGUN | IT_SUPER_SHOTGUN))
	{
		if (pPlayer->ammo_shells >= IT_MAX_SHELLS * backpack) //100
			return FALSE;
		pPlayer->ammo_shells += pev->armorvalue;

		if (pPlayer->ammo_shells >= IT_MAX_SHELLS * backpack)
			pPlayer->ammo_shells = IT_MAX_SHELLS * backpack;
	}
	if (pev->team & (IT_NAILGUN | IT_SUPER_NAILGUN | IT_PISTOL))
	{
		if (pPlayer->ammo_nails >= IT_MAX_NAILS * backpack) //200
			return FALSE;
		pPlayer->ammo_nails += pev->armorvalue;

		if (pPlayer->ammo_nails >= IT_MAX_NAILS * backpack)
			pPlayer->ammo_nails = IT_MAX_NAILS * backpack;
	}
	if (pev->team & (IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER))
	{
		if (pPlayer->ammo_rockets >= IT_MAX_ROCKETS * backpack) //100
			return FALSE;
		pPlayer->ammo_rockets += pev->armorvalue;

		if (pPlayer->ammo_rockets >= IT_MAX_ROCKETS * backpack)
			pPlayer->ammo_rockets = IT_MAX_ROCKETS * backpack;
	}
	if (pev->team & IT_LIGHTNING)
	{
		if (pPlayer->ammo_cells >= IT_MAX_CELLS * backpack) //200
			return FALSE;
		pPlayer->ammo_cells += pev->armorvalue;

		if (pPlayer->ammo_cells >= IT_MAX_CELLS * backpack)
			pPlayer->ammo_cells = IT_MAX_CELLS * backpack;
	}

	//CLIENT_PRINTF(pPlayer->edict(), print_console, UTIL_VarArgs("You picked up %s\n", STRING(pev->netname)));
	pPlayer->m_sMessage = MAKE_STRING(UTIL_VarArgs("Picked up %s\n", STRING(pev->netname)));

	return TRUE;
}

CItemAmmo* CItemAmmo::DropAmmo(CBaseEntity* pVictim, int ammo, BOOL big)
{
	if (!pVictim)
		return NULL;

	CItemAmmo* pAmmoDrop = GetClassPtr((CItemAmmo*)NULL);

	UTIL_SetOrigin(pAmmoDrop->pev, pVictim->pev->origin + Vector(0, 0, 24));

	if (ammo & IT_SHELLS)
		pAmmoDrop->pev->classname = MAKE_STRING("item_shells");
	else if (ammo & IT_ROCKETS)
		pAmmoDrop->pev->classname = MAKE_STRING("item_rockets");
	else if (ammo & IT_NAILS)
		pAmmoDrop->pev->classname = MAKE_STRING("item_spikes");
	else if (ammo & IT_CELLS)
		pAmmoDrop->pev->classname = MAKE_STRING("item_cells");
	else
		return NULL;

	if (big)
		pAmmoDrop->pev->spawnflags = SF_WEAPON_BIG2;

	pAmmoDrop->pev->velocity.x = RANDOM_FLOAT(-100, 100);
	pAmmoDrop->pev->velocity.y = RANDOM_FLOAT(-100, 100);
	pAmmoDrop->pev->velocity.z = 100;

	pAmmoDrop->pev->frags = 1;

	pAmmoDrop->Spawn();

	return pAmmoDrop;
}

LINK_ENTITY_TO_CLASS(item_shells, CItemAmmo);
LINK_ENTITY_TO_CLASS(item_boxofshells, CItemAmmo);
LINK_ENTITY_TO_CLASS(item_spikes, CItemAmmo);	//compatibility
LINK_ENTITY_TO_CLASS(item_clip, CItemAmmo);
LINK_ENTITY_TO_CLASS(item_boxofbullets, CItemAmmo);
LINK_ENTITY_TO_CLASS(item_rockets, CItemAmmo);
LINK_ENTITY_TO_CLASS(item_cells, CItemAmmo);
LINK_ENTITY_TO_CLASS(item_weapon, CItemAmmo); //used?



void CItemWeapon::Precache(void)
{
	if (FClassnameIs(pev, "weapon_shotgun"))
		PRECACHE_MODEL("sprites/items/wshotgun.spr");
	if (FClassnameIs(pev, "weapon_supershotgun"))
		PRECACHE_MODEL("sprites/items/wsupershot.spr");
	else if (FClassnameIs(pev, "weapon_nailgun") || FClassnameIs(pev, "weapon_rifle"))
		PRECACHE_MODEL("sprites/items/wrifle.spr");
	else if (FClassnameIs(pev, "weapon_supernailgun") || FClassnameIs(pev, "weapon_chaingun"))
		PRECACHE_MODEL("sprites/items/wchaingun.spr");
	else if (FClassnameIs(pev, "weapon_grenadelauncher") || FClassnameIs(pev, "weapon_plasmarifle"))
		PRECACHE_MODEL("sprites/items/wplasma.spr");
	else if (FClassnameIs(pev, "weapon_rocketlauncher"))
		PRECACHE_MODEL("sprites/items/wrlauncher.spr");
	else if (FClassnameIs(pev, "weapon_lightning") || FClassnameIs(pev, "weapon_bfg"))
		PRECACHE_MODEL("sprites/items/wbfg.spr");
	if (FClassnameIs(pev, "weapon_chainsaw"))
		PRECACHE_MODEL("sprites/items/wchainsaw.spr");
}

void CItemWeapon::Spawn(void)
{
	Precache();
	if (FClassnameIs(pev, "weapon_shotgun"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wshotgun.spr");
		pev->netname = MAKE_STRING("Shotgun!");
		pev->team = IT_SHOTGUN;
	}
	else if (FClassnameIs(pev, "weapon_supershotgun"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wsupershot.spr");
		pev->netname = MAKE_STRING("Super Shotgun!");
		pev->team = IT_SUPER_SHOTGUN;
	}
	else if (FClassnameIs(pev, "weapon_nailgun") || FClassnameIs(pev, "weapon_rifle"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wrifle.spr");
		pev->netname = MAKE_STRING("Rifle!");
		pev->team = IT_NAILGUN;
	}
	else if (FClassnameIs(pev, "weapon_supernailgun") || FClassnameIs(pev, "weapon_chaingun"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wchaingun.spr");
		pev->netname = MAKE_STRING("Chaingun!");
		pev->team = IT_SUPER_NAILGUN;
	}
	else if (FClassnameIs(pev, "weapon_grenadelauncher") || FClassnameIs(pev, "weapon_plasmarifle"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wplasma.spr");
		pev->netname = MAKE_STRING("Plasma Rifle!");
		pev->team = IT_GRENADE_LAUNCHER;
	}
	else if (FClassnameIs(pev, "weapon_rocketlauncher"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wrlauncher.spr");
		pev->netname = MAKE_STRING("Rocket Launcher!");
		pev->team = IT_ROCKET_LAUNCHER;
	}
	else if (FClassnameIs(pev, "weapon_lightning") || FClassnameIs(pev, "weapon_bfg"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wbfg.spr");
		pev->netname = MAKE_STRING("BFG!");
		pev->team = IT_LIGHTNING;
	}
	else if (FClassnameIs(pev, "weapon_chainsaw"))
	{
		SET_MODEL(ENT(pev), "sprites/items/wchainsaw.spr");
		pev->netname = MAKE_STRING("Chainsaw!");
		pev->team = IT_CHAINSAW;
	}
	pev->noise = MAKE_STRING("items/pickup_weapon.wav");

	UTIL_SetSize(pev, g_vecZero, g_vecZero); //UTIL_SetSize(pev, Vector(-1, -1, 0), Vector(1, 1, 48));
	
	if (pev->frags == 1)
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->solid = SOLID_TRIGGER;
		SetTouch(&CItem::ItemTouch);
	}
	else
	{
		StartItem();
	}
}

BOOL CItemWeapon::MyTouch(CBasePlayer* pPlayer)
{
	BOOL leave;

	if (gpGlobals->deathmatch == 2.0 || gpGlobals->coop)
		leave = TRUE;
	else leave = FALSE;

	int multiplyer = 1;
	if (g_iSkillLevel == SKILL_EASY || g_iSkillLevel == SKILL_NIGHTMARE)
		multiplyer = 2;

	int backpack = 1;
	if (pPlayer->m_iItems & IT_BACKPACK)
		backpack = 2;

	if (pev->team & (IT_SHOTGUN | IT_SUPER_SHOTGUN))
	{
		pPlayer->ammo_shells += (8 * multiplyer);

		if (pPlayer->ammo_shells >= IT_MAX_SHELLS * backpack)
			pPlayer->ammo_shells = IT_MAX_SHELLS * backpack;
	}
	if (pev->team & (IT_NAILGUN | IT_SUPER_NAILGUN | IT_PISTOL))
	{
		pPlayer->ammo_nails += (20 * multiplyer);

		if (pPlayer->ammo_nails >= IT_MAX_NAILS * backpack)
			pPlayer->ammo_nails = IT_MAX_NAILS * backpack;
	}
	if (pev->team & (IT_ROCKET_LAUNCHER))
	{
		pPlayer->ammo_rockets += (2 * multiplyer);

		if (pPlayer->ammo_rockets >= IT_MAX_ROCKETS * backpack)
			pPlayer->ammo_rockets = IT_MAX_ROCKETS * backpack;
	}
	if (pev->team & (IT_GRENADE_LAUNCHER | IT_LIGHTNING))
	{
		pPlayer->ammo_cells += (40 * multiplyer);

		if (pPlayer->ammo_cells >= IT_MAX_CELLS * backpack)
			pPlayer->ammo_cells = IT_MAX_CELLS * backpack;
	}

	//CLIENT_PRINTF( pPlayer->edict(), print_console, UTIL_VarArgs( "You got the %s\n", STRING( pev->netname )));
	pPlayer->m_sMessage = MAKE_STRING(UTIL_VarArgs("Picked up the %s\n", STRING(pev->netname)));

	pPlayer->CheckAmmo();

	if (!(pPlayer->m_iItems & pev->team)) //if we already have this gun dont switch to it.
		pPlayer->m_iCueWeapon = pev->team;

	pPlayer->m_iItems |= pev->team;

	return !leave;
}

CItemWeapon* CItemWeapon::DropWeapon(CBaseEntity* pVictim, int weapon)
{
	if (!pVictim)
		return NULL;

	CItemWeapon* pAmmoDrop = GetClassPtr((CItemWeapon*)NULL);

	UTIL_SetOrigin(pAmmoDrop->pev, pVictim->pev->origin + Vector(0, 0, 24));

	if (weapon & IT_SHOTGUN)
		pAmmoDrop->pev->classname = MAKE_STRING("weapon_shotgun");
	else if (weapon & IT_SUPER_SHOTGUN)
		pAmmoDrop->pev->classname = MAKE_STRING("weapon_supershotgun");
	else if (weapon & IT_NAILGUN)
		pAmmoDrop->pev->classname = MAKE_STRING("weapon_rifle");
	else if (weapon & IT_SUPER_NAILGUN)
		pAmmoDrop->pev->classname = MAKE_STRING("weapon_chaingun");
	else if (weapon & IT_ROCKET_LAUNCHER)
		pAmmoDrop->pev->classname = MAKE_STRING("weapon_rocketlauncher");
	else if (weapon & IT_GRENADE_LAUNCHER)
		pAmmoDrop->pev->classname = MAKE_STRING("weapon_plasmarifle");
	else if (weapon & IT_LIGHTNING)
		pAmmoDrop->pev->classname = MAKE_STRING("weapon_bfg");
	else
		return NULL;

	pAmmoDrop->pev->velocity.x = RANDOM_FLOAT(-100, 100);
	pAmmoDrop->pev->velocity.y = RANDOM_FLOAT(-100, 100);
	pAmmoDrop->pev->velocity.z = 100;

	pAmmoDrop->pev->frags = 1;

	pAmmoDrop->Spawn();

	return pAmmoDrop;
}

LINK_ENTITY_TO_CLASS(weapon_chainsaw, CItemWeapon);

LINK_ENTITY_TO_CLASS(weapon_shotgun, CItemWeapon);
LINK_ENTITY_TO_CLASS(weapon_supershotgun, CItemWeapon);

LINK_ENTITY_TO_CLASS(weapon_nailgun, CItemWeapon);			//compatibility
LINK_ENTITY_TO_CLASS(weapon_supernailgun, CItemWeapon);		//compatibility
LINK_ENTITY_TO_CLASS(weapon_rifle, CItemWeapon);
LINK_ENTITY_TO_CLASS(weapon_chaingun, CItemWeapon);

LINK_ENTITY_TO_CLASS(weapon_rocketlauncher, CItemWeapon);

LINK_ENTITY_TO_CLASS(weapon_grenadelauncher, CItemWeapon);	//compatibility
LINK_ENTITY_TO_CLASS(weapon_plasmarifle, CItemWeapon);

LINK_ENTITY_TO_CLASS(weapon_lightning, CItemWeapon);		//compatibility
LINK_ENTITY_TO_CLASS(weapon_bfg, CItemWeapon);


//*********************************************************
// weaponbox code:
//*********************************************************
LINK_ENTITY_TO_CLASS( backpack, CWeaponBox );

//=========================================================
//
//=========================================================
CWeaponBox *CWeaponBox::DropBackpack( CBaseEntity *pVictim, int weapon )
{
	if (!pVictim || !(pVictim->ammo_shells + pVictim->ammo_nails + pVictim->ammo_rockets + pVictim->ammo_cells))
		return NULL; // nothing in it

	CWeaponBox *pBackpack = GetClassPtr((CWeaponBox *)NULL );

	UTIL_SetOrigin( pBackpack->pev, pVictim->pev->origin + Vector( 0, 0, 24 ));

	// copy weapon and ammo
	pBackpack->pev->team = weapon;
	pBackpack->ammo_shells = pVictim->ammo_shells;
	pBackpack->ammo_nails = pVictim->ammo_nails;
	pBackpack->ammo_rockets = pVictim->ammo_rockets;
	pBackpack->ammo_cells = pVictim->ammo_cells;

	pBackpack->pev->velocity.x = RANDOM_FLOAT(-100, 100);
	pBackpack->pev->velocity.y = RANDOM_FLOAT(-100, 100);
	pBackpack->pev->velocity.z = 100;
	pBackpack->Spawn();
	pBackpack->pev->classname = MAKE_STRING("backpack");

	return pBackpack;
}

//=========================================================
// CWeaponBox - Spawn 
//=========================================================
void CWeaponBox::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	SET_MODEL(ENT(pev), "progs/backpack.mdl");

	UTIL_SetSize( pev, Vector( -1, -1, 0 ), Vector( 1, 1, 48 ));

	pev->nextthink = gpGlobals->time + 120;	// remove after 2 minutes
	SetThink( &CWeaponBox::SUB_Remove );
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================
void CWeaponBox::Touch( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer( ) || !pOther->IsAlive()) // only players may touch a weaponbox.
		return;
	
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	int best = pPlayer->W_BestWeapon();

	// change weapons
	pPlayer->ammo_shells += ammo_shells;
	pPlayer->ammo_nails += ammo_nails;
	pPlayer->ammo_rockets += ammo_rockets;
	pPlayer->ammo_cells += ammo_cells;
	pPlayer->m_iItems |= pev->team;
	pPlayer->CheckAmmo();

	CLIENT_PRINTF( pPlayer->edict(), print_console, "You get " );

	if (ammo_shells)
		CLIENT_PRINTF( pPlayer->edict(), print_console, UTIL_VarArgs( "%i shells", ammo_shells ));

	if (ammo_nails)
		CLIENT_PRINTF( pPlayer->edict(), print_console, UTIL_VarArgs( "%i nails", ammo_nails ));

	if (ammo_rockets)
		CLIENT_PRINTF( pPlayer->edict(), print_console, UTIL_VarArgs( "%i rockets", ammo_rockets ));

	if (ammo_cells)
		CLIENT_PRINTF( pPlayer->edict(), print_console, UTIL_VarArgs( "%i cells", ammo_cells ));

	CLIENT_PRINTF( pPlayer->edict(), print_console, "\n" );


	if (pev->team & IT_SHOTGUN)
		EMIT_SOUND(pOther->edict(), CHAN_ITEM, "items/pickup_weapon.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(pOther->edict(), CHAN_ITEM, "items/pickup_item.wav", 1, ATTN_NORM);

	// send bonus flash (same as command "bf\n")
	BONUS_FLASH( pPlayer->edict() );

	SetTouch(NULL);
	UTIL_Remove(this);
}

void CWeaponBox::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + pev->mins;
	pev->absmax = pev->origin + pev->maxs;

	// to make items easier to pick up and allow them to be grabbed off
	// of shelves, the abs sizes are expanded
	pev->absmin.x -= 15;
	pev->absmin.y -= 15;
	pev->absmax.x += 15;
	pev->absmax.y += 15;
}