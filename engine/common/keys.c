/*
keys.c - console key events
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "input.h"
#include "client.h"

typedef struct key_s
{
	qboolean		down;
	qboolean		gamedown;
	int		repeats;	// if > 1, it is autorepeating
	const char	*binding;
} key_t;

typedef struct keyname_s
{
	char		*name;	// key name
	int		keynum;	// key number
	const char	*binding;	// default bind
} keyname_t;

key_t	keys[256];

// magic nipples - removing default functions from here because default.cfg exists. 
// these load into the menu but not when a server is launched for the first time.
keyname_t keynames[] =
{
{"TAB",		K_TAB,		""		},
{"ENTER",		K_ENTER,		""		},
{"ESCAPE",	K_ESCAPE, 	"escape"		}, // hardcoded magic nipples - leaving this one alone just in case!!
{"SPACE",		K_SPACE,		""		},
{"BACKSPACE",	K_BACKSPACE,	""		},
{"UPARROW",	K_UPARROW,	""	},
{"DOWNARROW",	K_DOWNARROW,	""		},
{"LEFTARROW",	K_LEFTARROW,	""		},
{"RIGHTARROW",	K_RIGHTARROW,	""		},
{"ALT",		K_ALT,		""		},
{"CTRL",		K_CTRL,		""		},
{"SHIFT",		K_SHIFT,		""		},
{"CAPSLOCK",	K_CAPSLOCK,	""		},
{"SCROLLOCK",	K_SCROLLOCK,	""		},
{"F1",		K_F1,		""	},
{"F2",		K_F2,		""	},
{"F3",		K_F3,		""	},
{"F4",		K_F4,		""	},
{"F5",		K_F5,		""	},
{"F6",		K_F6,		""	},
{"F7",		K_F7,		""	},
{"F8",		K_F8,		""		},
{"F9",		K_F9,		""		},
{"F10",		K_F10,		""	},
{"F11",		K_F11,		""		},
{"F12",		K_F12,		""	},
{"INS",		K_INS,		""		},
{"DEL",		K_DEL,		""	},
{"PGDN",		K_PGDN,		""		},
{"PGUP",		K_PGUP,		""		},
{"HOME",		K_HOME,		""		},
{"END",		K_END,		""	},

// mouse buttouns
{"MOUSE1",	K_MOUSE1,		""		},
{"MOUSE2",	K_MOUSE2,		""	},
{"MOUSE3",	K_MOUSE3,		""		},
{"MOUSE4",	K_MOUSE4,		""		},
{"MOUSE5",	K_MOUSE5,		""		},
{"MWHEELUP",	K_MWHEELUP,	""		},
{"MWHEELDOWN",	K_MWHEELDOWN,	""		},

// digital keyboard
{"KP_HOME",	K_KP_HOME,	""		},
{"KP_UPARROW",	K_KP_UPARROW,	""	},
{"KP_PGUP",	K_KP_PGUP,	""		},
{"KP_LEFTARROW",	K_KP_LEFTARROW,	""		},
{"KP_5",		K_KP_5,		""		},
{"KP_RIGHTARROW",	K_KP_RIGHTARROW,	""		},
{"KP_END",	K_KP_END,		""	},
{"KP_DOWNARROW",	K_KP_DOWNARROW,	""		},
{"KP_PGDN",	K_KP_PGDN,	"" 	},
{"KP_ENTER",	K_KP_ENTER,	""		},
{"KP_INS",	K_KP_INS,		""		},
{"KP_DEL",	K_KP_DEL,		""	},
{"KP_SLASH",	K_KP_SLASH,	""		},
{"KP_MINUS",	K_KP_MINUS,	""		},
{"KP_PLUS",	K_KP_PLUS,	""		},
{"PAUSE",		K_PAUSE,		""		},

//magic nipples - for controllers
{"K_JOY1",	K_JOY1,	""		},
{"K_JOY2",	K_JOY2,	""		},
{"K_JOY3",	K_JOY3,	""		},
{"K_JOY4",	K_JOY4,	""		},

{"K_AUX1",	K_AUX1,	""		},
{"K_AUX2",	K_AUX2,	""		},
{"K_AUX3",	K_AUX3,	""		},
{"K_AUX4",	K_AUX4,	""		},
{"K_AUX5",	K_AUX5,	""		},
{"K_AUX6",	K_AUX6,	""		},
{"K_AUX7",	K_AUX7,	""		},
{"K_AUX8",	K_AUX8,	""		},
{"K_AUX9",	K_AUX9,	""		},
{"K_AUX10",	K_AUX10,	""		},

{"K_AUX11",	K_AUX11,	""		},
{"K_AUX12",	K_AUX12,	""		},
{"K_AUX13",	K_AUX13,	""		},
{"K_AUX14",	K_AUX14,	""		},
{"K_AUX15",	K_AUX15,	""		},
{"K_AUX16",	K_AUX16,	""		},
{"K_AUX17",	K_AUX17,	""		},
{"K_AUX18",	K_AUX18,	""		},
{"K_AUX19",	K_AUX19,	""		},
{"K_AUX20",	K_AUX20,	""		},

{"K_AUX21",	K_AUX21,	""		},
{"K_AUX22",	K_AUX22,	""		},
{"K_AUX23",	K_AUX23,	""		},
{"K_AUX24",	K_AUX24,	""		},
{"K_AUX25",	K_AUX25,	""		},
{"K_AUX26",	K_AUX26,	""		},
{"K_AUX27",	K_AUX27,	""		},
{"K_AUX28",	K_AUX28,	""		},
{"K_AUX29",	K_AUX29,	""		},
{"K_AUX30",	K_AUX30,	""		},

{"K_AUX31",	K_AUX31,	""		},
{"K_AUX32",	K_AUX32,	""		},

// raw semicolon seperates commands
{"SEMICOLON",	';',		""		},
{NULL,		0,		NULL		},
};

/*
===================
Key_IsDown
===================
*/
qboolean Key_IsDown( int keynum )
{
	if( keynum == -1 )
		return false;
	return keys[keynum].down;
}

/*
===================
Key_GetBind
===================
*/
const char *Key_IsBind( int keynum )
{
	if( keynum == -1 || !keys[keynum].binding )
		return NULL;
	return keys[keynum].binding;
}

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers

to be configured even if they don't have defined names.
===================
*/
int Key_StringToKeynum( const char *str )
{
	keyname_t		*kn;
	
	if( !str || !str[0] ) return -1;
	if( !str[1] ) return str[0];

	// check for hex code
	if( str[0] == '0' && str[1] == 'x' && Q_strlen( str ) == 4 )
	{
		int	n1, n2;
		
		n1 = str[2];
		if( n1 >= '0' && n1 <= '9' )
		{
			n1 -= '0';
		}
		else if( n1 >= 'a' && n1 <= 'f' )
		{
			n1 = n1 - 'a' + 10;
		}
		else n1 = 0;

		n2 = str[3];
		if( n2 >= '0' && n2 <= '9' )
		{
			n2 -= '0';
		}
		else if( n2 >= 'a' && n2 <= 'f' )
		{
			n2 = n2 - 'a' + 10;
		}
		else n2 = 0;

		return n1 * 16 + n2;
	}

	// scan for a text match
	for( kn = keynames; kn->name; kn++ )
	{
		if( !Q__stricmp( str, kn->name ))
			return kn->keynum;
	}

	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
const char *Key_KeynumToString( int keynum )
{
	keyname_t		*kn;	
	static char	tinystr[5];
	int		i, j;

	if ( keynum == -1 ) return "<KEY NOT FOUND>";
	if ( keynum < 0 || keynum > 255 ) return "<OUT OF RANGE>";

	// check for printable ascii (don't use quote)
	if( keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';' && keynum != K_SCROLLOCK )
	{
		tinystr[0] = keynum;
		tinystr[1] = 0;
		return tinystr;
	}

	// check for a key string
	for( kn = keynames; kn->name; kn++ )
	{
		if( keynum == kn->keynum )
			return kn->name;
	}

	// make a hex string
	i = keynum >> 4;
	j = keynum & 15;

	tinystr[0] = '0';
	tinystr[1] = 'x';
	tinystr[2] = i > 9 ? i - 10 + 'a' : i + '0';
	tinystr[3] = j > 9 ? j - 10 + 'a' : j + '0';
	tinystr[4] = 0;

	return tinystr;
}

/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding( int keynum, const char *binding )
{
	if( keynum == -1 ) return;

	// free old bindings
	if( keys[keynum].binding )
	{
		Mem_Free((char *)keys[keynum].binding );
		keys[keynum].binding = NULL;
	}
		
	// allocate memory for new binding
	keys[keynum].binding = copystring( binding );
}


/*
===================
Key_GetBinding
===================
*/
const char *Key_GetBinding( int keynum )
{
	if( keynum == -1 ) return NULL;
	return keys[keynum].binding;
}

/* 
===================
Key_GetKey
===================
*/
int Key_GetKey( const char *pBinding )
{
	int	i;

	if( !pBinding ) return -1;

	for( i = 0; i < 256; i++ )
	{
		if( !keys[i].binding )
			continue;

		if( *keys[i].binding == '+' )
                    {
			if( !Q__strnicmp( keys[i].binding + 1, pBinding, Q_strlen( pBinding )))
				return i;
		}
		else
		{
			if( !Q__strnicmp( keys[i].binding, pBinding, Q_strlen( pBinding )))
				return i;
		}
	}

	return -1;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f( void )
{
	int	b;

	if( Cmd_Argc() != 2 )
	{
		Con_Printf( S_USAGE "unbind <key> : remove commands from a key\n" );
		return;
	}
	
	b = Key_StringToKeynum( Cmd_Argv( 1 ));

	if( b == -1 )
	{
		Con_Printf( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ));
		return;
	}

	Key_SetBinding( b, "" );
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f( void )
{
	int	i;
	
	for( i = 0; i < 256; i++ )
	{
		if( keys[i].binding )
			Key_SetBinding( i, "" );
	}

	// set some defaults
	Key_SetBinding( K_ESCAPE, "cancelselect" );
}

/*
===================
Key_Reset_f
===================
*/
void Key_Reset_f( void )
{
	keyname_t	*kn;
	int	i;

	// clear all keys first	
	for( i = 0; i < 256; i++ )
	{
		if( keys[i].binding )
			Key_SetBinding( i, "" );
	}

	// apply default values
	for( kn = keynames; kn->name; kn++ )
		Key_SetBinding( kn->keynum, kn->binding ); 
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f( void )
{
	char	cmd[1024];
	int	i, c, b;
	
	c = Cmd_Argc();

	if( c < 2 )
	{
		Con_Printf( S_USAGE "bind <key> [command] : attach a command to a key\n" );
		return;
	}

	b = Key_StringToKeynum( Cmd_Argv( 1 ));

	if( b == -1 )
	{
		Con_Printf( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ));
		return;
	}

	if( c == 2 )
	{
		if( keys[b].binding )
			Con_Printf( "\"%s\" = \"%s\"\n", Cmd_Argv( 1 ), keys[b].binding );
		else Con_Printf( "\"%s\" is not bound\n", Cmd_Argv( 1 ));
		return;
	}
	
	// copy the rest of the command line
	cmd[0] = 0; // start out with a null string

	for( i = 2; i < c; i++ )
	{
		Q_strcat( cmd, Cmd_Argv( i ));
		if( i != ( c - 1 )) Q_strcat( cmd, " " );
	}

	Key_SetBinding( b, cmd );
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings( file_t *f )
{
	int	i;

	if( !f ) return;

	FS_Printf( f, "unbindall\n" );

	for( i = 0; i < 256; i++ )
	{
		if( !COM_CheckString( keys[i].binding ))
			continue;

		FS_Printf( f, "bind %s \"%s\"\n", Key_KeynumToString( i ), keys[i].binding );
	}
}

/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f( void )
{
	int	i;

	for( i = 0; i < 256; i++ )
	{
		if( !COM_CheckString( keys[i].binding ))
			continue;

		Con_Printf( "%s \"%s\"\n", Key_KeynumToString( i ), keys[i].binding );
	}
}

/*
==============================================================================

			LINE TYPING INTO THE CONSOLE

==============================================================================
*/
/*
===================
Key_Init
===================
*/
void Key_Init( void )
{
	keyname_t	*kn;

	// register our functions
	Cmd_AddCommand( "bind", Key_Bind_f, "binds a command to the specified key in bindmap" );
	Cmd_AddCommand( "unbind", Key_Unbind_f, "removes a command on the specified key in bindmap" );
	Cmd_AddCommand( "unbindall", Key_Unbindall_f, "removes all commands from all keys in bindmap" );
	Cmd_AddCommand( "resetkeys", Key_Reset_f, "reset all keys to their default values" );
	Cmd_AddCommand( "bindlist", Key_Bindlist_f, "display current key bindings" );
	Cmd_AddCommand( "makehelp", Key_EnumCmds_f, "write help.txt that contains all console cvars and cmds" ); 

	// setup default binding. "unbindall" from config.cfg will be reset it
	for( kn = keynames; kn->name; kn++ ) Key_SetBinding( kn->keynum, kn->binding ); 
}

/*
===================
Key_AddKeyCommands
===================
*/
void Key_AddKeyCommands( int key, const char *kb, qboolean down )
{
	char	button[1024];
	char	*buttonPtr;
	char	cmd[1024];
	int	i;

	if( !kb ) return;
	buttonPtr = button;

	for( i = 0; ; i++ )
	{
		if( kb[i] == ';' || !kb[i] )
		{
			*buttonPtr = '\0';
			if( button[0] == '+' )
			{
				// button commands add keynum as a parm
				if( down ) Q_sprintf( cmd, "%s %i\n", button, key );
				else Q_sprintf( cmd, "-%s %i\n", button + 1, key );
				Cbuf_AddText( cmd );
			}
			else if( down )
			{
				// down-only command
				Cbuf_AddText( button );
				Cbuf_AddText( "\n" );
			}

			buttonPtr = button;
			while(( kb[i] <= ' ' || kb[i] == ';' ) && kb[i] != 0 )
				i++;
		}

		*buttonPtr++ = kb[i];
		if( !kb[i] ) break;
	}
}

/*
===================
Key_IsAllowedAutoRepeat

List of keys that allows auto-repeat
===================
*/
qboolean Key_IsAllowedAutoRepeat( int key )
{
	switch( key )
	{
	case K_BACKSPACE:
	case K_PAUSE:
	case K_PGUP:
	case K_KP_PGUP:
	case K_PGDN:
	case K_KP_PGDN:
		return true;
	default:
		return false;
	} 
}

/*
===================
Key_Event

Called by the system for both key up and key down events
===================
*/
void Key_Event( int key, qboolean down )
{
	const char	*kb;

	// key was pressed before engine was run
	if( !keys[key].down && !down )
		return;

	kb = keys[key].binding;
	keys[key].down = down;

#ifdef HACKS_RELATED_HLMODS
	if(( cls.key_dest == key_game ) && ( cls.state == ca_cinematic ) && ( key != K_ESCAPE || !down ))
	{
		// only escape passed when cinematic is playing
		// HLFX 0.6 bug: crash in vgui3.dll while press +attack during movie playback
		return;
	}
#endif
	// distribute the key down event to the apropriate handler
	if( cls.key_dest == key_game && ( down || keys[key].gamedown ))
	{
		if( !clgame.dllFuncs.pfnKey_Event( down, key, keys[key].binding ))
		{
			if( keys[key].repeats == 0 && down )
			{
				keys[key].gamedown = true;
			}

			if( !down )
			{
				keys[key].gamedown = false;
				keys[key].repeats = 0;
			}
			return; // handled in client.dll
		}
	}

	// update auto-repeat status
	if( down )
	{
		keys[key].repeats++;

		if( !Key_IsAllowedAutoRepeat( key ) && keys[key].repeats > 1 )
		{
			// ignore most autorepeats
			return;
		}

		if( key >= 200 && !kb )
			Con_Printf( "%s is unbound.\n", Key_KeynumToString( key ));
	}
	else
	{
		keys[key].gamedown = false;
		keys[key].repeats = 0;
	}

	// console key is hardcoded, so the user can never unbind it
	if( key == '`' || key == '~' )
	{
		// we are in typing mode. So don't switch to console
		if( (word)GetKeyboardLayout( 0 ) == (word)0x419 )
		{
			if( cls.key_dest != key_game )
				return;
                    }

		if( !down ) return;
    		Con_ToggleConsole_f();
		return;
	}

	// escape is always handled special
	if( key == K_ESCAPE && down )
	{
		switch( cls.key_dest )
		{
		case key_game:
			if( CVAR_TO_BOOL( gl_showtextures ))
			{
				// close texture atlas
				Cvar_SetValue( "r_showtextures", 0.0f );
				return;
			}
			else if( host.mouse_visible && cls.state != ca_cinematic )
			{
				clgame.dllFuncs.pfnKey_Event( down, key, keys[key].binding );
				return; // handled in client.dll
			}
			break;
		case key_message:
			Key_Message( key );
			return;
		case key_console:
			if( cls.state == ca_active && !cl.background )
				Key_SetKeyDest( key_game );
			else UI_SetActiveMenu( true );
			return;
		case key_menu:
			UI_KeyEvent( key, true );
			return;
		default:	return;
		}
	}

	if( cls.key_dest == key_menu )
	{
		// only non printable keys passed
		UI_KeyEvent( key, down );
		return;
	}

	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing 
	// an action started before a mode switch.
	if( !down )
	{
		Key_AddKeyCommands( key, kb, down );
		return;
	}

	// distribute the key down event to the apropriate handler
	if( cls.key_dest == key_game )
	{
		Key_AddKeyCommands( key, kb, down );
	}
	else if( cls.key_dest == key_console )
	{
		Key_Console( key );
	}
	else if( cls.key_dest == key_message )
	{
		Key_Message( key );
	}
}

/*
=========
Key_SetKeyDest
=========
*/
void Key_SetKeyDest( int key_dest )
{
	IN_ToggleClientMouse( key_dest, cls.key_dest );

	switch( key_dest )
	{
	case key_game:
		cls.key_dest = key_game;
		break;
	case key_menu:
		cls.key_dest = key_menu;
		break;
	case key_console:
		cls.key_dest = key_console;
		break;
	case key_message:
		cls.key_dest = key_message;
		break;
	default:
		Host_Error( "Key_SetKeyDest: wrong destination (%i)\n", key_dest );
		break;
	}
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates( void )
{
	int	i;

	// don't clear keys during changelevel
	if( cls.changelevel ) return;

	for( i = 0; i < 256; i++ )
	{
		if( keys[i].down )
			Key_Event( i, false );

		keys[i].down = 0;
		keys[i].repeats = 0;
		keys[i].gamedown = 0;
	}

	if( clgame.hInstance )
		clgame.dllFuncs.IN_ClearStates();
}

/*
===================
CL_CharEvent

Normal keyboard characters, already shifted / capslocked / etc
===================
*/
void CL_CharEvent( int key )
{
	// the console key should never be used as a char
	if( key == '`' || key == '~' ) return;

	if( cls.key_dest == key_console && !Con_Visible( ))
	{
		if((char)key == '�' || (char)key == '�' )
			return; // don't pass '�' when we open the console 
	}

	// distribute the key down event to the apropriate handler
	if( cls.key_dest == key_console || cls.key_dest == key_message )
	{
		Con_CharEvent( key );
	}
	else if( cls.key_dest == key_menu )
	{
		UI_CharEvent( key );
	}
}