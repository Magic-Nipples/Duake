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

#define ART_BANNER			"gfx/shell/headers/header_gameplay"

#define SPIN_FOV			"gfx/shell/spin/spin_fov"
#define SPIN_VROLL			"gfx/shell/spin/spin_vroll"
#define SPIN_RES			"gfx/shell/spin/spin_res"

#define BTN_RUN				"gfx/shell/buttons/btn_run"
#define BTN_CROSSHAIR		"gfx/shell/buttons/btn_crosshair"
#define BTN_HUD				"gfx/shell/buttons/btn_hud"
#define BTN_VLAG			"gfx/shell/buttons/btn_vlag"
#define BTN_PALETTE			"gfx/shell/buttons/btn_pal"

#define BTN_TFILTER			"gfx/shell/buttons/btn_tfilter"
#define BTN_LIGHTMAP		"gfx/shell/buttons/btn_lightmap"
#define BTN_RIPPLE			"gfx/shell/buttons/btn_water"
#define BTN_SHADOW			"gfx/shell/buttons/btn_shadows"
#define BTN_LERP			"gfx/shell/buttons/btn_lerp"


#define ID_BACKGROUND			0
#define ID_BANNER				1

#define ID_FOV					2
#define ID_VIEW_ROLL			3
#define ID_RESOLUTION_SCALE		9

#define ID_ALWAYS_RUN			4
#define ID_CROSSHAIR			5
#define ID_WEAPON_LAG			6
#define ID_HUD_STYLE			7
#define ID_PALETTE_LIMIT		10

#define ID_FILTER_TEXTURE		13
#define ID_COLORED_LIGHTMAP		14
#define ID_WATER_RIPPLE			15
#define ID_MODEL_SHADOWS		11
#define ID_MODEL_LERP			12

#define ID_DONE					16

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s		background;
	menuBitmap_s		banner;

	menuSpinControl_s	vroll;
	menuSpinControl_s	fov;
	menuSpinControl_s	res;

	menuCheckBox_s		hud;
	menuCheckBox_s		crosshair;
	menuCheckBox_s		vlag;
	menuCheckBox_s		run;
	menuCheckBox_s		palette;

	menuCheckBox_s		filter;
	menuCheckBox_s		lightmap;
	menuCheckBox_s		ripple;
	menuCheckBox_s		shadow;
	menuCheckBox_s		lerp;

	menuPicButton_s	done;

} uiGameOptions_t;

static uiGameOptions_t	uiGameOptions;

typedef struct
{
	float	fovValue;
	float	vrollValue;
	float	resValue;

	int		hudsize;

	int		crosshair;
	int		vlag;
	int		run;
	int		palette;
	int		filter;
	int		lightmap;
	int		ripple;
	int		shadow;
	int		lerp;
} uiOptionValues_t;

static uiOptionValues_t	uiOptionsInit;


static void UI_GameOptions_SaveConfig(void)
{
	CVAR_SET_FLOAT("default_fov", uiOptionsInit.fovValue);
	CVAR_SET_FLOAT("cl_rollangle", uiOptionsInit.vrollValue);

	CVAR_SET_FLOAT("viewsize", uiOptionsInit.hudsize);

	CVAR_SET_FLOAT("crosshair", uiOptionsInit.crosshair);
	CVAR_SET_FLOAT("sv_viewlag", uiOptionsInit.vlag);
	CVAR_SET_FLOAT("cl_alwaysrun", uiOptionsInit.run);

	CVAR_SET_FLOAT("glsl_pixelate_size", uiOptionsInit.resValue);
	CVAR_SET_FLOAT("glsl_palette", uiOptionsInit.palette);

	CVAR_SET_FLOAT("gl_texture_nearest", !uiOptionsInit.filter);
	CVAR_SET_FLOAT("r_color_lightmap", uiOptionsInit.lightmap);
	CVAR_SET_FLOAT("r_ripple", uiOptionsInit.ripple);
	CVAR_SET_FLOAT("r_shadows", uiOptionsInit.shadow);
	CVAR_SET_FLOAT("r_lerpmodels", uiOptionsInit.lerp);
}
/*
=================
UI_GameOptions_UpdateConfig
=================
*/
static void UI_GameOptions_UpdateConfig(void)
{
	static char	fovText[8];
	static char	vrollText[8];
	static char	resText[8];

	sprintf(fovText, "%.0f", uiGameOptions.fov.curValue);
	uiGameOptions.fov.generic.name = fovText;
	uiOptionsInit.fovValue = uiGameOptions.fov.curValue; //CVAR_SET_FLOAT("default_fov", uiGameOptions.fov.curValue);

	sprintf(vrollText, "%.1f", uiGameOptions.vroll.curValue);
	uiGameOptions.vroll.generic.name = vrollText;	
	uiOptionsInit.vrollValue = uiGameOptions.vroll.curValue; //CVAR_SET_FLOAT("cl_rollangle", uiGameOptions.vroll.curValue);

	if (uiGameOptions.hud.enabled)
		uiOptionsInit.hudsize = 120;//CVAR_SET_FLOAT("viewsize", 120);
	else
		uiOptionsInit.hudsize = 110;//CVAR_SET_FLOAT("viewsize", 110);

	uiOptionsInit.crosshair = uiGameOptions.crosshair.enabled;//CVAR_SET_FLOAT("crosshair", uiGameOptions.crosshair.enabled);
	uiOptionsInit.vlag = uiGameOptions.vlag.enabled;//CVAR_SET_FLOAT("sv_viewlag", uiGameOptions.vlag.enabled);
	uiOptionsInit.run = uiGameOptions.run.enabled;//CVAR_SET_FLOAT("cl_alwaysrun", uiGameOptions.run.enabled);

	if (uiGameOptions.res.curValue > 0)
	{
		sprintf(resText, "%.0f", uiGameOptions.res.curValue);
		uiGameOptions.res.generic.name = resText;		
	}
	else
	{
		uiGameOptions.res.generic.name = "off";
	}
	uiOptionsInit.resValue = uiGameOptions.res.curValue;//CVAR_SET_FLOAT("glsl_pixelate_size", uiGameOptions.res.curValue);
	uiOptionsInit.palette = uiGameOptions.palette.enabled;//CVAR_SET_FLOAT("glsl_palette", uiGameOptions.palette.enabled);

	uiOptionsInit.filter = uiGameOptions.filter.enabled;//CVAR_SET_FLOAT("gl_texture_nearest", !uiGameOptions.filter.enabled);
	uiOptionsInit.lightmap = uiGameOptions.lightmap.enabled;//CVAR_SET_FLOAT("r_color_lightmap", uiGameOptions.lightmap.enabled);
	uiOptionsInit.ripple = uiGameOptions.ripple.enabled;//CVAR_SET_FLOAT("r_ripple", uiGameOptions.ripple.enabled);
	uiOptionsInit.shadow = uiGameOptions.shadow.enabled;//CVAR_SET_FLOAT("r_shadows", uiGameOptions.shadow.enabled);
	uiOptionsInit.lerp = uiGameOptions.lerp.enabled;//CVAR_SET_FLOAT("r_lerpmodels", uiGameOptions.lerp.enabled);
}

/*
=================
UI_GameOptions_GetConfig
=================
*/
static void UI_GameOptions_GetConfig(void)
{
	uiGameOptions.fov.curValue = CVAR_GET_FLOAT("default_fov");
	uiGameOptions.vroll.curValue = CVAR_GET_FLOAT("cl_rollangle");
	uiGameOptions.res.curValue = CVAR_GET_FLOAT("glsl_pixelate_size");

	if (CVAR_GET_FLOAT("viewsize") == 120)
	{
		uiGameOptions.hud.enabled = 1;
		uiGameOptions.hud.focusPic = SM_ON;
	}
	else
	{
		uiGameOptions.hud.enabled = 0;
		uiGameOptions.hud.focusPic = SM_OFF;
	}
	if (CVAR_GET_FLOAT("crosshair") >= 1)
	{
		uiGameOptions.crosshair.enabled = 1;
		uiGameOptions.crosshair.focusPic = SM_ON;
	}
	if (CVAR_GET_FLOAT("sv_viewlag") >= 1)
	{
		uiGameOptions.vlag.enabled = 1;
		uiGameOptions.vlag.focusPic = SM_ON;
	}
	if (CVAR_GET_FLOAT("cl_alwaysrun") >= 1)
	{
		uiGameOptions.run.enabled = 1;
		uiGameOptions.run.focusPic = SM_ON;
	}
	if (CVAR_GET_FLOAT("glsl_palette") >= 1)
	{
		uiGameOptions.palette.enabled = 1;
		uiGameOptions.palette.focusPic = SM_ON;
	}
	if (CVAR_GET_FLOAT("gl_texture_nearest") >= 1)
	{
		uiGameOptions.filter.enabled = 0;
		uiGameOptions.filter.focusPic = SM_OFF;
	}
	else
	{
		uiGameOptions.filter.enabled = 1;
		uiGameOptions.filter.focusPic = SM_ON;
	}
	if (CVAR_GET_FLOAT("r_color_lightmap") >= 1)
	{
		uiGameOptions.lightmap.enabled = 1;
		uiGameOptions.lightmap.focusPic = SM_ON;
	}
	if (CVAR_GET_FLOAT("r_ripple") >= 1)
	{
		uiGameOptions.ripple.enabled = 1;
		uiGameOptions.ripple.focusPic = SM_ON;
	}
	if (CVAR_GET_FLOAT("r_shadows") >= 1)
	{
		uiGameOptions.shadow.enabled = 1;
		uiGameOptions.shadow.focusPic = SM_ON;
	}
	if (CVAR_GET_FLOAT("r_lerpmodels") >= 1)
	{
		uiGameOptions.lerp.enabled = 1;
		uiGameOptions.lerp.focusPic = SM_ON;
	}
	UI_GameOptions_UpdateConfig();
}

/*
=================
UI_GameOptions_KeyFunc
=================
*/
static const char *UI_GameOptions_KeyFunc( int key, int down )
{
	if( down && key == K_ESCAPE )
		UI_GameOptions_SaveConfig();

	return UI_DefaultKey( &uiGameOptions.menu, key, down );
}

/*
=================
UI_GameOptions_Callback
=================
*/
static void UI_GameOptions_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
	case ID_HUD_STYLE:
	case ID_CROSSHAIR:
	case ID_WEAPON_LAG:
	case ID_ALWAYS_RUN:
	case ID_PALETTE_LIMIT:
	case ID_FILTER_TEXTURE:
	case ID_COLORED_LIGHTMAP:
	case ID_WATER_RIPPLE:
	case ID_MODEL_SHADOWS:
	case ID_MODEL_LERP:
		if (((menuCheckBox_s*)self)->enabled)
			((menuCheckBox_s*)self)->focusPic = SM_ON;
		else
			((menuCheckBox_s*)self)->focusPic = SM_OFF;
		break;
	}

	if( event == QM_CHANGED )
	{
		UI_GameOptions_UpdateConfig();
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_GameOptions_SaveConfig();
		UI_PopMenu();
		break;
	}
}

#define GAMEOPT_XOFFSET	-28
#define GAMEOPT_SPINX	76
/*
=================
UI_GameOptions_Init
=================
*/
static void UI_GameOptions_Init( void )
{
	memset( &uiGameOptions, 0, sizeof( uiGameOptions_t ));

	uiGameOptions.menu.vidInitFunc = UI_GameOptions_Init;
	uiGameOptions.menu.keyFunc = UI_GameOptions_KeyFunc;

	uiGameOptions.background.generic.id = ID_BACKGROUND;
	uiGameOptions.background.generic.type = QMTYPE_BITMAP_STRETCH;
	uiGameOptions.background.generic.flags = QMF_INACTIVE;
	uiGameOptions.background.generic.x = 0;
	uiGameOptions.background.generic.y = 0;
	uiGameOptions.background.generic.width = 1024;
	uiGameOptions.background.generic.height = 768;
	uiGameOptions.background.pic = ART_BACKGROUND;

	uiGameOptions.banner.generic.id = ID_BANNER;
	uiGameOptions.banner.generic.type = QMTYPE_BITMAP;
	uiGameOptions.banner.generic.flags = QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiGameOptions.banner.generic.x = UI_BANNER_POSX;
	uiGameOptions.banner.generic.y = UI_BANNER_POSY;
	uiGameOptions.banner.generic.width = 264;
	uiGameOptions.banner.generic.height = 24;
	uiGameOptions.banner.pic = ART_BANNER;
	uiGameOptions.banner.generic.color = uiColorDoomRed;

	// fov
	uiGameOptions.fov.generic.id = ID_FOV;
	uiGameOptions.fov.generic.type = QMTYPE_SPINCONTROL;
	uiGameOptions.fov.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.fov.generic.x = GAMEOPT_SPINX;
	uiGameOptions.fov.generic.y = 42;
	uiGameOptions.fov.generic.width = 64;
	uiGameOptions.fov.generic.height = 16;
	uiGameOptions.fov.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.fov.minValue = 60;
	uiGameOptions.fov.maxValue = 130;
	uiGameOptions.fov.range = 5;

	uiGameOptions.fov.header = SPIN_FOV;
	uiGameOptions.fov.xHeader = GAMEOPT_XOFFSET - 16;
	uiGameOptions.fov.widthHeader = 128;

	// view roll
	uiGameOptions.vroll.generic.id = ID_VIEW_ROLL;
	uiGameOptions.vroll.generic.type = QMTYPE_SPINCONTROL;
	uiGameOptions.vroll.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.vroll.generic.x = GAMEOPT_SPINX;
	uiGameOptions.vroll.generic.y = 56;
	uiGameOptions.vroll.generic.width = 64;
	uiGameOptions.vroll.generic.height = 16;
	uiGameOptions.vroll.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.vroll.minValue = -2.0f;
	uiGameOptions.vroll.maxValue = 2.0f;
	uiGameOptions.vroll.range = 0.5f;

	uiGameOptions.vroll.header = SPIN_VROLL;
	uiGameOptions.vroll.xHeader = GAMEOPT_XOFFSET - 16;
	uiGameOptions.vroll.widthHeader = 128;

	// res scale
	uiGameOptions.res.generic.id = ID_RESOLUTION_SCALE;
	uiGameOptions.res.generic.type = QMTYPE_SPINCONTROL;
	uiGameOptions.res.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.res.generic.x = GAMEOPT_SPINX;
	uiGameOptions.res.generic.y = 70;
	uiGameOptions.res.generic.width = 64;
	uiGameOptions.res.generic.height = 16;
	uiGameOptions.res.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.res.minValue = 0;
	uiGameOptions.res.maxValue = 480;
	uiGameOptions.res.range = 120;

	uiGameOptions.res.header = SPIN_RES;
	uiGameOptions.res.xHeader = GAMEOPT_XOFFSET - 16;
	uiGameOptions.res.widthHeader = 128;

	// always run
	uiGameOptions.run.generic.id = ID_ALWAYS_RUN;
	uiGameOptions.run.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.run.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.run.generic.x = GAMEOPT_XOFFSET;
	uiGameOptions.run.generic.y = 94;
	uiGameOptions.run.generic.width = 160;
	uiGameOptions.run.generic.height = 8;
	uiGameOptions.run.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.run.pic = BTN_RUN;

	uiGameOptions.run.generic.x2 = 0;
	uiGameOptions.run.generic.width2 = 32;
	uiGameOptions.run.focusPic = SM_OFF;

	// crosshair
	uiGameOptions.crosshair.generic.id = ID_CROSSHAIR;
	uiGameOptions.crosshair.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.crosshair.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.crosshair.generic.x = GAMEOPT_XOFFSET;
	uiGameOptions.crosshair.generic.y = 106;
	uiGameOptions.crosshair.generic.width = 160;
	uiGameOptions.crosshair.generic.height = 8;
	uiGameOptions.crosshair.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.crosshair.pic = BTN_CROSSHAIR;

	uiGameOptions.crosshair.generic.x2 = 0;
	uiGameOptions.crosshair.generic.width2 = 32;
	uiGameOptions.crosshair.focusPic = SM_OFF;

	// hud style
	uiGameOptions.hud.generic.id = ID_HUD_STYLE;
	uiGameOptions.hud.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.hud.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.hud.generic.x = GAMEOPT_XOFFSET;
	uiGameOptions.hud.generic.y = 118;
	uiGameOptions.hud.generic.width = 160;
	uiGameOptions.hud.generic.height = 8;
	uiGameOptions.hud.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.hud.pic = BTN_HUD;

	uiGameOptions.hud.generic.x2 = 0;
	uiGameOptions.hud.generic.width2 = 32;
	uiGameOptions.hud.focusPic = SM_OFF;

	// weapon lag
	uiGameOptions.vlag.generic.id = ID_WEAPON_LAG;
	uiGameOptions.vlag.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.vlag.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.vlag.generic.x = GAMEOPT_XOFFSET;
	uiGameOptions.vlag.generic.y = 130;
	uiGameOptions.vlag.generic.width = 160;
	uiGameOptions.vlag.generic.height = 8;
	uiGameOptions.vlag.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.vlag.pic = BTN_VLAG;

	uiGameOptions.vlag.generic.x2 = 0;
	uiGameOptions.vlag.generic.width2 = 32;
	uiGameOptions.vlag.focusPic = SM_OFF;

	// palette
	uiGameOptions.palette.generic.id = ID_PALETTE_LIMIT;
	uiGameOptions.palette.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.palette.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.palette.generic.x = GAMEOPT_XOFFSET;
	uiGameOptions.palette.generic.y = 142;
	uiGameOptions.palette.generic.width = 160;
	uiGameOptions.palette.generic.height = 8;
	uiGameOptions.palette.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.palette.pic = BTN_PALETTE;

	uiGameOptions.palette.generic.x2 = 0;
	uiGameOptions.palette.generic.width2 = 32;
	uiGameOptions.palette.focusPic = SM_OFF;

	// texture filter
	uiGameOptions.filter.generic.id = ID_FILTER_TEXTURE;
	uiGameOptions.filter.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.filter.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.filter.generic.x = GAMEOPT_XOFFSET;
	uiGameOptions.filter.generic.y = 162;
	uiGameOptions.filter.generic.width = 160;
	uiGameOptions.filter.generic.height = 8;
	uiGameOptions.filter.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.filter.pic = BTN_TFILTER;

	uiGameOptions.filter.generic.x2 = 0;
	uiGameOptions.filter.generic.width2 = 32;
	uiGameOptions.filter.focusPic = SM_ON;

	// lightmap
	uiGameOptions.lightmap.generic.id = ID_COLORED_LIGHTMAP;
	uiGameOptions.lightmap.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.lightmap.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.lightmap.generic.x = GAMEOPT_XOFFSET;
	uiGameOptions.lightmap.generic.y = 174;
	uiGameOptions.lightmap.generic.width = 160;
	uiGameOptions.lightmap.generic.height = 8;
	uiGameOptions.lightmap.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.lightmap.pic = BTN_LIGHTMAP;

	uiGameOptions.lightmap.generic.x2 = 0;
	uiGameOptions.lightmap.generic.width2 = 32;
	uiGameOptions.lightmap.focusPic = SM_OFF;

	// water ripple
	uiGameOptions.ripple.generic.id = ID_COLORED_LIGHTMAP;
	uiGameOptions.ripple.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.ripple.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.ripple.generic.x = GAMEOPT_XOFFSET;
	uiGameOptions.ripple.generic.y = 186;
	uiGameOptions.ripple.generic.width = 160;
	uiGameOptions.ripple.generic.height = 8;
	uiGameOptions.ripple.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.ripple.pic = BTN_RIPPLE;

	uiGameOptions.ripple.generic.x2 = 0;
	uiGameOptions.ripple.generic.width2 = 32;
	uiGameOptions.ripple.focusPic = SM_OFF;

	// models shadows
	uiGameOptions.shadow.generic.id = ID_MODEL_SHADOWS;
	uiGameOptions.shadow.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.shadow.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.shadow.generic.x = GAMEOPT_XOFFSET;
	uiGameOptions.shadow.generic.y = 198;
	uiGameOptions.shadow.generic.width = 160;
	uiGameOptions.shadow.generic.height = 8;
	uiGameOptions.shadow.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.shadow.pic = BTN_SHADOW;

	uiGameOptions.shadow.generic.x2 = 0;
	uiGameOptions.shadow.generic.width2 = 32;
	uiGameOptions.shadow.focusPic = SM_OFF;

	// lerp models
	uiGameOptions.lerp.generic.id = ID_MODEL_LERP;
	uiGameOptions.lerp.generic.type = QMTYPE_CHECKBOX;
	uiGameOptions.lerp.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiGameOptions.lerp.generic.x = GAMEOPT_XOFFSET;
	uiGameOptions.lerp.generic.y = 210;
	uiGameOptions.lerp.generic.width = 160;
	uiGameOptions.lerp.generic.height = 8;
	uiGameOptions.lerp.generic.callback = UI_GameOptions_Callback;
	uiGameOptions.lerp.pic = BTN_LERP;

	uiGameOptions.lerp.generic.x2 = 0;
	uiGameOptions.lerp.generic.width2 = 32;
	uiGameOptions.lerp.focusPic = SM_OFF;


	uiGameOptions.done.generic.id	= ID_DONE;
	uiGameOptions.done.generic.type = QMTYPE_BM_BUTTON;
	uiGameOptions.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW; 
	uiGameOptions.done.generic.x = UI_SELECTION_POSX;
	uiGameOptions.done.generic.y = 230;
	uiGameOptions.done.generic.name = "Done";
	uiGameOptions.done.generic.statusText = "Save changes and go back to the Customize Menu";
	uiGameOptions.done.generic.callback = UI_GameOptions_Callback;

	UI_UtilSetupPicButton( &uiGameOptions.done, PC_DONE );

	UI_GameOptions_GetConfig();

	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.background);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.banner);

	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.fov);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.vroll);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.res);

	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.run);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.crosshair);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.hud);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.vlag);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.palette);

	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.filter);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.lightmap);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.ripple);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.shadow);
	UI_AddItem(&uiGameOptions.menu, (void*)&uiGameOptions.lerp);

	//UI_AddItem( &uiGameOptions.menu, (void *)&uiGameOptions.done );
}

/*
=================
UI_GameOptions_Precache
=================
*/
void UI_GameOptions_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER, TF_NEAREST | TF_NOMIPMAP );
}

/*
=================
UI_GameOptions_Menu
=================
*/
void UI_GameOptions_Menu( void )
{
	UI_GameOptions_Precache();
	UI_GameOptions_Init();

	UI_GameOptions_UpdateConfig();
	UI_PushMenu( &uiGameOptions.menu );
}