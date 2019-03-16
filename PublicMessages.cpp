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
#include "PublicMessages.h"
#include <time.h>
#include "const.h"
//#include "convar.h"
#include <sourcehook/sourcehook.h>
#include <sh_vector.h>

extern float g_NextPublicMsgTime;
extern SourceHook::CVector<MapStuct *>g_MapList;
extern char g_CurrentMapName[MAX_MAPNAMELEN+1];
extern int g_NextMapIndex;
extern char g_NextMapName[MAX_MAPNAMELEN+1];

//PublicMessageConst g_PublicMessage[MESSAGE_MAXCOUNT];
SourceHook::CVector<PublicMessageConst *>g_PublicMessage;

extern ModSettingsStruct g_ModSettings;
int unsigned g_PublicCount;
int g_LastPublicMessage;
int g_PubMsgStatus = PM_ENABLED;

void PublicMessages::ReadPublicMessagesFile()
{
	char FileName[512];	//This gets the actual path to the users.ini file.
	_snprintf(FileName,511,"%s%s",g_BasePath,FILE_PUBMSG);
	FILE *in = fopen(FileName,"rt");

	if (!in)
	{
		g_BATCore.AddLogEntry("ERROR! Unable to find publicmessages.txt, should be in ( %s )",FileName);
		return;
	}
	unsigned int LastPubCount = g_PublicCount;
	g_PublicCount = 0;
	char line[MESSAGE_MAXLEN+1];
	int LineLen=0;
	int BadLines=0;

	while (!feof(in)) 
	{
		line[0] = '\0';
		fgets(line,MESSAGE_MAXLEN-1,in);
		LineLen = strlen(line);
		
		if(line[0] == ';' || line[0] == '/' && line[1] == '/' || LineLen <= 3 ) // The line is commented out
			continue;

		if(LineLen >= MESSAGE_MAXLEN)
		{
			g_BATCore.AddLogEntry("ERROR! Line containing '%s' was to long, max is: %d was %d",line,LineLen,MESSAGE_MAXLEN-1);
			continue;
		}
		
		if(g_PublicCount >= g_PublicMessage.size()-1 || g_PublicMessage.size() == 0)
			g_PublicMessage.push_back(new PublicMessageConst);

		_snprintf(g_PublicMessage[g_PublicCount]->Text,MESSAGE_MAXLEN-1,"\x01%s",line);
		//g_PublicMessage[g_PublicCount]->Text[LineLen] = '\0';

		g_BATCore.StrTrim(g_PublicMessage[g_PublicCount]->Text);
		g_PublicMessage[g_PublicCount]->TextLen = strlen(g_PublicMessage[g_PublicCount]->Text) + 1;

		//g_BATCore.ServerCommand("echo Debug - New public message: '%s' ('%s') Len: %d",g_PublicMessage[g_PublicCount]->Text,line,LineLen);

#if BAT_DEBUG == 1
		META_LOG(g_PLAPI,"Debug - New public message: '%s' Len: %d",g_PublicMessage[g_PublicCount]->Text,LineLen);
#endif
		g_PublicCount++;
		line[0] = '\0';
	}
	if(LastPubCount > g_PublicCount)	//This means the number of public messages has drooped, so we remove none used once
	{
		for(unsigned int i=g_PublicCount;i<LastPubCount;i++)
			g_PublicMessage[i] = NULL;

		//g_PublicMessage.clear();
	}
	fclose(in);

	if(g_ModSettings.GameMod == MOD_FIREARMS2)
	{
		g_PublicMessage.push_back(new PublicMessageConst);
		_snprintf(g_PublicMessage[g_PublicCount]->Text,MESSAGE_MAXLEN-1,"The \"creators\" of this mods are thiefs, for more information check http://www.firearmsmod.com");
		g_PublicMessage[g_PublicCount]->TextLen = strlen(g_PublicMessage[g_PublicCount]->Text) + 1;
		g_PublicCount++;
	}
}
void PublicMessages::ShowRandomPublicMessage()
{
	int PubMsgVar = g_BATCore.GetBATVar().GetPubMsgMode();
	int PubMsgTime = g_BATCore.GetBATVar().GetPubMsgTime();

	if(PubMsgTime == 0 || g_PublicCount == 0 || PubMsgVar == 0 )
	{
		g_PubMsgStatus = PM_DISABLED;
		return;
	}

	int MsgId = GetRandomMessageIndex();
	g_LastPublicMessage = MsgId;

	char PubMsg[MESSAGE_MAXLEN];
	_snprintf(PubMsg,g_PublicMessage[MsgId]->TextLen,"%s",g_PublicMessage[MsgId]->Text);	

	if(PubMsgVar == 5)
	{
		srand(time(NULL));
		PubMsgVar = rand()%4;
		PubMsgVar++;
	}

	ParsePubMessage(PubMsg,PubMsgVar);

	switch(PubMsgVar)
	{
		case 1:
			g_BATCore.MessagePlayer(0,PubMsg);
			break;

		case 2:
			g_BATCore.SendHintMsg(0,PubMsg);
			break;

		case 3:
			g_BATCore.ServerCommand("say %s",PubMsg);
			break;

		case 4:
			g_BATCore.SendCornerMsg(0,PubMsg);
			break;
	}

	g_NextPublicMsgTime = GetGlobals()->curtime + PubMsgTime;
}
int PublicMessages::GetRandomMessageIndex()
{
	if(g_PublicCount <= 1)
		return 0;
	
	srand(time(NULL));
	int MsgId = rand()%g_PublicCount;
	int CPS = 0;		// Crash prevention system - patent pending ;)
	
	while(g_LastPublicMessage == MsgId && CPS <= 50)
	{
		MsgId = rand()%g_PublicCount;
		CPS++;
	}
	
	return MsgId;
}
// \x03 Light green
// \x04 regular green
void PublicMessages::ParsePubMessage(char *pPubMsg,int PubMsgMode)
{
	// welcome to string working at its worste. Some day this should be fixed up, but not today it seems
	g_BATCore.StrReplace(pPubMsg,"#timeleft#",g_BATCore.GetTimeleftString(),MESSAGE_MAXLEN);
	g_BATCore.StrReplace(pPubMsg,"#currentmap#",g_CurrentMapName,MESSAGE_MAXLEN);
	g_BATCore.StrReplace(pPubMsg,"#nextmap#",g_NextMapName,MESSAGE_MAXLEN);

	
 	// Colors
	if(PubMsgMode == 1)
	{
		g_BATCore.StrReplace(pPubMsg,"#normal#","\x01",MESSAGE_MAXLEN);
		g_BATCore.StrReplace(pPubMsg,"#lgreen#","\x03",MESSAGE_MAXLEN);
		g_BATCore.StrReplace(pPubMsg,"#green#","\x04",MESSAGE_MAXLEN);
	}
	else // Whatever mode it is, it does not support colors so we remove them
	{
		pPubMsg[0] = ' ';

		g_BATCore.StrReplace(pPubMsg,"#normal#","",MESSAGE_MAXLEN);
		g_BATCore.StrReplace(pPubMsg,"#lgreen#","",MESSAGE_MAXLEN);
		g_BATCore.StrReplace(pPubMsg,"#green#","",MESSAGE_MAXLEN);
	}
	g_BATCore.StrTrim(pPubMsg);

	//g_BATCore.StrReplace(pPubMsg,"#normal#"," #n ",MESSAGE_MAXLEN);
	//g_BATCore.StrReplace(pPubMsg,"#green#"," #g ",MESSAGE_MAXLEN);
	//g_BATCore.StrReplace(pPubMsg,"#lgreen#"," #l ",MESSAGE_MAXLEN);
/*
	int iLen = strlen(pPubMsg) - 1;
	for(int i=0;i<iLen;i++)
	{

		if(pPubMsg[i] == '#' && pPubMsg[i+1] == 'g')
		{
			pPubMsg[i] = '\x03';
			pPubMsg[i+1] = ' ';
			continue;
		}

		if(pPubMsg[i] == '#' && pPubMsg[i+1] == 'l')
		{
			pPubMsg[i] = '\x04';
			pPubMsg[i+1] = ' ';
			continue;
		}

		if(pPubMsg[i] == '#' && pPubMsg[i+1] == 'n')
		{
			pPubMsg[i] = '\x02';
			pPubMsg[i+1] = ' ';
			continue;
		}
	}
	*/
}

