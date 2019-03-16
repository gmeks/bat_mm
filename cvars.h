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

#ifndef _INCLUDE_CVARS_H
#define _INCLUDE_CVARS_H

#include "hl2sdk/convar.h"
#include "icvar.h"

extern ConVar g_VarMapVotesRequired;
extern ConVar g_VarPlayerVotesRequired;
extern ConVar g_VarReservedSlotsRedirectAddress;
extern ConVar g_VarReservedSlotsRedirect;
extern ConVar g_VarPubTimeLeft;
extern ConVar g_VarHideAdminAction;

extern ConVar g_VarInterfaceSM;

// SQL Cvars
extern ConVar g_VarSQLEnabled;
extern ConVar g_VarSQLUserName;
extern ConVar g_VarSQLPassword;
extern ConVar g_VarSQLDateBase;
extern ConVar g_VarSQLServerIP;
extern ConVar g_VarSQLAdminTable;
extern ConVar g_VarSQLLogTable;
extern ConVar g_VarSQLAmxBansServerTable;

class BATVars 
{
public:
	//void	SetICvar(ICvar *pICvar) {g_pCVar = pICvar; }
	//ICvar  *GetICVar() {return g_pCVar; }
	
	int GetMapVotesRequired() { return g_VarMapVotesRequired.GetInt(); }
	int GetPlayerVotesRequired() { return g_VarPlayerVotesRequired.GetInt(); }

	int GetHideAdminName() { return g_VarHideAdminAction.GetInt(); }

	const char *GetReservedSlotsRedirectAddress() { return g_VarReservedSlotsRedirectAddress.GetString(); }
	bool GetReservedSlotsDoRedirect() { return g_VarReservedSlotsRedirect.GetBool(); }
	bool PubTimeleftEnabled() { return g_VarPubTimeLeft.GetBool(); }
	int GetSMInterface() { return g_VarInterfaceSM.GetInt(); }

	int GetMapExtendOption();
	bool ReloadSettingsOnMapChange();
	bool AllowNonAdminPlayersToSendAdminSay();
	bool GetUseSigScanner();
	int GetMapExtendTimes();
	int GetReservedSlotCount();
	int GetReservedSlots();
	int GetAuthTime();
	int GetPubMsgMode();
	int GetMapRockTheVote();
	int GetMenuType();
	int GetAdminInterfaceEnabled();
	int GetVoteBanTime();
	int GetWinLimit();
	int GetRoundLimit();
	float GetPubMsgTime();
	float GetMapChangeDelay();
	void GetSQLInfo();
	const char *GetHostName();
	const char *GetServerRules();
	const char *GetBATLanguage();

	ConVar* GetTimelimitCvar() {return g_pTimeLimit;}
	ConVar* GetWinlimitCvar() {return g_pWinLimit;}
	ConVar* SetNextMap() {return g_pNextMap;}

	void SetupCallBacks();

	const char* GetCvarString(char *CvarName);
	int			GetCvarInt(char *CvarName);
	ConVar *	GetCvarPointer(char *CvarName);
	
	bool HookCvars();


private:
	static void cbUpdateSQLInfo();
	static void cbUpdateAuthTime();
	static void cbUpdatePubMsg();
	static void cbUpdatePublicVoteCmds();
	static void cbBATMenuType();
	static void cbRulesCVAR();
	static void cbUpdateVotingCvars();


	ConVar *g_pTimeLimit;
	ConVar *g_pWinLimit;
	ConVar *g_pHostName;
	ConVar *g_pChatTime;
	ConVar *g_pRoundLimit;
	ConVar *g_pNextMap; // Nextmap for INs or any other map
};

extern BATVars g_BATVars;
extern char g_RulesURL[512];
extern bool g_RulesURLSet;

#endif //_INCLUDE_CVARS_H
