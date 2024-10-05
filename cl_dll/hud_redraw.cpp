//
// hud_redraw.cpp
//

#include <math.h>
#include "hud.h"
#include "cl_util.h"

#include <string.h>
#include <windows.h>
#include <GL/gl.h>
#include "gl/glext.h"


// Think
void CHud::Think(void)
{
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if (pList->p->m_iFlags & HUD_ACTIVE)
			pList->p->Think();
		pList = pList->pNext;
	}

	// think about default fov
	if ( m_iFOV == 0 )
	{
		// only let players adjust up in fov,  and only if they are not overriden by something else
		m_iFOV = max( default_fov->value, 90 );  
	}
}

// Redraw
// step through the local data,  placing the appropriate graphics & text as appropriate
// returns 1 if they've changed, 0 otherwise
int CHud :: Redraw( float flTime, int intermission )
{
	int xPos, yPos;

	GLint m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);
	float yscale = fabs(2400000 / m_viewport[3]) * 0.0001;

	m_fOldTime = m_flTime;	// save time of previous redraw
	m_flTime = flTime;
	m_flTimeDelta = (double)m_flTime - m_fOldTime;

	// Clock was reset, reset delta
	if ( m_flTimeDelta < 0 )
		m_flTimeDelta = 0;

	m_iIntermission = intermission;

	m_fFrameTimeAdjust = (1.0f / gHUD.m_flTimeDelta) * 0.01;

	// if no redrawing is necessary
	// return 0;

	// draw the classic Quake crosshair
	if ((gHUD.stats[STAT_ACTIVEWEAPON] != IT_AXE) && (gHUD.stats[STAT_ACTIVEWEAPON] != IT_CHAINSAW))
	{
		if (m_pCvarCrosshair->value && !m_iIntermission && !m_iHideHUDDisplay && stats[STAT_HEALTH] > 0)
		{
			char crossChar = '+';

			xPos = (ScreenMessageWidth * 0.5) - (gHUD.m_scrinfo.charWidths[crossChar] * 0.5);
			yPos = ((gHUD.m_iViewportHeight * yscale) * 0.5) - (gHUD.m_scrinfo.iCharHeight * 0.5) + 1;
			TextMessageDrawChar(xPos, yPos, crossChar, 255, 255, 255);
		}
	}
	
	if ( m_pCvarDraw->value )
	{
		HUDLIST *pList = m_pHudList;

		while (pList)
		{
			if ( !intermission )
			{
				if ( (pList->p->m_iFlags & HUD_ACTIVE) && !(m_iHideHUDDisplay & HIDEHUD_ALL) )
					pList->p->Draw(flTime);
			}
			else
			{  // it's an intermission,  so only draw hud elements that are set to draw during intermissions
				if ( pList->p->m_iFlags & HUD_INTERMISSION )
					pList->p->Draw( flTime );
			}

			pList = pList->pNext;
		}
	}

	return 1;
}

int CHud :: DrawHudString(int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b )
{
	// draw the string until we hit the null character or a newline character
	for ( ; *szIt != 0 && *szIt != '\n'; szIt++ )
	{
		int next = xpos + gHUD.m_scrinfo.charWidths[ *szIt ]; // variable-width fonts look cool
		if ( next > iMaxX )
			return xpos;

		TextMessageDrawChar( xpos, ypos, *szIt, r, g, b );
		xpos = next;		
	}

	return xpos;
}

int CHud :: DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b )
{
	char szString[32];
	sprintf( szString, "%d", iNumber );
	return DrawHudStringReverse( xpos, ypos, iMinX, szString, r, g, b );

}

// draws a string from right to left (right-aligned)
int CHud :: DrawHudStringReverse( int xpos, int ypos, int iMinX, char *szString, int r, int g, int b )
{
	char* szIt;
	// find the end of the string
	for ( szIt = szString; *szIt != 0; szIt++ )
	{ // we should count the length?		
	}

	// iterate throug the string in reverse
	for ( szIt--;  szIt != (szString-1);  szIt-- )	
	{
		int next = xpos - gHUD.m_scrinfo.charWidths[ *szIt ]; // variable-width fonts look cool
		if ( next < iMinX )
			return xpos;
		xpos = next;

		TextMessageDrawChar( xpos, ypos, *szIt, r, g, b );
	}

	return xpos;
}

int CHud::DrawNewString(int xpos, int ypos, char* szIt, int r, int g, int b, int a)
{
	char test[128];
	int m_HUD_Char;
	int nexXpos = 0;
	// draw the string until we hit the null character or a newline character

	if (xpos == -1) //request test auto center
	{
		char* copy = szIt;
		for (; *copy != 0 && *copy != '\n'; copy++)
		{
			sprintf(test, "char_%i", *copy);
			m_HUD_Char = GetSpriteIndex(test);
			int width = GetSpriteRect(m_HUD_Char).right - GetSpriteRect(m_HUD_Char).left;

			nexXpos += width;
		}

		nexXpos = int(ScreenWidth * 0.5) - int(nexXpos * 0.5);
		//gEngfuncs.Con_Printf("ScreenWidth: %i | Xpos: %i\n", ScreenWidth, nexXpos);
	}
	else
	{
		nexXpos = xpos;
	}

	for (; *szIt != 0 && *szIt != '\n'; szIt++)
	{
		sprintf(test, "char_%i", *szIt);//char(*szIt));
		//gEngfuncs.Con_Printf("%s\n", test); //tell you what number char_ is

		m_HUD_Char = GetSpriteIndex(test);
		int next = nexXpos + GetSpriteRect(m_HUD_Char).right - GetSpriteRect(m_HUD_Char).left;

		SPR_Set(GetSprite(m_HUD_Char), r, g, b);
		SPR_DrawHoles(0, nexXpos, ypos, &GetSpriteRect(m_HUD_Char));

		nexXpos = next;
	}

	return xpos;
}