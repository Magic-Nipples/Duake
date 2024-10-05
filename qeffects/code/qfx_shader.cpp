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
#include "qfx_opengl.hpp"
#include "qfx_shader.hpp"

//=========================================
// QFXShader class implementation
//-----------------------------------------
// Class for GLSL shader objects
//=========================================
using namespace gl;

QFXShader :: QFXShader( const char *szvp, const char *szfp ) : m_bValid( false ), m_hProgram( 0 ), m_hVShader( 0 ), m_hFShader( 0 )
{
	GLint objectStatus;
	char nameBuffer[16];

	if ( !szvp && !szfp )
		return;

	if ( szvp ) {
		qglGetError();
		m_hVShader = qglCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		GLint srcLen = strlen( szvp );
		qglShaderSourceARB( m_hVShader, 1, (const char**)&szvp, &srcLen );
		qglCompileShaderARB( m_hVShader );

		if ( qglGetError() != GL_NO_ERROR ) {
			QFXLog::Instance().Error( "Failed to compile vertex shader:\n\n%s\n", szvp );
			PrintInfoLog( m_hVShader );
			return;
		}
		
		qglGetObjectParameterivARB( m_hVShader, GL_OBJECT_COMPILE_STATUS_ARB, &objectStatus );
		if ( !objectStatus ) {
			QFXLog::Instance().Error( "Failed to compile vertex shader:\n\n%s\n", szvp );
			PrintInfoLog( m_hVShader );
			return;
		}
	}

	if ( szfp ) {
		qglGetError();
		m_hFShader = qglCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		GLint srcLen = strlen( szfp );
		qglShaderSourceARB( m_hFShader, 1, (const char**)&szfp, &srcLen);
		qglCompileShaderARB( m_hFShader );

		if ( qglGetError() != GL_NO_ERROR ) {
			QFXLog::Instance().Error( "Failed to compile fragment shader:\n\n%s\n", szfp );
			PrintInfoLog( m_hFShader );
			return;
		}
		
		qglGetObjectParameterivARB( m_hFShader, GL_OBJECT_COMPILE_STATUS_ARB, &objectStatus );
		if ( !objectStatus ) {
			QFXLog::Instance().Error( "Failed to compile fragment shader:\n\n%s\n", szfp );
			PrintInfoLog( m_hFShader );
			return;
		}
	}

	m_hProgram = qglCreateProgramObjectARB();
	if ( szvp ) qglAttachObjectARB( m_hProgram, m_hVShader );
	if ( szfp ) qglAttachObjectARB( m_hProgram, m_hFShader );

	qglGetError();
	qglLinkProgramARB( m_hProgram );

	if ( qglGetError() != GL_NO_ERROR ) {
		QFXLog::Instance().Error( "Failed to link shaders!\n\nVertex:\n%s\n\nFragment:\n%s\n", szvp ? szvp : "none", szfp ? szfp : "none" );
		PrintInfoLog( m_hProgram );
		return;
	}
		
	qglGetObjectParameterivARB( m_hProgram, GL_OBJECT_LINK_STATUS_ARB, &objectStatus );
	if ( !objectStatus ) {
		QFXLog::Instance().Error( "Failed to link shaders!\n\nVertex:\n%s\n\nFragment:\n%s\n", szvp ? szvp : "none", szfp ? szfp : "none" );
		PrintInfoLog( m_hProgram );
		return;
	}

	qglUseProgramObjectARB( m_hProgram );

	//Get uniforms
	for ( int c = 0; c < QFX_MAX_SHADER_UNIFORMS; ++c ) {
		sprintf_s( nameBuffer, "Local%i", c );
		m_iUniforms[c] = qglGetUniformLocationARB( m_hProgram, nameBuffer );
	}

	//Attach uniform textures
	for ( int c = 0; c < QFX_MAX_SHADER_TEXTURES; ++c ) {
		sprintf_s( nameBuffer, "Texture%i", c );
		GLint texunitloc = qglGetUniformLocationARB( m_hProgram, nameBuffer );
		if ( texunitloc != -1 ) qglUniform1iARB( texunitloc, c );
	}

	qglValidateProgramARB( m_hProgram );
	qglGetObjectParameterivARB( m_hProgram, GL_OBJECT_VALIDATE_STATUS_ARB, &objectStatus );
	if ( !objectStatus ) {
		QFXLog::Instance().Error( "Failed to validate shaders!\n\nVertex:\n%s\n\nFragment:\n%s\n", szvp ? szvp : "none", szfp ? szfp : "none" );
		PrintInfoLog( m_hProgram );
		return;
	}

	qglUseProgramObjectARB( 0 );
	m_bValid = true;
}

QFXShader :: ~QFXShader()
{
	if ( m_hVShader ) {
		qglDetachObjectARB( m_hProgram, m_hVShader );
		qglDeleteObjectARB( m_hVShader );
	}
	if ( m_hFShader ) {
		qglDetachObjectARB( m_hProgram, m_hFShader );
		qglDeleteObjectARB( m_hFShader );
	}

	qglDeleteObjectARB( m_hProgram );
}

void QFXShader :: Bind() const
{
	qglUseProgramObjectARB( m_hProgram );
}

void QFXShader :: Unbind() const
{
	qglUseProgramObjectARB( 0 );
}

void QFXShader :: SetParameter4f( int param, float x, float y, float z, float w ) const
{
	const unsigned int uparam = static_cast<unsigned int>( param );
	if ( uparam >= QFX_MAX_SHADER_UNIFORMS ) {
		QFXLog::Instance().Warning( "QFXShader::SetParameter4f: parameter index out of range (%i)\n", param );
		return;
	}

	qglUniform4fARB( m_iUniforms[uparam], x, y, z, w );
}

void QFXShader :: SetParameter4fv( int param, float* v ) const
{
	const unsigned int uparam = static_cast<unsigned int>( param );
	if ( uparam >= QFX_MAX_SHADER_UNIFORMS ) {
		QFXLog::Instance().Warning( "QFXShader::SetParameter4fv: parameter index out of range (%i)\n", param );
		return;
	}

	qglUniform4fvARB( m_iUniforms[uparam], 4, v );
}

void QFXShader :: PrintInfoLog( GLhandleARB object ) const
{
	int logMaxLen;
	qglGetObjectParameterivARB( object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logMaxLen );
	if ( logMaxLen <= 0 )
		return;

	char *plog = new char[logMaxLen+1];
	qglGetInfoLogARB( object, logMaxLen, nullptr, plog );

	if (strlen(plog))
		QFXLog::Instance().Printf("Compile log: %s\n", plog);

	delete [] plog;
}
