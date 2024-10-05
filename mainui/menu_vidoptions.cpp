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

#define ART_BANNER	  	"gfx/shell/head_vidoptions"
#define ART_GAMMA		"gfx/shell/gamma"

#define ID_BACKGROUND 	0
#define ID_BANNER	  	1
#define ID_DONE	  	2
#define ID_SCREEN_SIZE	3
#define ID_GAMMA		4
#define ID_GLARE_REDUCTION	5 

typedef struct
{
	int		outlineWidth;
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;
	menuBitmap_s	testImage;

	menuPicButton_s	done;

	menuSlider_s	screenSize;
	menuSlider_s	gammaIntensity;
	menuSlider_s	glareReduction;

	HIMAGE		hTestImage;
} uiVidOptions_t;

static uiVidOptions_t	uiVidOptions;


/*
=================
UI_VidOptions_GetConfig
=================
*/
static void UI_VidOptions_GetConfig( void )
{
	uiVidOptions.screenSize.curValue = RemapVal( CVAR_GET_FLOAT( "viewsize" ), 30.0f, 120.0f, 0.0f, 1.0f );
	uiVidOptions.glareReduction.curValue = RemapVal( CVAR_GET_FLOAT( "brightness" ), 0.0f, 3.0f, 0.0f, 1.0f );
	uiVidOptions.gammaIntensity.curValue = RemapVal( CVAR_GET_FLOAT( "gamma" ), 1.8f, 3.0f, 0.0f, 1.0f );
	PIC_SetGamma( uiVidOptions.hTestImage, 1.0f );

	uiVidOptions.outlineWidth = 2;
	UI_StretchCoords( NULL, NULL, &uiVidOptions.outlineWidth, NULL );
}

/*
=================
UI_VidOptions_UpdateConfig
=================
*/
static void UI_VidOptions_UpdateConfig( void )
{
	CVAR_SET_FLOAT( "viewsize", RemapVal( uiVidOptions.screenSize.curValue, 0.0f, 1.0f, 30.0f, 120.0f ));
	CVAR_SET_FLOAT( "brightness", RemapVal( uiVidOptions.glareReduction.curValue, 0.0f, 1.0f, 0.0f, 3.0f ));
	CVAR_SET_FLOAT( "gamma", RemapVal( uiVidOptions.gammaIntensity.curValue, 0.0f, 1.0f, 1.8f, 3.0f ));
	PIC_SetGamma( uiVidOptions.hTestImage, 1.0f );
}

static void UI_VidOptions_SetConfig( void )
{
	CVAR_SET_FLOAT( "viewsize", RemapVal( uiVidOptions.screenSize.curValue, 0.0f, 1.0f, 30.0f, 120.0f ));
	CVAR_SET_FLOAT( "brightness", RemapVal( uiVidOptions.glareReduction.curValue, 0.0f, 1.0f, 0.0f, 3.0f ));
	CVAR_SET_FLOAT( "gamma", RemapVal( uiVidOptions.gammaIntensity.curValue, 0.0f, 1.0f, 1.8f, 3.0f ));
}

/*
=================
UI_VidOptions_Ownerdraw
=================
*/
static void UI_VidOptions_Ownerdraw( void *self )
{
	menuCommon_s	*item = (menuCommon_s *)self;
	int		color = 0xFFFF0000; // 255, 0, 0, 255
	int		viewport[4];
	int		viewsize, size, sb_lines;

	viewsize = CVAR_GET_FLOAT( "viewsize" );

	if( viewsize >= 120 )
		sb_lines = 0;	// no status bar at all
	else if( viewsize >= 110 )
		sb_lines = 24;	// no inventory
	else sb_lines = 48;

	size = min( viewsize, 100 );

	viewport[2] = item->width * size / 100;
	viewport[3] = item->height * size / 100;

	if( viewport[3] > item->height - sb_lines )
		viewport[3] = item->height - sb_lines;
	if( viewport[3] > item->height )
		viewport[3] = item->height;

	viewport[2] &= ~7;
	viewport[3] &= ~1;

	viewport[0] = (item->width - viewport[2]) / 2;
	viewport[1] = (item->height - sb_lines - viewport[3]) / 2;

	UI_DrawPic( item->x + viewport[0], item->y + viewport[1], viewport[2], viewport[3], uiColorWhite, ((menuBitmap_s *)self)->pic );
	UI_DrawRectangleExt( item->x, item->y, item->width, item->height, color, uiVidOptions.outlineWidth );
}

/*
=================
UI_VidOptions_Callback
=================
*/
static void UI_VidOptions_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

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
		UI_VidOptions_SetConfig();
		UI_PopMenu();
		break;
	}
}

/*
=================
UI_VidOptions_Init
=================
*/
static void UI_VidOptions_Init( void )
{
	memset( &uiVidOptions, 0, sizeof( uiVidOptions_t ));

	uiVidOptions.hTestImage = PIC_Load( ART_GAMMA, PIC_KEEP_SOURCE );

	uiVidOptions.menu.vidInitFunc = UI_VidOptions_Init;

	uiVidOptions.background.generic.id = ID_BACKGROUND;
	uiVidOptions.background.generic.type = QMTYPE_BITMAP;
	uiVidOptions.background.generic.flags = QMF_INACTIVE;
	uiVidOptions.background.generic.x = 0;
	uiVidOptions.background.generic.y = 0;
	uiVidOptions.background.generic.width = 1024;
	uiVidOptions.background.generic.height = 768;
	uiVidOptions.background.pic = ART_BACKGROUND;

	uiVidOptions.banner.generic.id = ID_BANNER;
	uiVidOptions.banner.generic.type = QMTYPE_BITMAP;
	uiVidOptions.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_ADDITIVE;
	uiVidOptions.banner.generic.x = UI_BANNER_POSX;
	uiVidOptions.banner.generic.y = UI_BANNER_POSY;
	uiVidOptions.banner.generic.width = UI_BANNER_WIDTH;
	uiVidOptions.banner.generic.height = UI_BANNER_HEIGHT;
	uiVidOptions.banner.pic = ART_BANNER;

	uiVidOptions.testImage.generic.id = ID_BANNER;
	uiVidOptions.testImage.generic.type = QMTYPE_BITMAP;
	uiVidOptions.testImage.generic.flags = QMF_INACTIVE;
	uiVidOptions.testImage.generic.x = 390;
	uiVidOptions.testImage.generic.y = 225;
	uiVidOptions.testImage.generic.width = 480;
	uiVidOptions.testImage.generic.height = 450;
	uiVidOptions.testImage.pic = ART_GAMMA;
	uiVidOptions.testImage.generic.ownerdraw = UI_VidOptions_Ownerdraw;

	uiVidOptions.done.generic.id = ID_DONE;
	uiVidOptions.done.generic.type = QMTYPE_BM_BUTTON;
	uiVidOptions.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiVidOptions.done.generic.x = UI_SELECTION_POSX;
	uiVidOptions.done.generic.y = 435;
	uiVidOptions.done.generic.name = "Done";
	uiVidOptions.done.generic.statusText = "Go back to the Video Menu";
	uiVidOptions.done.generic.callback = UI_VidOptions_Callback;

	UI_UtilSetupPicButton( &uiVidOptions.done, PC_DONE );

	uiVidOptions.screenSize.generic.id = ID_SCREEN_SIZE;
	uiVidOptions.screenSize.generic.type = QMTYPE_SLIDER;
	uiVidOptions.screenSize.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiVidOptions.screenSize.generic.name = "Screen size";
	uiVidOptions.screenSize.generic.x = UI_SELECTION_POSX;
	uiVidOptions.screenSize.generic.y = 280;
	uiVidOptions.screenSize.generic.callback = UI_VidOptions_Callback;
	uiVidOptions.screenSize.generic.statusText = "Set the screen size";
	uiVidOptions.screenSize.minValue = 0.0;
	uiVidOptions.screenSize.maxValue = 1.0;
	uiVidOptions.screenSize.range = 0.05f;

	uiVidOptions.gammaIntensity.generic.id = ID_GAMMA;
	uiVidOptions.gammaIntensity.generic.type = QMTYPE_SLIDER;
	uiVidOptions.gammaIntensity.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiVidOptions.gammaIntensity.generic.name = "Gamma";
	uiVidOptions.gammaIntensity.generic.x = UI_SELECTION_POSX;
	uiVidOptions.gammaIntensity.generic.y = 340;
	uiVidOptions.gammaIntensity.generic.callback = UI_VidOptions_Callback;
	uiVidOptions.gammaIntensity.generic.statusText = "Set gamma value (1.8 - 3.0)";
	uiVidOptions.gammaIntensity.minValue = 0.0;
	uiVidOptions.gammaIntensity.maxValue = 1.0;
	uiVidOptions.gammaIntensity.range = 0.05f;

	uiVidOptions.glareReduction.generic.id = ID_GLARE_REDUCTION;
	uiVidOptions.glareReduction.generic.type = QMTYPE_SLIDER;
	uiVidOptions.glareReduction.generic.flags = QMF_PULSEIFFOCUS|QMF_DROPSHADOW;
	uiVidOptions.glareReduction.generic.name = "Glare reduction";
	uiVidOptions.glareReduction.generic.x = UI_SELECTION_POSX;
	uiVidOptions.glareReduction.generic.y = 400;
	uiVidOptions.glareReduction.generic.callback = UI_VidOptions_Callback;
	uiVidOptions.glareReduction.generic.statusText = "Set glare reduction level";
	uiVidOptions.glareReduction.minValue = 0.0;
	uiVidOptions.glareReduction.maxValue = 1.0;
	uiVidOptions.glareReduction.range = 0.05f;

	UI_VidOptions_GetConfig();

	UI_AddItem( &uiVidOptions.menu, (void *)&uiVidOptions.background );
	UI_AddItem( &uiVidOptions.menu, (void *)&uiVidOptions.banner );
	UI_AddItem( &uiVidOptions.menu, (void *)&uiVidOptions.done );
	UI_AddItem( &uiVidOptions.menu, (void *)&uiVidOptions.screenSize );
	UI_AddItem( &uiVidOptions.menu, (void *)&uiVidOptions.gammaIntensity );
	UI_AddItem( &uiVidOptions.menu, (void *)&uiVidOptions.glareReduction );
	UI_AddItem( &uiVidOptions.menu, (void *)&uiVidOptions.testImage );
}

/*
=================
UI_VidOptions_Precache
=================
*/
void UI_VidOptions_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER );
}

/*
=================
UI_VidOptions_Menu
=================
*/
void UI_VidOptions_Menu( void )
{
	UI_VidOptions_Precache();
	UI_VidOptions_Init();

	UI_PushMenu( &uiVidOptions.menu );
}