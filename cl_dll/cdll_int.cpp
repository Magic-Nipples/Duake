//
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include "hud.h"
#include "cl_util.h"
#include "entity_state.h"
#include "usercmd.h"

extern "C"
{
#include "pm_shared.h"
}

#include <string.h>
#include <windows.h>

#define DLLEXPORT __declspec( dllexport )


cl_enginefunc_t gEngfuncs;
CHud gHUD;
int g_iXashEngine = FALSE;

void InitInput (void);
void EV_HookEvents( void );
void IN_Commands( void );

/*
========================== 
    Initialize

Called when the DLL is first loaded.
==========================
*/
extern "C" 
{
int	DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion );
int	DLLEXPORT HUD_VidInit( void );
void	DLLEXPORT HUD_Init( void );
int	DLLEXPORT HUD_Redraw( float flTime, int intermission );
int	DLLEXPORT HUD_UpdateClientData( client_data_t *cdata, float flTime );
void	DLLEXPORT HUD_Reset ( void );
void	DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server );
void	DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove );
char	DLLEXPORT HUD_PlayerMoveTexture( char *name );
int	DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
void	DLLEXPORT HUD_PostRunCmd( local_state_t *from, local_state_t *to, usercmd_t *cmd, int runfuncs, double time, unsigned int random_seed );
int	DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void	DLLEXPORT HUD_Frame( double time );
void	DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking);
void	DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf );
void	DLLEXPORT Demo_ReadBuffer( int size, unsigned char *buffer );
void	DLLEXPORT HUD_DrawNormalTriangles( void );
void	DLLEXPORT HUD_DrawTransparentTriangles( void );
}

/*
================================
HUD_GetRect

Vgui stub
================================
*/
int *HUD_GetRect( void )
{
	RECT wrect;
	static int extent[4];

	if( GetWindowRect( GetActiveWindow(), &wrect ))
          {
		if( !wrect.left )
		{
			extent[0] = wrect.left;	//+4
			extent[1] = wrect.top;	//+30
			extent[2] = wrect.right;	//-4
			extent[3] = wrect.bottom;	//-4
		}
		else
		{
			extent[0] = wrect.left + 4;	//+4
			extent[1] = wrect.top + 30;	//+30
			extent[2] = wrect.right - 4;	//-4
			extent[3] = wrect.bottom - 4;	//-4
		}
	}
	return extent;	
}

/*
================================
HUD_GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;

	switch ( hullnumber )
	{
	case 0:				// Normal player
		Vector(-16, -16, -24).CopyToArray( mins );
		Vector( 16,  16,  32).CopyToArray( maxs );
		iret = 1;
		break;
	case 1:
	case 2:				// Point based hull
		Vector( 0, 0, 0 ).CopyToArray( mins );
		Vector( 0, 0, 0 ).CopyToArray( maxs );
		iret = 1;
		break;
	case 3:				// Large hull
		Vector(-32, -32, -24 ).CopyToArray( mins );
		Vector( 32,  32,  64 ).CopyToArray( maxs );
		iret = 1;
		break;
	}

	return iret;
}

/*
================================
HUD_ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void DLLEXPORT HUD_PostRunCmd( local_state_t *from, local_state_t *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed )
{
}

void DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove )
{
	PM_Init( ppmove );
}

char DLLEXPORT HUD_PlayerMoveTexture( char *name )
{
	return PM_FindTextureType( name );
}

void DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server )
{
	PM_Move( ppmove, server );
}

int DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion )
{
	gEngfuncs = *pEnginefuncs;

	if (iVersion != CLDLL_INTERFACE_VERSION)
		return 0;

	memcpy(&gEngfuncs, pEnginefuncs, sizeof(cl_enginefunc_t));

	if (gEngfuncs.pfnGetCvarPointer( "host_clientloaded" ))
		g_iXashEngine = TRUE;

	return 1;
}


/*
==========================
	HUD_VidInit

Called when the game initializes
and whenever the vid_mode is changed
so the HUD can reinitialize itself.
==========================
*/
int DLLEXPORT HUD_VidInit( void )
{
	gHUD.VidInit();

	return 1;
}

/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/
void DLLEXPORT HUD_Init( void )
{
	InitInput();
	gHUD.Init();
}


/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/
int DLLEXPORT HUD_Redraw( float time, int intermission )
{
	gHUD.Redraw( time, intermission );

	return 1;
}


/*
==========================
	HUD_UpdateClientData

called every time shared client
dll/engine data gets changed,
and gives the cdll a chance
to modify the data.

returns 1 if anything has been changed, 0 otherwise.
==========================
*/
int DLLEXPORT HUD_UpdateClientData(client_data_t *pcldata, float flTime )
{
	IN_Commands();

	return gHUD.UpdateClientData(pcldata, flTime );
}

/*
==========================
	HUD_Reset

Called at start and end of demos to restore to "non"HUD state.
==========================
*/
void DLLEXPORT HUD_Reset( void )
{
	gHUD.VidInit();
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/

void DLLEXPORT HUD_Frame( double time )
{
	gEngfuncs.VGui_ViewportPaintBackground( HUD_GetRect( ));
}

/*
==========================
HUD_VoiceStatus

Called when a player starts or stops talking.
==========================
*/

void DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking)
{
}

/*
==========================
HUD_DirectorEvent

Called when a director event message was received
==========================
*/

void DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf )
{
}

/*
=====================
Demo_ReadBuffer

Engine wants us to parse some data from the demo stream
=====================
*/
void DLLEXPORT Demo_ReadBuffer( int size, unsigned char *buffer )
{
}

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles( void )
{
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
#include "cl_entity.h"
#include "triangleapi.h"
#include <GL/gl.h>
#include "gl/glext.h"

#define FADESIDES 8192

void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
	cl_entity_t* player;
	Vector camForward, camRight, camUp;
	vec3_t viewangles;
	int i, dist;
	float fade;

	if (CVAR_GET_FLOAT("doom_fog") > 0)
	{
		player = gEngfuncs.GetLocalPlayer();
		if (!player)
			return;

		if (gHUD.m_iWaterLevel > 2)
			return;

		gEngfuncs.GetViewAngles((float*)viewangles);
		AngleVectors(viewangles, camForward, camRight, camUp);

		for (i = 0; i < 14; i++)
		{
			switch (i)
			{
			case 0: dist = 1024;	fade = 0.18;	break;
			case 1: dist = 960;		fade = 0.16;	break;
			case 2: dist = 896;		fade = 0.14;	break;
			case 3: dist = 832;		fade = 0.12;	break;
			case 4: dist = 768;		fade = 0.10;	break;
			case 5: dist = 704;		fade = 0.09;	break;
			case 6: dist = 640;		fade = 0.08;	break;
			case 7: dist = 576;		fade = 0.07;	break;
			case 8: dist = 512;		fade = 0.06;	break;
			case 9:	dist = 448;		fade = 0.05;	break;
			case 10: dist = 384;	fade = 0.04;	break;
			case 11: dist = 320;	fade = 0.03;	break;
			case 12: dist = 256;	fade = 0.02;	break;
			case 13: dist = 192;	fade = 0.01;	break;
			}

			float* beansupleft = player->origin + (camForward * dist) + (camRight * FADESIDES) + (camUp * FADESIDES);
			float* beansupright = player->origin + (camForward * dist) + (camRight * -FADESIDES) + (camUp * FADESIDES);
			float* beansdownright = player->origin + (camForward * dist) + (camRight * -FADESIDES) + (camUp * -FADESIDES);
			float* beansdownleft = player->origin + (camForward * dist) + (camRight * FADESIDES) + (camUp * -FADESIDES);

			// Create a triangle, sigh
			glDisable(GL_TEXTURE_2D);
			//glEnable(GL_CULL_FACE);
			glEnable(GL_BLEND);

			glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(GL_ZERO, GL_ZERO, GL_ZERO, fade);

			glDisable(GL_CULL_FACE);
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3fv(beansupleft);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3fv(beansupright);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3fv(beansdownright);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3fv(beansdownleft);
			glEnd();

			glEnable(GL_CULL_FACE);
			glDisable(GL_BLEND);
			glEnable(GL_TEXTURE_2D);
		}
	}
}