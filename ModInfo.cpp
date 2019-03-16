/* ======== Basic Admin tool ========
* Copyright (C) 2004-2007 Erling K. Sæterdal
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Erling K. Sæterdal ( EKS )
* Credits:
*	Helping on misc errors/functions: BAILOPAN,karma,LDuke,sslice,devicenull,PMOnoTo,cybermind ( most who idle in #sourcemod on GameSurge realy )
* ============================ */

#include "ModInfo.h"
#include "const.h"
#include "ismmapi.h"
#include "BATCore.h"

ModSettingsStruct g_ModSettings;
IServerGameDLL *g_ServerDll;
IVEngineServer *g_Engine;
extern ISmmAPI *g_SMAPI;

void ModInfo::GetModInformation(IServerGameDLL *pServerDll,IVEngineServer *pEngine)
{
	g_ServerDll = pServerDll;
	g_Engine = pEngine;

	GetModName();
	GetMsgNumberInformation();

	// We now add the mods we know support radio menus
	if(g_ModSettings.GameMod == MOD_CSTRIKE || g_ModSettings.GameMod == MOD_DOD || g_ModSettings.GameMod == MOD_HL2CTF || g_ModSettings.GameMod == MOD_SRCFORTS || 
	   g_ModSettings.GameMod == MOD_DYSTOPIA || g_ModSettings.GameMod == MOD_INSURGENCY || g_ModSettings.GameMod == MOD_ETERNALSILENCE || g_ModSettings.GameMod == MOD_TF2 || g_ModSettings.GameMod == MOD_EMPIRES)
		g_ModSettings.DefaultRadioMenuSupport = true;
	else
		g_ModSettings.DefaultRadioMenuSupport = false;

	// We now check if the mod supports win limit
	if(g_ModSettings.GameMod == MOD_CSTRIKE || g_ModSettings.GameMod == MOD_DOD)
		g_ModSettings.SupportsWinlimit = true;
	else
		g_ModSettings.SupportsWinlimit = false;	

	if(g_ModSettings.GameMod == MOD_CSTRIKE || g_ModSettings.GameMod == MOD_DOD || g_ModSettings.GameMod == MOD_EMPIRES)
		g_ModSettings.ValveCenterSay = true;
	else
		g_ModSettings.ValveCenterSay = false;	

	g_ServerDll = NULL;
	g_Engine = NULL;

	//PluginCore.PluginSpesficLogPrint("BAT Debug: Modname %s (%d)",g_ModSettings.GameModName,g_ModSettings.GameMod);
	
	if(g_ModSettings.GameMod == MOD_UNKNOWN)
		PluginCore.PluginSpesficLogPrint("Warning: The mod ( %s )you have loaded BAT on does hot have hardcoded support, this could create bugs with things like mapvote",g_ModSettings.GameModName);
}

void ModInfo::GetMsgNumberInformation()	// Gets the menu message id, for the running mod
{
	g_ModSettings.HintText = GetMsgNum("HintText");
	g_ModSettings.HudMsg = GetMsgNum("HudMsg");
	g_ModSettings.MenuMsg = GetMsgNum("ShowMenu");
	g_ModSettings.TextMsg = GetMsgNum("TextMsg");
	g_ModSettings.SayText = GetMsgNum("SayText");
	g_ModSettings.VGUIMenu = GetMsgNum("VGUIMenu");	
}

void ModInfo::GetModName()		// This function gets the actual mod name, and not the full pathname
{
	char Path[256];
	g_Engine->GetGameDir(Path,255);	

	const char *ModName = Path;
	for (const char *iter = Path; *iter; ++iter)
	{
		if (*iter == '/' || *iter == '\\') // or something
			ModName = iter + 1;
	}
	_snprintf(g_ModSettings.GameModName,31,"%s",ModName);

	if(strcmp(ModName,"cstrike") == 0)
		g_ModSettings.GameMod = MOD_CSTRIKE;
	else if(strcmp(ModName,"dod") == 0 || stricmp(ModName,"DODC") == 0)
		g_ModSettings.GameMod = MOD_DOD;
	else if(strcmp(ModName,"hl2mp") == 0)
		g_ModSettings.GameMod = MOD_HL2MP;
	else if(strcmp(ModName,"tf") == 0)
		g_ModSettings.GameMod = MOD_TF2;
	else if(strcmp(ModName,"hl2ctf") == 0)
		g_ModSettings.GameMod = MOD_HL2CTF;
	else if(strcmp(ModName,"esmod") == 0)
		g_ModSettings.GameMod = MOD_ETERNALSILENCE;
	else if(strcmp(ModName,"dystopia_v1") == 0)
		g_ModSettings.GameMod = MOD_DYSTOPIA;
	else if(strcmp(ModName,"sourceforts") == 0)
		g_ModSettings.GameMod = MOD_SRCFORTS;
	else if(strcmp(ModName,"empires") == 0 || strcmp(ModName,"Empires") == 0)
		g_ModSettings.GameMod = MOD_EMPIRES;
	else if(strcmp(ModName,"pvkii") == 0)
		g_ModSettings.GameMod = MOD_PVKII;
	else if(stricmp(ModName,"Insurgency") == 0)
		g_ModSettings.GameMod = MOD_INSURGENCY;
	else if(stricmp(ModName,"Firearms2") == 0)
		g_ModSettings.GameMod = MOD_FIREARMS2;
	else 
		g_ModSettings.GameMod = MOD_UNKNOWN;

}
int ModInfo::GetMsgNum(const char *MsgName)
{
	char TempMsgName[40];
	int dontdoit=0;
	int MaxScan= g_SMAPI->GetUserMessageCount();

	for (int i=0;i<MaxScan;i++) 
	{
		g_ServerDll->GetUserMessageInfo(i,TempMsgName,39,dontdoit);
		if(strcmp(MsgName,TempMsgName) == 0)
			return i;
	}
	PluginCore.PluginSpesficLogPrint("ERROR: Failed to find the message number for %s, this could create all sorts of errors,crashes and bugs",MsgName);
	return 0;
}