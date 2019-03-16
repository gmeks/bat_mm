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
#include "BATCore.h"
#include "BATMaps.h"
#include "const.h"
//#include "convar.h"
#include "sh_string.h"
#include <sh_vector.h>

char g_CurrentMapName[MAX_MAPNAMELEN+1];

SourceHook::CVector<MapStuct *>g_MapList;
SourceHook::CVector<MapsVotesStruct *>g_VotesOnMap;
int g_MapCount=0;

int g_CurrentMapIndex;
int g_NextMapIndex;
char g_NextMapName[MAX_MAPNAMELEN+1]; // Used if the map is out of mapcycle

void BATMaps::LoadMapFiles() 
{
	char TempMapCycle[256],TempAdminMaps[256],TempVoteMaps[256] ,tdir[256];	//This gets the actual path to the users.ini file.
	g_BATCore.GetEngine()->GetGameDir(tdir,255);
	
	_snprintf(TempMapCycle,255,"%s/%s",tdir,g_BATCore.GetBATVar().GetCvarString("mapcyclefile"));
	FILE *fMapCycleFile = fopen(TempMapCycle,"rt");
	
	g_BATCore.GetFilePath(FILE_VOTEMAP,TempVoteMaps);
	FILE *fVoteMaps = fopen(TempVoteMaps,"rt");

	g_BATCore.GetFilePath(FILE_ADMINMAPS,TempAdminMaps);
	FILE *fAdminMaps = fopen(TempAdminMaps,"rt");

	if (!fMapCycleFile)
	{
		g_BATCore.AddLogEntry("ERROR! Unable to find mapcycle file, should be in ( %s )",TempMapCycle);
	}
	if (!fAdminMaps)
	{
		g_BATCore.AddLogEntry("ERROR! Unable to find admin map file, should be in ( %s )",TempAdminMaps);
	}
	if (!fVoteMaps)
	{
		g_BATCore.AddLogEntry("ERROR! Unable to find vote map file, should be in ( %s )",TempVoteMaps);
	}
	int LastMapCount = g_MapCount;
	g_MapCount = 0;
	
	if(fMapCycleFile)
		ReadFile(fMapCycleFile,MAPTYPE_MAPCYCLEMAP);
	if(fVoteMaps)
		ReadFile(fVoteMaps,MAPTYPE_VOTEMAP);
	if(fAdminMaps)
		ReadFile(fAdminMaps,MAPTYPE_ADMINMAP);

	g_BATCore.AddLogEntry("Loaded a total of %d maps from mapcycle",g_MapCount);
}
void BATMaps::ReadFile(FILE *in,_MapType MapType)
{
	char line[256];
	int LineLen=0;

	while (!feof(in)) 
	{
		line[0] = '\0';

		fgets(line,255,in);
		LineLen = strlen(line);

		if (line[0] == '\0' ||line[0] == '/' || line[0] == ';' || LineLen <= 2) 
			continue;

		if(g_MapCount >= (int)g_MapList.size()-1 || g_MapList.size() == 0)
		{
			g_MapList.push_back(new MapStuct);
			g_VotesOnMap.push_back(new MapsVotesStruct);
		}
		_snprintf(g_MapList[g_MapCount]->MapName,MAX_MAPNAMELEN,"%s",line);
		g_BATCore.StrTrim(g_MapList[g_MapCount]->MapName);

		if(g_BATCore.GetEngine()->IsMapValid(g_MapList[g_MapCount]->MapName))
		{
#if BAT_DEBUG == 1
			g_BATCore.ServerCommand("echo Debug - %d)Added map '%s' to %s",g_MapCount,g_MapList[g_MapCount]->MapName,GetMapTypeName(MapType));
#endif

			if( GetMapIndex(g_MapList[g_MapCount]->MapName) != -1)
			{
				int MapIndex = GetMapIndex(g_MapList[g_MapCount]->MapName);

				g_BATCore.AddLogEntry("Warning: %s is in %s and %s duplicate entries are ignored",g_MapList[g_MapCount]->MapName,GetMapTypeName(MapType),GetMapTypeName(g_MapList[MapIndex]->MapType));
				return;
			}

			g_MapList[g_MapCount]->MapType = MapType;
			g_MapList[g_MapCount]->TextLen = strlen(g_MapList[g_MapCount]->MapName);
			g_MapCount++;
		}
		else 
			g_BATCore.AddLogEntry("'%s' is not a valid map was in %s",g_MapList[g_MapCount]->MapName,GetMapTypeName(MapType));
	}
	fclose(in);
}
int BATMaps::GetMapIndex(const char *CurMap)
{
	int TextLen = strlen(CurMap);

	for(int i=0;i<g_MapCount;i++)
	{
		if(TextLen == g_MapList[i]->TextLen && strcmp(CurMap,g_MapList[i]->MapName) == 0)
			return i;
	}
	return -1;
}
const char *BATMaps::GetMapTypeName(int MapType)
{
	switch(MapType)
	{
	case  MAPTYPE_MAPCYCLEMAP:
		return "mapcycle file";

	case MAPTYPE_ADMINMAP:
		return "adminmaps.ini";

	case MAPTYPE_VOTEMAP:
	    return "votemaps.ini";
	}

	return "ERROR Faulty Maptype";
}
int BATMaps::GetNextmap()
{
	if(g_MapCount == 0)
		return 0;

	int NextMap = g_CurrentMapIndex + 1;
	if(NextMap == g_MapCount)
		NextMap = 0;

	int CPS=0;
	while(g_MapList[NextMap]->MapType != MAPTYPE_MAPCYCLEMAP)
	{
		NextMap++;
		CPS++;
		if(NextMap == g_MapCount)
			NextMap = 0;	

		if(CPS == g_MapCount)
			return 0;
	}
	return NextMap;
}