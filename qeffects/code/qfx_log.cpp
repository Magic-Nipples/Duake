/***************************************************************************
* Copyright (C) 2012-2016, Crystice Softworks.
* 
* This file is part of QeffectsGL source code.
* 
* QeffectsGL source code is free software; you can redistribute it 
* and/or modify it under the terms of the GNU General Public License 
* as published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
* 
* QeffectsGL source code is distributed in the hope that it will be 
* useful, but WITHOUT ANY WARRANTY; without even the implied 
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
* See the GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software 
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
***************************************************************************/
#include "qfx_library.hpp"
#include "qfx_log.hpp"

//=========================================
// QFXLog class implementation
//-----------------------------------------
// Print messages to log file
//=========================================

#define QFX_LOG_FILENAME	"QFXShadersLog.log"//QFX_LIBRARY_TITLE ".log"
#define QFX_LOG_MAXSTRING	8192

QFXLog :: QFXLog() : m_pFile( nullptr ), m_pszLogString( new char[QFX_LOG_MAXSTRING] )
{
	char timeBuf[64];
	time_t t;

	if ( fopen_s( &m_pFile, QFX_LOG_FILENAME, "w" ) )
		return;
	
	memset( &t, 0, sizeof(t) );
	time( &t );
	memset( timeBuf, 0, sizeof(timeBuf) );
	ctime_s( timeBuf, sizeof(timeBuf), &t );

	fprintf( m_pFile, "==================================================================\n" );
	fprintf( m_pFile, " " QFX_LIBRARY_TITLE " initialized at %s", timeBuf );
	fprintf( m_pFile, "==================================================================\n" );

	fprintf( m_pFile, "\n" );
	fflush( m_pFile );
}

QFXLog :: ~QFXLog()
{
	delete [] m_pszLogString;

	if ( m_pFile ) {
		char timeBuf[64];
		time_t t;

		memset( &t, 0, sizeof(t) );
		time( &t );
		memset( timeBuf, 0, sizeof(timeBuf) );
		ctime_s( timeBuf, sizeof(timeBuf), &t );

		fprintf( m_pFile, "\n==================================================================\n" );
		fprintf( m_pFile, " " QFX_LIBRARY_TITLE " shutdown at %s", timeBuf );
		fprintf( m_pFile, "==================================================================\n" );

		fclose( m_pFile );
	}
}

void QFXLog :: Printf( const char *fmt, ... )
{
	va_list argptr;

	if ( !m_pFile || !m_pszLogString )
		return;

	va_start( argptr, fmt );
	_vsnprintf_s( m_pszLogString, QFX_LOG_MAXSTRING, QFX_LOG_MAXSTRING-1, fmt, argptr );
	va_end( argptr );

	fputs( m_pszLogString, m_pFile );
	fflush( m_pFile );
}

void QFXLog :: Error( const char *fmt, ... )
{
	va_list argptr;

	if ( !m_pFile || !m_pszLogString )
		return;

	va_start( argptr, fmt );
	_vsnprintf_s( m_pszLogString, QFX_LOG_MAXSTRING, QFX_LOG_MAXSTRING-1, fmt, argptr );
	va_end( argptr );

	fprintf( m_pFile, "ERROR: %s", m_pszLogString );
	fflush( m_pFile );
}

void QFXLog :: Warning( const char *fmt, ... )
{
	va_list argptr;

	if ( !m_pFile || !m_pszLogString )
		return;

	va_start( argptr, fmt );
	_vsnprintf_s( m_pszLogString, QFX_LOG_MAXSTRING, QFX_LOG_MAXSTRING-1, fmt, argptr );
	va_end( argptr );

	fprintf( m_pFile, "WARNING: %s", m_pszLogString );
	fflush( m_pFile );
}
