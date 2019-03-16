/* ======== Basic Admin tool ========
* Copyright (C) 2004-2007 Erling K. Sæterdal
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Erling K. Sæterdal ( EKS )
* Credits:
*	Menu code based on code from CSDM ( http://www.tcwonline.org/~dvander/cssdm ) Created by BAILOPAN
*	Helping on misc errors/functions: BAILOPAN,karma,LDuke,sslice,devicenull,PMOnoTo,cybermind ( most who idle in #sourcemod on GameSurge realy )
* ============================ */

#ifndef _INCLUDE_MAPS_H
#define _INCLUDE_MAPS_H

#include "BATCore.h"
#include "const.h"
//#include "convar.h"
#include "sh_string.h"
#include <sh_vector.h>

extern char g_CurrentMapName[MAX_MAPNAMELEN+1];
extern SourceHook::CVector<MapStuct *>g_MapList;
extern SourceHook::CVector<MapsVotesStruct *>g_VotesOnMap;
extern int g_MapCount;

extern int g_CurrentMapIndex;
extern int g_NextMapIndex;
extern char g_NextMapName[MAX_MAPNAMELEN+1]; // Used if the map is out of mapcycle


class BATMaps
{
public:
	void LoadMapFiles();
	int GetMapIndex(const char *CurMap);
	int GetNextmap();

private:
	void ReadFile(FILE *in,_MapType IsMapCycleFile);
	const char *GetMapTypeName(int MapType);	
};
#endif //_INCLUDE_CVARS_H
