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
#include "AdminCmdManager.h"
#include "Translation.h"
#include "const.h"

extern AdminCommandStruct g_AdminCommands[MAX_ADMINCOMMANDS+1];
extern AdminCmdManager *m_AdminCmdMngr;
extern BATMenuMngr g_MenuMngr;
extern Translation *m_Translation;
extern unsigned int g_ArgCount;

extern bool g_IsViaSay;

unsigned int g_AdminCmdCount=0;
char g_OrgSayText[192];

#if BAT_ORANGEBOX == 1
void AdminCmdManager::CheckSrvCmd(const CCommand &args)
{
#else
void AdminCmdManager::CheckSrvCmd()
{
	CCommand args;
#endif

#if BAT_DEBUG == 1
	g_BATCore.ServerCommand("echo %s was used",g_LastCCommand.Arg(0));
#endif
	g_LastCCommand = args;

	m_AdminCmdMngr->CheckCmd(ID_SERVER,g_LastCCommand.Arg(0));
}
bool AdminCmdManager::CheckSayCmd(int id,const char *Cmd)
{
	g_ArgCount = g_LastCCommand.ArgC();
	int len=0;
	g_IsViaSay = true;

	if(g_ModSettings.GameMod != MOD_INSURGENCY)
	{
		if(g_ArgCount >= 2)
		{
			for(unsigned int i=1;i<g_ArgCount;i++)
			{
				if(i == 1)
					len += _snprintf(&(g_OrgSayText[len]),191-len,"%s",g_LastCCommand.Arg(i));
				else
					len += _snprintf(&(g_OrgSayText[len]),191-len," %s",g_LastCCommand.Arg(i));
			}

			g_OrgSayText[0] = ' ';		// Remove the #
			g_BATCore.StrReplace(g_OrgSayText,Cmd,"",191);
			g_BATCore.StrTrimLeft(g_OrgSayText);
		}
		else 
			g_OrgSayText[0] = '\0';
	}
	else
	{
		if(g_ArgCount >= 4)
		{
			for(unsigned int i=3;i<g_ArgCount;i++)
			{
				if(i == 3)
					len += _snprintf(&(g_OrgSayText[len]),191-len,"%s",g_LastCCommand.Arg(i));
				else
					len += _snprintf(&(g_OrgSayText[len]),191-len," %s",g_LastCCommand.Arg(i));
			}

			g_OrgSayText[0] = ' ';		// Remove the #
			g_BATCore.StrReplace(g_OrgSayText,Cmd,"",191);
			g_BATCore.StrTrimLeft(g_OrgSayText);
		}
		else 
			g_OrgSayText[0] = '\0';
	}
	
	
	return m_AdminCmdMngr->CheckCmd(id,Cmd);
}

bool AdminCmdManager::CheckCmd(int id,const char *Cmd)
{
	for(unsigned int i=0;i<g_AdminCmdCount;i++)
	{
		if(strcmp(g_AdminCommands[i].Cmd,Cmd) == 0 || g_IsViaSay && strcmp(g_AdminCommands[i].SayCmd,Cmd) == 0)
		{
			if(!g_BATCore.HasAccess(id,g_AdminCommands[i].AccessRequired))
			{
				g_BATCore.GetAdminCmds()->NoticePlayer(id,m_Translation->GetTranslation("CmdNoAccess"),g_AdminCommands[i].Cmd);
				g_IsViaSay = false;
				return true;
			}

			g_ArgCount = GetArgCount();
			if((int)g_ArgCount <= g_AdminCommands[i].CmdArgsRequired && g_AdminCommands[i].CmdArgsRequired != 0)
			{
				g_BATCore.GetAdminCmds()->NoticePlayer(id,"[BAT] %s",m_Translation->GetTranslation(g_AdminCommands[i].CmdUsage));
				g_IsViaSay = false;
				return true;
			}			
			g_AdminCommands[i].pFunc(id);
			g_IsViaSay = false;
			return true;
		}
	}
	return false;
}
void AdminCmdManager::RegisterCommand(char *Cmd,char *CmdUsage,char *CmdDesc,CmdFuncIntArg pFunc,int CmdFlags,int ArgCountReq)
{
	//AdminCommands[g_AdminCmdCount].pFunc = (CmdFuncIntArg)&BATCore::AdminHelp;
	if(strlen(g_AdminCommands[g_AdminCmdCount].Cmd) > MAX_CMDSIZE || strlen(g_AdminCommands[g_AdminCmdCount].Description) > MAX_CMDDESCSIZE )
		return;

	_snprintf(g_AdminCommands[g_AdminCmdCount].Cmd,MAX_CMDSIZE,"%s",Cmd);
	_snprintf(g_AdminCommands[g_AdminCmdCount].SayCmd,MAX_CMDSIZE,"%s",Cmd);
	_snprintf(g_AdminCommands[g_AdminCmdCount].Description,MAX_CMDDESCSIZE,"%s",CmdDesc);
	_snprintf(g_AdminCommands[g_AdminCmdCount].CmdUsage,MAX_CMDDESCSIZE,"%s",CmdUsage);
	g_AdminCommands[g_AdminCmdCount].pFunc = pFunc;
	g_AdminCommands[g_AdminCmdCount].AccessRequired = CmdFlags;
	g_AdminCommands[g_AdminCmdCount].CmdArgsRequired = ArgCountReq;

	g_BATCore.StrReplace(g_AdminCommands[g_AdminCmdCount].SayCmd,"admin_","",MAX_CMDSIZE);

	// We register the command in the server console
	ConCommand *SrvCMD = new ConCommand(Cmd,AdminCmdManager::CheckSrvCmd,CmdDesc);

	//AdminCommands[g_AdminCmdCount].pFunc = (CmdFuncIntArg)&BATCore::AdminHelp;
	g_AdminCmdCount++;
}
SourceHook::CVector<ArgListStruct *>g_ArgList;

int AdminCmdManager::GetArgCount()
{
	if(!g_IsViaSay)
		return g_LastCCommand.ArgC();
	else
	{
		if(g_ArgList.size() == 0 || g_ArgList.size() == 1)
			g_ArgList.push_back(new ArgListStruct);

		char seps[]   = " ,\t\n";
		char *token;
		g_ArgCount = 0;

		token = strtok( g_OrgSayText, seps );
		if(token != NULL)
		{
			_snprintf(g_ArgList[g_ArgCount]->Text,MAX_CMDSIZE,"%s",token);
			g_ArgCount++;
		}

		while( token != NULL )
		{
			/* Get next token: */
			token = strtok( NULL, seps );
			if(token == NULL)
				break;
			
			while(g_ArgCount >= g_ArgList.size()-1)
				g_ArgList.push_back(new ArgListStruct);

			_snprintf(g_ArgList[g_ArgCount]->Text,MAX_CMDSIZE,"%s",token);
			g_ArgCount++;
		}
		if(g_ArgCount != 0)
			g_ArgCount++;		// Since we dont catch the original command here, we add +1

		return g_ArgCount;
	}
}