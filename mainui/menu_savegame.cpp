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

#define ART_BANNER	     	"gfx/shell/headers/header_savegame"

#define BTN_SAVE1			"gfx/shell/saves/save1"
#define BTN_SAVE2			"gfx/shell/saves/save2"
#define BTN_SAVE3			"gfx/shell/saves/save3"
#define BTN_SAVE4			"gfx/shell/saves/save4"

#define BTN_SAVE			"gfx/shell/buttons/btn_save"
#define BTN_DELETE			"gfx/shell/buttons/btn_delete"

#define DEL_MSG				"gfx/shell/saves/del_mes"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_SAVE		2
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

	menuBitmap_s	savePic;

	menuBitmap_s	save;
	menuBitmap_s	remove;
	menuBitmap_s	cancel;

	// prompt dialog
	menuBitmap_s	deleteMessage1;
	menuBitmap_s	yes;
	menuBitmap_s	no;
} uiSaveGame_t;

static uiSaveGame_t		uiSaveGame;

static void UI_DeleteDialog(void)
{
	// toggle main menu between active\inactive
	// show\hide remove dialog
	uiSaveGame.save.generic.flags ^= QMF_HIDDEN;
	uiSaveGame.remove.generic.flags ^= QMF_HIDDEN;
	uiSaveGame.cancel.generic.flags ^= QMF_HIDDEN;

	uiSaveGame.save1.generic.flags ^= QMF_HIDDEN;
	uiSaveGame.save2.generic.flags ^= QMF_HIDDEN;
	uiSaveGame.save3.generic.flags ^= QMF_HIDDEN;
	uiSaveGame.save4.generic.flags ^= QMF_HIDDEN;
	uiSaveGame.savePic.generic.flags ^= QMF_HIDDEN;

	uiSaveGame.deleteMessage1.generic.flags ^= QMF_HIDDEN;
	uiSaveGame.no.generic.flags ^= QMF_HIDDEN;
	uiSaveGame.yes.generic.flags ^= QMF_HIDDEN;
}

/*
=================
UI_SaveGame_KeyFunc
=================
*/
static const char* UI_SaveGame_KeyFunc(int key, int down)
{
	if (down && key == K_ESCAPE)
	{
		if (!(uiSaveGame.deleteMessage1.generic.flags & QMF_HIDDEN))
		{
			UI_DeleteDialog();
			return uiSoundOut;
		}
		else
		{
			UI_PopMenu();
		}
	}
	return UI_DefaultKey(&uiSaveGame.menu, key, down);
}

/*
=================
UI_SaveGame_Callback
=================
*/
static void UI_SaveGame_Callback(void* self, int event)
{
	menuCommon_s* item = (menuCommon_s*)self;

	if (event != QM_ACTIVATED)
		return;

	switch (item->id)
	{
	case ID_SAVE1:
		if (uiSaveGame.valuetest != 1)
			uiSaveGame.valuetest = 1;
		break;

	case ID_SAVE2:
		if (uiSaveGame.valuetest != 2)
			uiSaveGame.valuetest = 2;
		break;

	case ID_SAVE3:
		if (uiSaveGame.valuetest != 3)
			uiSaveGame.valuetest = 3;
		break;

	case ID_SAVE4:
		if (uiSaveGame.valuetest != 4)
			uiSaveGame.valuetest = 4;
		break;

	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_SAVE:
		if (uiSaveGame.valuetest)
		{
			char	cmd[128];
			sprintf(cmd, "save slot%i\n", uiSaveGame.valuetest);

			CLIENT_COMMAND(FALSE, cmd);
			UI_CloseMenu();
		}
		break;
	case ID_NO:
	case ID_DELETE:
		UI_DeleteDialog();
		break;
	case ID_YES:
		if (uiSaveGame.valuetest)
		{
			char	cmd[128];
			sprintf(cmd, "killsave slot%i\n", uiSaveGame.valuetest);

			CLIENT_COMMAND(TRUE, cmd);

			sprintf(cmd, "save/slot%i.bmp\n", uiSaveGame.valuetest);

			PIC_Free(cmd);

			// restarts the menu
			uiSaveGame.valuetest = 0;
			UI_PopMenu();
			UI_SaveGame_Menu();
			return;
		}
		break;
	}
}

static void UI_Load_BitmapDraw(void* self)
{
	menuCommon_s* item = (menuCommon_s*)self;
	char saveshot[128];

	if ((menuCommon_s*)self != (menuCommon_s*)UI_ItemAtCursor(uiSaveGame.save.generic.parent))
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
		sprintf(saveshot, "save/slot%i.bmp", uiSaveGame.valuetest);

		if (uiSaveGame.valuetest && g_engfuncs.pfnFileExists(saveshot, TRUE))
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
	case ID_SAVE:
		if (uiSaveGame.valuetest)
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
		if (uiSaveGame.valuetest == 1)
		{
			uiSaveGame.save1.generic.flags &= ~QMF_INACTIVE;
			uiSaveGame.save1.generic.color = uiSaveGame.save1.generic.focusColor = uiColorDoomSave;
		}
		else
		{
			uiSaveGame.save1.generic.flags &= ~QMF_INACTIVE;
			uiSaveGame.save1.generic.color = uiColorDoomRed;
			uiSaveGame.save1.generic.focusColor = uiColorDoomSelect;
		}
		break;

	case ID_SAVE2:
		if (uiSaveGame.valuetest == 2)
		{
			uiSaveGame.save2.generic.flags &= ~QMF_INACTIVE;
			uiSaveGame.save2.generic.color = uiSaveGame.save2.generic.focusColor = uiColorDoomSave;
		}
		else
		{
			uiSaveGame.save2.generic.flags &= ~QMF_INACTIVE;
			uiSaveGame.save2.generic.color = uiColorDoomRed;
			uiSaveGame.save2.generic.focusColor = uiColorDoomSelect;
		}
		break;

	case ID_SAVE3:
		if (uiSaveGame.valuetest == 3)
		{
			uiSaveGame.save3.generic.flags &= ~QMF_INACTIVE;
			uiSaveGame.save3.generic.color = uiSaveGame.save3.generic.focusColor = uiColorDoomSave;
		}
		else
		{
			uiSaveGame.save3.generic.flags &= ~QMF_INACTIVE;
			uiSaveGame.save3.generic.color = uiColorDoomRed;
			uiSaveGame.save3.generic.focusColor = uiColorDoomSelect;
		}
		break;

	case ID_SAVE4:
		if (uiSaveGame.valuetest == 4)
		{
			uiSaveGame.save4.generic.flags &= ~QMF_INACTIVE;
			uiSaveGame.save4.generic.color = uiSaveGame.save4.generic.focusColor = uiColorDoomSave;
		}
		else
		{
			uiSaveGame.save4.generic.flags &= ~QMF_INACTIVE;
			uiSaveGame.save4.generic.color = uiColorDoomRed;
			uiSaveGame.save4.generic.focusColor = uiColorDoomSelect;
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
		sprintf(saveshot, "save/slot%i.bmp", uiSaveGame.valuetest);
		sprintf(savename, "save/slot%i.sav", uiSaveGame.valuetest);

		if (uiSaveGame.valuetest && g_engfuncs.pfnFileExists(saveshot, TRUE))
		{
			uiSaveGame.savePic.pic = saveshot;
		}
		else
		{
			uiSaveGame.savePic.pic = PIC_NOSAVE;
		}

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
UI_SaveGame_Init
=================
*/
static void UI_SaveGame_Init(void)
{
	memset(&uiSaveGame, 0, sizeof(uiSaveGame_t));

	uiSaveGame.menu.vidInitFunc = UI_SaveGame_Init;
	uiSaveGame.menu.keyFunc = UI_SaveGame_KeyFunc;

	uiSaveGame.background.generic.id = ID_BACKGROUND;
	uiSaveGame.background.generic.type = QMTYPE_BITMAP_STRETCH;
	uiSaveGame.background.generic.flags = QMF_INACTIVE;
	uiSaveGame.background.generic.x = 0;
	uiSaveGame.background.generic.y = 0;
	uiSaveGame.background.generic.width = 1024;
	uiSaveGame.background.generic.height = 768;
	uiSaveGame.background.pic = ART_BACKGROUND;

	uiSaveGame.banner.generic.id = ID_BANNER;
	uiSaveGame.banner.generic.type = QMTYPE_BITMAP;
	uiSaveGame.banner.generic.flags = QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.banner.generic.x = UI_BANNER_POSX;
	uiSaveGame.banner.generic.y = UI_BANNER_POSY;
	uiSaveGame.banner.generic.width = 264;
	uiSaveGame.banner.generic.height = 24;
	uiSaveGame.banner.pic = ART_BANNER;
	uiSaveGame.banner.generic.color = uiColorDoomRed;

	uiSaveGame.save1.generic.id = ID_SAVE1;
	uiSaveGame.save1.generic.type = QMTYPE_BITMAP;
	uiSaveGame.save1.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.save1.generic.x = -56;
	uiSaveGame.save1.generic.y = 60;
	uiSaveGame.save1.generic.width = 104;
	uiSaveGame.save1.generic.height = 16;
	uiSaveGame.save1.generic.callback = UI_SaveGame_Callback;
	uiSaveGame.save1.pic = BTN_SAVE1;
	uiSaveGame.save1.generic.color = uiColorDoomRed;
	uiSaveGame.save1.generic.focusColor = uiColorDoomSelect;
	uiSaveGame.save1.generic.ownerdraw = UI_Load_BitmapDraw;

	uiSaveGame.save2.generic.id = ID_SAVE2;
	uiSaveGame.save2.generic.type = QMTYPE_BITMAP;
	uiSaveGame.save2.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.save2.generic.x = -56;
	uiSaveGame.save2.generic.y = 84;
	uiSaveGame.save2.generic.width = 104;
	uiSaveGame.save2.generic.height = 16;
	uiSaveGame.save2.generic.callback = UI_SaveGame_Callback;
	uiSaveGame.save2.pic = BTN_SAVE2;
	uiSaveGame.save2.generic.color = uiColorDoomRed;
	uiSaveGame.save2.generic.focusColor = uiColorDoomSelect;
	uiSaveGame.save2.generic.ownerdraw = UI_Load_BitmapDraw;

	uiSaveGame.save3.generic.id = ID_SAVE3;
	uiSaveGame.save3.generic.type = QMTYPE_BITMAP;
	uiSaveGame.save3.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.save3.generic.x = -56;
	uiSaveGame.save3.generic.y = 108;
	uiSaveGame.save3.generic.width = 104;
	uiSaveGame.save3.generic.height = 16;
	uiSaveGame.save3.generic.callback = UI_SaveGame_Callback;
	uiSaveGame.save3.pic = BTN_SAVE3;
	uiSaveGame.save3.generic.color = uiColorDoomRed;
	uiSaveGame.save3.generic.focusColor = uiColorDoomSelect;
	uiSaveGame.save3.generic.ownerdraw = UI_Load_BitmapDraw;

	uiSaveGame.save4.generic.id = ID_SAVE4;
	uiSaveGame.save4.generic.type = QMTYPE_BITMAP;
	uiSaveGame.save4.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.save4.generic.x = -56;
	uiSaveGame.save4.generic.y = 132;
	uiSaveGame.save4.generic.width = 104;
	uiSaveGame.save4.generic.height = 16;
	uiSaveGame.save4.generic.callback = UI_SaveGame_Callback;
	uiSaveGame.save4.pic = BTN_SAVE4;
	uiSaveGame.save4.generic.color = uiColorDoomRed;
	uiSaveGame.save4.generic.focusColor = uiColorDoomSelect;
	uiSaveGame.save4.generic.ownerdraw = UI_Load_BitmapDraw;

	// pic
	uiSaveGame.savePic.generic.id = ID_SAVEPIC;
	uiSaveGame.savePic.generic.type = QMTYPE_BITMAP;
	uiSaveGame.savePic.generic.flags = QMF_INACTIVE | QMF_CENTERED;
	uiSaveGame.savePic.generic.x = 48;
	uiSaveGame.savePic.generic.y = 66;
	uiSaveGame.savePic.generic.width = 128;
	uiSaveGame.savePic.generic.height = 96;
	uiSaveGame.savePic.pic = PIC_NOSAVE;
	uiSaveGame.savePic.generic.ownerdraw = UI_Load_SavePicDraw;

	// save
	uiSaveGame.save.generic.id = ID_SAVE;
	uiSaveGame.save.generic.type = QMTYPE_BITMAP;
	uiSaveGame.save.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.save.generic.x = -68;
	uiSaveGame.save.generic.y = 166;
	uiSaveGame.save.generic.width = 80;
	uiSaveGame.save.generic.height = 16;
	uiSaveGame.save.generic.callback = UI_SaveGame_Callback;
	uiSaveGame.save.pic = BTN_SAVE;
	uiSaveGame.save.generic.color = uiColorDoomRed;
	uiSaveGame.save.generic.focusColor = uiColorDoomSelect;
	uiSaveGame.save.generic.ownerdraw = UI_Load_BitmapDraw;

	// delete
	uiSaveGame.remove.generic.id = ID_DELETE;
	uiSaveGame.remove.generic.type = QMTYPE_BITMAP;
	uiSaveGame.remove.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.remove.generic.x = -68;
	uiSaveGame.remove.generic.y = 184;
	uiSaveGame.remove.generic.width = 80;
	uiSaveGame.remove.generic.height = 16;
	uiSaveGame.remove.generic.callback = UI_SaveGame_Callback;
	uiSaveGame.remove.pic = BTN_DELETE;
	uiSaveGame.remove.generic.color = uiColorDoomRed;
	uiSaveGame.remove.generic.focusColor = uiColorDoomSelect;
	uiSaveGame.remove.generic.ownerdraw = UI_Load_BitmapDraw;

	// done
	uiSaveGame.cancel.generic.id = ID_CANCEL;
	uiSaveGame.cancel.generic.type = QMTYPE_BITMAP;
	uiSaveGame.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.cancel.generic.x = -68;
	uiSaveGame.cancel.generic.y = 202;
	uiSaveGame.cancel.generic.width = 80;
	uiSaveGame.cancel.generic.height = 16;
	uiSaveGame.cancel.generic.callback = UI_SaveGame_Callback;
	uiSaveGame.cancel.pic = BTN_DONES;
	uiSaveGame.cancel.generic.color = uiColorDoomRed;
	uiSaveGame.cancel.generic.focusColor = uiColorDoomSelect;

	uiSaveGame.deleteMessage1.generic.id = ID_DELMESSAGE;
	uiSaveGame.deleteMessage1.generic.type = QMTYPE_BITMAP;
	uiSaveGame.deleteMessage1.generic.flags = QMF_HIDDEN | QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.deleteMessage1.generic.x = 0;
	uiSaveGame.deleteMessage1.generic.y = 84;
	uiSaveGame.deleteMessage1.generic.width = 256;
	uiSaveGame.deleteMessage1.generic.height = 24;
	uiSaveGame.deleteMessage1.pic = DEL_MSG;
	uiSaveGame.deleteMessage1.generic.color = uiColorDoomRed;

	uiSaveGame.yes.generic.id = ID_YES;
	uiSaveGame.yes.generic.type = QMTYPE_BITMAP;
	uiSaveGame.yes.generic.flags = QMF_HIDDEN | QMF_HIGHLIGHTIFFOCUS | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.yes.generic.x = -UI_YESNO_POSX;
	uiSaveGame.yes.generic.y = 128;
	uiSaveGame.yes.generic.width = 56;
	uiSaveGame.yes.generic.height = 24;
	uiSaveGame.yes.generic.callback = UI_SaveGame_Callback;
	uiSaveGame.yes.pic = BTN_YES;
	uiSaveGame.yes.generic.color = uiColorDoomRed;
	uiSaveGame.yes.generic.focusColor = uiColorDoomSelect;

	uiSaveGame.no.generic.id = ID_NO;
	uiSaveGame.no.generic.type = QMTYPE_BITMAP;
	uiSaveGame.no.generic.name = "No";
	uiSaveGame.no.generic.flags = QMF_HIDDEN | QMF_HIGHLIGHTIFFOCUS | QMF_ACT_ONRELEASE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiSaveGame.no.generic.x = UI_YESNO_POSX;
	uiSaveGame.no.generic.y = 128;
	uiSaveGame.no.generic.width = 48;
	uiSaveGame.no.generic.height = 24;
	uiSaveGame.no.generic.callback = UI_SaveGame_Callback;
	uiSaveGame.no.pic = BTN_NO;
	uiSaveGame.no.generic.color = uiColorDoomRed;
	uiSaveGame.no.generic.focusColor = uiColorDoomSelect;

	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.background);
	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.banner);

	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.save1);
	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.save2);
	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.save3);
	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.save4);

	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.savePic);

	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.save);
	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.remove);
	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.cancel);

	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.deleteMessage1);
	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.yes);
	UI_AddItem(&uiSaveGame.menu, (void*)&uiSaveGame.no);
}

/*
=================
UI_SaveGame_Precache
=================
*/
void UI_SaveGame_Precache(void)
{
	PIC_Load(ART_BACKGROUND);
	PIC_Load(ART_BANNER, TF_NEAREST | TF_NOMIPMAP);
}

/*
=================
UI_SaveGame_Menu
=================
*/
void UI_SaveGame_Menu(void)
{
	if (gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY)
	{
		// completely ignore save\save menus for multiplayer_only
		return;
	}

	if (!CheckGameDll()) return;

	UI_SaveGame_Precache();
	UI_SaveGame_Init();

	UI_PushMenu(&uiSaveGame.menu);
}