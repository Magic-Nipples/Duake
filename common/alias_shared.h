/***
*
*	ALIAS_SHARED - USED FOR GETTING SPECIFC SEQUENCE INFO FROM QUAKE 1 MODELS
*	CREDIT TO OVERFLOATER FOR SHARING THIS
*
****/

#ifndef ALIAS_SHARED_H
#define ALIAS_SHARED_H

#define MAX_ALIAS_SEQUENCES		256
#define MAX_ALIAS_MODELS		512
#define MAX_ALIAS_SEQ_EVENTS	16

struct alias_sequence_t
{
	char	name[64];
	int		startframe;
	int		numframes;
	int		activity;
	bool	looped;
};

struct alias_extradata_t
{
	// Name in model_t->name
	char name[256];

	// Sequence data
	alias_sequence_t sequences[MAX_ALIAS_SEQUENCES];
	int numsequences;

	// Pointer to model
	model_t* pmodel;
};

void Alias_GetSequenceInfo( const aliashdr_t* paliashdr, alias_extradata_t* pextradata );
void Alias_LoadSequenceInfo( const char* pszmodelname, const char* pszgamedir, alias_extradata_t* pextradata );
#endif //ALIAS_SHARED_H