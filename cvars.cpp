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
#include "cvars.h"
#include "ReservedSlotsSystem.h"
#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>
#include <sourcehook.h>
#include "sourcehook.h"

extern stSQLSettings g_SQLSettings;
extern stSQLStatus g_SQLStatus;
extern int g_AmxBansAddServerTries;
extern BATMenuMngr g_MenuMngr;

extern int unsigned g_PublicCount;
extern int g_PubMsgStatus;
extern float g_NextPublicMsgTime;
extern int g_PublicVoteCmdsMode;

char g_RulesURL[512];
bool g_RulesURLSet = false;

extern int g_MapVotePercentageRequired;					// Percentage of map votes required
extern int g_PlayerVotePercentageRequired;				// Percentage of player votes like vote kick/ban votes required

ConVar g_VarBATVersion("bat_version",BAT_VERSION, FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "The version of Basic Admin Tool");
ConVar g_VarAdminNoneSay("bat_allownoneadmintalk", "1", 0, "Allow none admin players to send admin say messages to admins ( But they cannot ready them)");
ConVar g_VarHideAdminAction("bat_hideadminname", "0", 0, "0=Disabled\n1=Hides admins name from admin actions like kick/slap");

ConVar g_VarMapExtend("bat_mapextend", "1", 0, "\n0 = Map vote at the end of map is disabled (Say nextmap is also disabled)\n1 = Enable map vote at the end of map\n2 = Chat based voting\n3 = Enables only responding to say nextmap");
ConVar g_VarMapRockTheVote("bat_maprockthevote", "1", 0, "\n0 = say rockthevote disabled\n1 = say rockthevote enabled");
ConVar g_VarMapExtendTimes("bat_mapextend_time", "3", 0, "How many times the map can be extended");
ConVar g_VarMapVotesRequired("bat_votemap_required", "50", 0, "How big a percentage of players that needed for a successful map vote (custom map votes and rtv)");
ConVar g_VarPlayerVotesRequired("bat_voteplayer_required", "60", 0, "How big a percentage of players that needed for a successful player vote (votekick and voteban)");

ConVar g_VarPublicVoteCmds("bat_publicvotecmds", "0", 0, "0 = Disabled\n1= Allow votekick\n2= Allow voteban\n3= Allow votekick and voteban");
ConVar g_VarVoteBanTime("bat_votebantime", "60", 0, "How long a voteban lasts in minutes");

ConVar g_VarPubTimeLeft("bat_timeleft", "1", 0, "If BAT should respond to say timeleft and thetime commands");
ConVar g_VarPubMsgTime("bat_publicmsg_time", "5", 0, "How often in minutes to show a public message");
ConVar g_VarPubMsgMode("bat_publicmsg_mode", "2", 0, "0 = System Disabled\n1 = Enabled via normal chat messages\n2 = Enabled via Hint messages\n3 = Enabled via console say\n4 = Sendt via corner message\n5 = Send via a random method");
ConVar g_VarSettingsReload("bat_loadsettings", "1", 0, "Load settings on every mapchange ( mapcycle / users.ini / public messages / menucmds.ini )\n0 = Only loads once");

ConVar g_VarAuthTime("bat_authtime", "15", 0, "How long a player has to authenticate his steamid after he is fully connected, if it does not auth he gets kicked");
ConVar g_VarMenuType("bat_menutype", "1", 0, "1 = Regular radio style menu ( Like those from hl1)\n2 = ESC based menus, works in any mod");
ConVar g_VarSignatureScanner("bat_sigscanner", "1", 0, "0 = Disables \n1 = Enable BAT to use a signature scanner to get access to functions valve/mods dont export");
ConVar g_VarServerRules("bat_serverrulesurl", "none", 0, "The url to the website that hosts the server rules(Must be a http link)");

ConVar g_VarInterface("bat_interface", "1", 0, "0 = Disables the AdminInterface ( The cross admin mod interface for other plugins )\n1 = Enables it");
ConVar g_VarInterfaceSM("bat_interfacesourcemod", "0", 0, "0 = Disabled\n1=Gets admin rights from sourcemod\n2=Sets admin rights in sourcemod");


ConVar g_VarLanguage("bat_language", "en",0,"What language BAT should use, bat_language list to see languages supported");
ConVar g_VarLanguageOptimize("bat_languageoptimize", "1",0,"0 = Disabled\n1 = Full optimize at map change, updates translation.dat\n2 = Only optimizes translation list in memory");

ConVar g_VarReservedSlotsCount("bat_reservedslotscount", "2", FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "The number of player slots reserved for admins ( Hidden from from the public)");
ConVar g_VarReservedSlotsSystem("bat_reservedslots", "2",FCVAR_SPONLY|FCVAR_REPLICATED|FCVAR_NOTIFY, "\n0 = Disabled\n1 = Constantly hides 2 slots\n2 = Keeps X slots for admins\n3 = Will allways keep 1 slot open, kicking a player with no reservedslots right when a admin joins");
ConVar g_VarReservedSlotsRedirect("bat_reservedslotsredirect", "0",0,"If players that are being kicked due to slot reservation should get a option to be redirected instead");
ConVar g_VarReservedSlotsRedirectAddress("bat_reservedslotsredirectaddress", "none",0,"The IP address the user should be redirected to");

// SQL Cvars
ConVar g_VarSQLEnabled("bat_mysql", "0",FCVAR_PROTECTED, "0 = Checking for users in MySQL is disabled\n1 = Enabled\n2 = Use AmxBans SQL database\n3 = Use sourcebans SQL database");
ConVar g_VarSQLUserName("bat_sqlusername", "none",FCVAR_PROTECTED, "The user name to login to your mysql server");
ConVar g_VarSQLPassword("bat_sqlpassword", "none",FCVAR_PROTECTED, "The password to login to the mysql server");
ConVar g_VarSQLDateBase("bat_sqldatabase", "none",FCVAR_PROTECTED, "What database to use on the SQL server");
ConVar g_VarSQLServerIP("bat_sqlserverip", "none",FCVAR_PROTECTED, "What the ip to the MySQL server is");
ConVar g_VarSQLAdminTable("bat_sqladmintable", "admin",FCVAR_PROTECTED, "Change if you want BAT to check another table for the admin info");
ConVar g_VarSQLLogTable("bat_sqllogtable", "log",FCVAR_PROTECTED, "Change if you want BAT to check another table for the log info");

ConVar g_VarSQLAmxBansServerTable("bat_sqlamxbansservertable", "none",FCVAR_PROTECTED, "The Amxbans server table name");

void BATVars::SetupCallBacks() // Yes i know we read all the cvars, when just 1 changed ( Or we can end up reading all 4 chars 4 times. But who cares, this should only happen on a mapchange and im lazy )
{
	// SQL related Vars
	g_VarSQLEnabled.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateSQLInfo);
	g_VarSQLUserName.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateSQLInfo);
	g_VarSQLPassword.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateSQLInfo);
	g_VarSQLDateBase.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateSQLInfo);
	g_VarSQLServerIP.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateSQLInfo);
	g_VarSQLAdminTable.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateSQLInfo);
	g_VarSQLLogTable.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateSQLInfo);
	g_VarSQLAmxBansServerTable.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateSQLInfo);

	g_VarAuthTime.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateAuthTime);

	g_VarPubMsgTime.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdatePubMsg);
	
	g_VarReservedSlotsCount.InstallChangeCallback((FnChangeCallback_t) &ReservedSlotsSystem::cbCvarUpdate);
	g_VarReservedSlotsSystem.InstallChangeCallback((FnChangeCallback_t) &ReservedSlotsSystem::cbCvarUpdate);
	g_VarReservedSlotsRedirectAddress.InstallChangeCallback((FnChangeCallback_t) &ReservedSlotsSystem::cbCvarUpdate);
	g_VarReservedSlotsRedirect.InstallChangeCallback((FnChangeCallback_t) &ReservedSlotsSystem::cbCvarUpdate);

	g_VarPublicVoteCmds.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdatePublicVoteCmds);
	g_VarSignatureScanner.InstallChangeCallback((FnChangeCallback_t) &Utils::CallBackUseSigScannerCvar);
	
	g_VarMapExtend.InstallChangeCallback((FnChangeCallback_t) &MapVote::CallbackMapVoteCvar);
	g_VarMapVotesRequired.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateVotingCvars);
	g_VarPlayerVotesRequired.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbUpdateVotingCvars);

	g_VarMenuType.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbBATMenuType);
	g_VarServerRules.InstallChangeCallback((FnChangeCallback_t) &BATVars::cbRulesCVAR);

	g_VarLanguage.InstallChangeCallback((FnChangeCallback_t) &Translation::UpdateCurLanguage);
}

void BATVars::cbUpdateVotingCvars()
{
	g_MapVotePercentageRequired = g_VarMapVotesRequired.GetInt();
	g_PlayerVotePercentageRequired = g_VarPlayerVotesRequired.GetInt();

	if(g_MapVotePercentageRequired <= 0 || g_MapVotePercentageRequired >= 100)
	{
		g_VarMapVotesRequired.SetValue(50);
	}
	if(g_PlayerVotePercentageRequired <= 0 || g_PlayerVotePercentageRequired >= 100)
	{
		g_VarPlayerVotesRequired.SetValue(60);
	}
}
void BATVars::cbUpdateAuthTime()
{
	int iTemp = g_VarAuthTime.GetInt();
	if(iTemp <= 0 || iTemp > 50)
	{
		g_VarAuthTime.SetValue(15); // Yes, BAT includes this nice DRM part here. Surly no hacker could ever break this fool proof system
	}
}
void BATVars::cbUpdatePubMsg()
{
	if(g_BATVars.GetPubMsgTime() > 0 && g_BATCore.GetBATVar().GetPubMsgMode() > 0)
	{
		g_PubMsgStatus = PM_ENABLED;
		g_NextPublicMsgTime = g_BATVars.GetPubMsgTime();
	}
	else 
		g_PubMsgStatus = PM_DISABLED;
}
void BATVars::cbUpdatePublicVoteCmds()
{
	g_PublicVoteCmdsMode = g_VarPublicVoteCmds.GetInt();
}
void BATVars::cbRulesCVAR()
{
	_snprintf(g_RulesURL,511,"%s",g_VarServerRules.GetString());
	if(stricmp(g_RulesURL,"none") == 0)
	{
		g_RulesURL[0] = '\0';
		g_RulesURLSet = false;
	}
	else
	{
		g_BATCore.StrReplace(g_RulesURL,"http://","",511);
		g_RulesURLSet = true;
	}
}
void BATVars::cbUpdateSQLInfo()
{
	g_BATVars.GetSQLInfo();
}
void BATVars::cbBATMenuType()
{
	int VarValue = g_VarMenuType.GetInt();
	if(VarValue <= 0 || VarValue >= 3)
	{
		g_BATCore.AddLogEntry("ERROR: Bad value of %s , only 1 or 2 is allowed",g_VarMenuType.GetString());
		VarValue = 2;
		g_VarMenuType.SetValue(VarValue);
		return;
	}

	g_MenuMngr.SetMenuType(VarValue);
}


bool BATVars::AllowNonAdminPlayersToSendAdminSay()
{
	return g_VarAdminNoneSay.GetBool();
}
int BATVars::GetVoteBanTime()
{
	int TempInt = g_VarVoteBanTime.GetInt();
	if(TempInt < 0)
		return 0;

	return TempInt;
}
void BATVars::GetSQLInfo() // Lets hope nobody sees this, as we read all the cvars if only 1 was updated :) 
{
	if(g_VarSQLEnabled.GetInt() == 0)
	{
		g_SQLStatus.Enabled = false;
		return;
	}

	switch(g_VarSQLEnabled.GetInt())
	{
	case 1:
		g_SQLSettings.DataBaseType = BAT_V2;
		break;

	case 2:
		g_SQLSettings.DataBaseType = AmxBans;
		break;

	case 3:
		g_SQLSettings.DataBaseType = SourceBans;
	    break;
	}		

	g_SQLStatus.Enabled = true;
	g_SQLSettings.AmxBansServerID = -1;
	g_AmxBansAddServerTries = 0;

	_snprintf(g_SQLSettings.Password,127,"%s",g_VarSQLPassword.GetString());
	_snprintf(g_SQLSettings.ServerIP,127,"%s",g_VarSQLServerIP.GetString());
	_snprintf(g_SQLSettings.UserName,127,"%s",g_VarSQLUserName.GetString());
	_snprintf(g_SQLSettings.DataBase,127,"%s",g_VarSQLDateBase.GetString());
	_snprintf(g_SQLSettings.AdminTable,127,"%s",g_VarSQLAdminTable.GetString());
	_snprintf(g_SQLSettings.LogTable,127,"%s",g_VarSQLLogTable.GetString());
	_snprintf(g_SQLSettings.AmxBansServerTable,127,"%s",g_VarSQLAmxBansServerTable.GetString());
}
const char *BATVars::GetServerRules()
{
	return g_VarServerRules.GetString();
}
int BATVars::GetAdminInterfaceEnabled()
{
	return g_VarInterface.GetInt();
}
const char *BATVars::GetBATLanguage()
{
	return g_VarLanguage.GetString();
}
int BATVars::GetMenuType()
{
	return g_VarMenuType.GetInt();
}
int BATVars::GetMapRockTheVote()
{
	return g_VarMapRockTheVote.GetInt();
}
int BATVars::GetPubMsgMode()
{
	return g_VarPubMsgMode.GetInt();
}
int BATVars::GetAuthTime()
{
	return g_VarAuthTime.GetInt();
}
int BATVars::GetRoundLimit()
{
	if(!g_pRoundLimit)
		g_pRoundLimit = GetCvarPointer("mp_roundlimit");

	return g_pRoundLimit->GetInt();
}
const char* BATVars::GetHostName()
{
	if(!g_pHostName)
		g_pHostName = GetCvarPointer("hostname");

	return g_pHostName->GetString();
}
int BATVars::GetWinLimit()
{
	if(!g_pWinLimit)
		g_pWinLimit = GetCvarPointer("mp_winlimit");

	return g_pWinLimit->GetInt();
}
float BATVars::GetMapChangeDelay()
{
	return g_pChatTime->GetFloat() - 1.5;
}
int BATVars::GetMapExtendOption()
{
	return g_VarMapExtend.GetInt();
}
bool BATVars::GetUseSigScanner()
{
	if(g_VarSignatureScanner.GetInt() == 1)
		return true;

	return false;
}
int BATVars::GetMapExtendTimes()
{
	return g_VarMapExtendTimes.GetInt();
}
int BATVars::GetReservedSlotCount()
{
	return g_VarReservedSlotsCount.GetInt();
}
int BATVars::GetReservedSlots()
{
	return g_VarReservedSlotsSystem.GetInt();
}
float BATVars::GetPubMsgTime()
{
	return (g_VarPubMsgTime.GetFloat() * 60.0);
}

bool BATVars::ReloadSettingsOnMapChange()
{
	if(g_VarSettingsReload.GetInt() == 1)
		return true;
	
	return false;
}
int BATVars::GetCvarInt(char *CvarName)
{
	ConVar *Var;
	ConCommandBase *pCmd = g_pCVar->FindVar(CvarName);
	if (pCmd && strcmpi(pCmd->GetName(),CvarName) == 0)
	{
		Var = (ConVar *)pCmd;
		return Var->GetInt();
	}
	g_BATCore.AddLogEntry("ERROR: Could not find %s cvar",CvarName);
	return -1;
}
ConVar * BATVars::GetCvarPointer(char *CvarName)
{

	//ConVar *Var;
	ConCommandBase *pCmd = g_pCVar->FindVar(CvarName);
	if (pCmd && strcmpi(pCmd->GetName(),CvarName) == 0)
	{
		return (ConVar *)pCmd;
	}
	g_BATCore.AddLogEntry("ERROR: Could not find %s cvar",CvarName);
	return NULL;
}
const char* BATVars::GetCvarString(char *CvarName)
{
	ConCommandBase *pCmd = g_pCVar->FindVar(CvarName);
	if (pCmd && strcmpi(pCmd->GetName(),CvarName) == 0 && !pCmd->IsCommand())
	{
		ConVar *Var = (ConVar *)pCmd;
		return Var->GetString();
	}
	g_BATCore.AddLogEntry("ERROR: Could not find %s cvar",CvarName);
	return " ";
}


bool BATVars::HookCvars()
{
	ConCommandBase *pCmd = g_pCVar->GetCommands();
	while (pCmd)
	{
		if (!(pCmd->IsCommand()) && strcmp(pCmd->GetName(),"mp_timelimit") == 0) 
		{
			g_pTimeLimit = (ConVar *)pCmd;	
		}
		else if (!(pCmd->IsCommand()) && strcmp(pCmd->GetName(),"hostname") == 0) 
		{
			g_pHostName = (ConVar *)pCmd;			
		}
		else if (!(pCmd->IsCommand()) && strcmp(pCmd->GetName(),"mp_chattime") == 0) 
		{
			g_pChatTime = (ConVar *)pCmd;			
		}
		else if (!(pCmd->IsCommand()) && strcmp(pCmd->GetName(),"mp_winlimit") == 0) 
		{
			g_pWinLimit = (ConVar *)pCmd;			
		}

		if(g_pHostName && g_pTimeLimit && g_pChatTime && g_pWinLimit) // We found all the cvars.
		{
			break;
		}

		pCmd = const_cast<ConCommandBase *>(pCmd->GetNext());
	}
	if(g_ModSettings.GameMod == MOD_INSURGENCY && !g_pChatTime)
		g_pChatTime = g_BATVars.GetCvarPointer("ins_endgametime"); // Why anyone would remove a standard cvar and rename it to something else is retarded.
	if(g_ModSettings.GameMod == MOD_INSURGENCY && !g_pTimeLimit)
		g_pTimeLimit = g_BATVars.GetCvarPointer("ins_timelimit"); // Why anyone would remove a standard cvar and rename it to something else is retarded.

	if(!g_pWinLimit && g_ModSettings.SupportsWinlimit)
	{
		g_BATCore.AddLogEntry("ERROR: Hooking mp_winlimit FAILED! ( Crashing could be coming very soon )");
		g_BATCore.WriteLogBuffer();
		return false;
	}
	if(!g_pTimeLimit || !g_pHostName || !g_pChatTime)
	{
		g_BATCore.AddLogEntry("ERROR: Hooking mp_timelimit,hostname or mp_chattime FAILED! ( Crashing could be coming very soon )");
		g_BATCore.WriteLogBuffer();
		return false;
	}
	return true;
}
