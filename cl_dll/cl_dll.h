//
//  cl_dll.h
//

//
//  This DLL is linked by the client when they first initialize.
// This DLL is responsible for the following tasks:
//		- Loading the HUD graphics upon initialization
//		- Drawing the HUD graphics every frame
//		- Handling the custum HUD-update packets
//
typedef unsigned char byte;
typedef unsigned short word;
typedef float vec_t;
typedef int (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);

#include "util_vector.h"
#define EXPORT	_declspec( dllexport )

#include "../engine/cdll_int.h"
#include "../dlls/quakedef.h"

extern cl_enginefunc_t gEngfuncs;
extern struct ref_params_s *gpViewParams;