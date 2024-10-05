//
// EHANDLE. Safe way to point to CBaseEntities who may die between frames
//
#ifndef EHANDLE_H
#define EHANDLE_H


class EHANDLE
{
private:
	edict_t	*m_pent;
	int	m_serialnumber;
public:
	edict_t *Get( void );
	edict_t *Set( edict_t *pent );

	operator int ( void );

	operator CBaseEntity *( void );

	CBaseEntity *operator = (CBaseEntity *pEntity);
	CBaseEntity *operator ->( void );
};

inline edict_t *EHANDLE::Get( void ) 
{ 
	if (m_pent)
	{
		if (m_pent->serialnumber == m_serialnumber) 
			return m_pent; 
		else
			return NULL;
	}
	return NULL; 
}

inline edict_t * EHANDLE::Set( edict_t *pent ) 
{ 
	m_pent = pent;  
	if (pent) 
		m_serialnumber = m_pent->serialnumber; 
	return pent; 
}

inline EHANDLE :: operator CBaseEntity* ( void ) 
{ 
	return (CBaseEntity *)GET_PRIVATE( Get( ) ); 
}

inline CBaseEntity * EHANDLE :: operator = (CBaseEntity *pEntity)
{
	if (pEntity)
	{
		m_pent = ENT( pEntity );
		if (m_pent)
			m_serialnumber = m_pent->serialnumber;
	}
	else
	{
		m_pent = NULL;
		m_serialnumber = 0;
	}
	return pEntity;
}

inline EHANDLE :: operator int ( void )
{
	return Get() != NULL;
}

inline CBaseEntity * EHANDLE :: operator ->( void )
{
	return (CBaseEntity *)GET_PRIVATE( Get( ) ); 
}

#endif//EHANDLE_H