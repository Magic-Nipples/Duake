/*

===== buttons.cpp ========================================================

  button-related code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "doors.h"

/*QUAKED func_button (0 .5 .8) ?
When a button is touched, it moves some distance in the direction of it's angle,
triggers all of it's targets, waits some time, then returns to it's original position
where it can be triggered again.

"angle"	determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"	override the default 40 speed
"wait"	override the default 1 second wait (-1 = never return)
"lip"	override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"sounds"
0) steam metal
1) wooden clunk
2) metallic click
3) in-out
*/
class CBaseButton : public CBaseToggle
{
public:
	void Spawn( void );
	virtual void Precache( void );

	void ButtonActivate( );

	void EXPORT ButtonTouch( CBaseEntity *pOther );
	void EXPORT TriggerAndWait( void );
	void EXPORT ButtonReturn( void );
	void EXPORT ButtonBackHome( void );
	void EXPORT ButtonUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	virtual void Killed( entvars_t *pevAttacker, int iGib );
};

LINK_ENTITY_TO_CLASS( func_button, CBaseButton );

void CBaseButton::Precache( void )
{
	switch( m_sounds )
	{
	case 0:
		PRECACHE_SOUND( "buttons/airbut1.wav" );
		pev->noise = MAKE_STRING( "buttons/airbut1.wav" );
		break;
	case 1:
		PRECACHE_SOUND( "buttons/switch21.wav" );
		pev->noise = MAKE_STRING( "buttons/switch21.wav" );
		break;
	case 2:
		PRECACHE_SOUND( "buttons/switch02.wav" );
		pev->noise = MAKE_STRING( "buttons/switch02.wav" );
		break;
	case 3:
		PRECACHE_SOUND( "buttons/switch04.wav" );
		pev->noise = MAKE_STRING( "buttons/switch04.wav" );
		break;
	default:
		PRECACHE_SOUND( "misc/null.wav" );
		pev->noise = MAKE_STRING( "misc/null.wav" );
		break;
	}
}

int CBaseButton :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (!pev->takedamage)
		return 0;

	pev->health -= flDamage;

	if (pev->health <= 0)
	{
		Killed( pevAttacker, GIB_NORMAL );
		return 0;
	}

	return 1;
}

void CBaseButton::Killed( entvars_t *pevAttacker, int iGib )
{
	m_hActivator = CBaseEntity::Instance( pevAttacker );
	pev->health = pev->max_health;
	pev->takedamage = DAMAGE_NO;	// wil be reset upon return

	ButtonActivate( );
}

void CBaseButton::Spawn( void )
{ 
	Precache();

	SetMovedir(pev);

	pev->movetype	= MOVETYPE_PUSH;
	pev->solid	= SOLID_BSP;
	SET_MODEL(ENT(pev), STRING(pev->model));

	SetUse( &CBaseButton::ButtonUse );
	
	if (pev->speed == 0)
		pev->speed = 40;

	if (pev->health > 0)
	{
		pev->takedamage = DAMAGE_YES;
		pev->max_health = pev->health;
	}
	else
	{
		SetTouch(&CBaseButton::ButtonTouch );
	}

	if (m_flWait == 0)
		m_flWait = 1;
	if (m_flLip == 0)
		m_flLip = 4;

	m_toggle_state = TS_AT_BOTTOM;
	m_vecPosition1 = pev->origin;

	// Subtract 2 from size because the engine expands bboxes by 1 in all directions making the size too big
	m_vecPosition2 = m_vecPosition1 + pev->movedir * (fabs(DotProduct( pev->movedir, pev->size )) - m_flLip);
}

//
// Button's Use function
//
void CBaseButton::ButtonUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	m_hActivator = pActivator;
	ButtonActivate ();
}

//
// Touching a button simply "activates" it.
//
void CBaseButton:: ButtonTouch( CBaseEntity *pOther )
{
	// Ignore touches by anything but players
	if (!FClassnameIs(pOther->pev, "player"))
		return;

	m_hActivator = pOther;
	ButtonActivate( );
}

//
// Starts the button moving "in/up".
//
void CBaseButton::ButtonActivate( void )
{
	if (m_toggle_state == TS_GOING_UP || m_toggle_state == TS_AT_TOP)
		return;

	EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);

	m_toggle_state = TS_GOING_UP;
	
	SetMoveDone( &CBaseButton::TriggerAndWait );

	LinearMove( m_vecPosition2, pev->speed );
}

//
// Button has reached the "in/up" position.  Activate its "targets", and pause before "popping out".
//
void CBaseButton::TriggerAndWait( void )
{
	m_toggle_state = TS_AT_TOP;
	
	pev->nextthink = pev->ltime + m_flWait;
	SetThink( &CBaseButton::ButtonReturn );

	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 );
	pev->frame = 1; // use alternate textures
}


//
// Starts the button moving "out/down".
//
void CBaseButton::ButtonReturn( void )
{
	m_toggle_state = TS_GOING_DOWN;
	
	SetMoveDone( &CBaseButton::ButtonBackHome );

	LinearMove( m_vecPosition1, pev->speed);

	pev->frame = 0; // use normal textures

	if (pev->health)
		pev->takedamage = DAMAGE_YES;	// can be shot again
}


//
// Button has returned to start state.  Quiesce it.
//
void CBaseButton::ButtonBackHome( void )
{
	m_toggle_state = TS_AT_BOTTOM;
}