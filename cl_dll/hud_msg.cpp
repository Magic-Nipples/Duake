//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "view.h"

#define MAX_CLIENTS 32

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	ASSERT( iSize == 0 );

	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;
	m_iHideHUDDisplay = 0;

	return 1;
}

void CAM_ToFirstPerson(void);

void CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}
}


int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	return 1;
}


int CHud :: MsgFunc_Damage(const char *pszName, int iSize, void *pbuf )
{
	int	i, armor, blood;
	Vector	from;
	float	count;
	
	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i=0 ; i<3 ; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	//faceanimtime = gEngfuncs.GetClientTime() + 0.2; // but sbar face into pain frame

	/*cshifts[CSHIFT_DAMAGE].percent += 3 * count;
	if (cshifts[CSHIFT_DAMAGE].percent < 0)
		cshifts[CSHIFT_DAMAGE].percent = 0;
	if (cshifts[CSHIFT_DAMAGE].percent > 150)
		cshifts[CSHIFT_DAMAGE].percent = 150;

	if (armor > blood)		
	{
		cshifts[CSHIFT_DAMAGE].destcolor[0] = 200;
		cshifts[CSHIFT_DAMAGE].destcolor[1] = 100;
		cshifts[CSHIFT_DAMAGE].destcolor[2] = 100;
	}
	else if (armor)
	{
		cshifts[CSHIFT_DAMAGE].destcolor[0] = 220;
		cshifts[CSHIFT_DAMAGE].destcolor[1] = 50;
		cshifts[CSHIFT_DAMAGE].destcolor[2] = 50;
	}
	else
	{
		cshifts[CSHIFT_DAMAGE].destcolor[0] = 255;
		cshifts[CSHIFT_DAMAGE].destcolor[1] = 0;
		cshifts[CSHIFT_DAMAGE].destcolor[2] = 0;
	}*/

	V_CalcDamage( from, count );

	return 1;
}

int CHud::MsgFunc_SpriteWeapon(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);

	m_iSpriteCur = READ_SHORT();	//current weapon
	m_iSpriteFrame = READ_BYTE();	//current frame

	return 1;
}

int CHud :: MsgFunc_HideHUD( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	m_iHideHUDDisplay = READ_BYTE();

	if (gEngfuncs.IsSpectateOnly())
		return 1;

	if ( m_iHideHUDDisplay & HIDEHUD_ALL )
	{
		static wrect_t nullrc;
		SetCrosshair( 0, nullrc, 0, 0, 0 );
	}

	return 1;
}

/*
=================
CL_ParseBeam
=================
*/
void HUD_ParseBeam( const char *modelname )
{
	int	ent;
	Vector	start, end;

	ent = READ_SHORT ();
	
	start.x = READ_COORD ();
	start.y = READ_COORD ();
	start.z = READ_COORD ();
	
	end.x = READ_COORD ();
	end.y = READ_COORD ();
	end.z = READ_COORD ();
 
	CL_AllocBeam( modelname, ent, start, end );
}

int CHud :: MsgFunc_TempEntity( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int type = READ_BYTE();

	Vector pos;
	dlight_t *dl;
	int colorStart, colorLength;
	TEMPENTITY *pTemp;

	if (type != TE_LIGHTNING1 && type != TE_LIGHTNING2 && type != TE_LIGHTNING3)
	{
		// all quake temp ents begin from pos
		pos.x = READ_COORD ();
		pos.y = READ_COORD ();
		pos.z = READ_COORD ();
	}

	switch (type)
	{
	case TE_SPIKE:
	case TE_SUPERSPIKE:
		gEngfuncs.pEfxAPI->R_RunParticleEffect( pos, vec3_origin, 0, (type == TE_SPIKE) ? 10 : 20 );

		if(gEngfuncs.pfnRandomLong( 0, 5 ) % 5)
		{
			gEngfuncs.pfnPlaySoundByNameAtLocation( "weapons/tink1.wav", 1.0, pos );
		}
		else
		{
			char soundpath[32];
			sprintf( soundpath, "weapons/ric%i.wav", gEngfuncs.pfnRandomLong( 1, 3 ));
			gEngfuncs.pfnPlaySoundByNameAtLocation( soundpath, 1.0, pos );
		}
		break;
	case TE_GUNSHOT:
		gEngfuncs.pEfxAPI->R_RunParticleEffect( pos, vec3_origin, 0, 20 ); //WOLF 3D - used by guns for qparticle - wolf palette is 24 for 3rd entry
		break;
	case TE_EXPLOSION:
		dl = gEngfuncs.pEfxAPI->CL_AllocDlight (0);
		dl->origin = pos;
		dl->radius = 350;
		dl->color.r = dl->color.g = dl->color.b = 250;
		dl->die = gEngfuncs.GetClientTime() + 0.5;
		dl->decay = 300;
		pTemp = gEngfuncs.pEfxAPI->R_DefaultSprite( pos, gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/s_explod.spr" ), 0 );
		if (!pTemp) return 1;
		pTemp->entity.curstate.effects = EF_FULLBRIGHT;
		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderamt = 255;
		gEngfuncs.pfnPlaySoundByNameAtLocation( "weapons/explode1.wav", 1.0, pos );
		gEngfuncs.pEfxAPI->R_ParticleExplosion( pos );
		break;
	case TE_TAREXPLOSION:
		pTemp = gEngfuncs.pEfxAPI->R_DefaultSprite( pos, gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/s_explod.spr" ), 0 );
		if (!pTemp) return 1;
		pTemp->entity.curstate.effects = EF_FULLBRIGHT;
		pTemp->entity.curstate.rendermode = kRenderTransAlpha;
		pTemp->entity.curstate.renderamt = 255;
		gEngfuncs.pfnPlaySoundByNameAtLocation( "weapons/explode1.wav", 1.0, pos );
		gEngfuncs.pEfxAPI->R_BlobExplosion( pos );
		break;
	case TE_LIGHTNING1:
		HUD_ParseBeam( "progs/bolt.mdl" );
		break;
	case TE_LIGHTNING2:
		HUD_ParseBeam( "progs/bolt2.mdl" );
		break;
	case TE_WIZSPIKE:
		gEngfuncs.pEfxAPI->R_RunParticleEffect( pos, vec3_origin, 20, 30 );
		gEngfuncs.pfnPlaySoundByNameAtLocation( "wizard/hit.wav", 1.0, pos );
                    {
			int decalIndex = gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( "{spit1" ));
			gEngfuncs.pEfxAPI->R_DecalShoot( decalIndex, 0, 0, pos, 0 );
		}
		break;
	case TE_KNIGHTSPIKE:
		gEngfuncs.pEfxAPI->R_RunParticleEffect( pos, vec3_origin, 226, 20 );
		gEngfuncs.pfnPlaySoundByNameAtLocation( "hknight/hit.wav", 1.0, pos );
		break;
	case TE_LIGHTNING3:
		HUD_ParseBeam( "progs/bolt3.mdl" );
		break;
	case TE_LAVASPLASH:
		gEngfuncs.pEfxAPI->R_LavaSplash( pos );
		break;
	case TE_TELEPORT:
		gEngfuncs.pEfxAPI->R_TeleportSplash( pos );
		break;
	case TE_EXPLOSION2:
		colorStart = READ_BYTE ();
		colorLength = READ_BYTE ();
		dl = gEngfuncs.pEfxAPI->CL_AllocDlight (0);
		dl->origin = pos;
		dl->radius = 350;
		dl->color.r = dl->color.g = dl->color.b = 250;
		dl->die = gEngfuncs.GetClientTime() + 0.5;
		dl->decay = 300;
		gEngfuncs.pfnPlaySoundByNameAtLocation( "weapons/explode1.wav", 1.0, pos );
		gEngfuncs.pEfxAPI->R_ParticleExplosion2( pos, colorStart, colorLength );
		break;
	case TE_BEAM: // grappling hook beam
		HUD_ParseBeam( "progs/beam.mdl" );
		break;
	}
	return 1;
}