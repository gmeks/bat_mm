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
#include "MenuRconCmds.h"
#include <time.h>
#include "const.h"
//#include "convar.h"
//#include <sourcehook/sourcehook.h>
#include <sh_vector.h>

SourceHook::CVector<RconMenuCmdsStruct *>g_MenuRconCmds;
int unsigned g_MenuRconCmdsCount=0;

void MenuRconCmds::ReadMenuCmdsFile() 
{
	//Begin loading admins
	char TempFile[256];	//This gets the actual path to the users.ini file.
	g_BATCore.GetFilePath(FILE_MENUCMDS,TempFile);
	FILE *is = fopen(TempFile,"rt");

	char line[512];
	int iLineLen=0;

	int unsigned LastRconCmdsCount = g_MenuRconCmdsCount;
	g_MenuRconCmdsCount = 0;

	if (!is)
	{
		g_BATCore.AddLogEntry("ERROR! Unable to find menu cmds ini file, should be in ( %s%s )",g_BasePath,FILE_MENUCMDS);
		return;
	}

//	char TmpChar[64];
	char *TempBuffer;
	int TextIndex=0;
	int LineNum=0;

	while (!feof(is)) 
	{
		if(g_MenuRconCmdsCount >= g_MenuRconCmds.size()-1 || g_MenuRconCmds.size() == 0)
			g_MenuRconCmds.push_back(new RconMenuCmdsStruct);

		fgets(line,512,is);
		iLineLen = strlen(line);
		LineNum++;
		if (line[0] == '/' || line[0] == ';' || iLineLen <= 2) continue;

		int FirstPara =-1;
		int FirstLength = -1;
		int SecPara = -1;
		int SecLength = -1;
		
		for(int i=0;i<iLineLen;i++)
		{
			if(FirstPara == -1 && line[i] == '"')
				FirstPara = i + 1;
			else if(FirstLength == -1 && line[i] == '"')
				FirstLength = i - 1;
			else if(SecPara == -1 && line[i] == '"')
				SecPara = i + 1;
			else if(SecLength == -1 && line[i] == '"')
				SecLength = i;
		}
		if(FirstPara == -1 || FirstLength == -1 || SecPara == -1 || SecLength == -1)
		{
			g_BATCore.AddLogEntry("ERROR! Parsing menucmds.ini file, on line %d",LineNum);
			line[0] = '\0';
			continue;
		}

		TempBuffer = (char *) line;

#ifndef WIN32
		FirstLength++;
		SecLength++;
#endif

		_snprintf(g_MenuRconCmds[g_MenuRconCmdsCount]->Description,FirstLength,"%s",TempBuffer + FirstPara);
		g_MenuRconCmds[g_MenuRconCmdsCount]->Description[FirstLength] = '\0';
		
		SecLength -= SecPara;
		_snprintf(g_MenuRconCmds[g_MenuRconCmdsCount]->Command,SecLength,"%s",TempBuffer + SecPara);
		g_MenuRconCmds[g_MenuRconCmdsCount]->Command[SecLength] = '\0';		

		g_MenuRconCmdsCount++;
		line[0] = '\0';
	}
	fclose(is);

	if(LastRconCmdsCount > g_MenuRconCmdsCount)	//This means the number of admin accounts has drooped, we should really clear memory
	{
		for(unsigned int i=g_MenuRconCmdsCount;i<LastRconCmdsCount;i++)
			g_MenuRconCmds[i] = NULL;
	}
}
const char *MenuRconCmds::ParseRconCmdsString(int Index,bool UseDisc)
{
	static char TempBuffer[512];
	//strcpy(TempBuffer,)
	if(UseDisc)
		_snprintf(TempBuffer,sizeof(TempBuffer),"%s",g_MenuRconCmds[Index]->Description);
	else
		_snprintf(TempBuffer,sizeof(TempBuffer),"%s",g_MenuRconCmds[Index]->Command);

	g_BATCore.StrReplace(TempBuffer,"#timeleft#",g_BATCore.GetTimeleftString(),MESSAGE_MAXLEN);
	g_BATCore.StrReplace(TempBuffer,"#currentmap#",g_CurrentMapName,MESSAGE_MAXLEN);

	char DateDay[5],DateMonth[5];
	time_t td; time(&td);
	strftime(DateDay, 4, "%d", localtime(&td));
	strftime(DateMonth, 4, "%m", localtime(&td));

	g_BATCore.StrReplace(TempBuffer,"#month#",DateMonth,MESSAGE_MAXLEN);
	g_BATCore.StrReplace(TempBuffer,"#day#",DateDay,MESSAGE_MAXLEN);

	if(g_NextMapIndex == -1)
		g_BATCore.StrReplace(TempBuffer,"#nextmap#",g_NextMapName,MESSAGE_MAXLEN);
	else
		g_BATCore.StrReplace(TempBuffer,"#nextmap#",g_MapList[g_NextMapIndex]->MapName,MESSAGE_MAXLEN);

	return TempBuffer;
}