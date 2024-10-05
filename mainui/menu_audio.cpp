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

#define HEADER_AUDIO		"gfx/shell/headers/header_audio"

#define SHEAD_SOUNDVOL		"gfx/shell/slider/slide_vol"
#define SHEAD_MUSICVOL		"gfx/shell/slider/slide_mus"

#define BTN_SLERP			"gfx/shell/buttons/btn_slerp"
#define BTN_DSP				"gfx/shell/buttons/btn_dsp"

#define S_LINEAR			"gfx/shell/buttons/s_linear"
#define S_CUBIC				"gfx/shell/buttons/s_cubic"

#define ID_BACKGROUND		0
#define ID_BANNER			1
#define ID_DONE				2
#define ID_SUITVOLUME		3
#define ID_SOUNDVOLUME		4
#define ID_MUSICVOLUME		5
#define ID_INTERP			6
#define ID_NODSP			7
#define ID_MSGHINT			8

typedef struct
{
	float		soundVolume;
	float		musicVolume;
	float		suitVolume;
} uiAudioValues_t;

static uiAudioValues_t	uiAudioInitial;

typedef struct
{
	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuSlider_s	soundVolume;
	menuSlider_s	musicVolume;
	menuCheckBox_s	lerping;
	menuCheckBox_s	noDSP;

	menuBitmap_s	done;
} uiAudio_t;

static uiAudio_t		uiAudio;

/*
=================
UI_Audio_GetConfig
=================
*/
static void UI_Audio_GetConfig( void )
{
	uiAudio.soundVolume.curValue = CVAR_GET_FLOAT( "volume" );
	uiAudio.musicVolume.curValue = CVAR_GET_FLOAT( "MP3Volume" );

	if (CVAR_GET_FLOAT("s_lerping"))
	{
		uiAudio.lerping.enabled = CVAR_GET_FLOAT("s_lerping");

		if(uiAudio.lerping.enabled == 1)
			uiAudio.lerping.focusPic = S_LINEAR;
		else if (uiAudio.lerping.enabled == 2)
			uiAudio.lerping.focusPic = S_CUBIC;
	}

	if (CVAR_GET_FLOAT("s_dsp"))
	{
		uiAudio.noDSP.enabled = 1;
		uiAudio.noDSP.focusPic = S_ON;
	}

	// save initial values
	uiAudioInitial.soundVolume = uiAudio.soundVolume.curValue;
	uiAudioInitial.musicVolume = uiAudio.musicVolume.curValue;
}

/*
=================
UI_Audio_SetConfig
=================
*/
static void UI_Audio_SetConfig( void )
{
	CVAR_SET_FLOAT( "volume", uiAudio.soundVolume.curValue );
	CVAR_SET_FLOAT( "MP3Volume", uiAudio.musicVolume.curValue );
	CVAR_SET_FLOAT( "s_lerping", uiAudio.lerping.enabled );
	CVAR_SET_FLOAT( "s_dsp", uiAudio.noDSP.enabled );
}

/*
=================
UI_Audio_UpdateConfig
=================
*/
static void UI_Audio_UpdateConfig( void )
{
	CVAR_SET_FLOAT( "volume", uiAudio.soundVolume.curValue );
	CVAR_SET_FLOAT( "MP3Volume", uiAudio.musicVolume.curValue );
	CVAR_SET_FLOAT( "s_lerping", uiAudio.lerping.enabled );
	CVAR_SET_FLOAT( "s_dsp", uiAudio.noDSP.enabled );
}

/*
=================
UI_Audio_Callback
=================
*/
static void UI_Audio_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	switch( item->id )
	{
	case ID_INTERP:
		if (((menuCheckBox_s*)self)->enabled == 1)
			((menuCheckBox_s*)self)->focusPic = S_LINEAR;
		else if(((menuCheckBox_s*)self)->enabled == 2)
			((menuCheckBox_s*)self)->focusPic = S_CUBIC;
		else
			((menuCheckBox_s*)self)->focusPic = S_OFF;
		break;
	case ID_NODSP:
		if (((menuCheckBox_s*)self)->enabled)
			((menuCheckBox_s*)self)->focusPic = S_ON;
		else
			((menuCheckBox_s*)self)->focusPic = S_OFF;
		break;
	}

	if( event == QM_CHANGED )
	{
		UI_Audio_UpdateConfig();
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
UI_Audio_Init
=================
*/
static void UI_Audio_Init( void )
{
	memset( &uiAudio, 0, sizeof( uiAudio_t ));

	uiAudio.menu.vidInitFunc = UI_Audio_Init;
	
	// background
	uiAudio.background.generic.id	= ID_BACKGROUND;
	uiAudio.background.generic.type = QMTYPE_BITMAP_STRETCH;
	uiAudio.background.generic.flags = QMF_INACTIVE;
	uiAudio.background.generic.x = 0;
	uiAudio.background.generic.y = 0;
	uiAudio.background.generic.width = 1024;
	uiAudio.background.generic.height = 768;
	uiAudio.background.pic = ART_BACKGROUND;

	// header
	uiAudio.banner.generic.id = ID_BANNER;
	uiAudio.banner.generic.type = QMTYPE_BITMAP;
	uiAudio.banner.generic.flags = QMF_INACTIVE|QMF_DRAW_HOLES|QMF_CENTERED;
	uiAudio.banner.generic.x = UI_BANNER_POSX;
	uiAudio.banner.generic.y = UI_BANNER_POSY;
	uiAudio.banner.generic.width = 256;
	uiAudio.banner.generic.height = 24;
	uiAudio.banner.pic = HEADER_AUDIO;
	uiAudio.banner.generic.color = uiColorDoomRed;

	uiAudio.soundVolume.generic.id = ID_SOUNDVOLUME;
	uiAudio.soundVolume.generic.type = QMTYPE_SLIDER;
	uiAudio.soundVolume.generic.flags = QMF_CENTERED;
	uiAudio.soundVolume.generic.name = "Game sound volume";
	uiAudio.soundVolume.generic.x = -28;
	uiAudio.soundVolume.generic.y = 76;
	uiAudio.soundVolume.generic.callback = UI_Audio_Callback;
	uiAudio.soundVolume.minValue	= 0.0;
	uiAudio.soundVolume.maxValue	= 1.0;
	uiAudio.soundVolume.range = 0.05f;
	uiAudio.soundVolume.pic = SHEAD_SOUNDVOL;

	uiAudio.musicVolume.generic.id = ID_MUSICVOLUME;
	uiAudio.musicVolume.generic.type = QMTYPE_SLIDER;
	uiAudio.musicVolume.generic.flags = QMF_CENTERED;
	uiAudio.musicVolume.generic.name = "Game music volume";
	uiAudio.musicVolume.generic.x = -28;
	uiAudio.musicVolume.generic.y = 110;
	uiAudio.musicVolume.generic.callback = UI_Audio_Callback;
	uiAudio.musicVolume.minValue = 0.0;
	uiAudio.musicVolume.maxValue = 1.0;
	uiAudio.musicVolume.range = 0.05f;
	uiAudio.musicVolume.pic = SHEAD_MUSICVOL;

	uiAudio.lerping.generic.id = ID_INTERP;
	uiAudio.lerping.generic.type = QMTYPE_CHECKBOX;
	uiAudio.lerping.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiAudio.lerping.generic.name = "Enable sound interpolation";
	uiAudio.lerping.generic.x = 0;
	uiAudio.lerping.generic.y = 122;
	uiAudio.lerping.generic.width = 184;
	uiAudio.lerping.generic.height = 24;
	uiAudio.lerping.generic.callback = UI_Audio_Callback;
	uiAudio.lerping.pic = BTN_SLERP;
	uiAudio.lerping.states = 2;

	uiAudio.lerping.generic.x2 = 41;
	uiAudio.lerping.generic.width2 = 88;
	uiAudio.lerping.focusPic = S_OFF;

	uiAudio.noDSP.generic.id = ID_NODSP;
	uiAudio.noDSP.generic.type = QMTYPE_CHECKBOX;
	uiAudio.noDSP.generic.flags = QMF_ACT_ONRELEASE | QMF_CENTERED;
	uiAudio.noDSP.generic.name = "Disable DSP effects";
	uiAudio.noDSP.generic.x = 0;
	uiAudio.noDSP.generic.y = 140;
	uiAudio.noDSP.generic.width = 184;
	uiAudio.noDSP.generic.height = 24;
	uiAudio.noDSP.generic.callback = UI_Audio_Callback;
	uiAudio.noDSP.pic = BTN_DSP;

	uiAudio.noDSP.generic.x2 = 135;
	uiAudio.noDSP.generic.width2 = 88;
	uiAudio.noDSP.focusPic = S_OFF;
	
	// done
	uiAudio.done.generic.id = ID_DONE;
	uiAudio.done.generic.type = QMTYPE_BITMAP;
	uiAudio.done.generic.name = "Back";
	uiAudio.done.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiAudio.done.generic.x = -52;
	uiAudio.done.generic.y = 166;
	uiAudio.done.generic.width = 80;
	uiAudio.done.generic.height = 16;
	uiAudio.done.generic.callback = UI_Audio_Callback;
	uiAudio.done.pic = BTN_DONES;
	uiAudio.done.generic.color = uiColorDoomRed;
	uiAudio.done.generic.focusColor = uiColorDoomSelect;

	UI_Audio_GetConfig();

	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.background );
	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.banner );

	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.soundVolume );
	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.musicVolume );

	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.lerping );
	UI_AddItem( &uiAudio.menu, (void *)&uiAudio.noDSP );

	UI_AddItem(&uiAudio.menu, (void*)&uiAudio.done);
}

/*
=================
UI_Audio_Precache
=================
*/
void UI_Audio_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( HEADER_AUDIO, TF_NEAREST | TF_NOMIPMAP);
}

/*
=================
UI_Audio_Menu
=================
*/
void UI_Audio_Menu( void )
{
	UI_Audio_Precache();
	UI_Audio_Init();

	UI_Audio_UpdateConfig();
	UI_PushMenu( &uiAudio.menu );
}