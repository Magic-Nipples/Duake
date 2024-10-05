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
#include "../cl_dll/kbutton.h"
#include "menu_btnsbmp_table.h"
#include "menu_strings.h"

#define HEADER_MOUSE     	"gfx/shell/headers/header_msettings"

#define BTN_MOUSE			"gfx/shell/buttons/btn_enablem"
#define BTN_UPDOWN			"gfx/shell/buttons/btn_mupdown"
#define BTN_INVERT			"gfx/shell/buttons/btn_invert"
#define BTN_FILTER			"gfx/shell/buttons/btn_filterm"

#define SHEAD_SENSE			"gfx/shell/slider/slide_msense"

#define BTN_SPRING			"gfx/shell/buttons/btn_spring"
#define BTN_STRAFE			"gfx/shell/buttons/btn_lookstrafe"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_MOUSELOOK		2
#define ID_MOUSEUPDOWN		3
#define ID_INVERTMOUSE		4
#define ID_MOUSEFILTER		5
#define ID_SENSITIVITY		6
#define ID_LOOKSPRING		7
#define ID_LOOKSTRAFE		8
#define ID_DONE				9


typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuCheckBox_s	mouseLook;
	menuCheckBox_s	mouseupdown;
	menuCheckBox_s	invertMouse;
	menuCheckBox_s	mouseFilter;

	menuSlider_s	sensitivity;

	menuCheckBox_s	lookSpring;
	menuCheckBox_s	lookStrafe;

	menuBitmap_s	done;
} uiAdvControls_t;

static uiAdvControls_t	uiAdvControls;

/*
=================
UI_AdvControls_UpdateConfig
=================
*/
static void UI_AdvControls_UpdateConfig( void )
{
	if( uiAdvControls.invertMouse.enabled && CVAR_GET_FLOAT( "m_pitch" ) > 0 )
		CVAR_SET_FLOAT( "m_pitch", -CVAR_GET_FLOAT( "m_pitch" ));
	else if( !uiAdvControls.invertMouse.enabled && CVAR_GET_FLOAT( "m_pitch" ) < 0 )
		CVAR_SET_FLOAT( "m_pitch", fabs( CVAR_GET_FLOAT( "m_pitch" )));

	CVAR_SET_FLOAT( "lookspring", uiAdvControls.lookSpring.enabled );
	CVAR_SET_FLOAT( "lookstrafe", uiAdvControls.lookStrafe.enabled );
	CVAR_SET_FLOAT( "m_filter", uiAdvControls.mouseFilter.enabled );
	CVAR_SET_FLOAT( "sensitivity", (uiAdvControls.sensitivity.curValue * 20.0f) + 0.1f );
	CVAR_SET_FLOAT("m_updown", uiAdvControls.mouseupdown.enabled);
	
	if( uiAdvControls.mouseLook.enabled )
	{
		uiAdvControls.mouseupdown.generic.flags &= ~QMF_GRAYED;
		uiAdvControls.invertMouse.generic.flags &= ~QMF_GRAYED;
		uiAdvControls.mouseFilter.generic.flags &= ~QMF_GRAYED;
		uiAdvControls.lookSpring.generic.flags |= QMF_GRAYED;
		uiAdvControls.lookStrafe.generic.flags |= QMF_GRAYED;
		CLIENT_COMMAND( TRUE, "+mlook" );
	}
	else
	{
		uiAdvControls.invertMouse.generic.flags |= QMF_GRAYED;
		uiAdvControls.mouseupdown.generic.flags |= QMF_GRAYED;
		uiAdvControls.mouseFilter.generic.flags |= QMF_GRAYED;
		uiAdvControls.lookSpring.generic.flags &= ~QMF_GRAYED;
		uiAdvControls.lookStrafe.generic.flags &= ~QMF_GRAYED;
		CLIENT_COMMAND( TRUE, "-mlook" );
	}

	if (uiAdvControls.mouseupdown.enabled && uiAdvControls.mouseLook.enabled)
		uiAdvControls.invertMouse.generic.flags &= ~QMF_GRAYED;
	else
		uiAdvControls.invertMouse.generic.flags |= QMF_GRAYED;
}

/*
=================
UI_AdvControls_GetConfig
=================
*/
static void UI_AdvControls_GetConfig( void )
{
	kbutton_t	*mlook;

	if (CVAR_GET_FLOAT("m_pitch") < 0)
	{
		uiAdvControls.invertMouse.enabled = true;
		uiAdvControls.invertMouse.focusPic = SM_ON;
	}

	mlook = (kbutton_s *)Key_GetState( "in_mlook" );
	if (mlook && mlook->state & 1)
	{
		uiAdvControls.mouseLook.enabled = 1;
		uiAdvControls.mouseLook.focusPic = SM_ON;
	}

	if (CVAR_GET_FLOAT("m_updown"))
	{
		uiAdvControls.mouseupdown.enabled = 1;
		uiAdvControls.mouseupdown.focusPic = SM_ON;
	}

	if (CVAR_GET_FLOAT("lookspring"))
	{
		uiAdvControls.lookSpring.enabled = 1;
		uiAdvControls.lookSpring.focusPic = SM_ON;
	}

	if (CVAR_GET_FLOAT("lookstrafe"))
	{
		uiAdvControls.lookStrafe.enabled = 1;
		uiAdvControls.lookStrafe.focusPic = SM_ON;
	}

	if (CVAR_GET_FLOAT("m_filter"))
	{
		uiAdvControls.mouseFilter.enabled = 1;
		uiAdvControls.mouseFilter.focusPic = SM_ON;
	}

	uiAdvControls.sensitivity.curValue = (CVAR_GET_FLOAT( "sensitivity" ) - 0.1f) / 20.0f;

	if( uiAdvControls.mouseLook.enabled )
	{
		uiAdvControls.lookSpring.generic.flags |= QMF_GRAYED;
		uiAdvControls.lookStrafe.generic.flags |= QMF_GRAYED;
	}
	else
	{
		uiAdvControls.lookSpring.generic.flags &= ~QMF_GRAYED;
		uiAdvControls.lookStrafe.generic.flags &= ~QMF_GRAYED;
	}
}

/*
=================
UI_AdvControls_Callback
=================
*/
static void UI_AdvControls_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
	case ID_INVERTMOUSE:
	case ID_MOUSELOOK:
	case ID_LOOKSPRING:
	case ID_LOOKSTRAFE:
	case ID_MOUSEFILTER:
	case ID_MOUSEUPDOWN:
		if (((menuCheckBox_s*)self)->enabled)
			((menuCheckBox_s*)self)->focusPic = SM_ON;
		else
			((menuCheckBox_s*)self)->focusPic = SM_OFF;
		break;
	}

	if( event == QM_CHANGED )
	{
		UI_AdvControls_UpdateConfig();
		return;
	}

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
		UI_PopMenu();
		break;
	}
}

/*
=================
UI_AdvControls_Init
=================
*/
static void UI_AdvControls_Init( void )
{
	memset( &uiAdvControls, 0, sizeof( uiAdvControls_t ));

	uiAdvControls.menu.vidInitFunc = UI_AdvControls_Init;

	uiAdvControls.background.generic.id = ID_BACKGROUND;
	uiAdvControls.background.generic.type = QMTYPE_BITMAP_STRETCH;
	uiAdvControls.background.generic.flags = QMF_INACTIVE;
	uiAdvControls.background.generic.x = 0;
	uiAdvControls.background.generic.y = 0;
	uiAdvControls.background.generic.width = 1024;
	uiAdvControls.background.generic.height = 768;
	uiAdvControls.background.pic = ART_BACKGROUND;

	uiAdvControls.banner.generic.id = ID_BANNER;
	uiAdvControls.banner.generic.type = QMTYPE_BITMAP;
	uiAdvControls.banner.generic.flags = QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiAdvControls.banner.generic.x = UI_BANNER_POSX;
	uiAdvControls.banner.generic.y = UI_BANNER_POSY;
	uiAdvControls.banner.generic.width = 200;
	uiAdvControls.banner.generic.height = 24;
	uiAdvControls.banner.pic = HEADER_MOUSE;
	uiAdvControls.banner.generic.color = uiColorDoomRed;

	// mouse look
	uiAdvControls.mouseLook.generic.id = ID_MOUSELOOK;
	uiAdvControls.mouseLook.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.mouseLook.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiAdvControls.mouseLook.generic.name = "Enable Mouse";
	uiAdvControls.mouseLook.generic.x = -18;
	uiAdvControls.mouseLook.generic.y = 72;
	uiAdvControls.mouseLook.generic.width = 160;
	uiAdvControls.mouseLook.generic.height = 8;
	uiAdvControls.mouseLook.generic.callback = UI_AdvControls_Callback;
	uiAdvControls.mouseLook.pic = BTN_MOUSE;

	uiAdvControls.mouseLook.generic.x2 = 12;
	uiAdvControls.mouseLook.generic.width2 = 32;
	uiAdvControls.mouseLook.focusPic = SM_OFF;

	// mouse look up/down
	uiAdvControls.mouseupdown.generic.id = ID_MOUSEUPDOWN;
	uiAdvControls.mouseupdown.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.mouseupdown.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiAdvControls.mouseupdown.generic.name = "Mouse Look Up/Down";
	uiAdvControls.mouseupdown.generic.x = -18;
	uiAdvControls.mouseupdown.generic.y = 84;
	uiAdvControls.mouseupdown.generic.width = 160;
	uiAdvControls.mouseupdown.generic.height = 8;
	uiAdvControls.mouseupdown.generic.callback = UI_AdvControls_Callback;
	uiAdvControls.mouseupdown.pic = BTN_UPDOWN;

	uiAdvControls.mouseupdown.generic.x2 = 12;
	uiAdvControls.mouseupdown.generic.width2 = 32;
	uiAdvControls.mouseupdown.focusPic = SM_OFF;

	// mouse invert pitch
	uiAdvControls.invertMouse.generic.id = ID_INVERTMOUSE;
	uiAdvControls.invertMouse.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.invertMouse.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiAdvControls.mouseupdown.generic.name = "Mouse invert";
	uiAdvControls.invertMouse.generic.x = -18;
	uiAdvControls.invertMouse.generic.y = 96;
	uiAdvControls.invertMouse.generic.width = 160;
	uiAdvControls.invertMouse.generic.height = 8;
	uiAdvControls.invertMouse.generic.callback = UI_AdvControls_Callback;
	uiAdvControls.invertMouse.pic = BTN_INVERT;

	uiAdvControls.invertMouse.generic.x2 = 12;
	uiAdvControls.invertMouse.generic.width2 = 32;
	uiAdvControls.invertMouse.focusPic = SM_OFF;

	// filter mouse
	uiAdvControls.mouseFilter.generic.id = ID_MOUSEFILTER;
	uiAdvControls.mouseFilter.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.mouseFilter.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiAdvControls.mouseFilter.generic.name = "Filter mouse movement";
	uiAdvControls.mouseFilter.generic.x = -18;
	uiAdvControls.mouseFilter.generic.y = 108;
	uiAdvControls.mouseFilter.generic.width = 160;
	uiAdvControls.mouseFilter.generic.height = 8;
	uiAdvControls.mouseFilter.generic.callback = UI_AdvControls_Callback;
	uiAdvControls.mouseFilter.pic = BTN_FILTER;

	uiAdvControls.mouseFilter.generic.x2 = 12;
	uiAdvControls.mouseFilter.generic.width2 = 32;
	uiAdvControls.mouseFilter.focusPic = SM_OFF;

	// mouse sensitivity
	uiAdvControls.sensitivity.generic.id = ID_SENSITIVITY;
	uiAdvControls.sensitivity.generic.type = QMTYPE_SLIDER;
	uiAdvControls.sensitivity.generic.flags = QMF_CENTERED;
	uiAdvControls.sensitivity.generic.name = "Mouse sensitivity";
	uiAdvControls.sensitivity.generic.x = -34;
	uiAdvControls.sensitivity.generic.y = 136;
	uiAdvControls.sensitivity.generic.callback = UI_AdvControls_Callback;
	uiAdvControls.sensitivity.minValue = 0.0;
	uiAdvControls.sensitivity.maxValue = 1.0;
	uiAdvControls.sensitivity.range = 0.05f;
	uiAdvControls.sensitivity.pic = SHEAD_SENSE;

	// look spring
	uiAdvControls.lookSpring.generic.id = ID_LOOKSPRING;
	uiAdvControls.lookSpring.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.lookSpring.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiAdvControls.lookSpring.generic.name = "Look spring";
	uiAdvControls.lookSpring.generic.x = -18;
	uiAdvControls.lookSpring.generic.y = 156;
	uiAdvControls.lookSpring.generic.width = 160;
	uiAdvControls.lookSpring.generic.height = 8;
	uiAdvControls.lookSpring.generic.callback = UI_AdvControls_Callback;
	uiAdvControls.lookSpring.pic = BTN_SPRING;

	uiAdvControls.lookSpring.generic.x2 = 12;
	uiAdvControls.lookSpring.generic.width2 = 32;
	uiAdvControls.lookSpring.focusPic = SM_OFF;

	// look strafe
	uiAdvControls.lookStrafe.generic.id = ID_LOOKSTRAFE;
	uiAdvControls.lookStrafe.generic.type = QMTYPE_CHECKBOX;
	uiAdvControls.lookStrafe.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiAdvControls.lookStrafe.generic.name = "Look strafe";
	uiAdvControls.lookStrafe.generic.x = -18;
	uiAdvControls.lookStrafe.generic.y = 168;
	uiAdvControls.lookStrafe.generic.width = 160;
	uiAdvControls.lookStrafe.generic.height = 8;
	uiAdvControls.lookStrafe.generic.callback = UI_AdvControls_Callback;
	uiAdvControls.lookStrafe.pic = BTN_STRAFE;

	uiAdvControls.lookStrafe.generic.x2 = 12;
	uiAdvControls.lookStrafe.generic.width2 = 32;
	uiAdvControls.lookStrafe.focusPic = SM_OFF;

	// done
	uiAdvControls.done.generic.id = ID_DONE;
	uiAdvControls.done.generic.type = QMTYPE_BITMAP;
	uiAdvControls.done.generic.name = "Back";
	uiAdvControls.done.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiAdvControls.done.generic.x = -58;
	uiAdvControls.done.generic.y = 178;
	uiAdvControls.done.generic.width = 80;
	uiAdvControls.done.generic.height = 16;
	uiAdvControls.done.generic.callback = UI_AdvControls_Callback;
	uiAdvControls.done.pic = BTN_DONES;
	uiAdvControls.done.generic.color = uiColorDoomRed;
	uiAdvControls.done.generic.focusColor = uiColorDoomSelect;

	UI_AdvControls_GetConfig();

	UI_AddItem(&uiAdvControls.menu, (void*)&uiAdvControls.background);
	UI_AddItem(&uiAdvControls.menu, (void*)&uiAdvControls.banner);
	UI_AddItem(&uiAdvControls.menu, (void*)&uiAdvControls.mouseLook);
	UI_AddItem(&uiAdvControls.menu, (void*)&uiAdvControls.mouseupdown);

	UI_AddItem( &uiAdvControls.menu, (void *)&uiAdvControls.done );
	UI_AddItem( &uiAdvControls.menu, (void *)&uiAdvControls.invertMouse );
	
	UI_AddItem( &uiAdvControls.menu, (void *)&uiAdvControls.lookSpring );
	UI_AddItem( &uiAdvControls.menu, (void *)&uiAdvControls.lookStrafe );
	UI_AddItem( &uiAdvControls.menu, (void *)&uiAdvControls.mouseFilter );
	UI_AddItem( &uiAdvControls.menu, (void *)&uiAdvControls.sensitivity );
}

/*
=================
UI_AdvControls_Precache
=================
*/
void UI_AdvControls_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( HEADER_MOUSE, TF_NEAREST|TF_NOMIPMAP );
}

/*
=================
UI_AdvControls_Menu
=================
*/
void UI_AdvControls_Menu( void )
{
	UI_AdvControls_Precache();
	UI_AdvControls_Init();

	UI_AdvControls_UpdateConfig();
	UI_PushMenu( &uiAdvControls.menu );
}