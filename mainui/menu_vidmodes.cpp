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
#include "menu_btnsbmp_table.h"
#include "menu_strings.h"

#define HEADER_VIDEO    	"gfx/shell/headers/header_video"

#define LST_RES				"gfx/shell/lists/list_res"

#define SHEAD_BRIGHT		"gfx/shell/slider/slide_bright"
#define SHEAD_GAMMA			"gfx/shell/slider/slide_gamma"
#define BTN_VSYNC			"gfx/shell/buttons/btn_sync"
#define BTN_MSAA			"gfx/shell/buttons/btn_msaa"
#define BTN_WINDOW			"gfx/shell/buttons/btn_window"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_VIDMODELIST	2
#define ID_BRIGHTNESS	3
#define ID_GAMMA		4
#define ID_VERTICALSYNC	5
#define ID_MSAA			6
#define ID_FULLSCREEN	7
#define ID_APPLY		8
#define ID_DONE			9

#define ID_CWIDTH		10
#define ID_CHEIGHT		11

#define MAX_VIDMODES	65

typedef struct
{
	const char	*videoModesPtr[MAX_VIDMODES];

	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuScrollList_s	vidList;
	menuSlider_s		brightness;
	menuSlider_s		gamma;
	menuCheckBox_s		vsync;
	menuCheckBox_s		msaa;
	menuCheckBox_s		windowed;

	menuBitmap_s	ok;
	menuBitmap_s	cancel;

	menuField_s	cwidth;
	menuField_s	cheight;
} uiVidModes_t;

static uiVidModes_t	uiVidModes;

/*
=================
UI_VidModes_GetModesList
=================
*/
static void UI_VidModes_GetConfig( void )
{
	int i;

	strncpy(uiVidModes.cwidth.buffer, CVAR_GET_STRING("custom_width"), sizeof(uiVidModes.cwidth.buffer));
	strncpy(uiVidModes.cheight.buffer, CVAR_GET_STRING("custom_height"), sizeof(uiVidModes.cheight.buffer));

	for( i = 0; i < MAX_VIDMODES-1; i++ )
	{
		const char* chr;
		chr = VID_GET_MODE( i );

		if (i == 0)
			chr = "^3[Custom Size]^7";

		uiVidModes.videoModesPtr[i] = chr;

		if( !uiVidModes.videoModesPtr[i] )
			break; // end of list
	}

	uiVidModes.videoModesPtr[i] = NULL;	// terminator

	uiVidModes.vidList.itemNames = uiVidModes.videoModesPtr;
	uiVidModes.vidList.curItem = CVAR_GET_FLOAT( "vid_mode" );

	if (CVAR_GET_FLOAT("fullscreen"))
		uiVidModes.windowed.focusPic = SM_OFF;
	else
		uiVidModes.windowed.enabled = 1;

	if (CVAR_GET_FLOAT("gl_vsync"))
	{
		uiVidModes.vsync.enabled = 1;
		uiVidModes.vsync.focusPic = SM_ON;
	}

	if (CVAR_GET_FLOAT("gl_msaa"))
	{
		uiVidModes.msaa.enabled = 1;
		uiVidModes.msaa.focusPic = SM_ON;
	}

	uiVidModes.brightness.curValue = RemapVal(CVAR_GET_FLOAT("r_lighting_modulate"), 0.25f, 1.25f, 0.0f, 1.0f);
	uiVidModes.gamma.curValue = RemapVal(CVAR_GET_FLOAT("gamma"), 0.0f, 3.0f, 0.0f, 1.0f);
}

/*
=================
UI_VidModes_SetConfig
=================
*/
static void UI_VidOptions_SetConfig( void )
{
	CVAR_SET_STRING("custom_width", uiVidModes.cwidth.buffer); //uses string but input box is numbers only so it should be safe :)
	CVAR_SET_STRING("custom_height", uiVidModes.cheight.buffer);

	if (CVAR_GET_FLOAT("custom_width") < 320)
		CVAR_SET_FLOAT("custom_width", 320);

	if (CVAR_GET_FLOAT("custom_height") < 240)
		CVAR_SET_FLOAT("custom_height", 240);

	CVAR_SET_FLOAT("vid_mode", uiVidModes.vidList.curItem);
	CVAR_SET_FLOAT("fullscreen", !uiVidModes.windowed.enabled);
	CVAR_SET_FLOAT("gl_vsync", uiVidModes.vsync.enabled);
	CVAR_SET_FLOAT("gl_msaa", uiVidModes.msaa.enabled);

	CVAR_SET_FLOAT("r_lighting_modulate", RemapVal(uiVidModes.brightness.curValue, 0.0f, 1.0f, 0.25f, 1.25f));
	CVAR_SET_FLOAT("gamma", RemapVal(uiVidModes.gamma.curValue, 0.0f, 1.0f, 0.0f, 3.0f));
}

/*
=================
UI_VidModes_UpdateConfig
=================
*/
static void UI_VidOptions_UpdateConfig( void )
{
	CVAR_SET_FLOAT("gl_vsync", uiVidModes.vsync.enabled);
	CVAR_SET_FLOAT("gl_msaa", uiVidModes.msaa.enabled);
	CVAR_SET_FLOAT("r_lighting_modulate", RemapVal(uiVidModes.brightness.curValue, 0.0f, 1.0f, 0.25f, 1.25f));
	CVAR_SET_FLOAT("gamma", RemapVal(uiVidModes.gamma.curValue, 0.0f, 1.0f, 0.0f, 3.0f));
}

/*
=================
UI_VidModes_Callback
=================
*/
static void UI_VidModes_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
	case ID_FULLSCREEN:
	case ID_VERTICALSYNC:
	case ID_MSAA:
		if (((menuCheckBox_s*)self)->enabled)
			((menuCheckBox_s*)self)->focusPic = SM_ON;
		else
			((menuCheckBox_s*)self)->focusPic = SM_OFF;
		break;
	}

	if( event == QM_CHANGED )
	{
		UI_VidOptions_UpdateConfig();
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_PopMenu();
		break;
	case ID_APPLY:
		UI_VidOptions_SetConfig ();
		//UI_PopMenu();
		break;
	}
}

/*
=================
UI_VidModes_Init
=================
*/
static void UI_VidModes_Init( void )
{
	memset( &uiVidModes, 0, sizeof( uiVidModes_t ));

	uiVidModes.menu.vidInitFunc = UI_VidModes_Init;
	
	uiVidModes.background.generic.id = ID_BACKGROUND;
	uiVidModes.background.generic.type = QMTYPE_BITMAP_STRETCH;
	uiVidModes.background.generic.flags = QMF_INACTIVE;
	uiVidModes.background.generic.x = 0;
	uiVidModes.background.generic.y = 0;
	uiVidModes.background.generic.width = 1024;
	uiVidModes.background.generic.height = 768;
	uiVidModes.background.pic = ART_BACKGROUND;

	uiVidModes.banner.generic.id = ID_BANNER;
	uiVidModes.banner.generic.type = QMTYPE_BITMAP;
	uiVidModes.banner.generic.flags = QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiVidModes.banner.generic.x = UI_BANNER_POSX;
	uiVidModes.banner.generic.y = UI_BANNER_POSY;
	uiVidModes.banner.generic.width = 216;
	uiVidModes.banner.generic.height = 24;
	uiVidModes.banner.pic = HEADER_VIDEO;
	uiVidModes.banner.generic.color = uiColorDoomRed;

	// resoltutions
	uiVidModes.vidList.generic.id = ID_VIDMODELIST;
	uiVidModes.vidList.generic.type = QMTYPE_SCROLLLIST;
	uiVidModes.vidList.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_SMALLFONT;
	uiVidModes.vidList.generic.x = 86; //80
	uiVidModes.vidList.generic.y = 64;
	uiVidModes.vidList.generic.width = 110;
	uiVidModes.vidList.generic.height = 200;
	uiVidModes.vidList.generic.callback = UI_VidModes_Callback;
	uiVidModes.vidList.pic = LST_RES;

	// brightness
	uiVidModes.brightness.generic.id = ID_BRIGHTNESS;
	uiVidModes.brightness.generic.type = QMTYPE_SLIDER;
	uiVidModes.brightness.generic.flags = QMF_CENTERED;
	uiVidModes.brightness.generic.name = "Brightness";
	uiVidModes.brightness.generic.x = -48 - 6;
	uiVidModes.brightness.generic.y = 58;
	uiVidModes.brightness.generic.callback = UI_VidModes_Callback;
	uiVidModes.brightness.minValue = 0.0;
	uiVidModes.brightness.maxValue = 1.0;
	uiVidModes.brightness.range = 0.05f;
	uiVidModes.brightness.pic = SHEAD_BRIGHT;

	// gamma
	uiVidModes.gamma.generic.id = ID_GAMMA;
	uiVidModes.gamma.generic.type = QMTYPE_SLIDER;
	uiVidModes.gamma.generic.flags = QMF_CENTERED;
	uiVidModes.gamma.generic.name = "Gamma";
	uiVidModes.gamma.generic.x = -48 - 6;
	uiVidModes.gamma.generic.y = 90;
	uiVidModes.gamma.generic.callback = UI_VidModes_Callback;
	uiVidModes.gamma.minValue = 0.0;
	uiVidModes.gamma.maxValue = 1.0;
	uiVidModes.gamma.range = 0.05f;
	uiVidModes.gamma.pic = SHEAD_GAMMA;

	// vsync
	uiVidModes.vsync.generic.id = ID_VERTICALSYNC;
	uiVidModes.vsync.generic.type = QMTYPE_CHECKBOX;
	uiVidModes.vsync.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiVidModes.vsync.generic.name = "Vertical sync";
	uiVidModes.vsync.generic.x = -32 - 6;
	uiVidModes.vsync.generic.y = 112;
	uiVidModes.vsync.generic.width = 160;
	uiVidModes.vsync.generic.height = 8;
	uiVidModes.vsync.generic.callback = UI_VidModes_Callback;
	uiVidModes.vsync.pic = BTN_VSYNC;

	uiVidModes.vsync.generic.x2 = 56;
	uiVidModes.vsync.generic.width2 = 32;
	uiVidModes.vsync.focusPic = SM_OFF;

	uiVidModes.msaa.generic.id = ID_MSAA;
	uiVidModes.msaa.generic.type = QMTYPE_CHECKBOX;
	uiVidModes.msaa.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiVidModes.msaa.generic.name = "msaa";
	uiVidModes.msaa.generic.x = -32 - 6;
	uiVidModes.msaa.generic.y = 124;
	uiVidModes.msaa.generic.width = 160;
	uiVidModes.msaa.generic.height = 8;
	uiVidModes.msaa.generic.callback = UI_VidModes_Callback;
	uiVidModes.msaa.pic = BTN_MSAA;

	uiVidModes.msaa.generic.x2 = 56;
	uiVidModes.msaa.generic.width2 = 32;
	uiVidModes.msaa.focusPic = SM_OFF;

	// windowed
	uiVidModes.windowed.generic.id = ID_FULLSCREEN;
	uiVidModes.windowed.generic.type = QMTYPE_CHECKBOX;
	uiVidModes.windowed.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiVidModes.windowed.generic.name = "windowed";
	uiVidModes.windowed.generic.x = -32 - 6;
	uiVidModes.windowed.generic.y = 124; //136;
	uiVidModes.windowed.generic.width = 160;
	uiVidModes.windowed.generic.height = 8;
	uiVidModes.windowed.generic.callback = UI_VidModes_Callback;
	uiVidModes.windowed.pic = BTN_WINDOW;

	uiVidModes.windowed.generic.x2 = 56;
	uiVidModes.windowed.generic.width2 = 32;
	uiVidModes.windowed.focusPic = SM_ON;

	// apply
	uiVidModes.ok.generic.id = ID_APPLY;
	uiVidModes.ok.generic.type = QMTYPE_BITMAP;
	uiVidModes.ok.generic.name = "Apply";
	uiVidModes.ok.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiVidModes.ok.generic.x = -72 - 6;
	uiVidModes.ok.generic.y = 182;
	uiVidModes.ok.generic.width = 80;
	uiVidModes.ok.generic.height = 16;
	uiVidModes.ok.generic.callback = UI_VidModes_Callback;
	uiVidModes.ok.pic = BTN_APPLY;
	uiVidModes.ok.generic.color = uiColorDoomRed;
	uiVidModes.ok.generic.focusColor = uiColorDoomSelect;

	// done
	uiVidModes.cancel.generic.id = ID_DONE;
	uiVidModes.cancel.generic.type = QMTYPE_BITMAP;
	uiVidModes.cancel.generic.name = "Apply";
	uiVidModes.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiVidModes.cancel.generic.x = -72 - 6;
	uiVidModes.cancel.generic.y = 200;
	uiVidModes.cancel.generic.width = 80;
	uiVidModes.cancel.generic.height = 16;
	uiVidModes.cancel.generic.callback = UI_VidModes_Callback;
	uiVidModes.cancel.pic = BTN_DONES;
	uiVidModes.cancel.generic.color = uiColorDoomRed;
	uiVidModes.cancel.generic.focusColor = uiColorDoomSelect;

	uiVidModes.cwidth.generic.name = "Width";
	uiVidModes.cwidth.generic.id = ID_CWIDTH;
	uiVidModes.cwidth.generic.type = QMTYPE_FIELD;
	uiVidModes.cwidth.generic.flags = QMF_CENTER_JUSTIFY | QMF_HIGHLIGHTIFFOCUS | QMF_DROPSHADOW | QMF_NUMBERSONLY | QMF_SMALLFONT;
	uiVidModes.cwidth.generic.x = 84;
	uiVidModes.cwidth.generic.y = 420;
	uiVidModes.cwidth.generic.width = 100;
	uiVidModes.cwidth.generic.height = 20;
	uiVidModes.cwidth.generic.callback = UI_VidModes_Callback;
	uiVidModes.cwidth.generic.statusText = "Custom resolution width";
	uiVidModes.cwidth.maxLength = 4;

	uiVidModes.cheight.generic.name = "Height";
	uiVidModes.cheight.generic.id = ID_CWIDTH;
	uiVidModes.cheight.generic.type = QMTYPE_FIELD;
	uiVidModes.cheight.generic.flags = QMF_CENTER_JUSTIFY | QMF_HIGHLIGHTIFFOCUS | QMF_DROPSHADOW | QMF_NUMBERSONLY | QMF_SMALLFONT;
	uiVidModes.cheight.generic.x = 84;
	uiVidModes.cheight.generic.y = 480;
	uiVidModes.cheight.generic.width = 100;
	uiVidModes.cheight.generic.height = 20;
	uiVidModes.cheight.generic.callback = UI_VidModes_Callback;
	uiVidModes.cheight.generic.statusText = "Custom resolution height";
	uiVidModes.cheight.maxLength = 4;

	UI_VidModes_GetConfig();

	UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.background);
	UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.banner);
	UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.vidList);
	UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.brightness);
	UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.gamma);

	UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.windowed);
	UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.vsync);
	//UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.msaa);

	UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.ok);
	UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.cancel);

	if (uiVidModes.vidList.curItem <= 0)
	{
		UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.cwidth);
		UI_AddItem(&uiVidModes.menu, (void*)&uiVidModes.cheight);
	}
}

/*
=================
UI_VidModes_Precache
=================
*/
void UI_VidModes_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load(HEADER_VIDEO, TF_NEAREST|TF_NOMIPMAP);
}

/*
=================
UI_VidModes_Menu
=================
*/
void UI_VidModes_Menu( void )
{
	UI_VidModes_Precache();
	UI_VidModes_Init();

	UI_PushMenu( &uiVidModes.menu );
}