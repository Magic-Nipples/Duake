/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "extdll.h"
#include "basemenu.h"
#include "utils.h"
#include "keydefs.h"
#include "menu_btnsbmp_table.h"
#include "menu_strings.h"

#define ART_MINIMIZE_N	"gfx/shell/min_n"
#define ART_MINIMIZE_F	"gfx/shell/min_f"
#define ART_MINIMIZE_D	"gfx/shell/min_d"
#define ART_CLOSEBTN_N	"gfx/shell/cls_n"
#define ART_CLOSEBTN_F	"gfx/shell/cls_f"
#define ART_CLOSEBTN_D	"gfx/shell/cls_d"

//#define GRAPHIC_LOGO		"gfx/shell/logo_doom"
#define GRAPHIC_LOGO		"gfx/shell/logo1a"
#define GRAPHIC_LOGO_B		"gfx/shell/logo1b"
#define GRAPHIC_LOGO2		"gfx/shell/logo2a"
#define GRAPHIC_LOGO2_B		"gfx/shell/logo2b"
//#define GRAPHIC_LOGO_B		"gfx/shell/logo_doom_b"
#define GRAPHIC_QUAKE		"gfx/shell/logo_quake"

#define BTN_CONSOLE		"gfx/shell/buttons/btn_console"
#define BTN_RESUME		"gfx/shell/buttons/btn_resumegame"
#define BTN_NEWGAME		"gfx/shell/buttons/btn_newgame"
#define BTN_TESTMAP		"gfx/shell/buttons/btn_testmap"
#define BTN_LOADGAME	"gfx/shell/buttons/btn_loadgame"
#define BTN_SAVEGAME	"gfx/shell/buttons/btn_savegame"
#define BTN_OPTIONS		"gfx/shell/buttons/btn_options"
#define BTN_QUIT		"gfx/shell/buttons/btn_quit"

#define QMSG_1			"gfx/shell/quit_msg_1"
#define QMSG_2			"gfx/shell/quit_msg_2"
#define QMSG_3			"gfx/shell/quit_msg_3"
#define QMSG_4			"gfx/shell/quit_msg_4"
#define QMSG_5			"gfx/shell/quit_msg_5"
#define QMSG_6			"gfx/shell/quit_msg_6"
#define QMSG_7			"gfx/shell/quit_msg_7"
#define QMSG_8			"gfx/shell/quit_msg_8"

#define NEW_MSG			"gfx/shell/new_msg"

#define ID_BACKGROUND		0
#define ID_CONSOLE			1
#define ID_RESUME			2
#define ID_NEWGAME			3
#define ID_HAZARDCOURSE		4
#define ID_CONFIGURATION	5
#define ID_LOADSAVE			6
#define ID_SAVEGAME			7
#define ID_QUIT				8
#define ID_QUITTEXT			10
#define ID_MSGTEXT	 		11
#define ID_YES	 			12
#define ID_NO	 			13

#define ID_LOGO		 		14
#define ID_QUAKE	 		15

#define IT_SLIDE			16

typedef struct
{
	//menuPicButton_s
	menuFramework_s	menu;

	menuBitmap_s	background;

	menuBitmap_s	logo;
	menuBitmap_s	quake;

	menuBitmap_s	console;

	menuBitmap_s	resumeGame;
	menuBitmap_s	newGame;
	menuBitmap_s	hazardCourse;
	menuBitmap_s	configuration;

	menuBitmap_s	loadSave;
	menuBitmap_s	saveGame;
	menuBitmap_s	quit;

	menuBitmap_s	quitMessage1;
	menuBitmap_s	dlgMessage1;

	menuBitmap_s	yes;
	menuBitmap_s	no;

	menuSlider_s	slidertest;
} uiMain_t;

static uiMain_t		uiMain;

/*
=================
UI_Background_Ownerdraw
=================
*/
static void UI_Background_Ownerdraw( void *self )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	// map has background
	if( CVAR_GET_FLOAT( "cl_background" ))
		return;

	UI_DrawBackground_Callback( self );
}

static void UI_QuitDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide quit dialog
	switch (RANDOM_LONG(0, 7))
	{
	case 0: uiMain.quitMessage1.pic = QMSG_1; break;
	case 1: uiMain.quitMessage1.pic = QMSG_2; break;
	case 2: uiMain.quitMessage1.pic = QMSG_3; break;
	case 3: uiMain.quitMessage1.pic = QMSG_4; break;
	case 4: uiMain.quitMessage1.pic = QMSG_5; break;
	case 5: uiMain.quitMessage1.pic = QMSG_6; break;
	case 6: uiMain.quitMessage1.pic = QMSG_7; break;
	case 7: uiMain.quitMessage1.pic = QMSG_8; break;
	}

	uiMain.logo.generic.flags ^= QMF_HIDDEN;
	uiMain.quake.generic.flags ^= QMF_HIDDEN;

	uiMain.console.generic.flags ^= QMF_HIDDEN;

	if (CL_IsActive())
	{
		uiMain.resumeGame.generic.flags ^= QMF_HIDDEN;
		uiMain.saveGame.generic.flags ^= QMF_HIDDEN;
	}

	uiMain.newGame.generic.flags ^= QMF_HIDDEN;
	uiMain.hazardCourse.generic.flags ^= QMF_HIDDEN;
	uiMain.loadSave.generic.flags ^= QMF_HIDDEN;
	uiMain.configuration.generic.flags ^= QMF_HIDDEN;
	uiMain.quit.generic.flags ^= QMF_HIDDEN;

	uiMain.quitMessage1.generic.flags ^= QMF_HIDDEN;
	uiMain.no.generic.flags ^= QMF_HIDDEN;
	uiMain.yes.generic.flags ^= QMF_HIDDEN;

}

static void UI_PromptDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide quit dialog
	uiMain.logo.generic.flags ^= QMF_HIDDEN;
	uiMain.quake.generic.flags ^= QMF_HIDDEN;

	uiMain.console.generic.flags ^= QMF_HIDDEN;

	if (CL_IsActive())
	{
		uiMain.resumeGame.generic.flags ^= QMF_HIDDEN;
		uiMain.saveGame.generic.flags ^= QMF_HIDDEN;
	}

	uiMain.newGame.generic.flags ^= QMF_HIDDEN;
	uiMain.hazardCourse.generic.flags ^= QMF_HIDDEN;
	uiMain.loadSave.generic.flags ^= QMF_HIDDEN;
	uiMain.configuration.generic.flags ^= QMF_HIDDEN;
	uiMain.quit.generic.flags ^= QMF_HIDDEN;

	uiMain.dlgMessage1.generic.flags ^= QMF_HIDDEN;
	uiMain.no.generic.flags ^= QMF_HIDDEN;
	uiMain.yes.generic.flags ^= QMF_HIDDEN;

}

static void UI_EscapeHide(void)
{
	uiMain.logo.generic.flags ^= QMF_HIDDEN;
	uiMain.quake.generic.flags ^= QMF_HIDDEN;
	uiMain.console.generic.flags ^= QMF_HIDDEN;

	if (CL_IsActive())
	{
		uiMain.resumeGame.generic.flags ^= QMF_HIDDEN;
		uiMain.saveGame.generic.flags ^= QMF_HIDDEN;
	}

	uiMain.newGame.generic.flags ^= QMF_HIDDEN;
	uiMain.hazardCourse.generic.flags ^= QMF_HIDDEN;
	uiMain.loadSave.generic.flags ^= QMF_HIDDEN;
	uiMain.configuration.generic.flags ^= QMF_HIDDEN;
	uiMain.quit.generic.flags ^= QMF_HIDDEN;
}

/*
=================
UI_Main_KeyFunc
=================
*/
static const char *UI_Main_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE )
	{
		if (CL_IsActive())
		{
			if(!( uiMain.dlgMessage1.generic.flags & QMF_HIDDEN ))
				UI_PromptDialog();
			else if(!( uiMain.quitMessage1.generic.flags & QMF_HIDDEN ))
				UI_QuitDialog();
			else
			{
				UI_StartSound(uiSoundOut);
				UI_CloseMenu();
			}
		}
		else //UI_QuitDialog();
		{
			if (!(uiMain.quitMessage1.generic.flags & QMF_HIDDEN))
				UI_QuitDialog();
			else
				UI_EscapeHide();
		}

		if (!CL_IsActive())
		{
			if (uiMain.logo.generic.flags & QMF_HIDDEN)
				return uiSoundHide;
			else
				return uiSoundUnhide;
		}
		else
		{
			return uiSoundNull;
		}

	}
	return UI_DefaultKey( &uiMain.menu, key, down );
}

/*
=================
UI_Main_ActivateFunc
=================
*/
static void UI_Main_ActivateFunc( void )
{
	if (!CL_IsActive())
	{
		uiMain.resumeGame.generic.flags |= QMF_HIDDEN;
		uiMain.saveGame.generic.flags |= QMF_HIDDEN;
	}
}

static void UI_Main_NewGame(void)
{
	if (CVAR_GET_FLOAT("host_serverstate") && CVAR_GET_FLOAT("maxplayers") > 1)
		HOST_ENDGAME("end of the game");

	CVAR_SET_FLOAT("skill", 1.0f);
	CVAR_SET_FLOAT("deathmatch", 0.0f);
	CVAR_SET_FLOAT("teamplay", 0.0f);
	CVAR_SET_FLOAT("pausable", 1.0f); // singleplayer is always allowing pause
	CVAR_SET_FLOAT("coop", 0.0f);

	BACKGROUND_TRACK(NULL, NULL);

	CVAR_SET_FLOAT("v_dark", 1.0f);
	CLIENT_COMMAND(FALSE, "newgame\n");
}

/*
=================
UI_Main_HazardCourse
=================
*/
static void UI_Main_HazardCourse( void )
{
	if( CVAR_GET_FLOAT( "host_serverstate" ) && CVAR_GET_FLOAT( "maxplayers" ) > 1 )
		HOST_ENDGAME( "end of the game" );

	CVAR_SET_FLOAT( "skill", 1.0f );
	CVAR_SET_FLOAT( "deathmatch", 0.0f );
	CVAR_SET_FLOAT( "teamplay", 0.0f );
	CVAR_SET_FLOAT( "pausable", 1.0f ); // singleplayer is always allowing pause
	CVAR_SET_FLOAT( "coop", 0.0f );

	BACKGROUND_TRACK( NULL, NULL );

	CLIENT_COMMAND( FALSE, "hazardcourse\n" );
}

/*
=================
UI_Main_Callback
=================
*/
static void UI_Main_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_CONSOLE:
		UI_SetActiveMenu( FALSE );
		KEY_SetDest( KEY_CONSOLE );
		break;
	case ID_RESUME:
		UI_CloseMenu();
		break;
	case ID_NEWGAME:
		if (CL_IsActive())
			UI_PromptDialog();
		else UI_Main_NewGame();
		break;
	case ID_HAZARDCOURSE:
		UI_Main_HazardCourse();
		break;
	case ID_CONFIGURATION:
		UI_Options_Menu();
		break;
	case ID_LOADSAVE:
		UI_LoadGame_Menu();
		break;
	case ID_SAVEGAME:
		UI_SaveGame_Menu();
		break;
	case ID_QUIT:
		UI_QuitDialog();
		break;
	case ID_YES:
		if (!(uiMain.quitMessage1.generic.flags & QMF_HIDDEN))
		{
			CLIENT_COMMAND(FALSE, "wait; wait; wait; wait; wait; wait; wait; wait; wait; wait\n");
			CLIENT_COMMAND(FALSE, "wait; wait; wait; wait; wait; wait; wait; wait; wait; wait\n");
			CLIENT_COMMAND(FALSE, "wait; wait; wait; wait; wait; wait; wait; wait; wait; wait\n");
			CLIENT_COMMAND(FALSE, "quit\n");
		}
		else UI_Main_NewGame();
		break;
	case ID_NO:
		if( !( uiMain.quitMessage1.generic.flags & QMF_HIDDEN ))
			UI_QuitDialog();
		else UI_PromptDialog();
		break;
	}
}

/*
=================
UI_Main_Init
=================
*/
static void UI_Main_Init( void )
{
	bool bTrainMap;
	bool bResume = false;

	if (CL_IsActive())
		bResume = true;

	memset( &uiMain, 0, sizeof( uiMain_t ));

	// training map is present and not equal to startmap
	if( strlen( gMenu.m_gameinfo.trainmap ) && _stricmp( gMenu.m_gameinfo.trainmap, gMenu.m_gameinfo.startmap ))
		bTrainMap = true;
	else bTrainMap = false;

	uiMain.menu.vidInitFunc = UI_Main_Init;
	uiMain.menu.keyFunc = UI_Main_KeyFunc;
	uiMain.menu.activateFunc = UI_Main_ActivateFunc;

	uiMain.background.generic.id = ID_BACKGROUND;
	uiMain.background.generic.type = QMTYPE_BITMAP_STRETCH;
	uiMain.background.generic.flags = QMF_INACTIVE;
	uiMain.background.generic.x = 0;
	uiMain.background.generic.y = 0;
	uiMain.background.generic.width = 1024;
	uiMain.background.generic.height = 768;
	uiMain.background.pic = ART_BACKGROUND;
	uiMain.background.generic.ownerdraw = UI_Background_Ownerdraw;

	uiMain.logo.generic.id = ID_LOGO;
	uiMain.logo.generic.type = QMTYPE_BITMAP;
	uiMain.logo.generic.flags = QMF_INACTIVE | QMF_DRAW_ADDHOLES | QMF_CENTERED;
	uiMain.logo.generic.x = 0;
	uiMain.logo.generic.y = 10;
	uiMain.logo.generic.width = 160;//128;
	uiMain.logo.generic.height = 52;//64;

	if (RANDOM_LONG(0, 50) == 50)
	{
		uiMain.logo.pic = GRAPHIC_LOGO2;
		uiMain.logo.focusPic = GRAPHIC_LOGO2_B;
	}
	else
	{
		uiMain.logo.pic = GRAPHIC_LOGO;
		uiMain.logo.focusPic = GRAPHIC_LOGO_B;
	}
	

	uiMain.quake.generic.id = ID_QUAKE;
	uiMain.quake.generic.type = QMTYPE_BITMAP;
	uiMain.quake.generic.flags = QMF_INACTIVE | QMF_CENTERED;
	uiMain.quake.generic.x = -100;
	uiMain.quake.generic.y = 42;
	uiMain.quake.generic.width = 32;
	uiMain.quake.generic.height = 154;//144;
	uiMain.quake.pic = GRAPHIC_QUAKE;

	// console
	uiMain.console.generic.id = ID_CONSOLE;
	uiMain.console.generic.type = QMTYPE_BITMAP;
	uiMain.console.generic.name = "Console";
	uiMain.console.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES;
	uiMain.console.generic.x = 2;
	uiMain.console.generic.y = 2;
	uiMain.console.generic.width = 56;
	uiMain.console.generic.height = 8;
	uiMain.console.generic.callback = UI_Main_Callback;
	uiMain.console.pic = BTN_CONSOLE;
	uiMain.console.generic.color = uiColorDoomRed;
	uiMain.console.generic.focusColor = uiColorDoomSelect;

	// resume game
	uiMain.resumeGame.generic.id = ID_RESUME;
	uiMain.resumeGame.generic.type = QMTYPE_BITMAP;
	uiMain.resumeGame.generic.name = "Resume game";
	uiMain.resumeGame.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.resumeGame.generic.x = UI_MAINSELECTION_POSX;
	uiMain.resumeGame.generic.y = 72;
	uiMain.resumeGame.generic.width = 152;
	uiMain.resumeGame.generic.height = 24;
	uiMain.resumeGame.generic.callback = UI_Main_Callback;
	uiMain.resumeGame.pic = BTN_RESUME;
	uiMain.resumeGame.generic.color = uiColorDoomRed;
	uiMain.resumeGame.generic.focusColor = uiColorDoomSelect;

	// new game
	uiMain.newGame.generic.id = ID_NEWGAME;
	uiMain.newGame.generic.type = QMTYPE_BITMAP;
	uiMain.newGame.generic.name = "New game";
	uiMain.newGame.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.newGame.generic.x = UI_MAINSELECTION_POSX;
	uiMain.newGame.generic.y = bResume ? 90 : 72;
	uiMain.newGame.generic.width = 152;
	uiMain.newGame.generic.height = 24;
	uiMain.newGame.generic.callback = UI_Main_Callback;
	uiMain.newGame.pic = BTN_NEWGAME;
	uiMain.newGame.generic.color = uiColorDoomRed;
	uiMain.newGame.generic.focusColor = uiColorDoomSelect;

	// testmap
	uiMain.hazardCourse.generic.id = ID_HAZARDCOURSE;
	uiMain.hazardCourse.generic.type = QMTYPE_BITMAP;
	uiMain.hazardCourse.generic.name = "Test map";
	uiMain.hazardCourse.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.hazardCourse.generic.x = UI_MAINSELECTION_POSX;
	uiMain.hazardCourse.generic.y = bResume ? 108 : 90;
	uiMain.hazardCourse.generic.width = 152;
	uiMain.hazardCourse.generic.height = 24;
	uiMain.hazardCourse.generic.callback = UI_Main_Callback;
	uiMain.hazardCourse.pic = BTN_TESTMAP;
	uiMain.hazardCourse.generic.color = uiColorDoomRed;
	uiMain.hazardCourse.generic.focusColor = uiColorDoomSelect;

	//uiMain.loadSave.generic.id = ID_LOADSAVE;
	//uiMain.loadSave.generic.type = QMTYPE_BM_BUTTON;
	//uiMain.loadSave.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_NOTIFY;

	// server.dll needs for reading savefiles or startup newgame
	if( !CheckGameDll( ))
	{
		uiMain.loadSave.generic.flags |= QMF_GRAYED;
		uiMain.hazardCourse.generic.flags |= QMF_GRAYED;
		uiMain.newGame.generic.flags |= QMF_GRAYED;
	}

	// load
	uiMain.loadSave.generic.id = ID_LOADSAVE;
	uiMain.loadSave.generic.type = QMTYPE_BITMAP;
	uiMain.loadSave.generic.name = "Load game";
	uiMain.loadSave.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.loadSave.generic.x = UI_MAINSELECTION_POSX;
	uiMain.loadSave.generic.y = (bResume) ? (bTrainMap ? 126 : 108) : (bTrainMap ? 108 : 90);
	uiMain.loadSave.generic.width = 152;
	uiMain.loadSave.generic.height = 24;
	uiMain.loadSave.generic.callback = UI_Main_Callback;
	uiMain.loadSave.pic = BTN_LOADGAME;
	uiMain.loadSave.generic.color = uiColorDoomRed;
	uiMain.loadSave.generic.focusColor = uiColorDoomSelect;

	// save
	uiMain.saveGame.generic.id = ID_SAVEGAME;
	uiMain.saveGame.generic.type = QMTYPE_BITMAP;
	uiMain.saveGame.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.saveGame.generic.x = UI_MAINSELECTION_POSX;
	uiMain.saveGame.generic.y = (bResume) ? (bTrainMap ? 144 : 126) : (bTrainMap ? 126 : 108);
	uiMain.saveGame.generic.width = 152;
	uiMain.saveGame.generic.height = 24;
	uiMain.saveGame.generic.callback = UI_Main_Callback;
	uiMain.saveGame.pic = BTN_SAVEGAME;
	uiMain.saveGame.generic.color = uiColorDoomRed;
	uiMain.saveGame.generic.focusColor = uiColorDoomSelect;

	// options
	uiMain.configuration.generic.id = ID_CONFIGURATION;
	uiMain.configuration.generic.type = QMTYPE_BITMAP;
	uiMain.configuration.generic.name = "Options";
	uiMain.configuration.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.configuration.generic.x = UI_MAINSELECTION_POSX;
	uiMain.configuration.generic.y = (bResume) ? (bTrainMap ? 162 : 144) : (bTrainMap ? 126 : 108);
	uiMain.configuration.generic.width = 152;
	uiMain.configuration.generic.height = 24;
	uiMain.configuration.generic.callback = UI_Main_Callback;
	uiMain.configuration.pic = BTN_OPTIONS;
	uiMain.configuration.generic.color = uiColorDoomRed;
	uiMain.configuration.generic.focusColor = uiColorDoomSelect;

	// quit
	uiMain.quit.generic.id = ID_QUIT;
	uiMain.quit.generic.type = QMTYPE_BITMAP;
	uiMain.quit.generic.name = "Quit game";
	uiMain.quit.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.quit.generic.x = UI_MAINSELECTION_POSX;
	uiMain.quit.generic.y = (bResume) ? (bTrainMap ? 182 : 164) : (bTrainMap ? 146 : 128);
	uiMain.quit.generic.width = 152;
	uiMain.quit.generic.height = 24;
	uiMain.quit.generic.callback = UI_Main_Callback;
	uiMain.quit.pic = BTN_QUIT;
	uiMain.quit.generic.color = uiColorDoomRed;
	uiMain.quit.generic.focusColor = uiColorDoomSelect;

	uiMain.quitMessage1.generic.id = ID_QUITTEXT;
	uiMain.quitMessage1.generic.type = QMTYPE_BITMAP;
	uiMain.quitMessage1.generic.name = "Quit message lump";
	uiMain.quitMessage1.generic.flags = QMF_HIDDEN | QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.quitMessage1.generic.x = 0;
	uiMain.quitMessage1.generic.y = 84;
	uiMain.quitMessage1.generic.width = 256;
	uiMain.quitMessage1.generic.height = 24;
	uiMain.quitMessage1.pic = QMSG_1;
	uiMain.quitMessage1.generic.color = uiColorDoomRed;

	uiMain.dlgMessage1.generic.id = ID_MSGTEXT;
	uiMain.dlgMessage1.generic.type = QMTYPE_BITMAP;
	uiMain.dlgMessage1.generic.name = "New message lump";
	uiMain.dlgMessage1.generic.flags = QMF_HIDDEN | QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.dlgMessage1.generic.x = 0;
	uiMain.dlgMessage1.generic.y = 84;
	uiMain.dlgMessage1.generic.width = 256;
	uiMain.dlgMessage1.generic.height = 24;
	uiMain.dlgMessage1.pic = NEW_MSG;
	uiMain.dlgMessage1.generic.color = uiColorDoomRed;

	uiMain.yes.generic.id = ID_YES;
	uiMain.yes.generic.type = QMTYPE_BITMAP;
	uiMain.yes.generic.name = "Yes";
	uiMain.yes.generic.flags = QMF_HIDDEN | QMF_HIGHLIGHTIFFOCUS | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.yes.generic.x = -UI_YESNO_POSX;
	uiMain.yes.generic.y = 128;
	uiMain.yes.generic.width = 56;
	uiMain.yes.generic.height = 24;
	uiMain.yes.generic.callback = UI_Main_Callback;
	uiMain.yes.pic = BTN_YES;
	uiMain.yes.generic.color = uiColorDoomRed;
	uiMain.yes.generic.focusColor = uiColorDoomSelect;

	uiMain.no.generic.id = ID_NO;
	uiMain.no.generic.type = QMTYPE_BITMAP;
	uiMain.no.generic.name = "No";
	uiMain.no.generic.flags = QMF_HIDDEN | QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiMain.no.generic.x = UI_YESNO_POSX;
	uiMain.no.generic.y = 128;
	uiMain.no.generic.width = 48;
	uiMain.no.generic.height = 24;
	uiMain.no.generic.callback = UI_Main_Callback;
	uiMain.no.pic = BTN_NO;
	uiMain.no.generic.color = uiColorDoomRed;
	uiMain.no.generic.focusColor = uiColorDoomSelect;

	uiMain.slidertest.generic.id = IT_SLIDE;
	uiMain.slidertest.generic.type = QMTYPE_SLIDER;
	uiMain.slidertest.generic.flags = QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiMain.slidertest.generic.name = "test slider";
	uiMain.slidertest.generic.x = 0;
	uiMain.slidertest.generic.y = 25;
	uiMain.slidertest.generic.callback = UI_Main_Callback;
	uiMain.slidertest.generic.statusText = "Set master volume level";
	uiMain.slidertest.minValue = 0.0;
	uiMain.slidertest.maxValue = 1.0;
	uiMain.slidertest.range = 0.05f;

	UI_AddItem(&uiMain.menu, (void*)&uiMain.background);

	UI_AddItem(&uiMain.menu, (void*)&uiMain.logo);
	//UI_AddItem(&uiMain.menu, (void*)&uiMain.quake);

	if (gpGlobals->allow_console)
		UI_AddItem(&uiMain.menu, (void*)&uiMain.console);

	UI_AddItem(&uiMain.menu, (void*)&uiMain.resumeGame);
	UI_AddItem(&uiMain.menu, (void*)&uiMain.newGame);

	if (bTrainMap)
		UI_AddItem(&uiMain.menu, (void*)&uiMain.hazardCourse);

	UI_AddItem(&uiMain.menu, (void*)&uiMain.loadSave);
	UI_AddItem(&uiMain.menu, (void*)&uiMain.saveGame);
	UI_AddItem(&uiMain.menu, (void*)&uiMain.configuration);

	UI_AddItem(&uiMain.menu, (void*)&uiMain.quit);
	UI_AddItem(&uiMain.menu, (void*)&uiMain.quitMessage1);
	UI_AddItem(&uiMain.menu, (void*)&uiMain.dlgMessage1);
	UI_AddItem(&uiMain.menu, (void*)&uiMain.no);
	UI_AddItem(&uiMain.menu, (void*)&uiMain.yes);

	//UI_AddItem(&uiMain.menu, (void*)&uiMain.slidertest);
}

/*
=================
UI_Main_Precache
=================
*/
void UI_Main_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_MINIMIZE_N );
	PIC_Load( ART_MINIMIZE_F );
	PIC_Load( ART_MINIMIZE_D );
	PIC_Load( ART_CLOSEBTN_N );
	PIC_Load( ART_CLOSEBTN_F );
	PIC_Load( ART_CLOSEBTN_D );

	PIC_Load(QMSG_1, TF_NEAREST | TF_NOMIPMAP);
	PIC_Load(QMSG_2, TF_NEAREST | TF_NOMIPMAP);
	PIC_Load(QMSG_3, TF_NEAREST | TF_NOMIPMAP);
	PIC_Load(QMSG_4, TF_NEAREST | TF_NOMIPMAP);
	PIC_Load(QMSG_5, TF_NEAREST | TF_NOMIPMAP);
	PIC_Load(QMSG_6, TF_NEAREST | TF_NOMIPMAP);
	PIC_Load(QMSG_7, TF_NEAREST | TF_NOMIPMAP);
	PIC_Load(QMSG_8, TF_NEAREST | TF_NOMIPMAP);
}

/*
=================
UI_Main_Menu
=================
*/
void UI_Main_Menu( void )
{
	UI_Main_Precache();
	UI_Main_Init();

	UI_PushMenu( &uiMain.menu );
	UI_StartSound(uiSoundLaunch);
}