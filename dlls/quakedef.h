//
//  quakedef.h

// this file is included by both the game-dll and the client-dll,

#ifndef CDLL_DLL_H
#define CDLL_DLL_H

//#define WOLF3DGAME

/***
*	HOLSTERING
*
*	Holstering is done by checking if the cued weapon on the server is different then the client
*	it sets pev->iuser4 to 151 and the client will see that value change and start lowering the gun
*	the client will set impulse to 151 to match and will go on from there. Impulse is used since
*	that variable is sent from the client to the server and we do not want any other weapon change
*	inputs happening while the next gun is being queued up anyways.
* 
*	151-155 need to be reserved for iuser4 and impulse so don't place any new cheats or inputs under those values.
* 
*	This is not the best way to go about this since it involves client->server communication which is
*	not viable in multiplayer games. For that you would want to do all the calculations on the server
*	and send that data to the client or use timing on the server to assume when the client is done
*	switching weapons. I chose this since vanilla Xash is not good at multiplayer games and that wasn't
*	really a goal for this project anyways.
* 
***/
#define HOLSTER_CLIENT_IN		gHUD.m_iuser4
#define HOLSTER_CLIENT_OUT		gHUD.m_impulse

#define HOLSTER_SERVER_IN		(int)pev->impulse
#define HOLSTER_SERVER_OUT		pev->iuser4

#define HOLSTER_STATE_NONE		0
#define HOLSTER_STATE_START		151
#define HOLSTER_STATE_LOWERED	152
#define HOLSTER_STATE_SET		153
#define HOLSTER_STATE_RAISING	154
#define HOLSTER_STATE_DONE		155


#define PLAYER_FATAL_FALL_SPEED			1024			// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED		650				// approx 20 feet
#define DAMAGE_FOR_FALL_SPEED			(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )	// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED			200
#define PLAYER_FALL_PUNCH_THRESHHOLD	(float)300		// won't punch player's screen/make scrape noise unless player falling at least this fast.


#define HIDEHUD_HUD			( 1<<0 )
#define HIDEHUD_ALL			( 1<<1 )

#define HUD_PRINTNOTIFY		1
#define HUD_PRINTCONSOLE	2
#define HUD_PRINTTALK		3
#define HUD_PRINTCENTER		4

// Quake custom messages for gmsgTempEntity
#define TE_SPIKE			0
#define TE_SUPERSPIKE		1
// TE_GUNSHOT already defined
// TE_EXPLOSION already defined
// TE_TAREXPLOSION already defined
#define TE_LIGHTNING1		5
#define TE_LIGHTNING2		6
#define TE_WIZSPIKE			7
#define TE_KNIGHTSPIKE		8
#define TE_LIGHTNING3		9
// TE_LAVASPLASH already defined
// TE_TELEPORT already defined
// TE_EXPLOSION2 already defined

//
// Quake items
//
// weapons
#define IT_AXE					(1<<0)		//fists
#define IT_PISTOL				(1<<1)
#define IT_SHOTGUN				(1<<2)
#define IT_SUPER_SHOTGUN		(1<<3)
#define IT_NAILGUN				(1<<4)		//machinegun
#define IT_SUPER_NAILGUN		(1<<5)		//chaingun
#define IT_ROCKET_LAUNCHER		(1<<6)
#define IT_GRENADE_LAUNCHER		(1<<7)		//plasma rifle
#define IT_LIGHTNING			(1<<8)		//bfg
#define IT_CHAINSAW				(1<<9)

// ammo
#define IT_SHELLS				(1<<10)
#define IT_NAILS				(1<<11)
#define IT_ROCKETS				(1<<12)
#define IT_CELLS				(1<<13)

// max ammo capacity - these double with the backpack
#define IT_MAX_SHELLS			50	//100
#define IT_MAX_NAILS			200
#define IT_MAX_ROCKETS			50	//100
#define IT_MAX_CELLS			300	//200

// armor
#define IT_ARMOR1				(1<<14)
#define IT_ARMOR2				(1<<15)
#define IT_ARMOR3				(1<<16)
#define IT_SUPERHEALTH			(1<<17)

// keys
#define IT_KEY1					(1<<18)
#define IT_KEY2					(1<<19)
#define IT_KEY3					(1<<20)

// artifacts
#define IT_INVISIBILITY			(1<<21)
#define IT_INVULNERABILITY		(1<<22)
#define IT_SUIT					(1<<23)
#define IT_QUAD					(1<<24)

#define IT_EXTRA1				(1<<25) //extras for modding
#define IT_EXTRA2				(1<<26)

//DOOM - backpack to increase ammo
#define IT_BACKPACK		(1<<27) //max number before rune/sigil

//
// Quake stats are integers communicated to the client by the server
//
#define	STAT_HEALTH				0
#define	STAT_FRAGS				1
#define	STAT_WEAPON				2
#define	STAT_AMMO				3
#define	STAT_ARMOR				4
#define	STAT_WEAPONFRAME		5
#define	STAT_SHELLS				6
#define	STAT_NAILS				7
#define	STAT_ROCKETS			8
#define	STAT_CELLS				9
#define	STAT_ACTIVEWEAPON		10
#define	STAT_TOTALSECRETS		11
#define	STAT_TOTALMONSTERS		12
#define	STAT_SECRETS			13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS			14		// bumped by svc_killedmonster
#define	STAT_WOLDTYPE			15
#define	STAT_TOTALITEMS			16
#define	STAT_ITEMS				17
#define	MAX_STATS				32

#define WORLDTYPE_MEDIEVAL			0
#define WORLDTYPE_RUNIC				1
#define WORLDTYPE_PRESENT			2

#endif// CDLL_DLL_H