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
#include "menu_btnsbmp_table.h"\

#define OPTIONS_X			10

#define HEADER_OPTIONS     	"gfx/shell/headers/header_options"

#define BTN_CONTROLS		"gfx/shell/buttons/btn_keybinds"
#define BTN_MOUSE			"gfx/shell/buttons/btn_mouse"
#define BTN_AUDIO			"gfx/shell/buttons/btn_sound"
#define BTN_VIDEO			"gfx/shell/buttons/btn_display"
#define BTN_CUSTOMIZE		"gfx/shell/buttons/btn_customize"

#define ID_BACKGROUND    	0
#define ID_BANNER	     	1

#define ID_CONTROLS			2
#define ID_MOUSE	     	3
#define ID_AUDIO	     	4
#define ID_VIDEO	     	5
#define ID_CUSTOMIZE     	6
#define ID_DONE	     		7

typedef struct
{
	menuFramework_s	menu;
	
	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuBitmap_s	controls;
	menuBitmap_s	mouse;
	menuBitmap_s	audio;
	menuBitmap_s	video;
	menuBitmap_s	customize;
	menuBitmap_s	done;

} uiOptions_t;

static uiOptions_t		uiOptions;


/*
=================
UI_Options_Callback
=================
*/
static void UI_Options_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_PopMenu();
		break;
	case ID_CONTROLS:
		UI_Controls_Menu();
		break;
	case ID_MOUSE:
		UI_AdvControls_Menu();
		break;
	case ID_AUDIO:
		UI_Audio_Menu();
		break;
	case ID_VIDEO:
		UI_VidModes_Menu();
		break;
	case ID_CUSTOMIZE:
		UI_GameOptions_Menu();
		break;	
	}
}

/*
=================
UI_Options_Init
=================
*/
static void UI_Options_Init( void )
{
	memset( &uiOptions, 0, sizeof( uiOptions_t ));

	uiOptions.menu.vidInitFunc = UI_Options_Init;

	uiOptions.background.generic.id = ID_BACKGROUND;
	uiOptions.background.generic.type = QMTYPE_BITMAP_STRETCH;
	uiOptions.background.generic.flags = QMF_INACTIVE;
	uiOptions.background.generic.x = 0;
	uiOptions.background.generic.y = 0;
	uiOptions.background.generic.width = 1024;
	uiOptions.background.generic.height = 768;
	uiOptions.background.pic = ART_BACKGROUND;

	// header
	uiOptions.banner.generic.id = ID_BANNER;
	uiOptions.banner.generic.type = QMTYPE_BITMAP;
	uiOptions.banner.generic.flags = QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiOptions.banner.generic.x = UI_BANNER_POSX;
	uiOptions.banner.generic.y = UI_BANNER_POSY;
	uiOptions.banner.generic.width = 152;
	uiOptions.banner.generic.height = 24;
	uiOptions.banner.pic = HEADER_OPTIONS;
	uiOptions.banner.generic.color = uiColorDoomRed;

	
	uiOptions.controls.generic.id = ID_CONTROLS;
	uiOptions.controls.generic.type = QMTYPE_BITMAP;
	uiOptions.controls.generic.name = "Controls";
	uiOptions.controls.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiOptions.controls.generic.x = OPTIONS_X;
	uiOptions.controls.generic.y = 54;
	uiOptions.controls.generic.width = 152;
	uiOptions.controls.generic.height = 24;
	uiOptions.controls.generic.callback = UI_Options_Callback;
	uiOptions.controls.pic = BTN_CONTROLS;
	uiOptions.controls.generic.color = uiColorDoomRed;
	uiOptions.controls.generic.focusColor = uiColorDoomSelect;

	uiOptions.mouse.generic.id = ID_MOUSE;
	uiOptions.mouse.generic.type = QMTYPE_BITMAP;
	uiOptions.mouse.generic.name = "Mouse";
	uiOptions.mouse.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiOptions.mouse.generic.x = OPTIONS_X;
	uiOptions.mouse.generic.y = 72;
	uiOptions.mouse.generic.width = 152;
	uiOptions.mouse.generic.height = 24;
	uiOptions.mouse.generic.callback = UI_Options_Callback;
	uiOptions.mouse.pic = BTN_MOUSE;
	uiOptions.mouse.generic.color = uiColorDoomRed;
	uiOptions.mouse.generic.focusColor = uiColorDoomSelect;

	uiOptions.audio.generic.id = ID_AUDIO;
	uiOptions.audio.generic.type = QMTYPE_BITMAP;
	uiOptions.audio.generic.name = "Audio";
	uiOptions.audio.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiOptions.audio.generic.x = OPTIONS_X;
	uiOptions.audio.generic.y = 90;
	uiOptions.audio.generic.width = 152;
	uiOptions.audio.generic.height = 24;
	uiOptions.audio.generic.callback = UI_Options_Callback;
	uiOptions.audio.pic = BTN_AUDIO;
	uiOptions.audio.generic.color = uiColorDoomRed;
	uiOptions.audio.generic.focusColor = uiColorDoomSelect;

	uiOptions.video.generic.id = ID_VIDEO;
	uiOptions.video.generic.type = QMTYPE_BITMAP;
	uiOptions.video.generic.name = "Video";
	uiOptions.video.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiOptions.video.generic.x = OPTIONS_X;
	uiOptions.video.generic.y = 108;
	uiOptions.video.generic.width = 152;
	uiOptions.video.generic.height = 24;
	uiOptions.video.generic.callback = UI_Options_Callback;
	uiOptions.video.pic = BTN_VIDEO;
	uiOptions.video.generic.color = uiColorDoomRed;
	uiOptions.video.generic.focusColor = uiColorDoomSelect;

	uiOptions.customize.generic.id = ID_CUSTOMIZE;
	uiOptions.customize.generic.type = QMTYPE_BITMAP;
	uiOptions.customize.generic.name = "Customize";
	uiOptions.customize.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiOptions.customize.generic.x = OPTIONS_X;
	uiOptions.customize.generic.y = 126;
	uiOptions.customize.generic.width = 152;
	uiOptions.customize.generic.height = 24;
	uiOptions.customize.generic.callback = UI_Options_Callback;
	uiOptions.customize.pic = BTN_CUSTOMIZE;
	uiOptions.customize.generic.color = uiColorDoomRed;
	uiOptions.customize.generic.focusColor = uiColorDoomSelect;

	uiOptions.done.generic.id = ID_DONE;
	uiOptions.done.generic.type = QMTYPE_BITMAP;
	uiOptions.done.generic.name = "Back";
	uiOptions.done.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiOptions.done.generic.x = OPTIONS_X;
	uiOptions.done.generic.y = 144; //162
	uiOptions.done.generic.width = 152;
	uiOptions.done.generic.height = 24;
	uiOptions.done.generic.callback = UI_Options_Callback;
	uiOptions.done.pic = BTN_DONE;
	uiOptions.done.generic.color = uiColorDoomRed;
	uiOptions.done.generic.focusColor = uiColorDoomSelect;

	UI_AddItem(&uiOptions.menu, (void*)&uiOptions.background);
	UI_AddItem(&uiOptions.menu, (void*)&uiOptions.banner);

	UI_AddItem(&uiOptions.menu, (void*)&uiOptions.controls);
	UI_AddItem(&uiOptions.menu, (void*)&uiOptions.mouse);
	UI_AddItem(&uiOptions.menu, (void*)&uiOptions.audio);
	UI_AddItem(&uiOptions.menu, (void*)&uiOptions.video);
	UI_AddItem(&uiOptions.menu, (void*)&uiOptions.customize);

	UI_AddItem( &uiOptions.menu, (void *)&uiOptions.done );
}

/*
=================
UI_Options_Precache
=================
*/
void UI_Options_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
}

/*
=================
UI_Options_Menu
=================
*/
void UI_Options_Menu( void )
{
	UI_Options_Precache();
	UI_Options_Init();
	
	UI_PushMenu( &uiOptions.menu );
}