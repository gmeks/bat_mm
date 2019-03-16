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
#include "ReservedSlotsSystem.h"
#include "const.h"

int g_ReservedSlotsUsers=0; 
int g_PlayerCount=0;

int g_ReservedSlotsCount = 0;
int g_ReservedSlotsSystem = 0;
bool g_ReservedSlotsUseRedirect=false;
char g_ReservedSlotsIPAddress[128];
bool g_BlockIDCheck;

ConVar *g_pBotQuota;
ConVar *g_pSourceTVName;

#define DEBUG_RESEREVEDSLOTS 0

ReservedSlotsSystem::ReservedSlotsSystem()
{
	g_VisibleMaxPlayers = g_BATCore.GetBATVar().GetCvarPointer("sv_visiblemaxplayers");
	g_pSourceTVName = g_BATCore.GetBATVar().GetCvarPointer("tv_name");
}

bool ReservedSlotsSystem::HandleReservedSlotsRequest(int id)
{
	if(g_ReservedSlotsSystem == 0 || g_ReservedSlotsSystem >= 4)
		return true;

	if(!AllowServerJoin(id))
	{
		KickOrRedirectPlayer(id);
		return false;
	}
#if DEBUG_RESEREVEDSLOTS == 1
	else
		g_BATCore.AddLogEntry("Reserved slots debug: %s was allowed to connect: Reserved slots access: %d",g_BATCore.GetPlayerName(id),g_BATCore.HasAccess(id,ADMIN_RESERVEDSLOTS));
#endif

	if(g_ReservedSlotsSystem == 2)
		UpdateVisibleSlots();
	return true;
}

bool ReservedSlotsSystem::AllowServerJoin(int id)
{
	int MaxPublicFreeSlots = GetFreeNoneAdminSlots();
	if(MaxPublicFreeSlots == -1)
	{
		g_BATCore.AddLogEntry("Your reserved slots system has a incorrect cvar value");
		return true;
	}
	bool HasResevedSlotsAccess = g_BATCore.HasAccess(id,ADMIN_RESERVEDSLOTS);

	switch(g_ReservedSlotsSystem)
	{
	case 1:
		if(HasResevedSlotsAccess)
			return true;
		
		if(MaxPublicFreeSlots <= 0)
			return false;

		return true;

	case 2:
		if(HasResevedSlotsAccess)
			return true;

		if(MaxPublicFreeSlots <= 0)
			return false;
		
		return true;

	case 3:
		if(!HasResevedSlotsAccess) // Its not a admin, we check if there are any free slots for him
		{
			if(MaxPublicFreeSlots <= 0)
				return false;

			return true;
		}
		// Its a admin, we now need to check if a none admin needs to be kicked.
		if(MaxPublicFreeSlots >= 1)
		{
#if DEBUG_RESEREVEDSLOTS == 1
			g_BATCore.AddLogEntry("Reserved slots debug: admin named %s connected, no need to kick random none admin. %d",g_BATCore.GetPlayerName(id),MaxPublicFreeSlots);
#endif
			return true;
		}

		int PlayerToKick = GetPlayerToKick();
		if(PlayerToKick <= 0)
		{
#if DEBUG_RESEREVEDSLOTS == 1
			g_BATCore.AddLogEntry("Reserved slots debug: Tried to find player to kick, but failed Players %d/%d",g_PlayerCount,MaxPublicFreeSlots);
#endif
			return true;
		}
		// We now either kick or redirect the player that needs to leave.
		KickOrRedirectPlayer(PlayerToKick);
		return true;
	}
	return true;
}
void ReservedSlotsSystem::KickOrRedirectPlayer(int id)
{
	g_BlockIDCheck = true;

	if(g_UserInfo[id].IsBot && g_ModSettings.GameMod == MOD_CSTRIKE)
	{
		g_BATCore.ConsolePrint(0,"[BAT] A Bot was kicked due to slot reservation");

		if(!g_pBotQuota) g_pBotQuota = g_BATCore.GetBATVar().GetCvarPointer("bot_quota");

		int OldValue = g_pBotQuota->GetInt() - 1;
		g_pBotQuota->SetValue(OldValue);

#if DEBUG_RESEREVEDSLOTS == 1
		g_BATCore.AddLogEntry("Reserved slots debug: Lowing bot_quota to %d",OldValue);
#endif
		return;
	}

	if(g_ReservedSlotsUseRedirect)
	{
		g_MenuMngr.ShowClientReConnectDialog(id);
		g_BATCore.GetTaskSystem()->CreateTask(ReservedSlotsSystem::cbTaskKickPlayer,MAX_REDIRECTTIME + 1,id);
		g_BATCore.MessagePlayer(id,"[BAT] Due to slot reservation you will be kicked in %.0f seconds, or you can redirect to the other server (Default F3)",MAX_REDIRECTTIME);

#if DEBUG_RESEREVEDSLOTS == 1
		g_BATCore.AddLogEntry("Reserved slots debug: Offered %s to redirect will be kicked in %.0f seconds",g_BATCore.GetPlayerName(id),MAX_REDIRECTTIME);
#endif
	}
	else
	{
#if DEBUG_RESEREVEDSLOTS == 1
		g_BATCore.AddLogEntry("Reserved slots debug: Kicked %s to make room for a admin",g_BATCore.GetPlayerName(id));
#endif

		g_BATCore.ConsolePrint(0,"[BAT] %s was kicked due to slot reservation",g_BATCore.GetPlayerName(id));
		g_BATCore.ServerCommand("kickid %d Kicked due to slot reservation",g_UserInfo[id].Userid);
	}
}
int ReservedSlotsSystem::GetFreeNoneAdminSlots()
{
	int FreeSlots = 0;

	switch(g_ReservedSlotsSystem)
	{
	case 1:
		FreeSlots =  g_MaxClients - (g_ReservedSlotsCount + g_PlayerCount);

#if DEBUG_RESEREVEDSLOTS == 1
		g_BATCore.AddLogEntry("Reserved slots debug: FreeSlots %d",FreeSlots);
#endif
		return FreeSlots;
	case 2:
		FreeSlots = g_MaxClients - g_ReservedSlotsCount + g_ReservedSlotsUsers;

#if DEBUG_RESEREVEDSLOTS == 1
		g_BATCore.AddLogEntry("Reserved slots debug: FreeSlots %d",FreeSlots);
#endif
		return FreeSlots;
	case 3:
		FreeSlots = g_MaxClients - g_PlayerCount;

#if DEBUG_RESEREVEDSLOTS == 1
		g_BATCore.AddLogEntry("Reserved slots debug: FreeSlots %d - %d/%d",FreeSlots,g_PlayerCount,g_MaxClients);
#endif
		return FreeSlots;
	}

	return -100;
}
int ReservedSlotsSystem::GetPlayerToKick()
{
	float ShortestConnectionTime = GetGlobals()->curtime + 5;
	int ShortestConnectionInt=-1;

	for(int i=1;i<g_MaxClients;i++) 
	{
		if(g_IsConnected[i] && !g_BATCore.HasAccess(i,ADMIN_RESERVEDSLOTS) && g_UserInfo[i].ConnectTime < ShortestConnectionTime || g_IsConnected[i] && g_UserInfo[i].IsBot )
		{ 
			if(g_UserInfo[i].IsBot && strcmp(g_BATCore.GetPlayerName(i),g_pSourceTVName->GetString()) == 0)
				continue;

			ShortestConnectionTime = g_UserInfo[i].ConnectTime;
			ShortestConnectionInt = i;

			if(g_UserInfo[i].IsBot) 
				return i; // We found a BOT we kick that instead of any player
		}
	}
#if DEBUG_RESEREVEDSLOTS == 1
	g_BATCore.AddLogEntry("Reserved slots debug: Planning to kick %s (IsBot %d) connection time %.0f",g_BATCore.GetPlayerName(ShortestConnectionInt),g_UserInfo[ShortestConnectionInt].IsBot,ShortestConnectionTime);
#endif
	return ShortestConnectionInt;
}

void ReservedSlotsSystem::UpdateVisibleSlots()
{
#if DEBUG_RESEREVEDSLOTS == 1
	g_BATCore.AddLogEntry("Reserved slots debug: Updated sv_VisibleMaxPlayers settings");
#endif

	if(g_ReservedSlotsSystem == 0)
		g_VisibleMaxPlayers->SetValue(-1);

	else if(g_ReservedSlotsSystem == 1)
		g_VisibleMaxPlayers->SetValue(g_MaxClients-g_ReservedSlotsCount);

	else if(g_ReservedSlotsSystem == 2)
	{
		int ReservedSlots = g_ReservedSlotsCount - g_ReservedSlotsUsers;
		if(ReservedSlots <= 0 )	
            g_VisibleMaxPlayers->SetValue(g_MaxClients);
		else
			g_VisibleMaxPlayers->SetValue(g_MaxClients-ReservedSlots);
	}
	else if(g_ReservedSlotsSystem == 3)
	{
		g_VisibleMaxPlayers->SetValue(g_MaxClients-1);
	}	
}
void ReservedSlotsSystem::cbTaskKickPlayer(int id)
{
	g_BATCore.MessagePlayer(0,"[BAT] %s was kicked due to slot reservation",g_BATCore.GetPlayerName(id));
	g_BATCore.ServerCommand("kickid %d Kicked due to slot reservation",g_UserInfo[id].Userid);
	g_BlockIDCheck = true;

#if DEBUG_RESEREVEDSLOTS == 1
	g_BATCore.AddLogEntry("Reserved slots debug: Kicked %s as he failed to choose to redirect",g_BATCore.GetPlayerName(id));
#endif
}

void ReservedSlotsSystem::cbCvarUpdate()
{	
	g_ReservedSlotsSystem = g_BATVars.GetReservedSlots();
	g_ReservedSlotsCount = g_BATVars.GetReservedSlotCount();
	g_ReservedSlotsUseRedirect = g_BATVars.GetReservedSlotsDoRedirect();
	_snprintf(g_ReservedSlotsIPAddress,127,"%s",g_BATVars.GetReservedSlotsRedirectAddress());	

	if(g_ReservedSlotsSystem < 0 && g_ReservedSlotsSystem > 3)
	{
		g_BATCore.AddLogEntry("ERROR! bat_reservedslots can only have values between 0 and 3");
		g_BATCore.ServerCommand("bat_reservedslots 0");
		return;
	}

	if(g_ReservedSlotsSystem != 0 && g_ReservedSlotsSystem != 3 && g_ReservedSlotsCount < 0 && g_ReservedSlotsCount > g_MaxClients )
	{
		g_BATCore.AddLogEntry("ERROR! bat_reservedslotscount can only have values between 0 and max players");
		g_BATCore.ServerCommand("bat_reservedslots 0");
	}

	if(g_ReservedSlotsSystem != 0 && g_ReservedSlotsUseRedirect && ( strlen(g_ReservedSlotsIPAddress) == 0 || stricmp(g_ReservedSlotsIPAddress,"none") == 0 ))
	{
		g_BATCore.AddLogEntry("ERROR! bat_reservedslotsredirectaddress needs to be set if you want to redirect players");
		g_BATCore.ServerCommand("bat_reservedslotsredirect 0");
	}
 	g_BATCore.GetReservedSlotsSystem()->UpdateVisibleSlots();

#if DEBUG_RESEREVEDSLOTS == 1
	g_BATCore.AddLogEntry("Reserved slots debug: Updated cvar settings");
#endif
}