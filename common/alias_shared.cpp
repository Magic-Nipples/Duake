
#include <windows.h>

#include "extdll.h"
#include "vector.h"
#include "const.h"
#include "com_model.h"
#include "alias.h"
#include "alias_shared.h"
#include "activity.h"

#ifndef ENGINECALLBACK_H
#include "enginecallback.h"
#endif

/*
====================
SkipWhitespace

====================
*/
char* SkipWhitespace( char* pszin )
{
	char *pszchar = pszin;
	while(isspace(*pszchar) && (*pszchar) != '\0')
		pszchar++;

	if((*pszchar) == '\0')
		return NULL;

	return pszchar;
}

/*
====================
ReadToken

====================
*/
char* ReadToken( char* pszin, char* pszout )
{
	int length = 0;
	char *pszchar = SkipWhitespace(pszin);
	
	if((*pszchar) == '\"')
	{
		// Skip the quote
		pszchar++;

		while((*pszchar) != '\"' && (*pszchar) != '\0' 
			&& (*pszchar) != '\r' && (*pszchar) != '\n')
			pszout[length++] = *pszchar++;

		if((*pszchar) == '\"')
			pszchar++;
	}
	else
	{
		while(!isspace(*pszchar) && (*pszchar) != '\0')
			pszout[length++] = *pszchar++;
	}

	// Close it off before error check
	pszout[length] = '\0';

	if((*pszchar) == '\0')
		return NULL;

	return SkipWhitespace(pszchar);
}

/*
====================
Alias_LoadSequenceInfo

====================
*/
void Alias_LoadSequenceInfo( const char* pszmodelname, const char* pszgamedir, alias_extradata_t* pextradata )
{
	char szpath[MAX_PATH];
	sprintf(szpath, "%s/scripts/%s", pszgamedir, pszmodelname);
	strcpy(&szpath[strlen(szpath)-4], "_seqdata.txt");

	//ALERT(at_console, "sequence data file should be in: %s\n", szpath);

	FILE* pf = fopen(szpath, "r");
	if(!pf)
	{
		// If it's not available, write out the sequence list
		/*pf = fopen(szpath, "w");
		if(!pf)
			return;

		for(int i = 0; i < pextradata->numsequences; i++)
			fprintf(pf, "%s ACT_RESET\r\n", pextradata->sequences[i].name);

		fclose(pf);*/
		ALERT(at_console, "no sequence data found!\n");
		return;
	}

	char sztoken1[64];
	char sztoken2[64];
	char szbuffer[512];
	while(fgets(szbuffer, 512, pf) != NULL)
	{
		// Parse it
		char* pszchar = szbuffer;
		while(pszchar && *pszchar != '\0')
		{
			// Read in seq name
			pszchar = ReadToken(pszchar, sztoken1);
			if(!pszchar)
			{
				ALERT(at_console, "%s: Syntax error: Incomplete line: %s\n", szpath, szbuffer);
				break;
			}

			// Read in activity name
			pszchar = ReadToken(pszchar, sztoken2);

			// Find the sequence for this
			alias_sequence_t* psequence = NULL;
			for(int i = 0; i < pextradata->numsequences; i++)
			{
				if(!strcmp(sztoken1, pextradata->sequences[i].name))
				{
					psequence = &pextradata->sequences[i];
					break;
				}
			}

			if(!psequence)
			{
				ALERT(at_console, "%s: Couldn't match sequence %s\n", szpath, psequence->name);
				break;
			}

			int index = 0;
			while(1)
			{
				activity_map_t* pactivity = GetActivityAtIndex(index++);
				if(!pactivity->type)
					break;

				if(!strcmp(sztoken2, pactivity->name))
				{
					psequence->activity = pactivity->type;
					break;
				}
			}

			if(psequence->activity == ACT_RESET)
			{
				ALERT(at_console, "%s: Couldn't match activity name %s for sequence %s\n", szpath, sztoken2, psequence->name);
				break;
			}

			while(pszchar && *pszchar != '\0')
			{
				// Read next token
				pszchar = ReadToken(pszchar, sztoken1);
				if(!strcmp(sztoken1, "looped"))
				{
					psequence->looped = true;
					continue;
				}
				else
				{
					ALERT(at_console, "%s: Syntax error: Unexpected token %s on line %s\n", szpath, sztoken1, szbuffer);
					break;
				}
			}
		}
	}

	fclose(pf);
}

/*
====================
Alias_GetSequenceInfo

====================
*/
void Alias_GetSequenceInfo( const aliashdr_t* paliashdr, alias_extradata_t* pextradata )
{
	// Get header ptr
	const maliasframedesc_t* pframes = paliashdr->frames;

	// Parse the frames
	for(int i = 0; i < paliashdr->numframes; i++)
	{
		char szname[32];
		strcpy(szname, pframes[i].name);
		
		// Seek out the first non-digit char
		int j = strlen(szname)-1;
		for(; j >= 0 && isdigit(szname[j]); j--);

		// If not zero, resize it
		if(j != 0) 
			szname[j+1] = '\0';

		if(!strlen(szname))
			strcpy(szname, "null");

		// Try to find an existing entry with this name
		j = 0;
		for(; j < pextradata->numsequences; j++)
		{
			if(!strcmp(pextradata->sequences[j].name, szname))
			{
				pextradata->sequences[j].numframes++;
				break;
			}
		}

		// Not found, add it
		if(j == pextradata->numsequences)
		{
			if(pextradata->numsequences == MAX_ALIAS_SEQUENCES)
			{
				ALERT(at_console, "Warning: Exceeded MAX_ALIAS_SEQUENCES on %s, skipping %s\n", pextradata->name, szname);
				continue;
			}

			alias_sequence_t *psequence = &pextradata->sequences[pextradata->numsequences];
			pextradata->numsequences++;

			psequence->activity = ACT_RESET;
			psequence->startframe = i;
			psequence->numframes = 1;
			psequence->looped = false;

			strcpy(psequence->name, szname);
		}
	}
}