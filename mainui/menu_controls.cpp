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

#define HEADER_KEYBINDS     	"gfx/shell/headers/header_keybinds"

#define ID_BACKGROUND	0
#define ID_BANNER		1
#define ID_DONE			4
#define ID_CANCEL		5
#define ID_KEYLIST		6
#define ID_TABLEHINT	7
#define ID_MSGBOX1	 	8
#define ID_MSGBOX2	 	9
#define ID_MSGTEXT	 	10
#define ID_PROMPT	 	11
#define ID_YES	 		130
#define ID_NO	 		131

#define MAX_KEYS		256
#define CMD_LENGTH		38
#define KEY1_LENGTH		20+CMD_LENGTH
#define KEY2_LENGTH		20+KEY1_LENGTH

typedef struct
{
	char		keysBind[MAX_KEYS][CMD_LENGTH];
	char		firstKey[MAX_KEYS][20];
	char		secondKey[MAX_KEYS][20];
	char		keysDescription[MAX_KEYS][256];
	char*		keysDescriptionPtr[MAX_KEYS];
	char		keysInput1[MAX_KEYS][256];
	char*		keysInput1Ptr[MAX_KEYS];
	char		keysInput2[MAX_KEYS][256];
	char*		keysInput2Ptr[MAX_KEYS];

	menuFramework_s	menu;

	menuBitmap_s	background;
	menuBitmap_s	banner;
	menuPicButton_s	done;
	menuPicButton_s	cancel;

	// redefine key wait dialog
	menuAction_s	msgBox1;	// small msgbox
	menuAction_s	msgBox2;	// large msgbox
	menuAction_s	dlgMessage;
	menuAction_s	promptMessage;
	menuPicButton_s	yes;
	menuPicButton_s	no;

	menuScrollList_s	keysList;
	menuScrollList_s	keysInputP;
	menuScrollList_s	keysInputS;
	menuAction_s	hintMessage;
	char		hintText[MAX_HINT_TEXT];

	int		bind_grab;	// waiting for key input
} uiControls_t;

static uiControls_t		uiControls;
extern bool		hold_button_stack;

static void UI_ResetToDefaultsDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide reset to defaults dialog
	uiControls.done.generic.flags ^= QMF_INACTIVE;
	uiControls.cancel.generic.flags ^= QMF_INACTIVE;

	uiControls.keysList.generic.flags ^= QMF_INACTIVE;

	uiControls.msgBox2.generic.flags ^= QMF_HIDDEN;
	uiControls.promptMessage.generic.flags ^= QMF_HIDDEN;
	uiControls.yes.generic.flags ^= QMF_HIDDEN;
	uiControls.no.generic.flags ^= QMF_HIDDEN;
}

/*
=================
UI_Controls_GetKeyBindings
=================
*/
static void UI_Controls_GetKeyBindings( const char *command, int *twoKeys )
{
	int		i, count = 0;
	const char	*b;

	twoKeys[0] = twoKeys[1] = -1;

	for( i = 0; i < 256; i++ )
	{
		b = KEY_GetBinding( i );
		if( !b ) continue;

		if( !_stricmp( command, b ))
		{
			twoKeys[count] = i;
			count++;

			if( count == 2 ) break;
		}
	}

	// swap keys if needed
	if( twoKeys[0] != -1 && twoKeys[1] != -1 )
	{
		int tempKey = twoKeys[1];
		twoKeys[1] = twoKeys[0];
		twoKeys[0] = tempKey;
	}
}

void UI_UnbindCommand( const char *command )
{
	int i, l;
	const char *b;

	l = strlen( command );

	for( i = 0; i < 256; i++ )
	{
		b = KEY_GetBinding( i );
		if( !b ) continue;

		if( !strncmp( b, command, l ))
			KEY_SetBinding( i, "" );
	}
}

static void UI_Controls_ParseKeysList( void )
{
	char *afile = (char *)LOAD_FILE( "gfx/shell/kb_act.lst", NULL );
	char *pfile = afile;
	char token[1024];
	int i = 0;

	if( !afile )
	{
		for (; i < MAX_KEYS; i++) uiControls.keysDescriptionPtr[i] = NULL;
		uiControls.keysList.itemNames = (const char**)uiControls.keysDescriptionPtr;

		for (; i < MAX_KEYS; i++) uiControls.keysInput1Ptr[i] = NULL;
		uiControls.keysInputP.itemNames = (const char**)uiControls.keysInput1Ptr;

		for (; i < MAX_KEYS; i++) uiControls.keysInput2Ptr[i] = NULL;
		uiControls.keysInputS.itemNames = (const char**)uiControls.keysInput2Ptr;
	
		Con_Printf( "UI_Parse_KeysList: kb_act.lst not found\n" );
		return;
	}

	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		char	str[128];

		if( !_stricmp( token, "blank" ))
		{
			// seperator
			pfile = COM_ParseFile( pfile, token );
			if( !pfile ) break;	// technically an error

			sprintf(str, "^1%s^7", token); //RED COLOR

			StringConcat( uiControls.keysDescription[i], str, strlen( str ) + 1 );
			StringConcat( uiControls.keysDescription[i], uiEmptyString, 256 );	// empty
			uiControls.keysDescriptionPtr[i] = uiControls.keysDescription[i];

			StringConcat(uiControls.keysInput1[0], "^1Key/Button^7", strlen("^1Key/Button^7") + 1);
			StringConcat(uiControls.keysInput2[0], "^1Alt^7", strlen("^1Alt^7") + 1);
			uiControls.keysInput1Ptr[i] = uiControls.keysInput1[i];
			uiControls.keysInput2Ptr[i] = uiControls.keysInput2[i];

			strcpy( uiControls.keysBind[i], "" );
			strcpy( uiControls.firstKey[i], "" );
			strcpy( uiControls.secondKey[i], "" );
			i++;
		}
		else
		{
			// key definition
			int	keys[2];

			UI_Controls_GetKeyBindings( token, keys );
			strncpy( uiControls.keysBind[i], token, sizeof( uiControls.keysBind[i] ));

			pfile = COM_ParseFile( pfile, token );
			if( !pfile ) break; // technically an error

			sprintf( str, "^7%s^7", token );	//WHITE COLOR

			if( keys[0] == -1 ) strcpy( uiControls.firstKey[i], "" );
			else strncpy( uiControls.firstKey[i], KEY_KeynumToString( keys[0] ), sizeof( uiControls.firstKey[i] ));

			if( keys[1] == -1 ) strcpy( uiControls.secondKey[i], "" ); 
			else strncpy( uiControls.secondKey[i], KEY_KeynumToString( keys[1] ), sizeof( uiControls.secondKey[i] ));

			StringConcat( uiControls.keysDescription[i], str, CMD_LENGTH );
			StringConcat( uiControls.keysDescription[i], uiEmptyString, CMD_LENGTH );

			// HACKHACK this color should be get from kb_keys.lst
			if( !_strnicmp( uiControls.firstKey[i], "MOUSE", 5 ))
				sprintf( str, "^5%s^7", uiControls.firstKey[i] );	// cyan
			else sprintf( str, "^4%s^7", uiControls.firstKey[i] );	// yellow "^3%s^7"
			StringConcat( uiControls.keysInput1[i], str, KEY1_LENGTH );
			StringConcat( uiControls.keysInput1[i], uiEmptyString, KEY1_LENGTH );

			// HACKHACK this color should be get from kb_keys.lst
			if( !_strnicmp( uiControls.secondKey[i], "MOUSE", 5 ))
				sprintf( str, "^5%s^7", uiControls.secondKey[i] );// cyan
			else sprintf( str, "^4%s^7", uiControls.secondKey[i] );	// yellow "^3%s^7"

			StringConcat( uiControls.keysInput2[i], str, KEY2_LENGTH );
			StringConcat( uiControls.keysInput2[i], uiEmptyString, KEY2_LENGTH );

			uiControls.keysDescriptionPtr[i] = uiControls.keysDescription[i];

			uiControls.keysInput1Ptr[i] = uiControls.keysInput1[i];
			uiControls.keysInput2Ptr[i] = uiControls.keysInput2[i];
			i++;
		}
	}

	FREE_FILE( afile );

	for( ; i < MAX_KEYS; i++ ) uiControls.keysDescriptionPtr[i] = NULL;
	uiControls.keysList.itemNames = (const char **)uiControls.keysDescriptionPtr;

	for (; i < MAX_KEYS; i++) uiControls.keysInput1Ptr[i] = NULL;
	uiControls.keysInputP.itemNames = (const char**)uiControls.keysInput1Ptr;

	for (; i < MAX_KEYS; i++) uiControls.keysInput2Ptr[i] = NULL;
	uiControls.keysInputS.itemNames = (const char**)uiControls.keysInput2Ptr;
}

static void UI_PromptDialog( void )
{
	// toggle main menu between active\inactive
	// show\hide quit dialog
	uiControls.done.generic.flags ^= QMF_INACTIVE;
	uiControls.cancel.generic.flags ^= QMF_INACTIVE;

	uiControls.keysList.generic.flags ^= QMF_INACTIVE;

	uiControls.msgBox1.generic.flags ^= QMF_HIDDEN;
	uiControls.dlgMessage.generic.flags ^= QMF_HIDDEN;
}

static void UI_Controls_RestartMenu( void )
{
	int lastSelectedKey = uiControls.keysList.curItem;
	int lastTopItem = uiControls.keysList.topItem;

	// HACK to prevent mismatch anim stack
	hold_button_stack = true;

	// restarts the menu
	UI_PopMenu();
	UI_Controls_Menu();

	hold_button_stack = false;

	// restore last key and top item
	uiControls.keysList.curItem = lastSelectedKey;
	uiControls.keysList.topItem = lastTopItem;

	uiControls.keysInputS.curItem = -1;
}

static void UI_Controls_ResetKeysList( void )
{
	char *afile = (char *)LOAD_FILE( "gfx/shell/kb_def.lst", NULL );
	char *pfile = afile;
	char token[1024];
	int i = 0;

	if( !afile )
	{
		Con_Printf( "UI_Parse_KeysList: kb_act.lst not found\n" );
		return;
	}

	while(( pfile = COM_ParseFile( pfile, token )) != NULL )
	{
		char	key[32];

		strncpy( key, token, sizeof( key ));

		pfile = COM_ParseFile( pfile, token );
		if( !pfile ) break;	// technically an error

		char	cmd[128];

		if( key[0] == '\\' && key[1] == '\\' )
		{
			key[0] = '\\';
			key[1] = '\0';
		}

		UI_UnbindCommand( token );

		sprintf( cmd, "bind \"%s\" \"%s\"\n", key, token );
		CLIENT_COMMAND( TRUE, cmd );
	}

	FREE_FILE( afile );
	UI_Controls_RestartMenu ();
}

/*
=================
UI_Controls_KeyFunc
=================
*/
static const char *UI_Controls_KeyFunc( int key, int down )
{
	char	cmd[128];
	
	if( down )
	{
		if( uiControls.bind_grab )	// assume we are in grab-mode
		{
			// defining a key
			if( key == '`' || key == '~' )
			{
				return uiSoundBuzz;
			}
			else if( key != K_ESCAPE )
			{
				const char *bindName = uiControls.keysBind[uiControls.keysList.curItem];
				sprintf( cmd, "bind \"%s\" \"%s\"\n", KEY_KeynumToString( key ), bindName );
				CLIENT_COMMAND( TRUE, cmd );
			}

			uiControls.bind_grab = false;
			UI_Controls_RestartMenu();

			return uiSoundLaunch;
		}

		if(( key == K_ENTER || key == K_KP_ENTER ) && uiControls.dlgMessage.generic.flags & QMF_HIDDEN )
		{
			if( !strlen( uiControls.keysBind[uiControls.keysList.curItem] ))
			{
				// probably it's a seperator
				return uiSoundBuzz;
			}

			// entering to grab-mode
			const char *bindName = uiControls.keysBind[uiControls.keysList.curItem];
			int keys[2];
	
			UI_Controls_GetKeyBindings( bindName, keys );
			if( keys[1] != -1 ) UI_UnbindCommand( bindName );
			uiControls.bind_grab = true;

			UI_PromptDialog();	// show prompt
			return uiSoundKey;
		}

		if(( key == K_BACKSPACE || key == K_DEL ) && uiControls.dlgMessage.generic.flags & QMF_HIDDEN )
		{
			// delete bindings

			if( !strlen( uiControls.keysBind[uiControls.keysList.curItem] ))
			{
				// probably it's a seperator
				return uiSoundNull;
			}

			const char *bindName = uiControls.keysBind[uiControls.keysList.curItem];
			UI_UnbindCommand( bindName );
			UI_StartSound( uiSoundRemoveKey );
			UI_Controls_RestartMenu();

			return uiSoundNull;
		}
	}
	return UI_DefaultKey( &uiControls.menu, key, down );
}

/*
=================
UI_MsgBox_Ownerdraw
=================
*/
static void UI_MsgBox_Ownerdraw( void *self )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	UI_FillRect( item->x, item->y, item->width, item->height, uiPromptBgColor );
}

/*
=================
UI_Controls_Callback
=================
*/
static void UI_Controls_Callback( void *self, int event )
{
	menuCommon_s	*item = (menuCommon_s *)self;

	if( event != QM_ACTIVATED )
		return;

	switch( item->id )
	{
	case ID_DONE:
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_NO:
		UI_ResetToDefaultsDialog ();
		break;
	case ID_YES:
		UI_Controls_ResetKeysList ();
		break;
	}
}

/*
=================
UI_Controls_Init
=================
*/
static void UI_Controls_Init( void )
{
	memset( &uiControls, 0, sizeof( uiControls_t ));

	uiControls.menu.vidInitFunc = UI_Controls_Init;
	uiControls.menu.keyFunc = UI_Controls_KeyFunc;

	StringConcat( uiControls.hintText, "Action", CMD_LENGTH );
	StringConcat( uiControls.hintText, uiEmptyString, CMD_LENGTH-4 );
	StringConcat( uiControls.hintText, "Key/Button", KEY1_LENGTH );
	StringConcat( uiControls.hintText, uiEmptyString, KEY1_LENGTH-8 );
	StringConcat( uiControls.hintText, "Alternate", KEY2_LENGTH );
	StringConcat( uiControls.hintText, uiEmptyString, KEY2_LENGTH );

	uiControls.background.generic.id = ID_BACKGROUND;
	uiControls.background.generic.type = QMTYPE_BITMAP_STRETCH;
	uiControls.background.generic.flags = QMF_INACTIVE;
	uiControls.background.generic.x = 0;
	uiControls.background.generic.y = 0;
	uiControls.background.generic.width = 1024;
	uiControls.background.generic.height = 768;
	uiControls.background.pic = ART_BACKGROUND;

	uiControls.banner.generic.id = ID_BANNER;
	uiControls.banner.generic.type = QMTYPE_BITMAP;
	uiControls.banner.generic.flags = QMF_INACTIVE | QMF_DRAW_HOLES | QMF_CENTERED;
	uiControls.banner.generic.x = UI_BANNER_POSX;
	uiControls.banner.generic.y = UI_BANNER_POSY - 8;
	uiControls.banner.generic.width = 152;
	uiControls.banner.generic.height = 24;
	uiControls.banner.pic = HEADER_KEYBINDS;
	uiControls.banner.generic.color = uiColorDoomRed;

	uiControls.done.generic.id = ID_DONE;
	uiControls.done.generic.type = QMTYPE_BM_BUTTON;
	uiControls.done.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiControls.done.generic.x = UI_SELECTION_POSX;
	uiControls.done.generic.y = 330;
	uiControls.done.generic.name = "Ok";
	uiControls.done.generic.statusText = "Save changes and return to configuration menu";
	uiControls.done.generic.callback = UI_Controls_Callback;

	UI_UtilSetupPicButton( &uiControls.done, PC_DONE );

	uiControls.cancel.generic.id = ID_CANCEL;
	uiControls.cancel.generic.type = QMTYPE_BM_BUTTON;
	uiControls.cancel.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW;
	uiControls.cancel.generic.x = UI_SELECTION_POSX;
	uiControls.cancel.generic.y = 380;
	uiControls.cancel.generic.name = "Cancel";
	uiControls.cancel.generic.statusText = "Discard changes and return to configuration menu";
	uiControls.cancel.generic.callback = UI_Controls_Callback;

	UI_UtilSetupPicButton( &uiControls.cancel, PC_CANCEL );

	uiControls.hintMessage.generic.id = ID_TABLEHINT;
	uiControls.hintMessage.generic.type = QMTYPE_ACTION;
	uiControls.hintMessage.generic.flags = QMF_INACTIVE|QMF_SMALLFONT;
	uiControls.hintMessage.generic.color = uiColorHelp;
	uiControls.hintMessage.generic.name = uiControls.hintText;
	uiControls.hintMessage.generic.x = 360;
	uiControls.hintMessage.generic.y = 120;

	uiControls.keysList.generic.id = ID_KEYLIST;
	uiControls.keysList.generic.type = QMTYPE_SCROLLLIST;
	uiControls.keysList.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_SMALLFONT;
	uiControls.keysList.generic.x = -60;
	uiControls.keysList.generic.y = 42;
	uiControls.keysList.generic.width = 116;
	uiControls.keysList.generic.height = 240;
	uiControls.keysList.generic.callback = UI_Controls_Callback;

	uiControls.keysInputP.generic.id = ID_KEYLIST;
	uiControls.keysInputP.generic.type = QMTYPE_SCROLLLIST;
	uiControls.keysInputP.generic.flags = QMF_INACTIVE | QMF_GRAYED | QMF_SMALLFONT;
	uiControls.keysInputP.generic.x = 35;
	uiControls.keysInputP.generic.y = 35;
	uiControls.keysInputP.generic.width = 56;
	uiControls.keysInputP.generic.height = 240;
	uiControls.keysInputP.generic.callback = UI_Controls_Callback;

	uiControls.keysInputS.generic.id = ID_KEYLIST;
	uiControls.keysInputS.generic.type = QMTYPE_SCROLLLIST;
	uiControls.keysInputS.generic.flags = QMF_INACTIVE | QMF_GRAYED | QMF_SMALLFONT;
	uiControls.keysInputS.generic.x = 95;
	uiControls.keysInputS.generic.y = 35;
	uiControls.keysInputS.generic.width = 56;
	uiControls.keysInputS.generic.height = 240;
	uiControls.keysInputS.generic.callback = UI_Controls_Callback;

	UI_Controls_ParseKeysList();

	uiControls.msgBox1.generic.id = ID_MSGBOX1;
	uiControls.msgBox1.generic.type = QMTYPE_ACTION;
	uiControls.msgBox1.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiControls.msgBox1.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiControls.msgBox1.generic.x = 192;
	uiControls.msgBox1.generic.y = 256;
	uiControls.msgBox1.generic.width = 640;
	uiControls.msgBox1.generic.height = 128;

	uiControls.msgBox2.generic.id = ID_MSGBOX2;
	uiControls.msgBox2.generic.type = QMTYPE_ACTION;
	uiControls.msgBox2.generic.flags = QMF_INACTIVE|QMF_HIDDEN;
	uiControls.msgBox2.generic.ownerdraw = UI_MsgBox_Ownerdraw; // just a fill rectangle
	uiControls.msgBox2.generic.x = 192;
	uiControls.msgBox2.generic.y = 256;
	uiControls.msgBox2.generic.width = 640;
	uiControls.msgBox2.generic.height = 256;

	uiControls.dlgMessage.generic.id = ID_MSGTEXT;
	uiControls.dlgMessage.generic.type = QMTYPE_ACTION;
	uiControls.dlgMessage.generic.flags = QMF_INACTIVE|QMF_HIDDEN|QMF_DROPSHADOW;
	uiControls.dlgMessage.generic.name = "Press a key or button";
	uiControls.dlgMessage.generic.x = 320;
	uiControls.dlgMessage.generic.y = 280;

	uiControls.promptMessage.generic.id = ID_PROMPT;
	uiControls.promptMessage.generic.type = QMTYPE_ACTION;
	uiControls.promptMessage.generic.flags = QMF_INACTIVE|QMF_DROPSHADOW|QMF_HIDDEN;
	uiControls.promptMessage.generic.name = "Reset buttons to default?";
	uiControls.promptMessage.generic.x = 290;
	uiControls.promptMessage.generic.y = 280;

	uiControls.yes.generic.id = ID_YES;
	uiControls.yes.generic.type = QMTYPE_BM_BUTTON;
	uiControls.yes.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiControls.yes.generic.name = "Ok";
	uiControls.yes.generic.x = 380;
	uiControls.yes.generic.y = 460;
	uiControls.yes.generic.callback = UI_Controls_Callback;

	UI_UtilSetupPicButton( &uiControls.yes, PC_OK );

	uiControls.no.generic.id = ID_NO;
	uiControls.no.generic.type = QMTYPE_BM_BUTTON;
	uiControls.no.generic.flags = QMF_HIGHLIGHTIFFOCUS|QMF_DROPSHADOW|QMF_HIDDEN;
	uiControls.no.generic.name = "Cancel";
	uiControls.no.generic.x = 530;
	uiControls.no.generic.y = 460;
	uiControls.no.generic.callback = UI_Controls_Callback;

	UI_UtilSetupPicButton(&uiControls.no, PC_CANCEL);

	UI_AddItem(&uiControls.menu, (void*)&uiControls.background);
	UI_AddItem(&uiControls.menu, (void*)&uiControls.banner);
	//UI_AddItem(&uiControls.menu, (void*)&uiControls.defaults);
	//UI_AddItem(&uiControls.menu, (void*)&uiControls.advanced);
	//UI_AddItem(&uiControls.menu, (void*)&uiControls.done);
	//UI_AddItem(&uiControls.menu, (void*)&uiControls.cancel);
	//UI_AddItem( &uiControls.menu, (void *)&uiControls.hintMessage );
	UI_AddItem(&uiControls.menu, (void*)&uiControls.keysList);
	UI_AddItem(&uiControls.menu, (void*)&uiControls.keysInputP);
	UI_AddItem(&uiControls.menu, (void*)&uiControls.keysInputS);
	UI_AddItem(&uiControls.menu, (void*)&uiControls.msgBox1);
	UI_AddItem(&uiControls.menu, (void*)&uiControls.msgBox2);
	UI_AddItem(&uiControls.menu, (void*)&uiControls.dlgMessage);
	UI_AddItem(&uiControls.menu, (void*)&uiControls.promptMessage);
	UI_AddItem(&uiControls.menu, (void*)&uiControls.no);
	UI_AddItem(&uiControls.menu, (void*)&uiControls.yes);
}

/*
=================
UI_Controls_Precache
=================
*/
void UI_Controls_Precache( void )
{
	PIC_Load(ART_BACKGROUND);
	PIC_Load(HEADER_KEYBINDS, TF_NEAREST|TF_NOMIPMAP);
}

/*
=================
UI_Controls_Menu
=================
*/
void UI_Controls_Menu( void )
{
	UI_Controls_Precache();
	UI_Controls_Init();

	UI_PushMenu( &uiControls.menu );
}