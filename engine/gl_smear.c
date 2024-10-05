/*
gl_smear.c - create a similar transitioning effect like in Doom

Copyright (C) 2024 Magic_Nipples

*/

#include "common.h"
#include "client.h"
#include "gl_local.h"

/*
=================
R_Sampling_InitTextures
=================
*/
void R_Sampling_InitTextures(void)
{
	int texture = tr.r_initsampletexture;
	tr.r_initsampletexture = GL_CreateTexture("*samplescreentexture", glState.width, glState.height, NULL, /*TF_LUMINANCE |*/ TF_NOMIPMAP | TF_NOROUND);
	tr.m_fSmearFadeTime = 1.0f;
}

/*
=================
R_InitDownSampleTextures
=================
*/
void R_InitDownSampleTextures(void)
{
	if (tr.r_initsampletexture)
		GL_FreeTexture(tr.r_initsampletexture);

	tr.r_initsampletexture = 0;

	R_Sampling_InitTextures();
}

/*
=================
R_DownSampling
=================
*/
//CL_IsIntermission()
void R_DownSampling(void)
{
	if(!r_smear->value)//if (GameState->curstate != STATE_RUNFRAME && !r_smear->value)
		return;

	if (GameState->curstate == STATE_RUNFRAME && r_smear->value == 1.0f)
	{
		R_InitDownSampleTextures();

		Cvar_SetValue("v_melt", 2.0f);

		// set up full screen workspace
		//pglScissor(0, 0, glState.width, glState.height);
		//pglViewport(0, 0, glState.width, glState.height);
		//pglMatrixMode(GL_PROJECTION);
		//pglLoadIdentity();

		//pglOrtho(0, glState.width, glState.height, 0, -10, 100);

		//pglMatrixMode(GL_MODELVIEW);
		//pglLoadIdentity();
	}

	pglDisable(GL_DEPTH_TEST);
	pglDisable(GL_ALPHA_TEST);
	pglDepthMask(GL_FALSE);
	pglEnable(GL_BLEND);

	GL_Cull(GL_NONE);

	pglColor4f(1.0f, 1.0f, 1.0f, tr.m_fSmearFadeTime);

	// copy the screen and draw resized
	GL_Bind(GL_TEXTURE0, tr.r_initsampletexture);

	if (GameState->curstate == STATE_RUNFRAME && (r_smear->value == 1 || r_smear->value == 2))
	{
		pglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, glState.width, glState.height);

		Cvar_SetValue("v_melt", 3.0f);
	}
	else if (r_smear->value == 3)
	{
		tr.m_fSmearFadeTime -= host.frametime * 1.5;

		if (tr.m_fSmearFadeTime <= 0.0f)
		{
			Cvar_SetValue("v_melt", 0.0f);
			tr.m_fSmearFadeTime = 1.0f;
		}
	}

	pglBegin(GL_QUADS);
	pglTexCoord2f(0, 1);
	pglVertex2f(0, 0);
	pglTexCoord2f(0, 0);
	pglVertex2f(0, 0 + glState.height);
	pglTexCoord2f(1, 0);
	pglVertex2f(0 + glState.width, 0 + glState.height);
	pglTexCoord2f(1, 1);
	pglVertex2f(0 + glState.width, 0);
	pglEnd();

	pglEnable(GL_DEPTH_TEST);
	pglDepthMask(GL_TRUE);
	pglDisable(GL_BLEND);
	GL_Cull(GL_FRONT);
}