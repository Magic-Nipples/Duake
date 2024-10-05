//
// Message.cpp
//
// implementation of CHudMessage class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"

DECLARE_MESSAGE( m_Message, HudText )
DECLARE_MESSAGE( m_Message, GameTitle )

#define MAX_CUSTOM_MESSAGES	8	// g-cont. Should be enough...

// 1 Global client_textmessage_t for custom messages that aren't in the titles.txt
client_textmessage_t	g_pCustomMessage[MAX_CUSTOM_MESSAGES];
char			g_pCustomText[MAX_CUSTOM_MESSAGES][1024];

char g_pCustomMessageName[MAX_CUSTOM_MESSAGES][16] =
{
"Custom0",
"Custom1",
"Custom2",
"Custom3",
"Custom4",
"Custom5",
"Custom6",
"Custom7",
}; 

int CHudMessage::Init(void)
{
	HOOK_MESSAGE( HudText );
	HOOK_MESSAGE( GameTitle );

	gHUD.AddHudElem(this);

	Reset();

	return 1;
};

int CHudMessage::VidInit( void )
{
	m_HUD_title_half = gHUD.GetSpriteIndex( "title_half" );
	m_HUD_title_life = gHUD.GetSpriteIndex( "title_life" );
	m_iFlags |= HUD_INTERMISSION;	// g-cont. allow episode finales

	return 1;
};


void CHudMessage::Reset( void )
{
 	memset( m_pMessages, 0, sizeof( m_pMessages[0] ) * maxHUDMessages );
	memset( m_startTime, 0, sizeof( m_startTime[0] ) * maxHUDMessages );
	
	m_gameTitleTime = 0;
	m_pGameTitle = NULL;

	// initialize text messages (game_text)
	for( int i = 0; i < MAX_CUSTOM_MESSAGES; i++ )
	{
		g_pCustomMessage[i].pName = g_pCustomMessageName[i];
		g_pCustomMessage[i].pMessage = g_pCustomText[i];
	}

	m_index = 0;
}


float CHudMessage::FadeBlend( float fadein, float fadeout, float hold, float localTime )
{
	float fadeTime = fadein + hold;
	float fadeBlend;

	if ( localTime < 0 )
		return 0;

	if ( localTime < fadein )
	{
		fadeBlend = 1 - ((fadein - localTime) / fadein);
	}
	else if ( localTime > fadeTime )
	{
		if ( fadeout > 0 )
			fadeBlend = 1 - ((localTime - fadeTime) / fadeout);
		else
			fadeBlend = 0;
	}
	else
		fadeBlend = 1;

	return fadeBlend;
}


int	CHudMessage::XPosition( float x, int width, int totalWidth )
{
	int xPos;

	if ( x == -1 )
	{
		xPos = (ScreenMessageWidth - width) / 2;
	}
	else
	{
		if ( x < 0 )
			xPos = (1.0 + x) * ScreenMessageWidth - totalWidth;	// Alight right
		else
			xPos = x * ScreenMessageWidth;
	}

	if ( xPos + width > ScreenMessageWidth )
		xPos = ScreenMessageWidth - width;
	else if ( xPos < 0 )
		xPos = 0;

	return xPos;
}


int CHudMessage::YPosition( float y, int height )
{
	int yPos;

	if (y == -1)
		yPos = (ScreenMessageHeight - height) * 0.5; //QUAKE
	else
	{
		// Alight bottom?
		if (y < 0)
			yPos = (1.0 + y) * ScreenMessageHeight - height;	// Alight bottom
		else // align top
			yPos = y * ScreenMessageHeight;
	}

	if ( yPos + height > ScreenMessageHeight )
		yPos = ScreenMessageHeight - height;
	else if ( yPos < 0 )
		yPos = 0;

	return yPos;
}


void CHudMessage::MessageScanNextChar( void )
{
	int srcRed, srcGreen, srcBlue, destRed, destGreen, destBlue;
	int blend;

	srcRed = m_parms.pMessage->r1;
	srcGreen = m_parms.pMessage->g1;
	srcBlue = m_parms.pMessage->b1;
	blend = 0;	// Pure source

	switch( m_parms.pMessage->effect )
	{
	// Fade-in / Fade-out
	case 0:
	case 1:
		destRed = destGreen = destBlue = 0;
		blend = m_parms.fadeBlend;
		break;

	case 2:
		m_parms.charTime += m_parms.pMessage->fadein;
		if ( m_parms.charTime > m_parms.time )
		{
			srcRed = srcGreen = srcBlue = 0;
			blend = 0;	// pure source
		}
		else
		{
			float deltaTime = m_parms.time - m_parms.charTime;

			destRed = destGreen = destBlue = 0;
			if ( m_parms.time > m_parms.fadeTime )
			{
				blend = m_parms.fadeBlend;
			}
			else if ( deltaTime > m_parms.pMessage->fxtime )
				blend = 0;	// pure dest
			else
			{
				destRed = m_parms.pMessage->r2;
				destGreen = m_parms.pMessage->g2;
				destBlue = m_parms.pMessage->b2;
				blend = 255 - (deltaTime * (1.0/m_parms.pMessage->fxtime) * 255.0 + 0.5);
			}
		}
		break;
	}
	if ( blend > 255 )
		blend = 255;
	else if ( blend < 0 )
		blend = 0;

	m_parms.r = ((srcRed * (255-blend)) + (destRed * blend)) >> 8;
	m_parms.g = ((srcGreen * (255-blend)) + (destGreen * blend)) >> 8;
	m_parms.b = ((srcBlue * (255-blend)) + (destBlue * blend)) >> 8;

	if ( m_parms.pMessage->effect == 1 && m_parms.charTime != 0 )
	{
		if ( m_parms.x >= 0 && m_parms.y >= 0 && (m_parms.x + gHUD.m_scrinfo.charWidths[ m_parms.text ]) <= ScreenMessageWidth )
			TextMessageDrawChar( m_parms.x, m_parms.y, m_parms.text, m_parms.pMessage->r2, m_parms.pMessage->g2, m_parms.pMessage->b2 );
	}
}


void CHudMessage::MessageScanStart( void )
{
	switch( m_parms.pMessage->effect )
	{
	// Fade-in / out with flicker
	case 1:
	case 0:
		m_parms.fadeTime = m_parms.pMessage->fadein + m_parms.pMessage->holdtime;
		

		if ( m_parms.time < m_parms.pMessage->fadein )
		{
			m_parms.fadeBlend = ((m_parms.pMessage->fadein - m_parms.time) * (1.0/m_parms.pMessage->fadein) * 255);
		}
		else if ( m_parms.time > m_parms.fadeTime )
		{
			if ( m_parms.pMessage->fadeout > 0 )
				m_parms.fadeBlend = (((m_parms.time - m_parms.fadeTime) / m_parms.pMessage->fadeout) * 255);
			else
				m_parms.fadeBlend = 255; // Pure dest (off)
		}
		else
			m_parms.fadeBlend = 0;	// Pure source (on)
		m_parms.charTime = 0;

		if ( m_parms.pMessage->effect == 1 && (rand()%100) < 10 )
			m_parms.charTime = 1;
		break;

	case 2:
		m_parms.fadeTime = (m_parms.pMessage->fadein * m_parms.length) + m_parms.pMessage->holdtime;
		
		//QUAKE - no fading on messages. just stop
		//if ( m_parms.time > m_parms.fadeTime && m_parms.pMessage->fadeout > 0 )
		//	m_parms.fadeBlend = (((m_parms.time - m_parms.fadeTime) / m_parms.pMessage->fadeout) * 255);
		//else
			m_parms.fadeBlend = 0;
		break;
	}
}


void CHudMessage::MessageDrawScan( client_textmessage_t *pMessage, float time )
{
	int i, j, length, width;
	const char *pText;
	unsigned char line[80];

	pText = pMessage->pMessage;
	// Count lines
	m_parms.lines = 1;
	m_parms.time = time;
	m_parms.pMessage = pMessage;
	length = 0;
	width = 0;
	m_parms.totalWidth = 0;
	while ( *pText )
	{
		if ( *pText == '\n' )
		{
			m_parms.lines++;
			if ( width > m_parms.totalWidth )
				m_parms.totalWidth = width;
			width = 0;
		}
		else
			width += gHUD.m_scrinfo.charWidths[*pText];
		pText++;
		length++;
	}
	m_parms.length = length;
	m_parms.totalHeight = (m_parms.lines * gHUD.m_scrinfo.iCharHeight);


	m_parms.y = YPosition( pMessage->y, m_parms.totalHeight );
	pText = pMessage->pMessage;

	m_parms.charTime = 0;

	MessageScanStart();

	for ( i = 0; i < m_parms.lines; i++ )
	{
		m_parms.lineLength = 0;
		m_parms.width = 0;
		while ( *pText && *pText != '\n' )
		{
			unsigned char c = *pText;
			line[m_parms.lineLength] = c;
			m_parms.width += gHUD.m_scrinfo.charWidths[c];
			m_parms.lineLength++;
			pText++;
		}
		pText++;		// Skip LF
		line[m_parms.lineLength] = 0;

		m_parms.x = XPosition( pMessage->x, m_parms.width, m_parms.totalWidth );

		for ( j = 0; j < m_parms.lineLength; j++ )
		{
			m_parms.text = line[j];
			int next = m_parms.x + gHUD.m_scrinfo.charWidths[ m_parms.text ];
			MessageScanNextChar();
			
			if ( m_parms.x >= 0 && m_parms.y >= 0 && next <= ScreenMessageWidth )
				TextMessageDrawChar( m_parms.x, m_parms.y, m_parms.text, m_parms.r, m_parms.g, m_parms.b );
			m_parms.x = next;
		}

		m_parms.y += gHUD.m_scrinfo.iCharHeight;
	}
}


int CHudMessage::Draw( float fTime )
{
	int i, drawn;
	client_textmessage_t *pMessage;
	float endTime;

	drawn = 0;

	if ( m_gameTitleTime > 0 )
	{
		float localTime = gHUD.m_flTime - m_gameTitleTime;
		float brightness;

		// Maybe timer isn't set yet
		if ( m_gameTitleTime > gHUD.m_flTime )
			m_gameTitleTime = gHUD.m_flTime;

		if ( localTime > (m_pGameTitle->fadein + m_pGameTitle->holdtime + m_pGameTitle->fadeout) )
			m_gameTitleTime = 0;
		else
		{
			brightness = FadeBlend( m_pGameTitle->fadein, m_pGameTitle->fadeout, m_pGameTitle->holdtime, localTime );

			int halfWidth = gHUD.GetSpriteRect(m_HUD_title_half).right - gHUD.GetSpriteRect(m_HUD_title_half).left;
			int fullWidth = halfWidth + gHUD.GetSpriteRect(m_HUD_title_life).right - gHUD.GetSpriteRect(m_HUD_title_life).left;
			int fullHeight = gHUD.GetSpriteRect(m_HUD_title_half).bottom - gHUD.GetSpriteRect(m_HUD_title_half).top;

			int x = XPosition( m_pGameTitle->x, fullWidth, fullWidth );
			int y = YPosition( m_pGameTitle->y, fullHeight );


			SPR_Set( gHUD.GetSprite(m_HUD_title_half), brightness * m_pGameTitle->r1, brightness * m_pGameTitle->g1, brightness * m_pGameTitle->b1 );
			SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect(m_HUD_title_half) );

			SPR_Set( gHUD.GetSprite(m_HUD_title_life), brightness * m_pGameTitle->r1, brightness * m_pGameTitle->g1, brightness * m_pGameTitle->b1 );
			SPR_DrawAdditive( 0, x + halfWidth, y, &gHUD.GetSpriteRect(m_HUD_title_life) );

			drawn = 1;
		}
	}
	// Fixup level transitions
	for ( i = 0; i < maxHUDMessages; i++ )
	{
		// Assume m_parms.time contains last time
		if ( m_pMessages[i] )
		{
			pMessage = m_pMessages[i];
			if ( m_startTime[i] > gHUD.m_flTime )
				m_startTime[i] = gHUD.m_flTime + m_parms.time - m_startTime[i] + 0.2;	// Server takes 0.2 seconds to spawn, adjust for this
		}
	}

	for ( i = 0; i < maxHUDMessages; i++ )
	{
		if ( m_pMessages[i] )
		{
			pMessage = m_pMessages[i];

			// This is when the message is over
			switch( pMessage->effect )
			{
			case 0:
			case 1:
				endTime = m_startTime[i] + pMessage->fadein + pMessage->fadeout + pMessage->holdtime;
				break;
			
			// Fade in is per character in scanning messages
			case 2:
				endTime = m_startTime[i] + (pMessage->fadein * strlen( pMessage->pMessage )) + pMessage->fadeout + pMessage->holdtime;
				break;
			}

			if ( fTime <= endTime )
			{
				float messageTime = fTime - m_startTime[i];

				// Draw the message
				// effect 0 is fade in/fade out
				// effect 1 is flickery credits
				// effect 2 is write out (training room)
				MessageDrawScan( pMessage, messageTime );

				drawn++;
			}
			else
			{
				// The message is over
				m_pMessages[i] = NULL;
			}
		}
	}

	// Remember the time -- to fix up level transitions
	m_parms.time = gHUD.m_flTime;
	// Don't call until we get another message
	if ( !drawn )
		m_iFlags &= ~HUD_ACTIVE;

	return 1;
}


void CHudMessage::MessageAdd( const char *pName, float time )
{
	int i,j;
	client_textmessage_t *tempMessage;

	for ( i = 0; i < maxHUDMessages; i++ )
	{
		if ( !m_pMessages[i] )
		{
			// Trim off a leading # if it's there
			if ( pName[0] == '#' ) 
				tempMessage = TextMessageGet( pName+1 );
			else
				tempMessage = TextMessageGet( pName );
			// If we couldnt find it in the titles.txt, just create it
			if ( !tempMessage )
			{
				// g-cont. a circular message buffer
				m_index = (m_index + 1) & (MAX_CUSTOM_MESSAGES - 1);
				tempMessage = &g_pCustomMessage[m_index];

				tempMessage->effect = 2; //0 is more like QUAKE but 2 looks better
				tempMessage->r1 = tempMessage->g1 = tempMessage->b1 = tempMessage->a1 = 255;
				tempMessage->r2 = 255;
				tempMessage->g2 = 0;
				tempMessage->b2 = 0;
				tempMessage->a2 = 0;
				tempMessage->x = -1;		// Centered
				tempMessage->y = 0.5;
				tempMessage->fadein = 0.01;
				tempMessage->fadeout = 0;//1.5;
				tempMessage->fxtime = 0.25;
				tempMessage->holdtime = 2.5; //5
				strcpy( (char *)tempMessage->pMessage, pName );
			}

			for ( j = 0; j < maxHUDMessages; j++ )
			{
				if ( m_pMessages[j] )
				{
					// is this message already in the list
					if ( !strcmp( tempMessage->pMessage, m_pMessages[j]->pMessage ) )
					{
						return;
					}

					// get rid of any other messages in same location (only one displays at a time)
					if ( fabs( tempMessage->y - m_pMessages[j]->y ) < 0.0001 )
					{
						if ( fabs( tempMessage->x - m_pMessages[j]->x ) < 0.0001 )
						{
							m_pMessages[j] = NULL;
						}
					}
				}
			}

			m_pMessages[i] = tempMessage;
			m_startTime[i] = time;
			return;
		}
	}
}


int CHudMessage::MsgFunc_HudText( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	char *pString = READ_STRING();

	MessageAdd( pString, gHUD.m_flTime );
	// Remember the time -- to fix up level transitions
	m_parms.time = gHUD.m_flTime;

	// Turn on drawing
	if ( !(m_iFlags & HUD_ACTIVE) )
		m_iFlags |= HUD_ACTIVE;

	return 1;
}


int CHudMessage::MsgFunc_GameTitle( const char *pszName,  int iSize, void *pbuf )
{
	m_pGameTitle = TextMessageGet( "GAMETITLE" );
	if ( m_pGameTitle != NULL )
	{
		m_gameTitleTime = gHUD.m_flTime;

		// Turn on drawing
		if ( !(m_iFlags & HUD_ACTIVE) )
			m_iFlags |= HUD_ACTIVE;
	}

	return 1;
}

void CHudMessage::MessageAdd(client_textmessage_t * newMessage )
{
	m_parms.time = gHUD.m_flTime;

	// Turn on drawing
	if ( !(m_iFlags & HUD_ACTIVE) )
		m_iFlags |= HUD_ACTIVE;
	
	for ( int i = 0; i < maxHUDMessages; i++ )
	{
		if ( !m_pMessages[i] )
		{
			m_pMessages[i] = newMessage;
			m_startTime[i] = gHUD.m_flTime;
			return;
		}
	}

}
