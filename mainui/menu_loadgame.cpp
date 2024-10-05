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

#define ART_BANNER	     	"gfx/shell/headers/header_loadgame"

#define BTN_SAVE1			"gfx/shell/saves/save1"
#define BTN_SAVE2			"gfx/shell/saves/save2"
#define BTN_SAVE3			"gfx/shell/saves/save3"
#define BTN_SAVE4			"gfx/shell/saves/save4"
#define BTN_SAVEQUICK		"gfx/shell/saves/savequick"

#define BTN_LOAD			"gfx/shell/buttons/btn_load"
#define BTN_SAVE			"gfx/shell/buttons/btn_save"
#define BTN_DELETE			"gfx/shell/buttons/btn_delete"

#define DEL_MSG				"gfx/shell/saves/del_mes"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_LOAD		2
#define ID_DELETE		3
#define ID_CANCEL		4
#define ID_DELMESSAGE 	9
#define ID_SAVEPIC	 	10
#define ID_YES	 	130
#define ID_NO	 	131

#define ID_SAVE1	 	11
#define ID_SAVE2	 	12
#define ID_SAVE3	 	13
#define ID_SAVE4	 	14
#define ID_SAVEQUICK 	15

#define LEVELSHOT_X		72
#define LEVELSHOT_Y		400
#define LEVELSHOT_W		192
#define LEVELSHOT_H		160

#define TIME_LENGTH		20
#define NAME_LENGTH		32+TIME_LENGTH
#define GAMETIME_LENGTH	15+NAME_LENGTH

typedef struct
{
	int valuetest;

	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;

	menuBitmap_s	save1;
	menuBitmap_s	save2;
	menuBitmap_s	save3;
	menuBitmap_s	save4;
	menuBitmap_s	savequick;

	menuBitmap_s	savePic;

	menuBitmap_s	load;
	menuBitmap_s	remove;
	menuBitmap_s	cancel;

	// prompt dialog
	menuBitmap_s	deleteMessage1;
	menuBitmap_s	yes;
	menuBitmap_s	no;
} uiLoadGame_t;

static uiLoadGame_t		uiLoadGame;

static void UI_DeleteDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide remove dialog
	uiLoadGame.load.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.remove.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.cancel.generic.flags ^= QMF_HIDDEN;

	uiLoadGame.save1.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.save2.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.save3.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.save4.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.savequick.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.savePic.generic.flags ^= QMF_HIDDEN;
	
	uiLoadGame.deleteMessage1.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.no.generic.flags ^= QMF_HIDDEN;
	uiLoadGame.yes.generic.flags ^= QMF_HIDDEN;
}

/*
=================
UI_LoadGame_KeyFunc
=================
*/
static const char *UI_LoadGame_KeyFunc( int key, int down )
{
	if (down && key == K_ESCAPE)
	{
		if (!(uiLoadGame.deleteMessage1.generic.flags & QMF_HIDDEN))
		{
			UI_DeleteDialog();
			return uiSoundOut;
		}
		else
		{
			UI_PopMenu();
		}
	}
	return UI_DefaultKey( &uiLoadGame.menu, key, down );
}

/*
=================
UI_LoadGame_Callback
=================
*/
static void UI_LoadGame_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;
	
	switch( item->id )
	{
	case ID_SAVE1:
		if (uiLoadGame.valuetest != 1)
			uiLoadGame.valuetest = 1;
		break;

	case ID_SAVE2:
		if (uiLoadGame.valuetest != 2)
			uiLoadGame.valuetest = 2;
		break;

	case ID_SAVE3:
		if (uiLoadGame.valuetest != 3)
			uiLoadGame.valuetest = 3;
		break;

	case ID_SAVE4:
		if (uiLoadGame.valuetest != 4)
			uiLoadGame.valuetest = 4;
		break;

	case ID_SAVEQUICK:
		if (uiLoadGame.valuetest != 5)
			uiLoadGame.valuetest = 5;
		break;

	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_LOAD:
		if (uiLoadGame.valuetest == 5)
		{
			BACKGROUND_TRACK(NULL, NULL);
			CVAR_SET_FLOAT("v_dark", 1.0f);
			CLIENT_COMMAND(FALSE, "load quick");
		}
		else if (uiLoadGame.valuetest)
		{
			char	cmd[128];
			sprintf( cmd, "load slot%i\n", uiLoadGame.valuetest);

			BACKGROUND_TRACK( NULL, NULL );

			CVAR_SET_FLOAT("v_dark", 1.0f);

			CLIENT_COMMAND( FALSE, cmd );
		}
		break;
	case ID_NO:
	case ID_DELETE:
		UI_DeleteDialog();
		break;
	case ID_YES:
		if (uiLoadGame.valuetest)
		{
			char	cmd[128];

			if (uiLoadGame.valuetest == 5)
				sprintf(cmd, "killsave quick\n");
			else
				sprintf(cmd, "killsave slot%i\n", uiLoadGame.valuetest);

			CLIENT_COMMAND(TRUE, cmd);

			if (uiLoadGame.valuetest == 5)
				sprintf(cmd, "save/quick.bmp\n");
			else
				sprintf(cmd, "save/slot%i.bmp\n", uiLoadGame.valuetest);

			PIC_Free(cmd);

			// restarts the menu
			uiLoadGame.valuetest = 0;
			UI_PopMenu();
			UI_LoadGame_Menu();
			return;
		}
		break;
	}
}

static void UI_Load_BitmapDraw(void* self)
{
	menuCommon_s* item = (menuCommon_s*)self;
	char saveshot[128];

	if ((menuCommon_s*)self != (menuCommon_s*)UI_ItemAtCursor(uiLoadGame.load.generic.parent))
	{
		UI_DrawPicHoles(((menuBitmap_s*)self)->generic.x, ((menuBitmap_s*)self)->generic.y,
			((menuBitmap_s*)self)->generic.width, ((menuBitmap_s*)self)->generic.height, ((menuBitmap_s*)self)->generic.color, ((menuBitmap_s*)self)->pic);
	}
	else
	{
		UI_DrawPicHoles(((menuBitmap_s*)self)->generic.x, ((menuBitmap_s*)self)->generic.y,
			((menuBitmap_s*)self)->generic.width, ((menuBitmap_s*)self)->generic.height, ((menuBitmap_s*)self)->generic.focusColor, ((menuBitmap_s*)self)->pic);
	}

	switch (item->id)
	{
	case ID_DELETE:
	case ID_LOAD:
		if (uiLoadGame.valuetest)
		{
			((menuBitmap_s*)self)->generic.color = uiColorDoomRed;
			((menuBitmap_s*)self)->generic.focusColor = uiColorDoomSelect;
			((menuBitmap_s*)self)->generic.flags &= ~QMF_INACTIVE;
		}
		else
		{
			((menuBitmap_s*)self)->generic.color = uiColorDkGrey;
			((menuBitmap_s*)self)->generic.focusColor = uiColorDkGrey;
			((menuBitmap_s*)self)->generic.flags |= QMF_INACTIVE;
		}
		break;

	case ID_SAVE1:
		sprintf(saveshot, "save/slot1.bmp");

		if (!g_engfuncs.pfnFileExists(saveshot, TRUE))
		{
			uiLoadGame.save1.generic.flags |= QMF_INACTIVE;
			uiLoadGame.save1.generic.color = uiColorDkGrey;
			uiLoadGame.save1.generic.focusColor = uiColorDkGrey;
		}
		else if (uiLoadGame.valuetest == 1)
		{
			uiLoadGame.save1.generic.flags &= ~QMF_INACTIVE;
			uiLoadGame.save1.generic.color = uiLoadGame.save1.generic.focusColor = uiColorDoomSave;
		}
		else
		{
			uiLoadGame.save1.generic.flags &= ~QMF_INACTIVE;
			uiLoadGame.save1.generic.color = uiColorDoomRed;
			uiLoadGame.save1.generic.focusColor = uiColorDoomSelect;
		}
		break;

	case ID_SAVE2:
		sprintf(saveshot, "save/slot2.bmp");

		if (!g_engfuncs.pfnFileExists(saveshot, TRUE))
		{
			uiLoadGame.save2.generic.flags |= QMF_INACTIVE;
			uiLoadGame.save2.generic.color = uiColorDkGrey;
			uiLoadGame.save2.generic.focusColor = uiColorDkGrey;
		}
		else if (uiLoadGame.valuetest == 2)
		{
			uiLoadGame.save2.generic.flags &= ~QMF_INACTIVE;
			uiLoadGame.save2.generic.color = uiLoadGame.save2.generic.focusColor = uiColorDoomSave;
		}
		else
		{
			uiLoadGame.save2.generic.flags &= ~QMF_INACTIVE;
			uiLoadGame.save2.generic.color = uiColorDoomRed;
			uiLoadGame.save2.generic.focusColor = uiColorDoomSelect;
		}
		break;

	case ID_SAVE3:
		sprintf(saveshot, "save/slot3.bmp");

		if (!g_engfuncs.pfnFileExists(saveshot, TRUE))
		{
			uiLoadGame.save3.generic.flags |= QMF_INACTIVE;
			uiLoadGame.save3.generic.color = uiColorDkGrey;
			uiLoadGame.save3.generic.focusColor = uiColorDkGrey;
		}
		else if (uiLoadGame.valuetest == 3)
		{
			uiLoadGame.save3.generic.flags &= ~QMF_INACTIVE;
			uiLoadGame.save3.generic.color = uiLoadGame.save3.generic.focusColor = uiColorDoomSave;
		}
		else
		{
			uiLoadGame.save3.generic.flags &= ~QMF_INACTIVE;
			uiLoadGame.save3.generic.color = uiColorDoomRed;
			uiLoadGame.save3.generic.focusColor = uiColorDoomSelect;
		}
		break;

	case ID_SAVE4:
		sprintf(saveshot, "save/slot4.bmp");

		if (!g_engfuncs.pfnFileExists(saveshot, TRUE))
		{
			uiLoadGame.save4.generic.flags |= QMF_INACTIVE;
			uiLoadGame.save4.generic.color = uiColorDkGrey;
			uiLoadGame.save4.generic.focusColor = uiColorDkGrey;
		}
		else if (uiLoadGame.valuetest == 4)
		{
			uiLoadGame.save4.generic.flags &= ~QMF_INACTIVE;
			uiLoadGame.save4.generic.color = uiLoadGame.save4.generic.focusColor = uiColorDoomSave;
		}
		else
		{
			uiLoadGame.save4.generic.flags &= ~QMF_INACTIVE;
			uiLoadGame.save4.generic.color = uiColorDoomRed;
			uiLoadGame.save4.generic.focusColor = uiColorDoomSelect;
		}
		break;

	case ID_SAVEQUICK:
		sprintf(saveshot, "save/quick.bmp");

		if (!g_engfuncs.pfnFileExists(saveshot, TRUE))
		{
			uiLoadGame.savequick.generic.flags |= QMF_INACTIVE;
			uiLoadGame.savequick.generic.color = uiColorDkGrey;
			uiLoadGame.savequick.generic.focusColor = uiColorDkGrey;
		}
		else if (uiLoadGame.valuetest == 5)
		{
			uiLoadGame.savequick.generic.flags &= ~QMF_INACTIVE;
			uiLoadGame.savequick.generic.color = uiLoadGame.savequick.generic.focusColor = uiColorDoomSave;
		}
		else
		{
			uiLoadGame.savequick.generic.flags &= ~QMF_INACTIVE;
			uiLoadGame.savequick.generic.color = uiColorDoomRed;
			uiLoadGame.savequick.generic.focusColor = uiColorDoomSelect;
		}
		break;
	}
}

static void UI_Load_SavePicDraw(void* self)
{
	menuCommon_s* item = (menuCommon_s*)self;
	char saveshot[128];
	char savename[128];
	char comment[128];
	int x, y, w, h;

	x = ((menuBitmap_s*)self)->generic.x;
	y = ((menuBitmap_s*)self)->generic.y;
	w = ((menuBitmap_s*)self)->generic.width;
	h = ((menuBitmap_s*)self)->generic.height;

	switch (item->id)
	{
	case ID_SAVEPIC:

		if (uiLoadGame.valuetest == 5)
		{
			sprintf(saveshot, "save/quick.bmp");
			sprintf(savename, "save/quick.sav");
		}
		else
		{
			sprintf(saveshot, "save/slot%i.bmp", uiLoadGame.valuetest);
			sprintf(savename, "save/slot%i.sav", uiLoadGame.valuetest);
		}

		if (uiLoadGame.valuetest)
			uiLoadGame.savePic.pic = saveshot;
		else
			uiLoadGame.savePic.pic = PIC_NOSAVE;

		UI_DrawPicHoles(x, y, w, h, ((menuBitmap_s*)self)->generic.color, ((menuBitmap_s*)self)->pic);
		UI_DrawRectangleExt(x, y, w, h, 0xff8b0000, 4);

		if (GET_SAVE_COMMENT(savename, comment))
		{
			if (strlen(comment))
			{
				//Con_Printf("save comment: %s\n", comment);
				int charWidth = UI_SMALL_CHAR_WIDTH;
				int charHeight = UI_SMALL_CHAR_HEIGHT;
				UI_ScaleCoords(NULL, NULL, &charWidth, &charHeight, false);

				x = ((menuBitmap_s*)self)->generic.x;
				y = ((menuBitmap_s*)self)->generic.y + ((menuBitmap_s*)self)->generic.height * 1.025;
				w = ((menuBitmap_s*)self)->generic.width;
				h = ((menuBitmap_s*)self)->generic.height;

				UI_DrawString(x, y, w, h, comment, uiColorDkGrey, true, charWidth, charHeight, 0, 0);
			}
		}
		break;
	}
}

/*
=================
UI_LoadGame_Init
=================
*/
static void UI_LoadGame_Init( void )
{
	memset( &uiLoadGame, 0, sizeof( uiLoadGame_t ));

	uiLoadGame.menu.vidInitFunc = UI_LoadGame_Init;
	uiLoadGame.menu.keyFunc = UI_LoadGame_KeyFunc;

	uiLoadGame.background.generic.id = ID_BACKGROUND;
	uiLoadGame.background.generic.type = QMTYPE_BITMAP_STRETCH;
	uiLoadGame.background.generic.flags = QMF_INACTIVE;
	uiLoadGame.background.generic.x = 0;
	uiLoadGame.background.generic.y = 0;
	uiLoadGame.background.generic.width = 1024;
	uiLoadGame.background.generic.height = 768;
	uiLoadGame.background.pic = ART_BACKGROUND;

	uiLoadGame.banner.generic.id = ID_BANNER;
	uiLoadGame.banner.generic.type = QMTYPE_BITMAP;
	uiLoadGame.banner.generic.flags = QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.banner.generic.x = UI_BANNER_POSX;
	uiLoadGame.banner.generic.y = UI_BANNER_POSY;
	uiLoadGame.banner.generic.width = 264;
	uiLoadGame.banner.generic.height = 24;
	uiLoadGame.banner.pic = ART_BANNER;
	uiLoadGame.banner.generic.color = uiColorDoomRed;

	uiLoadGame.save1.generic.id = ID_SAVE1;
	uiLoadGame.save1.generic.type = QMTYPE_BITMAP;
	uiLoadGame.save1.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.save1.generic.x = -68;
	uiLoadGame.save1.generic.y = 64;
	uiLoadGame.save1.generic.width = 104;
	uiLoadGame.save1.generic.height = 16;
	uiLoadGame.save1.generic.callback = UI_LoadGame_Callback;
	uiLoadGame.save1.pic = BTN_SAVE1;
	uiLoadGame.save1.generic.color = uiColorDoomRed;
	uiLoadGame.save1.generic.focusColor = uiColorDoomSelect;
	uiLoadGame.save1.generic.ownerdraw = UI_Load_BitmapDraw;

	uiLoadGame.save2.generic.id = ID_SAVE2;
	uiLoadGame.save2.generic.type = QMTYPE_BITMAP;
	uiLoadGame.save2.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.save2.generic.x = -68;
	uiLoadGame.save2.generic.y = 84;
	uiLoadGame.save2.generic.width = 104;
	uiLoadGame.save2.generic.height = 16;
	uiLoadGame.save2.generic.callback = UI_LoadGame_Callback;
	uiLoadGame.save2.pic = BTN_SAVE2;
	uiLoadGame.save2.generic.color = uiColorDoomRed;
	uiLoadGame.save2.generic.focusColor = uiColorDoomSelect;
	uiLoadGame.save2.generic.ownerdraw = UI_Load_BitmapDraw;

	uiLoadGame.save3.generic.id = ID_SAVE3;
	uiLoadGame.save3.generic.type = QMTYPE_BITMAP;
	uiLoadGame.save3.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.save3.generic.x = -68;
	uiLoadGame.save3.generic.y = 104;
	uiLoadGame.save3.generic.width = 104;
	uiLoadGame.save3.generic.height = 16;
	uiLoadGame.save3.generic.callback = UI_LoadGame_Callback;
	uiLoadGame.save3.pic = BTN_SAVE3;
	uiLoadGame.save3.generic.color = uiColorDoomRed;
	uiLoadGame.save3.generic.focusColor = uiColorDoomSelect;
	uiLoadGame.save3.generic.ownerdraw = UI_Load_BitmapDraw;

	uiLoadGame.save4.generic.id = ID_SAVE4;
	uiLoadGame.save4.generic.type = QMTYPE_BITMAP;
	uiLoadGame.save4.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.save4.generic.x = -68;
	uiLoadGame.save4.generic.y = 124;
	uiLoadGame.save4.generic.width = 104;
	uiLoadGame.save4.generic.height = 16;
	uiLoadGame.save4.generic.callback = UI_LoadGame_Callback;
	uiLoadGame.save4.pic = BTN_SAVE4;
	uiLoadGame.save4.generic.color = uiColorDoomRed;
	uiLoadGame.save4.generic.focusColor = uiColorDoomSelect;
	uiLoadGame.save4.generic.ownerdraw = UI_Load_BitmapDraw;

	uiLoadGame.savequick.generic.id = ID_SAVEQUICK;
	uiLoadGame.savequick.generic.type = QMTYPE_BITMAP;
	uiLoadGame.savequick.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.savequick.generic.x = -68;
	uiLoadGame.savequick.generic.y = 144;
	uiLoadGame.savequick.generic.width = 104;
	uiLoadGame.savequick.generic.height = 16;
	uiLoadGame.savequick.generic.callback = UI_LoadGame_Callback;
	uiLoadGame.savequick.pic = BTN_SAVEQUICK;
	uiLoadGame.savequick.generic.color = uiColorDoomRed;
	uiLoadGame.savequick.generic.focusColor = uiColorDoomSelect;
	uiLoadGame.savequick.generic.ownerdraw = UI_Load_BitmapDraw;

	// pic
	uiLoadGame.savePic.generic.id = ID_SAVEPIC;
	uiLoadGame.savePic.generic.type = QMTYPE_BITMAP;
	uiLoadGame.savePic.generic.flags = QMF_INACTIVE | QMF_CENTERED;
	uiLoadGame.savePic.generic.x = 48;
	uiLoadGame.savePic.generic.y = 66;
	uiLoadGame.savePic.generic.width = 128;
	uiLoadGame.savePic.generic.height = 96;
	uiLoadGame.savePic.pic = PIC_NOSAVE;
	uiLoadGame.savePic.generic.ownerdraw = UI_Load_SavePicDraw;

	// load
	uiLoadGame.load.generic.id = ID_LOAD;
	uiLoadGame.load.generic.type = QMTYPE_BITMAP;
	uiLoadGame.load.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.load.generic.x = -68;
	uiLoadGame.load.generic.y = 166;
	uiLoadGame.load.generic.width = 80;
	uiLoadGame.load.generic.height = 16;
	uiLoadGame.load.generic.callback = UI_LoadGame_Callback;
	uiLoadGame.load.pic = BTN_LOAD;
	uiLoadGame.load.generic.color = uiColorDoomRed;
	uiLoadGame.load.generic.focusColor = uiColorDoomSelect;
	uiLoadGame.load.generic.ownerdraw = UI_Load_BitmapDraw;

	// delete
	uiLoadGame.remove.generic.id = ID_DELETE;
	uiLoadGame.remove.generic.type = QMTYPE_BITMAP;
	uiLoadGame.remove.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.remove.generic.x = -68;
	uiLoadGame.remove.generic.y = 184;
	uiLoadGame.remove.generic.width = 80;
	uiLoadGame.remove.generic.height = 16;
	uiLoadGame.remove.generic.callback = UI_LoadGame_Callback;
	uiLoadGame.remove.pic = BTN_DELETE;
	uiLoadGame.remove.generic.color = uiColorDoomRed;
	uiLoadGame.remove.generic.focusColor = uiColorDoomSelect;
	uiLoadGame.remove.generic.ownerdraw = UI_Load_BitmapDraw;

	// done
	uiLoadGame.cancel.generic.id = ID_CANCEL;
	uiLoadGame.cancel.generic.type = QMTYPE_BITMAP;
	uiLoadGame.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.cancel.generic.x = -68;
	uiLoadGame.cancel.generic.y = 202;
	uiLoadGame.cancel.generic.width = 80;
	uiLoadGame.cancel.generic.height = 16;
	uiLoadGame.cancel.generic.callback = UI_LoadGame_Callback;
	uiLoadGame.cancel.pic = BTN_DONES;
	uiLoadGame.cancel.generic.color = uiColorDoomRed;
	uiLoadGame.cancel.generic.focusColor = uiColorDoomSelect;

	uiLoadGame.deleteMessage1.generic.id = ID_DELMESSAGE;
	uiLoadGame.deleteMessage1.generic.type = QMTYPE_BITMAP;
	uiLoadGame.deleteMessage1.generic.flags = QMF_HIDDEN | QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.deleteMessage1.generic.x = 0;
	uiLoadGame.deleteMessage1.generic.y = 84;
	uiLoadGame.deleteMessage1.generic.width = 256;
	uiLoadGame.deleteMessage1.generic.height = 24;
	uiLoadGame.deleteMessage1.pic = DEL_MSG;
	uiLoadGame.deleteMessage1.generic.color = uiColorDoomRed;

	uiLoadGame.yes.generic.id = ID_YES;
	uiLoadGame.yes.generic.type = QMTYPE_BITMAP;
	uiLoadGame.yes.generic.flags = QMF_HIDDEN | QMF_HIGHLIGHTIFFOCUS | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.yes.generic.x = -UI_YESNO_POSX;
	uiLoadGame.yes.generic.y = 128;
	uiLoadGame.yes.generic.width = 56;
	uiLoadGame.yes.generic.height = 24;
	uiLoadGame.yes.generic.callback = UI_LoadGame_Callback;
	uiLoadGame.yes.pic = BTN_YES;
	uiLoadGame.yes.generic.color = uiColorDoomRed;
	uiLoadGame.yes.generic.focusColor = uiColorDoomSelect;

	uiLoadGame.no.generic.id = ID_NO;
	uiLoadGame.no.generic.type = QMTYPE_BITMAP;
	uiLoadGame.no.generic.name = "No";
	uiLoadGame.no.generic.flags = QMF_HIDDEN | QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiLoadGame.no.generic.x = UI_YESNO_POSX;
	uiLoadGame.no.generic.y = 128;
	uiLoadGame.no.generic.width = 48;
	uiLoadGame.no.generic.height = 24;
	uiLoadGame.no.generic.callback = UI_LoadGame_Callback;
	uiLoadGame.no.pic = BTN_NO;
	uiLoadGame.no.generic.color = uiColorDoomRed;
	uiLoadGame.no.generic.focusColor = uiColorDoomSelect;

	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.background);
	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.banner);

	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.save1);
	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.save2);
	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.save3);
	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.save4);
	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.savequick);
	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.savePic);

	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.load);
	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.remove);
	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.cancel);

	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.deleteMessage1);
	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.yes);
	UI_AddItem(&uiLoadGame.menu, (void*)&uiLoadGame.no);
}

/*
=================
UI_LoadGame_Precache
=================
*/
void UI_LoadGame_Precache( void )
{
	PIC_Load( ART_BACKGROUND );
	PIC_Load( ART_BANNER, TF_NEAREST|TF_NOMIPMAP );
}

/*
=================
UI_LoadGame_Menu
=================
*/
void UI_LoadGame_Menu( void )
{
	if( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		// completely ignore save\load menus for multiplayer_only
		return;
	}

	if( !CheckGameDll( )) return;

	UI_LoadGame_Precache();
	UI_LoadGame_Init();

	UI_PushMenu( &uiLoadGame.menu );
}