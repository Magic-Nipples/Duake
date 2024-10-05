/*
*
*	Copyright (c) 2024, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit is provided! Enjoy!
*
*/

#ifndef EFFECTS_H
#define EFFECTS_H

#define SF_SPRITE_START_OFF		1
#define SF_SPRITE_HAS_OFF		2
#define SF_SPRITE_FULLBRIGHT	4
#define SF_SPRITE_PLAY_ONCE		8
#define SF_SPRITE_HOLD_FRAME	16

#define SF_SPRITE_SOLID			128

class CSprite : public CPointEntity
{
public:
	void Spawn(void);
	void Precache(void);

	void EXPORT StartThink(void);
	void EXPORT AnimateThink(void);
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void Animate();

	void TurnOff(void);
	void TurnOn(void);

	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];

private:
	float		m_maxFrame;
};

class CGargoyle : public CPointEntity
{
public:
	void Spawn(void);
	void Precache(void);

	void EXPORT StartThink(void);
	void EXPORT WakeThink(void);
	void EXPORT LookForThink(void);
	void EXPORT FireThink(void);

	void TurnOff(void);
	void TurnOn(void);

	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];

private:
	float		m_maxFrame;
	float		f_nextAttack;
	Vector		attackOrigin;
	Vector		vForward;
};

#endif