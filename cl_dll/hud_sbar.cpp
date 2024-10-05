//
// hud_sbar.cpp
//
// main hud graphic drawing
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "ref_params.h"


#define STAT_MINUS		10 // num frame for '-' stats digit

DECLARE_MESSAGE(m_sbar, Stats)
DECLARE_MESSAGE(m_sbar, Items)
DECLARE_MESSAGE(m_sbar, LevelName)
DECLARE_MESSAGE(m_sbar, HudMessage)
DECLARE_MESSAGE(m_sbar, FoundSecret)
DECLARE_MESSAGE(m_sbar, KillMonster)
DECLARE_MESSAGE(m_sbar, FoundItem)
DECLARE_MESSAGE(m_sbar, Finale)

int CHudSBar::Init(void)
{
	HOOK_MESSAGE(Stats);
	HOOK_MESSAGE(Items);
	HOOK_MESSAGE(LevelName);
	HOOK_MESSAGE(HudMessage);
	HOOK_MESSAGE(FoundSecret);
	HOOK_MESSAGE(KillMonster);
	HOOK_MESSAGE(FoundItem);
	HOOK_MESSAGE(Finale);

	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);

	return 1;
};

int CHudSBar::VidInit(void)
{
	memset( gHUD.item_gettime, 0, sizeof( gHUD.item_gettime ));

#ifdef WOLF3DGAME
	h_numbers = SPR_Load("sprites/numbers.spr");
#else
	h_numbers = SPR_Load("sprites/hud/bnumbers.spr");
	s_numbers = SPR_Load("sprites/hud/snumbers.spr");
	i_numbers = SPR_Load("sprites/hud/internnum.spr");

	sb_items[0] = SPR_Load("sprites/hud/keys.spr");
#endif

	m_iFlags |= HUD_INTERMISSION;	// g-cont. allow episode finales

	gHUD.m_iGunPos_y = 0; //DOOM - holstering | clear this between map change
	gHUD.m_fChainsawTime = -1;

	gHUD.m_iFaceOffset = 0;
	gHUD.faceanimtime = 1;
	gHUD.m_fFaceAngeredTime = 1;
	gHUD.m_fFacePickup = -1;
	gHUD.m_iOldHealth = 0;
	gHUD.m_bHurt = FALSE;

	memset(c_MsgRow1, 0, sizeof(c_MsgRow1));
	memset(c_MsgRow2, 0, sizeof(c_MsgRow2));
	memset(c_MsgRow3, 0, sizeof(c_MsgRow3));
	m_iMsgTime = -1;

	HOLSTER_CLIENT_OUT = HOLSTER_STATE_NONE;

	return 1;
};

void CHudSBar::DrawPic( int x, int y, int pic )
{
	if (!gHUD.m_iIntermission)
	{
		//x += ((ScreenWidth - 320)>>1); //WOLF 3D - no need for this.
		//y += (ScreenHeight - SBAR_HEIGHT);
	}

	SPR_Set( gHUD.GetSprite( pic ), 255, 255, 255 );
	SPR_Draw( 0,  x, y, &gHUD.GetSpriteRect( pic ));
}

void CHudSBar::DrawTransPic( int x, int y, int pic )
{
	if (!gHUD.m_iIntermission)
	{
		x += ((ScreenWidth - 320)>>1);
		y += (ScreenHeight - SBAR_HEIGHT);
	}

	SPR_Set( gHUD.GetSprite( pic ), 255, 255, 255 );
	SPR_DrawHoles( 0, x, y, &gHUD.GetSpriteRect( pic ));
}

/*int getdigit(int number, int digit)
{
	return (number / ((int)pow(10, digit)) % 10);
}*/

void CHudSBar::DrawNum( int x, int y, int num, int flags )
{
	int xSize, k;

	SPR_Set(h_numbers, 255, 255, 255);

#ifdef WOLF3DGAME
	xSize = SPR_Width(h_numbers, 0);
#else
	xSize = SPR_Width(h_numbers, 0) - 2;
#endif

	if (flags & NUM_DRAWCENTER)
	{
		if (num >= 100)
			x -= xSize;
		else if (num >= 10)
			x -= xSize * 0.5;
	}

	//if (digits < 1) return; //failsafe for larger or smaller numbers

	if (num > 0)
	{
		if (flags & NUM_5DIGITS)
		{	
			if (num >= 10000) // Draw 10000's
			{
				k = num / 10000;

				SPR_DrawHoles(k, x, y, NULL);
				x += xSize;
			}
			else if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO)
			{
				if (flags & NUM_DRAWZERO)
					SPR_DrawHoles(0, x, y, NULL);
				x += xSize;
			}
		
			if (num >= 1000) // Draw 1000's
			{
				k = (num % 10000) / 1000;

				SPR_DrawHoles(k, x, y, NULL);
				x += xSize;
			}
			else if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO)
			{
				if (flags & NUM_DRAWZERO)
					SPR_DrawHoles(0, x, y, NULL);
				x += xSize;
			}
		}
		
		if (!(flags & NUM_2DIGITS))
		{
			if (num >= 100) // Draw 100's
			{
				k = (num % 1000) / 100;

				SPR_DrawHoles(k, x, y, NULL);
				x += xSize;
			}
			else if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO)
			{
				if (flags & NUM_DRAWZERO)
					SPR_DrawHoles(0, x, y, NULL);
				x += xSize;
			}
		}
		
		if (num >= 10) // Draw 10's
		{
			k = (num % 100) / 10;
			SPR_DrawHoles(k, x, y, NULL);
			x += xSize;
		}
		else if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO)
		{
			if (flags & NUM_DRAWZERO)
				SPR_DrawHoles(0, x, y, NULL);
			x += xSize;
		}

		// Draw ones
		k = num % 10;
		SPR_DrawHoles(k, x, y, NULL);
		x += xSize;
	}
	else
	{
		if (flags & NUM_5DIGITS)
		{		
			if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO) // SPR_Draw 10000's
			{
				if (flags & NUM_DRAWZERO)
					SPR_DrawHoles(0, x, y, NULL);
				x += xSize;
			}
			if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO) // SPR_Draw 1000's
			{
				if (flags & NUM_DRAWZERO)
					SPR_DrawHoles(0, x, y, NULL);
				x += xSize;
			}
		}
		
		if (!(flags & NUM_2DIGITS))
		{
			if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO) // SPR_Draw 100's
			{
				if (flags & NUM_DRAWZERO)
					SPR_DrawHoles(0, x, y, NULL);
				x += xSize;
			}
		}
		if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO) // SPR_Draw 10's
		{
			if (flags & NUM_DRAWZERO)
				SPR_DrawHoles(0, x, y, NULL);
			x += xSize;
		}

		// SPR_Draw ones
		SPR_DrawHoles(0, x, y, NULL);
		x += xSize;
	}
#ifndef WOLF3DGAME
	if (flags & NUM_PERCENT)
	{
		SPR_DrawHoles(10, x, y, NULL);
		x += xSize;
	}
#endif
	/*for (i = digits; i-- > 0; ) //reverse for loop
	{
		int d = getdigit(num, i);
		SPR_Draw(k, x, y, NULL);
		x += xSize;
	}*/
}

void CHudSBar::DrawSmallNum(int x, int y, int num, int flags)
{
	int xSize, k;

	SPR_Set(s_numbers, 255, 255, 255);

	xSize = SPR_Width(s_numbers, 0);

	if (flags & NUM_DRAWCENTER)
	{
		if (num >= 100)
			x -= xSize;
		else if (num >= 10)
			x -= xSize * 0.5;
	}

	if (num > 0)
	{
		if (num >= 100) // Draw 100's
		{
			k = (num % 1000) / 100;

			SPR_DrawHoles(k, x, y, NULL);
			x += xSize;
		}
		else if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO)
		{
			if (flags & NUM_DRAWZERO)
				SPR_DrawHoles(0, x, y, NULL);
			x += xSize;
		}

		if (num >= 10) // Draw 10's
		{
			k = (num % 100) / 10;
			SPR_DrawHoles(k, x, y, NULL);
			x += xSize;
		}
		else if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO)
		{
			if (flags & NUM_DRAWZERO)
				SPR_DrawHoles(0, x, y, NULL);
			x += xSize;
		}

		// Draw ones
		k = num % 10;
		SPR_DrawHoles(k, x, y, NULL);
		x += xSize;
	}
	else
	{
		if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO) // SPR_Draw 100's
		{
			if (flags & NUM_DRAWZERO)
				SPR_DrawHoles(0, x, y, NULL);
			x += xSize;
		}
		if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO) // SPR_Draw 10's
		{
			if (flags & NUM_DRAWZERO)
				SPR_DrawHoles(0, x, y, NULL);
			x += xSize;
		}

		// SPR_Draw ones
		SPR_DrawHoles(0, x, y, NULL);
		x += xSize;
	}
}

void CHudSBar::DrawIntNum(int x, int y, int num, int flags)
{
	int xSize, k;

	SPR_Set(i_numbers, 255, 255, 255);

	xSize = SPR_Width(i_numbers, 0) - 5;

	if (flags & NUM_DRAWCENTER)
	{
		if (num >= 100)
			x -= xSize;
		else if (num >= 10)
			x -= xSize * 0.5;
	}

	if (num > 0)
	{
		if (!(flags & NUM_2DIGITS))
		{
			if (num >= 100) // Draw 100's
			{
				k = (num % 1000) / 100;

				SPR_DrawHoles(k, x, y, NULL);
				x += xSize;
			}
			else if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO)
			{
				if (flags & NUM_DRAWZERO)
					SPR_DrawHoles(0, x, y, NULL);
				x += xSize;
			}
		}
		if (num >= 10) // Draw 10's
		{
			k = (num % 100) / 10;
			SPR_DrawHoles(k, x, y, NULL);
			x += xSize;
		}
		else if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO)
		{
			if (flags & NUM_DRAWZERO)
				SPR_DrawHoles(0, x, y, NULL);
			x += xSize;
		}

		// Draw ones
		k = num % 10;
		SPR_DrawHoles(k, x, y, NULL);
		x += xSize;
	}
	else
	{
		if (!(flags & NUM_2DIGITS))
		{
			if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO) // SPR_Draw 100's
			{
				if (flags & NUM_DRAWZERO)
					SPR_DrawHoles(0, x, y, NULL);
				x += xSize;
			}
		}
		if (flags & NUM_DRAWRIGHT || flags & NUM_DRAWZERO) // SPR_Draw 10's
		{
			if (flags & NUM_DRAWZERO)
				SPR_DrawHoles(0, x, y, NULL);
			x += xSize;
		}

		// SPR_Draw ones
		SPR_DrawHoles(0, x, y, NULL);
		x += xSize;
	}

	if (flags & NUM_SLASH)
	{
		SPR_DrawHoles(11, x, y, NULL);
		x += xSize;
	}
	else if (flags & NUM_COLON)
	{
		SPR_DrawHoles(10, x, y, NULL);
		x += xSize;
	}
	else if (flags & NUM_PERCENT)
	{
		SPR_DrawHoles(12, x, y, NULL);
		x += xSize;
	}
}

void CHudSBar::DrawString( int x, int y, char *str )
{
	gEngfuncs.pfnDrawSetTextColor( 0.5, 0.5, 0.5 );
	DrawConsoleString( x + (( ScreenWidth - 320 ) >> 1), y + ScreenHeight - SBAR_HEIGHT, str );
}

void CHudSBar::DrawCharacter( int x, int y, int num )
{
	char str[3];

	str[0] = (char)num;
	str[1] = '\0';

	DrawString( x, y, str );
}

#ifdef WOLF3DGAME
void CHudSBar::DrawFace( float flTime )
{
	int xPos, yPos, faceframe;
	HLSPRITE hspr = SPR_Load("sprites/face.spr");
	SPR_Set(hspr, 255, 255, 255);

	xPos = h_bar_x + 128;
	yPos = h_bar_y + 1;

	if (gHUD.stats[STAT_HEALTH] >= 90) //near full and full
		faceframe = 0;
	else if (gHUD.stats[STAT_HEALTH] >= 80)
		faceframe = 3;
	else if (gHUD.stats[STAT_HEALTH] >= 60)
		faceframe = 6;
	else if (gHUD.stats[STAT_HEALTH] >= 40)
		faceframe = 9;
	else if (gHUD.stats[STAT_HEALTH] >= 20)
		faceframe = 12;
	else if (gHUD.stats[STAT_HEALTH] >= 10)
		faceframe = 15;
	else if (gHUD.stats[STAT_HEALTH] > 0)
		faceframe = 18;
	else
	{
		gHUD.m_iFaceOffset = 0;
		faceframe = 22;
	}

	if (gHUD.stats[STAT_HEALTH] > 0)
	{
		if (gHUD.faceanimtime < flTime)
		{
			gHUD.m_iFaceOffset = (gEngfuncs.pfnRandomLong(0, 2));

			if (gHUD.m_iFaceOffset > 0)
				gHUD.faceanimtime = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(0.25, 0.85);		
			else
				gHUD.faceanimtime = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(0.45, 1.15);
		}
	}

	SPR_DrawHoles(faceframe + gHUD.m_iFaceOffset, xPos, yPos, NULL);
}
#else
#define FACE_HURT_TURN 0.30f
void CHudSBar::DrawFace(float flTime)
{
	int xPos, yPos, faceframe;
	qboolean anger = FALSE;
	qboolean smile = FALSE;
	int healthdiff;

	HLSPRITE hspr = SPR_Load("sprites/hud/face.spr");
	SPR_SetAlpha(hspr, 255, 255, 255, 255);

	xPos = h_bar_x + 143;
	yPos = h_bar_y + 0;

	//really stupid way to reset old health without creating another variable and checking client server shit
	if (gHUD.faceanimtime == 1 && gHUD.m_fFaceAngeredTime == 1)
		gHUD.m_iOldHealth = gHUD.stats[STAT_HEALTH];

	if (gHUD.m_iOldHealth != gHUD.stats[STAT_HEALTH])
	{
		healthdiff = gHUD.m_iOldHealth - gHUD.stats[STAT_HEALTH];

		if (healthdiff > 0)
		{
			//gEngfuncs.Con_Printf("GOT HURT: %0.2f | %i\n", gHUD.m_fFaceDirection, healthdiff);

			gHUD.faceanimtime = gEngfuncs.GetClientTime() + 1.0f;
			gHUD.m_iFaceOffset = 0;
			gHUD.m_bHurt = TRUE;
		}

		gHUD.m_iOldHealth = gHUD.stats[STAT_HEALTH];
	}

	//sprite is idle so reset this until firing
	if (gHUD.m_iSpriteFrame == 0)
		gHUD.m_fFaceAngeredTime = flTime + 3.0f;

	//weapon has not returned to idle so get angry
	if (gHUD.m_fFaceAngeredTime < flTime)
	{
		anger = TRUE;
		gHUD.faceanimtime = gEngfuncs.GetClientTime() + 0.2f;
		gHUD.m_iFaceOffset = 0;
	}

	//smile when picking up new gun
	if ((gHUD.m_fFacePickup >= flTime) && (!gHUD.m_bHurt))
	{
		smile = TRUE;
		gHUD.m_iFaceOffset = 0;
	}
	
	if ((gHUD.items & IT_INVISIBILITY) && (gHUD.stats[STAT_HEALTH] > 0))
	{
		faceframe = 42;
	}
	else if ((gHUD.items & IT_INVULNERABILITY) && (gHUD.stats[STAT_HEALTH] > 0))
	{
		gHUD.faceanimtime = gEngfuncs.GetClientTime() + 0.2f;
		gHUD.m_iFaceOffset = 0;
		faceframe = 41;
	}
	else
	{
		if (gHUD.stats[STAT_HEALTH] >= 80) //near full and full
		{
			if (smile)
			{
				faceframe = 30;
			}
			else if (gHUD.m_bHurt)
			{
				if(gHUD.m_fFaceDirection > FACE_HURT_TURN)
					faceframe = 17;
				else if (gHUD.m_fFaceDirection < -FACE_HURT_TURN)
					faceframe = 16;
				else
					faceframe = 15;
			}
			else if(anger)
				faceframe = 15;
			else
				faceframe = 0;
		}
		else if (gHUD.stats[STAT_HEALTH] >= 60)
		{
			if (smile)
			{
				faceframe = 31;
			}
			else if (gHUD.m_bHurt)
			{
				if (gHUD.m_fFaceDirection > FACE_HURT_TURN)
					faceframe = 20;
				else if (gHUD.m_fFaceDirection < -FACE_HURT_TURN)
					faceframe = 19;
				else
					faceframe = 18;
			}
			else if (anger)
				faceframe = 18;
			else
				faceframe = 3;
		}
		else if (gHUD.stats[STAT_HEALTH] >= 40)
		{
			if (smile)
			{
				faceframe = 32;
			}
			else if (gHUD.m_bHurt)
			{
				if (gHUD.m_fFaceDirection > FACE_HURT_TURN)
					faceframe = 23;
				else if (gHUD.m_fFaceDirection < -FACE_HURT_TURN)
					faceframe = 22;
				else
					faceframe = 21;
			}
			else if (anger)
				faceframe = 21;
			else
				faceframe = 6;
		}
		else if (gHUD.stats[STAT_HEALTH] >= 20)
		{
			if (smile)
			{
				faceframe = 33;
			}
			else if (gHUD.m_bHurt)
			{
				if (gHUD.m_fFaceDirection > FACE_HURT_TURN)
					faceframe = 26;
				else if (gHUD.m_fFaceDirection < -FACE_HURT_TURN)
					faceframe = 25;
				else
					faceframe = 24;
			}
			else if (anger)
				faceframe = 24;
			else
				faceframe = 9;
		}
		else if (gHUD.stats[STAT_HEALTH] > 0)
		{
			if (smile)
			{
				faceframe = 34;
			}
			else if (gHUD.m_bHurt)
			{
				if (gHUD.m_fFaceDirection > FACE_HURT_TURN)
					faceframe = 29;
				else if (gHUD.m_fFaceDirection < -FACE_HURT_TURN)
					faceframe = 28;
				else
					faceframe = 27;
			}
			else if (anger)
				faceframe = 27;
			else
				faceframe = 12;
		}
		else if (gHUD.stats[STAT_HEALTH] <= 0)
		{
			gHUD.m_iFaceOffset = 0;
			faceframe = 40;
		}
	}

	if (gHUD.faceanimtime < flTime && gHUD.m_bHurt)
	{
		gHUD.m_bHurt = false;
		gHUD.m_iFaceOffset = 0;
		gHUD.faceanimtime = gEngfuncs.GetClientTime() + 0.2f;
	}

	if (gHUD.stats[STAT_HEALTH] > 0 && !anger && !gHUD.m_bHurt && (gHUD.m_fFacePickup < flTime)) //look left/right/center at random
	{
		if (gHUD.faceanimtime < flTime)
		{
			gHUD.m_iFaceOffset = (gEngfuncs.pfnRandomLong(0, 2));

			if (gHUD.m_iFaceOffset > 0)
				gHUD.faceanimtime = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(0.25, 0.85);
			else
				gHUD.faceanimtime = gEngfuncs.GetClientTime() + gEngfuncs.pfnRandomFloat(0.45, 1.15);
		}
	}

	SPR_DrawHoles(faceframe + gHUD.m_iFaceOffset, xPos, yPos, NULL);
}
#endif

#ifdef WOLF3DGAME
void CHudSBar::DrawWeaponIcon(float flTime)
{
	int xPos, yPos, icon;

	HLSPRITE hspr = SPR_Load("sprites/weapons.spr");
	SPR_Set(hspr, 255, 255, 255);

	xPos = h_bar_x + 248;
	yPos = h_bar_y + 5;

	switch (gHUD.m_iSpriteCur)
	{
	case IT_AXE:
		icon = 0; break;

	case IT_SHOTGUN:
		icon = 1; break;

	case IT_NAILGUN:
		icon = 2; break;

	case IT_SUPER_NAILGUN:
		icon = 3; break;
	}

	SPR_DrawHoles(icon, xPos, yPos, NULL);
}
#endif

void CHudSBar::DrawIntermission( float flTime )
{
	//int dig, num;
	int minutes, seconds, units;

	HLSPRITE hspr = SPR_Load("sprites/hud/interntxt.spr");
	SPR_Set(hspr, 255, 255, 255);
	int h_txt_width = SPR_Width(hspr, 0);

	int h_txt_x = (ScreenWidth * 0.5) - (h_txt_width * 0.5);
	int h_txt_y = (ScreenHeight * 0.5f) - 90;

	SPR_DrawHoles(0, h_txt_x, h_txt_y, NULL);


	// time
	minutes = completed_time / 60;
	seconds = completed_time - 60 * minutes;
	units = completed_time * 100;

	SPR_DrawHoles(2, (ScreenWidth * 0.5f) - 122, (ScreenHeight * 0.5f) - 60, NULL);

	DrawIntNum((ScreenWidth * 0.5f) + 34, (ScreenHeight * 0.5f) - 60, minutes, NUM_DRAWZERO | NUM_DRAWRIGHT | NUM_2DIGITS | NUM_COLON);

	DrawIntNum((ScreenWidth * 0.5f) + 67, (ScreenHeight * 0.5f) - 60, seconds, NUM_DRAWZERO | NUM_DRAWRIGHT | NUM_2DIGITS | NUM_COLON);

	DrawIntNum((ScreenWidth * 0.5f) + 100, (ScreenHeight * 0.5f) - 60, units, NUM_DRAWZERO | NUM_DRAWRIGHT | NUM_2DIGITS);


	// items
	SPR_Set(hspr, 255, 255, 255);
	SPR_DrawHoles(4, (ScreenWidth * 0.5f) - 122, (ScreenHeight * 0.5f) - 30, NULL);

	DrawIntNum((ScreenWidth * 0.5f) + 44, (ScreenHeight * 0.5f) - 30, gHUD.stats[STAT_ITEMS], NUM_DRAWZERO | NUM_DRAWRIGHT | NUM_SLASH);

	DrawIntNum((ScreenWidth * 0.5f) + 89, (ScreenHeight * 0.5f) - 30, gHUD.stats[STAT_TOTALITEMS], NUM_DRAWZERO | NUM_DRAWRIGHT);


	// secrets
	SPR_Set(hspr, 255, 255, 255);
	SPR_DrawHoles(1, (ScreenWidth * 0.5f) - 122, (ScreenHeight * 0.5f), NULL);

	DrawIntNum((ScreenWidth * 0.5f) + 44, (ScreenHeight * 0.5f), gHUD.stats[STAT_SECRETS], NUM_DRAWZERO | NUM_DRAWRIGHT | NUM_SLASH);

	DrawIntNum((ScreenWidth * 0.5f) + 89, (ScreenHeight * 0.5f), gHUD.stats[STAT_TOTALSECRETS], NUM_DRAWZERO | NUM_DRAWRIGHT);


	// kills
	SPR_Set(hspr, 255, 255, 255);
	SPR_DrawHoles(3, (ScreenWidth * 0.5f) - 122, (ScreenHeight * 0.5f) + 30, NULL);

	DrawIntNum((ScreenWidth * 0.5f) + 44, (ScreenHeight * 0.5f) + 30, gHUD.stats[STAT_MONSTERS], NUM_DRAWZERO | NUM_DRAWRIGHT | NUM_SLASH);

	DrawIntNum((ScreenWidth * 0.5f) + 89, (ScreenHeight * 0.5f) + 30, gHUD.stats[STAT_TOTALMONSTERS], NUM_DRAWZERO | NUM_DRAWRIGHT);
}

void CHudSBar::DrawFinale( float flTime )
{
	//DrawTransPic((ScreenWidth-288)*0.5f, 16, sb_finale );
}

#define TOPOFFSET 4
#define BOTOFFSET 2
#define HUD_BAR			0
#define HUD_BAR_WIDE	2
#define HUD_OUTLINE_L	4
#define HUD_OUTLINE_R	5
#define HUD_OUTLINE_T	6
#define HUD_OUTLINE_B	3

#define HUD_OUTLINE_TL	7
#define HUD_OUTLINE_TR	8
#define HUD_OUTLINE_BL	1
#define HUD_OUTLINE_BR	2

#ifdef WOLF3DGAME
void CHudSBar::DrawMainHud(float flTime) //WOLF - draw hud outline and main bar
{
	int xSize, ySize, yPos, xPos;
	HLSPRITE hspr = SPR_Load("sprites/hud.spr");
	SPR_Set(hspr, 255, 255, 255);

	if (gHUD.sb_lines == 2)
	{
		//left
		ySize = SPR_Height(hspr, HUD_OUTLINE_L);

		xPos = 0;
		yPos = 0;

		for (yPos = 0; yPos < ScreenHeight; yPos += ySize)
			SPR_Draw(HUD_OUTLINE_L, xPos, yPos, NULL);

		//right
		xSize = SPR_Width(hspr, HUD_OUTLINE_R);
		ySize = SPR_Height(hspr, HUD_OUTLINE_R);

		xPos = ScreenWidth - xSize;
		yPos = 0;

		for (yPos = 0; yPos < ScreenHeight; yPos += ySize)
			SPR_Draw(HUD_OUTLINE_R, xPos, yPos, NULL);

		//top
		xSize = SPR_Width(hspr, 6);

		yPos = -TOPOFFSET;

		for (xPos = 0; xPos < ScreenWidth; xPos += xSize)
			SPR_Draw(HUD_OUTLINE_T, xPos, yPos, NULL);
	}

	//bottom
	if (gHUD.sb_lines >= 1)
	{
		xSize = SPR_Width(hspr, HUD_OUTLINE_B);
		gHUD.hud_back_size_y = SPR_Height(hspr, HUD_OUTLINE_B);

		yPos = ScreenHeight - gHUD.hud_back_size_y + BOTOFFSET;

		for (xPos = 0; xPos < ScreenWidth; xPos += xSize)
			SPR_Draw(HUD_OUTLINE_B, xPos, yPos, NULL);
	}

	if (gHUD.sb_lines == 2)
	{
		//top left corner
		xPos = 0;
		yPos = -TOPOFFSET;

		SPR_Draw(HUD_OUTLINE_TL, xPos, yPos, NULL);

		//top right corner
		xSize = SPR_Width(hspr, HUD_OUTLINE_TR);

		xPos = ScreenWidth - xSize;
		yPos = -TOPOFFSET;

		SPR_Draw(HUD_OUTLINE_TR, xPos, yPos, NULL);

		//bottom left corner
		//xSize = SPR_Width(hspr, 2);
		ySize = SPR_Height(hspr, HUD_OUTLINE_BL);

		xPos = 0;//ScreenWidth - xSize;
		yPos = ScreenHeight - ySize + BOTOFFSET;

		SPR_Draw(HUD_OUTLINE_BL, xPos, yPos, NULL);

		//bottom right corner
		xSize = SPR_Width(hspr, HUD_OUTLINE_BR);
		ySize = SPR_Height(hspr, HUD_OUTLINE_BR);

		xPos = ScreenWidth - xSize;
		yPos = ScreenHeight - ySize + BOTOFFSET;

		SPR_Draw(HUD_OUTLINE_BR, xPos, yPos, NULL);
	}

	//bar
	h_bar_width = SPR_Width(hspr, HUD_BAR);
	h_bar_height = SPR_Height(hspr, HUD_BAR);

	h_bar_x = (ScreenWidth / 2) - (h_bar_width / 2);
	h_bar_y = ScreenHeight - h_bar_height;

	SPR_Draw(HUD_BAR, h_bar_x, h_bar_y, NULL);
}
#else
void CHudSBar::DrawMainHud(float flTime) //WOLF - draw hud outline and main bar
{
	int msgframe = 0;

	HLSPRITE hspr = SPR_Load("sprites/hud/hud.spr");

	if (m_iMsgTime >= flTime && (CVAR_GET_FLOAT("doom_msg") >= 1))
		msgframe = 1;

	if (gHUD.sb_lines > 0)
	{
		SPR_Set(hspr, 255, 255, 255);

		//bar
		h_bar_width = SPR_Width(hspr, HUD_BAR);
		h_bar_height = SPR_Height(hspr, HUD_BAR);

		h_bar_x = (ScreenWidth * 0.5) - (h_bar_width * 0.5);
		h_bar_y = ScreenHeight - h_bar_height;
		gHUD.hud_back_size_y = h_bar_height;

		SPR_Draw(msgframe, h_bar_x, h_bar_y, NULL);

		//widescreen bits
		if (h_bar_x > 0) //dont draw these graphics unless the hud starts past x pos 0 (meaning we're most likely wide screen)
		{
			SPR_Draw(HUD_BAR_WIDE, h_bar_x - SPR_Width(hspr, HUD_BAR_WIDE), h_bar_y, NULL);
			SPR_Draw(HUD_BAR_WIDE, h_bar_x + h_bar_width, h_bar_y, NULL);
		}
	}
	else
	{
		if ((gHUD.stats[STAT_ACTIVEWEAPON] == IT_SUPER_SHOTGUN))
			gHUD.hud_back_size_y = 2;
		else
			gHUD.hud_back_size_y = 6;
	}

	if (CVAR_GET_FLOAT("doom_msg") >= 1)
	{
		if (m_iMsgTime >= flTime)
		{
			gHUD.DrawNewString(h_bar_x + 108, h_bar_y + 1, c_MsgRow3, 255, 255, 255, 255);
			gHUD.DrawNewString(h_bar_x + 108, h_bar_y + 11, c_MsgRow2, 255, 255, 255, 255);
			gHUD.DrawNewString(h_bar_x + 108, h_bar_y + 21, c_MsgRow1, 255, 255, 255, 255);
		}
	}
	else
	{
		if (m_iMsgTime >= flTime)
		{
			gHUD.DrawNewString(2, 17, c_MsgRow3, 255, 255, 255, 255);
			gHUD.DrawNewString(2, 9, c_MsgRow2, 255, 255, 255, 255);
			gHUD.DrawNewString(2, 1, c_MsgRow1, 255, 255, 255, 255);
		}
		else
		{
			if (c_MsgRow1)
				memset(c_MsgRow1, 0, sizeof(c_MsgRow1));
			if (c_MsgRow2)
				memset(c_MsgRow2, 0, sizeof(c_MsgRow2));
			if (c_MsgRow3)
				memset(c_MsgRow3, 0, sizeof(c_MsgRow3));
		}
	}
}
#endif

#ifdef WOLF3DGAME
void CHudSBar::DrawWeapon(float flTime)
{
	int xPos, yPos, iWidth, iHeight;
	HLSPRITE wspr;

	switch (gHUD.m_iSpriteCur)
	{
	default:
	case IT_AXE:
		wspr = SPR_Load("sprites/knife.spr"); break;

	case IT_SHOTGUN:
		wspr = SPR_Load("sprites/pistol.spr"); break;

	case IT_NAILGUN:
		wspr = SPR_Load("sprites/machinegun.spr"); break;

	case IT_SUPER_NAILGUN:
		wspr = SPR_Load("sprites/chaingun.spr"); break;
	}

	SPR_Set(wspr, gHUD.m_fLightValue, gHUD.m_fLightValue, gHUD.m_fLightValue);

	iWidth = SPR_Width(wspr, 0);
	iHeight = SPR_Height(wspr, 0);

	xPos = (ScreenWeaponWidth / 2) - (iWidth / 2);

	float weaponoffset;
	switch (ScreenWeaponScale)
	{
	default:
	case 1: weaponoffset = 1; break;
	case 2: weaponoffset = 1.5; break;
	case 3: weaponoffset = 1.33; break;
	case 4: weaponoffset = 2.5; break;
	}
	yPos = ceil(ScreenHeight - gHUD.hud_back_size_y + BOTOFFSET - (iHeight * weaponoffset));
	//gHUD.m_iViewportHeight * ScreenWeaponScale + gHUD.m_iViewportY * ScreenWeaponScale - iHeight;
	//gEngfuncs.Con_Printf("height: %i | scale: %i\n", iHeight, ScreenWeaponScale);

	float mouseX = gHUD.m_fMouseX * -0.25;
	float mouseY = gHUD.m_fMouseY * -0.25;
	static float lag_x;
	static float lag_y;
	float frameadj = (1.0f / gHUD.m_flTimeDelta) * 0.01;

	lag_x = SmoothValues(lag_x, mouseX * frameadj, gHUD.m_flTimeDelta * 6);
	if (lag_x >= 35) lag_x = 35; if (lag_x <= -35) lag_x = -35;
	xPos += (int)(lag_x * 0.25 + .5);

	lag_y = SmoothValues(lag_y, mouseY * frameadj, gHUD.m_flTimeDelta * 6);
	if (lag_y >= 35) lag_y = 35; if (lag_y <= -35) lag_y = -35;
	yPos += (int)(lag_y * 0.25 + .5);

	xPos += gHUD.bob_x;
	yPos += gHUD.bob_y;

	SPR_DrawHolesWeapon(gHUD.m_iSpriteFrame, xPos, yPos, NULL);
}
#else
#include "cl_entity.h"
void CHudSBar::DrawWeapon(float flTime)
{
	int xPos, yPos, yPosStored, iWidth, iHeight;
	int vertoffset = 0;
	int storedoffset = 0;
	HLSPRITE wspr;
	HLSPRITE wmuz = NULL;
	Vector shake;
	static int m_iChainsawFrame = 0;
	static qboolean m_bChainsawUp = FALSE;

	float mouseX = gHUD.m_fMouseX * -0.25;
	float mouseY = gHUD.m_fMouseY * -0.25;
	static float lag_x, lag_y;
	int viscol = 255;
	int visalpha = 255;

	switch (gHUD.m_iSpriteCur)
	{
	default:
		if (gEngfuncs.pDemoAPI->IsRecording()) //fix for recording demos where the gun sprite errors.
			HOLSTER_CLIENT_OUT = HOLSTER_CLIENT_IN = HOLSTER_STATE_LOWERED;

		gEngfuncs.Con_Printf("ERROR: invalid weapon sprite\n");
		break;

	case IT_AXE:
		wspr = SPR_Load("sprites/weapons/fists.spr");
		vertoffset = 46; storedoffset = 6;
		break;

	case IT_CHAINSAW:
		wspr = SPR_Load("sprites/weapons/chainsaw.spr");
		vertoffset = 20; storedoffset = -10;
		break;

	case IT_PISTOL:
		wspr = SPR_Load("sprites/weapons/pistol.spr"); wmuz = SPR_Load("sprites/weapons/pistol_muz.spr");
		vertoffset = 46; storedoffset = 90;
		break;

	case IT_SHOTGUN:
		wspr = SPR_Load("sprites/weapons/shotgun.spr"); wmuz = SPR_Load("sprites/weapons/shotgun_muz.spr");
		vertoffset = 46; storedoffset = 154;
		break;

	case IT_SUPER_SHOTGUN:
		wspr = SPR_Load("sprites/weapons/supershot.spr"); wmuz = SPR_Load("sprites/weapons/supershot_muz.spr");
		vertoffset = 10; storedoffset = 126;
		break;

	case IT_NAILGUN:
		wspr = SPR_Load("sprites/weapons/machinegun.spr"); wmuz = SPR_Load("sprites/weapons/machinegun_muz.spr");
		vertoffset = 20; storedoffset = 0;
		break;

	case IT_SUPER_NAILGUN:
		wspr = SPR_Load("sprites/weapons/chaingun.spr"); wmuz = SPR_Load("sprites/weapons/chaingun_muz.spr");
		vertoffset = 48; storedoffset = 0;
		break;

	case IT_ROCKET_LAUNCHER:
		wspr = SPR_Load("sprites/weapons/rocketlauncher.spr"); wmuz = SPR_Load("sprites/weapons/rocketlauncher_muz.spr");
		vertoffset = 48; storedoffset = 10;
		break;

	case IT_GRENADE_LAUNCHER:
		wspr = SPR_Load("sprites/weapons/plasmarifle.spr");
		vertoffset = 16; storedoffset = 10;
		break;

	case IT_LIGHTNING:
		wspr = SPR_Load("sprites/weapons/bfg.spr"); wmuz = SPR_Load("sprites/weapons/bfg_muz.spr");
		vertoffset = 48; storedoffset = 10;
		break;
	}

	if (gHUD.items & IT_INVISIBILITY)
	{
		viscol = 25;
		visalpha = 75;
		SPR_SetAlpha(wspr, viscol, viscol, viscol, visalpha);
	}
	else
	{
		if (gHUD.m_iSpriteCur == IT_GRENADE_LAUNCHER && (gHUD.m_iSpriteFrame == 1 || gHUD.m_iSpriteFrame == 2))
			SPR_Set(wspr, 255, 255, 255);
		else
			SPR_Set(wspr, int(gHUD.m_iGunIllum * gHUD.m_vGunCol[0]), int(gHUD.m_iGunIllum * gHUD.m_vGunCol[1]), int(gHUD.m_iGunIllum * gHUD.m_vGunCol[2]));
	}
	
	iWidth = SPR_Width(wspr, 0);
	iHeight = abs(SPR_Height(wspr, 0));

	xPos = (ScreenWidth * 0.5) - (iWidth * 0.5);
	xPos += gHUD.bob_x;

	yPos = ScreenHeight - gHUD.hud_back_size_y - iHeight + vertoffset;
	yPos += gHUD.bob_y;

	//calculate view lag for guns
	if (gHUD.m_flTimeDelta != 0.0f)
	{
		lag_x = gHUD.SmoothValues(lag_x, mouseX * gHUD.m_fFrameTimeAdjust, gHUD.m_flTimeDelta * 6);
		if (lag_x >= 35) lag_x = 35; if (lag_x <= -35) lag_x = -35;
		xPos += (int)(lag_x * 0.25 + .5);

		lag_y = gHUD.SmoothValues(lag_y, mouseY * gHUD.m_fFrameTimeAdjust, gHUD.m_flTimeDelta * 6);
		if (lag_y >= 35) lag_y = 35; if (lag_y <= -35) lag_y = -35;
		yPos += (int)(lag_y * 0.25 + .5);
	}

	//save holstered position of gun so its quicker to unholster
	yPosStored = ScreenHeight - gHUD.hud_back_size_y - storedoffset;

	//gEngfuncs.Con_Printf("ypos: %i | gunpos: %i\n", yPos, gHUD.m_iGunPos_y);

	if(HOLSTER_CLIENT_IN == HOLSTER_STATE_START)// holster gun
	{
		if (gHUD.m_iGunPos_y < yPosStored)
		{
			gHUD.m_iGunPos_y += ceil(gHUD.m_flTimeDelta * 140);
			HOLSTER_CLIENT_OUT = HOLSTER_STATE_START;
		}
		else
		{
			gHUD.m_iGunPos_y = yPosStored;
			HOLSTER_CLIENT_OUT = HOLSTER_STATE_LOWERED;
		}
	}
	else if (HOLSTER_CLIENT_OUT == HOLSTER_STATE_RAISING) // unholster gun
	{
		if (CVAR_GET_FLOAT("v_melt") != 0) //only unholster gun once smearing is done
		{
			gHUD.m_iGunPos_y = yPosStored;
		}
		else
		{
			if (gHUD.m_iGunPos_y <= yPos)
			{
				gHUD.m_iGunPos_y = yPos;
				HOLSTER_CLIENT_OUT = HOLSTER_STATE_DONE;
			}
			else
			{
				gHUD.m_iGunPos_y -= ceil(gHUD.m_flTimeDelta * 140);

				if(gHUD.m_iGunPos_y <= yPos)
					gHUD.m_iGunPos_y = yPos;
			}
		}
		gHUD.bob_y = gHUD.bob_x = 0;
	}
	else if (HOLSTER_CLIENT_IN == HOLSTER_STATE_SET) // set new guns holstered positing first before unholstering
	{
		gHUD.m_iGunPos_y = yPosStored;
		HOLSTER_CLIENT_OUT = HOLSTER_STATE_RAISING;
	}
	else
	{
		gHUD.m_iGunPos_y = yPos;
	}

	//gEngfuncs.Con_Printf("Client IN: %i | Client OUT: %i\n", HOLSTER_CLIENT_IN, HOLSTER_CLIENT_OUT);

	if (CVAR_GET_FLOAT("test") == 1)
		gHUD.m_iGunPos_y = yPosStored;

	//add shake to guns
	gEngfuncs.V_CalcShake();
	shake[0] = xPos; shake[1] = gHUD.m_iGunPos_y;

	gEngfuncs.V_ApplyShake(shake, Vector(0, 0, 0), 1.5);
	xPos = shake[0]; gHUD.m_iGunPos_y = shake[1];

	//we do chainsaw here
	if (gHUD.m_iSpriteCur == IT_CHAINSAW)
	{
		if (HOLSTER_CLIENT_IN <= HOLSTER_STATE_NONE)
		{
			if (gHUD.m_fChainsawTime < flTime)
			{
				if (gHUD.m_iSpriteFrame == 0)
				{
					if (m_iChainsawFrame == 0)
					{
						m_iChainsawFrame = 1;
						gEngfuncs.pfnPlaySoundByName("weapons/chainsaw_idle.wav", 1.0);
					}
					else
					{
						m_iChainsawFrame = 0;
					}
					gHUD.m_fChainsawTime = flTime + 0.1;
				}
				else
				{
					if (m_iChainsawFrame == 2) m_iChainsawFrame = 3;
					else m_iChainsawFrame = 2;

					gHUD.m_fChainsawTime = flTime + 0.05;
				}
			}
			m_bChainsawUp = FALSE;
		}
		else if (HOLSTER_CLIENT_IN == HOLSTER_STATE_SET && !m_bChainsawUp)
		{
			gEngfuncs.pfnPlaySoundByName("weapons/chainsaw_up.wav", 1.0);
			m_bChainsawUp = TRUE;
		}
		else
		{
			m_iChainsawFrame = 0;
		}

		SPR_DrawHoles(m_iChainsawFrame, xPos, gHUD.m_iGunPos_y, NULL);
	}
	else
	{
		SPR_DrawHoles(gHUD.m_iSpriteFrame, xPos, gHUD.m_iGunPos_y, NULL);
	}

	if (gHUD.items & IT_INVISIBILITY)
		return;

	//render muzzleflash for guns that loaded one.
	if (wmuz)
	{
		SPR_SetAlpha(wmuz, viscol, viscol, viscol, visalpha);

		switch (gHUD.m_iSpriteCur)
		{
		case IT_SUPER_NAILGUN:
			if (gHUD.m_iSpriteFrame >= 1)
				SPR_DrawHoles(gHUD.m_iSpriteFrame - 1, xPos, gHUD.m_iGunPos_y, NULL);
			break;

		case IT_NAILGUN:
			if (gHUD.m_iSpriteFrame == 1)
				SPR_DrawHoles(0, xPos, gHUD.m_iGunPos_y, NULL);
			break;

		case IT_PISTOL:
			if (gHUD.m_iSpriteFrame == 2)
				SPR_DrawHoles(0, xPos, gHUD.m_iGunPos_y, NULL);
			break;

		case IT_SHOTGUN:
		case IT_SUPER_SHOTGUN:
			if (gHUD.m_iSpriteFrame == 1 || gHUD.m_iSpriteFrame == 2)
				SPR_DrawHoles(gHUD.m_iSpriteFrame - 1, xPos, gHUD.m_iGunPos_y, NULL);
			break;

		case IT_ROCKET_LAUNCHER:
			if (gHUD.m_iSpriteFrame >= 1 && gHUD.m_iSpriteFrame < 5)
				SPR_DrawHoles(gHUD.m_iSpriteFrame - 1, xPos, gHUD.m_iGunPos_y, NULL);

		case IT_LIGHTNING:
			if (gHUD.m_iSpriteFrame == 2 || gHUD.m_iSpriteFrame == 3)
				SPR_DrawHoles(gHUD.m_iSpriteFrame - 2, xPos, gHUD.m_iGunPos_y, NULL);
			break;
		}
	}
}
#endif

int CHudSBar::Draw(float fTime)
{
	int xPos, yPos;
	int backpack = 1;

	if (!gHUD.m_iIntermission)
	{
		completed_time = fTime;
	}
	else if (gHUD.m_iIntermission == 1)
	{
		DrawIntermission( fTime );
		return 1;
	}
	else if (gHUD.m_iIntermission == 2)
	{
		DrawFinale( fTime );
		return 1;
	}

	if( gHUD.m_iHideHUDDisplay & HIDEHUD_HUD )
		return 1;

	if ((gEngfuncs.GetMaxClients() == 1) && (gHUD.showscores))// || gHUD.stats[STAT_HEALTH] <= 0))
		return 1;

	//========================================================================================================

	if (gHUD.stats[STAT_HEALTH] > 0)
		DrawWeapon(fTime);

	DrawMainHud(fTime);

	if (gHUD.sb_lines > 0)
	{
#ifdef WOLF3DGAME
		DrawWeaponIcon(fTime);

		// floor
		xPos = h_bar_x + 13;
		yPos = h_bar_y + 13;

		float beans = gEngfuncs.pfnGetCvarFloat("_level");
		DrawNum(xPos, yPos, beans, NUM_DRAWCENTER);

		// score
		xPos = h_bar_x + 40;

		beans = gHUD.stats[STAT_BIGSCORE];//gEngfuncs.pfnGetCvarFloat("_score");
		DrawNum(xPos, yPos, beans, NUM_DRAWRIGHT | NUM_5DIGITS);

		// health
		xPos = h_bar_x + 160;
		DrawNum(xPos, yPos, gHUD.stats[STAT_HEALTH], NUM_DRAWRIGHT | NUM_PERCENT);

		// armor
		xPos = h_bar_x + 89;
		if (gHUD.items & IT_INVULNERABILITY)
		{
			DrawNum(xPos, yPos, 666, NUM_DRAWRIGHT);
		}
		else
		{
			DrawNum(xPos, yPos, gHUD.stats[STAT_ARMOR], NUM_DRAWRIGHT);

			//if (gHUD.items & IT_ARMOR3)
			//	DrawPic( 0, 0, sb_armor[2] );
			//else if (gHUD.items & IT_ARMOR2)
			//	DrawPic( 0, 0, sb_armor[1] );
			//else if (gHUD.items & IT_ARMOR1)
			//	DrawPic( 0, 0, sb_armor[0] );
		}

		// ammo icon
		/*xPos = h_bar_x + 260;

		if (gHUD.items & IT_SHELLS)
			DrawPic(xPos, yPos, sb_ammo[0] );
		else if (gHUD.items & IT_NAILS)
			DrawPic(xPos, yPos, sb_ammo[1] );
		else if (gHUD.items & IT_ROCKETS)
			DrawPic(xPos, yPos, sb_ammo[2] );
		else if (gHUD.items & IT_CELLS)
			DrawPic(xPos, yPos, sb_ammo[3] );*/

			// g-cont. hide ammo count on axe
		xPos = h_bar_x + 200;

		if (gHUD.stats[STAT_ACTIVEWEAPON] != IT_AXE)
			DrawNum(xPos, yPos, gHUD.stats[STAT_AMMO], NUM_DRAWRIGHT);
#else
		// ammo
		xPos = h_bar_x + 4;
		yPos = h_bar_y + 3;

		if ((gHUD.stats[STAT_ACTIVEWEAPON] != IT_AXE) && (gHUD.stats[STAT_ACTIVEWEAPON] != IT_CHAINSAW))
			DrawNum(xPos, yPos, gHUD.stats[STAT_AMMO], NUM_DRAWRIGHT);

		// health
		xPos = h_bar_x + 48;
		DrawNum(xPos, yPos, gHUD.stats[STAT_HEALTH], NUM_DRAWRIGHT | NUM_PERCENT);

		if (m_iMsgTime >= fTime && (CVAR_GET_FLOAT("doom_msg") >= 1))
			return 1;

		// mugshot
		if (gHUD.sb_lines > 0)
			DrawFace(fTime);

		// item
		//xPos = h_bar_x + 99;
		//DrawNum(xPos, yPos, gHUD.stats[STAT_WOLDTYPE], NUM_DRAWRIGHT);

		// sigil
		xPos = h_bar_x + 107;
		yPos = h_bar_y;
		HLSPRITE hsigil = SPR_Load("sprites/hud/sigil.spr");
		SPR_Set(hsigil, 255, 255, 255);

		for (int i = 0; i < 4; i++)
		{
			if (gHUD.items & (1 << (28 + i)))
				SPR_DrawHoles(i, xPos, yPos, NULL);
		}
		yPos = h_bar_y + 3; //set it back

		// armor
		xPos = h_bar_x + 179;
		DrawNum(xPos, yPos, gHUD.stats[STAT_ARMOR], NUM_DRAWRIGHT | NUM_PERCENT);

		// keys
		xPos = h_bar_x + 239;

		if (gHUD.items & IT_KEY1)
		{
			SPR_Set(sb_items[0], 255, 255, 255);

			if (gHUD.stats[STAT_WOLDTYPE] == WORLDTYPE_PRESENT)
				SPR_DrawHoles(0, xPos, yPos, NULL);
			else
				SPR_DrawHoles(3, xPos, yPos, NULL);
		}
		yPos = h_bar_y + 13;
		if (gHUD.items & IT_KEY2)
		{
			SPR_Set(sb_items[0], 255, 255, 255);

			if (gHUD.stats[STAT_WOLDTYPE] == WORLDTYPE_PRESENT)
				SPR_DrawHoles(1, xPos, yPos, NULL);
			else
				SPR_DrawHoles(4, xPos, yPos, NULL);
		}
		yPos = h_bar_y + 23;
		if (gHUD.items & IT_KEY3)
		{
			SPR_Set(sb_items[0], 255, 255, 255);

			if (gHUD.stats[STAT_WOLDTYPE] == WORLDTYPE_PRESENT)
				SPR_DrawHoles(2, xPos, yPos, NULL);
			else
				SPR_DrawHoles(5, xPos, yPos, NULL);
		}

		//ammo stash cur counts
		xPos = h_bar_x + 276;

		//nails
		yPos = h_bar_y + 5;
		DrawSmallNum(xPos, yPos, gHUD.stats[STAT_NAILS], NUM_DRAWRIGHT);

		//shells
		yPos = h_bar_y + 11;
		DrawSmallNum(xPos, yPos, gHUD.stats[STAT_SHELLS], NUM_DRAWRIGHT);

		//rockets
		yPos = h_bar_y + 17;
		DrawSmallNum(xPos, yPos, gHUD.stats[STAT_ROCKETS], NUM_DRAWRIGHT);

		//cells
		yPos = h_bar_y + 23;
		DrawSmallNum(xPos, yPos, gHUD.stats[STAT_CELLS], NUM_DRAWRIGHT);

		//ammo stash max counts
		xPos = h_bar_x + 302;

		if (gHUD.items & IT_BACKPACK)
			backpack = 2;

		//nails
		yPos = h_bar_y + 5;
		DrawSmallNum(xPos, yPos, IT_MAX_NAILS * backpack, NUM_DRAWRIGHT);

		//shells
		yPos = h_bar_y + 11;
		DrawSmallNum(xPos, yPos, IT_MAX_SHELLS * backpack, NUM_DRAWRIGHT);

		//rockets
		yPos = h_bar_y + 17;
		DrawSmallNum(xPos, yPos, IT_MAX_ROCKETS * backpack, NUM_DRAWRIGHT);

		//cells
		yPos = h_bar_y + 23;
		DrawSmallNum(xPos, yPos, IT_MAX_CELLS * backpack, NUM_DRAWRIGHT);
#endif
	}
	else
	{
		HLSPRITE htext = SPR_Load("sprites/hud/hud_txt.spr");
		
		// health
		SPR_SetAlpha(htext, 255, 255, 255, 178);

		xPos = 2;
		yPos = ScreenHeight - 10;
		SPR_DrawHoles(2, xPos, yPos, NULL);

		xPos = 24;
		yPos = ScreenHeight - 30;
		DrawIntNum(xPos, yPos, gHUD.stats[STAT_HEALTH], NUM_DRAWCENTER );

		// armor
		SPR_SetAlpha(htext, 255, 255, 255, 178);

		xPos = 58;
		yPos = ScreenHeight - 10;
		SPR_DrawHoles(1, xPos, yPos, NULL);

		xPos = 80;
		yPos = ScreenHeight - 30;
		DrawIntNum(xPos, yPos, gHUD.stats[STAT_ARMOR], NUM_DRAWCENTER);

		// ammo
		if ((gHUD.stats[STAT_ACTIVEWEAPON] != IT_AXE) && (gHUD.stats[STAT_ACTIVEWEAPON] != IT_CHAINSAW))
		{
			SPR_SetAlpha(htext, 255, 255, 255, 178);

			xPos = ScreenWidth - 58;
			yPos = ScreenHeight - 10;
			SPR_DrawHoles(0, xPos, yPos, NULL);

			xPos = ScreenWidth - 35;
			yPos = ScreenHeight - 30;
			DrawIntNum(xPos, yPos, gHUD.stats[STAT_AMMO], NUM_DRAWCENTER);
		}

		// keys
		SPR_SetAlpha(htext, 255, 255, 255, 178);

		xPos = ScreenWidth - 114;
		yPos = ScreenHeight - 10;
		SPR_DrawHoles(3, xPos, yPos, NULL);

		xPos += 12;
		yPos = ScreenHeight - 18;
		if (gHUD.items & IT_KEY1)
		{
			SPR_Set(sb_items[0], 255, 255, 255);

			if (gHUD.stats[STAT_WOLDTYPE] == WORLDTYPE_PRESENT)
				SPR_DrawHoles(0, xPos, yPos, NULL);
			else
				SPR_DrawHoles(3, xPos, yPos, NULL);
		}
		xPos += 12;
		if (gHUD.items & IT_KEY2)
		{
			SPR_Set(sb_items[0], 255, 255, 255);

			if (gHUD.stats[STAT_WOLDTYPE] == WORLDTYPE_PRESENT)
				SPR_DrawHoles(1, xPos, yPos, NULL);
			else
				SPR_DrawHoles(4, xPos, yPos, NULL);
		}
		xPos += 12;
		if (gHUD.items & IT_KEY3)
		{
			SPR_Set(sb_items[0], 255, 255, 255);

			if (gHUD.stats[STAT_WOLDTYPE] == WORLDTYPE_PRESENT)
				SPR_DrawHoles(2, xPos, yPos, NULL);
			else
				SPR_DrawHoles(5, xPos, yPos, NULL);
		}
	}
	return 1;
}

int CHudSBar::MsgFunc_Stats(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	int statnum = READ_BYTE();

	if( statnum < 0 || statnum >= MAX_STATS )
	{
		gEngfuncs.Con_Printf( "gmsgStats: bad stat %i\n", statnum );
		return 0;
	}

	// update selected stat
	gHUD.stats[statnum] = (unsigned int)READ_SHORT();

	return 1;
}

int CHudSBar::MsgFunc_Items(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	unsigned int newItems = (unsigned int)READ_LONG();

	if (gHUD.items != newItems)
	{
		// set flash times
		/*for (int i = 0; i < 32; i++)
			if(( newItems & ( 1<<i )) && !( gHUD.items & ( 1<<i )))
				gHUD.item_gettime[i] = gEngfuncs.GetClientTime();*/

		for (int i = 0; i < 8; i++)
		{
			if ((newItems & IT_SHOTGUN << i) && !(gHUD.items & IT_SHOTGUN << i)) //only srubs weapons from shotgun onwards and no items due to bit placement
			{
				//gEngfuncs.Con_Printf("picked up weapon\n");
				gHUD.m_fFacePickup = gEngfuncs.GetClientTime() + 1.5f;
			}
		}

		gHUD.items = newItems;
	}

	return 1;
}

int CHudSBar::MsgFunc_LevelName(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	strncpy(levelname, READ_STRING(), sizeof(levelname) - 1);

	return 1;
}

int CHudSBar::MsgFunc_HudMessage(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	strncpy(c_MsgRow3, c_MsgRow2, sizeof(c_MsgRow3) - 1);
	strncpy(c_MsgRow2, c_MsgRow1, sizeof(c_MsgRow2) - 1);

	strncpy(c_MsgRow1, READ_STRING(), sizeof(c_MsgRow1) - 1);

	m_iMsgTime = gEngfuncs.GetClientTime() + 2.1f;

	return 1;
}

int CHudSBar::MsgFunc_FoundSecret(const char* pszName, int iSize, void* pbuf)
{
	gHUD.stats[STAT_SECRETS]++;
	return 1;
}

int CHudSBar::MsgFunc_KillMonster(const char* pszName, int iSize, void* pbuf)
{
	gHUD.stats[STAT_MONSTERS]++;
	return 1;
}

int CHudSBar::MsgFunc_FoundItem(const char* pszName, int iSize, void* pbuf)
{
	gHUD.stats[STAT_ITEMS]++;
	return 1;
}

int CHudSBar::MsgFunc_Finale( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	if (gpViewParams)
		gpViewParams->intermission = 2;

	return 1;
}