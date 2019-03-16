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
#include <eiface.h>
#include <time.h>
#include "BATCore.h"
#include "BATMenu.h"
#include "hl2sdk/keyvalues.h"
#include "Translation.h"
#include "AdminCommands.h"
#include "cvars.h"
#include "ReservedSlotsSystem.h"
#include <bitbuf.h>
#include "hl2sdk/recipientfilters.h"
#include "networkvar.h"
#include "MenuRconCmds.h"

extern int g_MaxClients;
extern int g_PlayerCount;
extern unsigned int g_AdminCmdCount;

extern bool g_IsConnected[MAXPLAYERS+1];
extern bool g_IsConnecting[MAXPLAYERS+1];
extern int g_MenuPos[MAXPLAYERS+1];
extern int g_AdminMenu;

extern char g_NextMapName[MAX_MAPNAMELEN+1];
extern float g_ChangeMapTime;

extern float g_fMapChangeTime;
extern bool g_bChangeMapCountDown;

extern AdminCommandStruct g_AdminCommands[MAX_ADMINCOMMANDS+1];
extern VoteInfoStruct VoteInfo;
extern ModSettingsStruct g_ModSettings;
extern SourceHook::CVector<ArgListStruct *>g_ArgList;
extern BATMenuMngr g_MenuMngr;
extern Translation *m_Translation;
extern MapVote *m_MapVote;
extern ReservedSlotsSystem *m_ReservedSlots;

extern SourceHook::CVector<RconMenuCmdsStruct *>g_MenuRconCmds;
extern int unsigned g_MenuRconCmdsCount;

extern SourceHook::CVector<MapStuct *>g_MapList;
extern int g_MapCount;

unsigned int g_ArgCount = 0;

void AdminCmd::CmdNextMap(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);
	int MapIndex = g_BATCore.GetBATMaps()->GetMapIndex(arg1);
	if(!g_BATCore.GetEngine()->IsMapValid(arg1) || MapIndex == -1)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("NotValidMap"),arg1);
		return;
	}
	SetNextmap(admin_id,arg1);
}
void AdminCmd::CmdAddAdmin(int admin_id)
{
	//admin_addadmin "STEAM_ID_FAKE" "abcde" "EKSFAKE"

	char *AdminLogin = GetArg(1);	
	char *AdminFlags = GetArg(2);	
	char *AdminName = GetArg(3);

	if(!AdminName || !AdminLogin || !AdminFlags)
		return;

	AdminLogin = g_BATCore.StrRemoveQuotes(AdminLogin);
	AdminFlags = g_BATCore.StrRemoveQuotes(AdminFlags);
	AdminName = g_BATCore.StrRemoveQuotes(AdminName);

	if(stricmp("STEAM_ID_LAN",AdminLogin) == 0 || stricmp("STEAM_ID_PENDING",AdminLogin) == 0)
		return;
	
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
		g_BATCore.MessagePlayer(0,"%s<%s> added the following admin account %s,%s,%s",aName,g_UserInfo[admin_id].Steamid,AdminLogin,AdminFlags,AdminName);
	else
	{
		g_BATCore.MessagePlayer(0,"A admin added the following admin account %s,%s,%s",AdminLogin,AdminFlags,AdminName);
		g_BATCore.MessageAdmins("%s<%s> added the following admin account %s,%s,%s",aName,g_UserInfo[admin_id].Steamid,AdminLogin,AdminFlags,AdminName);
	}

	g_BATCore.AddLogEntry("A admin has added the following admin account %s,%s,%s",AdminLogin,AdminFlags,AdminName);
	
	g_BATCore.GetAdminLoader()->AddAdminAccount(AdminLogin,AdminFlags,AdminName);
	g_BATCore.GetAdminLoader()->WriteAdminFile();

	// We now try to find the user, if he is on the server we make him login fast :)
	int Target_id = g_BATCore.FindPlayer(AdminLogin);
	if(Target_id != -1)
		g_BATCore.SetupPlayerInfo(Target_id);
}
void AdminCmd::CmdSlay(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int Target_id = g_BATCore.FindPlayer(arg1);

	if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}
	else if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}

	Slap(Target_id,admin_id,Slap_SlayUser);
}
void AdminCmd::CmdGag(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int Target_id = g_BATCore.FindPlayer(arg1);

	if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}
	else if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}

	Gag(Target_id,admin_id);
}
void AdminCmd::CmdUnGag(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int Target_id = g_BATCore.FindPlayer(arg1);

	if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}
	else if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}

	UnGag(Target_id,admin_id);
}
void AdminCmd::CmdPrivateSay(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);
	char *TheMsg = GetReason(2);

	int Target_id = g_BATCore.FindPlayer(arg1);

	if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}
	else if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}

	g_BATCore.MessagePlayer(Target_id,m_Translation->GetTranslation("CmdpSayTheMsg"),g_BATCore.GetPlayerName(admin_id),TheMsg);
}
int g_LastMoveType[MAXPLAYERS+1];

void AdminCmd::CmdSetMoveType(int admin_id)
{
	if(g_BATCore.GetUtils()->UTIL_GetMoveType(g_UserInfo[admin_id].PlayerEdict) != MOVETYPE_NOCLIP)
	{
		g_LastMoveType[admin_id] = g_BATCore.GetUtils()->UTIL_GetMoveType(g_UserInfo[admin_id].PlayerEdict);
		g_BATCore.GetUtils()->UTIL_SetMoveType(g_UserInfo[admin_id].PlayerEdict,MOVETYPE_NOCLIP);
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("CmdMoveTypeOn"),g_BATCore.GetPlayerName(admin_id));
	}
	else
	{
		g_BATCore.GetUtils()->UTIL_SetMoveType(g_UserInfo[admin_id].PlayerEdict,g_LastMoveType[admin_id]);
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("CmdMoveTypeOff"),g_BATCore.GetPlayerName(admin_id));
	}
}
void AdminCmd::CmdVote(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);
	if(g_ArgCount > 1)
	{
		char *arg2 = GetArg(2);
		arg2 = g_BATCore.StrRemoveQuotes(arg2);
		m_MapVote->StartCmdCustomVote(arg1,arg2);
		g_BATCore.AddLogEntry("%s<%s> has started a vote about '%s' that executes '%s' in server console if successfully",g_BATCore.GetPlayerName(admin_id),g_UserInfo[admin_id].Steamid,arg1,arg2);
	}
	else
	{
		m_MapVote->StartCmdCustomVote(arg1);
		g_BATCore.AddLogEntry("%s<%s> has started a vote about '%s'",g_BATCore.GetPlayerName(admin_id),g_UserInfo[admin_id].Steamid,arg1);
	}	
}
void AdminCmd::CmdEjectComm(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int TargetTeam=0;

	if(strcmp(arg1,"nf") == 0)
	{
		TargetTeam = 2;
	}
	else if(strcmp(arg1,"be") == 0)
	{		
		TargetTeam = 3;
	}

	if(TargetTeam == 0)
	{
		int Target_id = g_BATCore.FindPlayer(arg1);
		if(Target_id > 0)
		{
			IPlayerInfo *pPlayerInfo = g_BATCore.GetPlayerInfo()->GetPlayerInfo(g_UserInfo[Target_id].PlayerEdict);
			if(!pPlayerInfo)
			{
				g_BATCore.AddLogEntry("ERROR! There was a error trying to get IPlayerInfo when trying to kick a user from comm veh");
				return;
			}
			TargetTeam = pPlayerInfo->GetTeamIndex();
		}
		else if (TargetTeam == 0 && (Target_id == -1  || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN))) 
		{
			g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("EmpiresCannotFindTarget"),arg1);
			return;
		}
		else if(Target_id == -2)
		{
			g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
			return;
		}
	}

	if(TargetTeam == 2)
	{
		g_BATCore.ServerCommand("emp_sv_kick_commander_nf");
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("EmpiresEjectCommNF"),g_BATCore.GetPlayerName(admin_id));
		g_BATCore.AddLogEntry("%s<%s> has removed the Northern Faction commander, from the command vehicle",g_BATCore.GetPlayerName(admin_id),g_UserInfo[admin_id].Steamid);
	}
	else if(TargetTeam == 3)
	{
		g_BATCore.ServerCommand("emp_sv_kick_commander_be");
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("EmpiresEjectCommBE"),g_BATCore.GetPlayerName(admin_id));
		g_BATCore.AddLogEntry("%s<%s> has removed the Brenodi Empire commander, from the command vehicle",g_BATCore.GetPlayerName(admin_id),g_UserInfo[admin_id].Steamid);
	}
	else
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,"[BAT] The target team is wrong %d (team name you supplied is '%s' valid are: nf/be)",TargetTeam,arg1);
}
void AdminCmd::CmdVoteBan(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int Target_id = g_BATCore.FindPlayer(arg1);

	if(Target_id == admin_id)
		return;

	if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}
	else if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}


	g_BATCore.MessageAdmins("%s<%s> has started a voteban against %s<%s>",g_BATCore.GetPlayerName(admin_id),g_UserInfo[admin_id].Steamid,g_BATCore.GetPlayerName(Target_id),g_UserInfo[Target_id].Steamid);
	g_BATCore.AddLogEntry("%s<%s> has started a voteban against %s<%s>",g_BATCore.GetPlayerName(admin_id),g_UserInfo[admin_id].Steamid,g_BATCore.GetPlayerName(Target_id),g_UserInfo[Target_id].Steamid);
	g_BATCore.GetMapVote()->StartVoteBan(Target_id);
}
void AdminCmd::CmdVoteKick(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int Target_id = g_BATCore.FindPlayer(arg1);
	
	if(Target_id == admin_id)
		return;

	if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}
	else if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}
	
	g_BATCore.MessageAdmins("%s<%s> has started a votekick against %s<%s>",g_BATCore.GetPlayerName(admin_id),g_UserInfo[admin_id].Steamid,g_BATCore.GetPlayerName(Target_id),g_UserInfo[Target_id].Steamid);
	g_BATCore.AddLogEntry("%s<%s> has started a votekick against %s<%s>",g_BATCore.GetPlayerName(admin_id),g_UserInfo[admin_id].Steamid,g_BATCore.GetPlayerName(Target_id),g_UserInfo[Target_id].Steamid);
	g_BATCore.GetMapVote()->StartVoteKick(Target_id);
}
void AdminCmd::CmdSlap(int admin_id)
{
	char *arg1 = GetArg(1);
	int Dmg = atoi(GetArg(2));

	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int Target_id = g_BATCore.FindPlayer(arg1);

	if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}
	else if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}


	if(Dmg < 0 && Dmg != -1)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("SlapIncorrectDmg"),Dmg);
		return;
	}
	Slap(Target_id,admin_id,Dmg);
}
void AdminCmd::CmdShowRules(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int Target_id = g_BATCore.FindPlayer(arg1);

	if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}
	else if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}

	Slap(Target_id,admin_id,Slap_SlayAndRulesUser);
}
void AdminCmd::CmdRcon(int admin_id)
{
	char SrvCmd[192] = "";
	int len=0;

	if(g_ArgCount >= 2)
	{
		for(unsigned int i=1;i<g_ArgCount;i++)
		{
			if(i == 1)
				len += _snprintf(&(SrvCmd[len]),191-len,"%s",GetArg(i));
			else
				len += _snprintf(&(SrvCmd[len]),191-len," %s",GetArg(i));
		}
	}
	else 
		_snprintf(SrvCmd,191,"%s",g_BATCore.StrRemoveQuotes(GetArg(1)));

	const char *Name = g_BATCore.GetPlayerName(admin_id);

	g_BATCore.ServerCommand(SrvCmd);
	
	if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("RconExec"),Name,SrvCmd);
	else
	{
		g_BATCore.MessageAdmins(m_Translation->GetTranslation("RconExec"),Name,SrvCmd);
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("RconExec"),"A Admin",SrvCmd);
	}
	
	g_BATCore.AddLogEntry("%s<%s> executed %s in server console",Name,g_UserInfo[admin_id].Steamid,SrvCmd);
}
void AdminCmd::CmdAdminMenu(int admin_id)
{
	g_MenuPos[admin_id] = MENU_ROOT;				// We reset the menu pos.
	g_MenuMngr.ShowMenu(admin_id,g_AdminMenu);
}
void AdminCmd::CmdChangeLevel(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	if (!g_BATCore.GetEngine()->IsMapValid(arg1)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("NotValidMap"),arg1);
		return;
	}

	Changelevel(admin_id,arg1);
}
void AdminCmd::CmdTeam(int admin_id)
{
	int TeamNum = atoi(GetArg(2));
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int Target_id = g_BATCore.FindPlayer(arg1);

	if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}
	else if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}

	ChangeTeam(Target_id,admin_id,TeamNum);		
}
void AdminCmd::CmdUnBan(int admin_id)
{
	if (g_BATCore.HasAccess(admin_id,ADMIN_VOTEONLY))
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("OnlyVoteCmds"));
		return;
	}

	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	if(strncmp(arg1,"STEAM_",6) == 0)
	{
		if(strcmp(arg1,"STEAM_ID_LAN") == 0 || strcmp(arg1,"STEAM_ID_PENDING") == 0)
		{
			g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,"[BAT] Cannot unban %s",arg1);
			return;
		}
		UnBanId(admin_id,arg1);
		return;
	}
	g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
	return;	
}
void AdminCmd::CmdBan(int admin_id)
{
	int BanTime = atoi(GetArg(1));
	char *arg1 = GetArg(2);
	char *reason = GetReason(3);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	if (g_BATCore.HasAccess(admin_id,ADMIN_TEMPBAN) && (BanTime == 0 || BanTime > 60))
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotBanTempOnly"));
		return;
	}
	if (g_BATCore.HasAccess(admin_id,ADMIN_VOTEONLY))
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("OnlyVoteCmds"));
		return;
	}
	
	int Target_id = g_BATCore.FindPlayer(arg1);
	if (Target_id > 0) 
	{
		if(!g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_NORULES))
			g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotBan"),arg1);
		else
			Ban(Target_id,admin_id,BanTime,reason);		
		return;
	}
	if(strnicmp(arg1,"STEAM_",6) == 0)
	{
		if(strcmp(arg1,"STEAM_ID_LAN") == 0 || strcmp(arg1,"STEAM_ID_PENDING") == 0)
		{
			g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotBan"),arg1);
			return;
		}
		BanId(admin_id,arg1,BanTime,reason);
		return;
	}
	g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
	return;	
}
void AdminCmd::CmdBanIP(int admin_id)
{
	if (g_BATCore.HasAccess(admin_id,ADMIN_VOTEONLY))
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("OnlyVoteCmds"));
		return;
	}

	int BanTime = atoi(GetArg(1));
	char *arg1 = GetArg(2);
	char *reason = GetReason(3);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	BanIP(admin_id,BanTime,arg1,reason);
	return;	
}
void AdminCmd::CmdUnBanIP(int admin_id)
{
	if (g_BATCore.HasAccess(admin_id,ADMIN_VOTEONLY))
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("OnlyVoteCmds"));
		return;
	}

	char *arg1 = GetArg(1);

	UnBanIP(admin_id,arg1);
	return;	
}
void AdminCmd::CmdKick(int admin_id)
{
	char *arg1 = GetArg(1);
	char *reason =  GetReason(2);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int Target_id = g_BATCore.FindPlayer(arg1);
	if (g_BATCore.HasAccess(admin_id,ADMIN_VOTEONLY))
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("OnlyVoteCmds"));
		return;
	}

	if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}
	else if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_NORULES)) 
	{
        g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}

	Kick(Target_id,admin_id,reason);
	return;
}
void AdminCmd::CmdChangeName(int admin_id)
{
	char *arg1 = GetArg(1);
	arg1 = g_BATCore.StrRemoveQuotes(arg1);

	int Target_id = g_BATCore.FindPlayer(arg1);
	if (Target_id == -1 || !g_BATCore.IsValidTarget(admin_id,Target_id,TARGET_HUMAN)) 
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("CannotFindTarget"),arg1);
		return;
	}
	else if(Target_id == -2)
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ToManyTargets"));
		return;
	}

	ChangeName(admin_id,Target_id);	
}
void AdminCmd::CmdSay(int admin_id)
{
	char *SayMsg = GetReason(1);

	Say(admin_id,SayMsg);
}
void AdminCmd::CmdAdminSay(int admin_id)
{
	char *SayMsg = GetReason(1);

	AdminChat(admin_id,SayMsg);
}
void AdminCmd::CmdCSay(int admin_id)
{
	char SayMsg[192] = "";
	int len=0;

	if(g_ArgCount >= 2)
	{
		for(unsigned int i=1;i<g_ArgCount;i++)
		{
			if(i == 1)
				len += _snprintf(&(SayMsg[len]),191-len,"%s",GetArg(i));
			else
				len += _snprintf(&(SayMsg[len]),191-len," %s",GetArg(i));
		}
	}
	else 
		len += _snprintf(&(SayMsg[len]),191-len,g_BATCore.StrRemoveQuotes(GetArg(1)));
    
	AdminTalkToAll(admin_id,SayMsg);
}
#if BAT_DEBUG == 1
void AdminCmd::CmdUpdateTrans(int admin_id)
{
	m_Translation->SortTranslationList();
}
void AdminCmd::CmdTest(int admin_id)
{
	PreformanceSampler::WriteLogFile("c:\\1.htm");
	for(int i=0;i<10;i++)
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("OMG THIS IS  A DELAYED MESSAGE %d",i);
	/*
	//g_MenuMngr.ShowESCMenu(admin_id);
	char MyMsg[192];


	_snprintf(MyMsg,191,"\x01 This message is number 1");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x02 This message is number 2");
	g_BATCore.MessagePlayer(0,MyMsg);	

	_snprintf(MyMsg,191,"\x03 This message is number 3");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x04 This message is number 4");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x05 This message is number 5");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x06 This message is number 6");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x07 This message is number 7");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x08 This message is number 8");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x09 This message is number 9");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x11 This message is number 11");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x12 This message is number 12");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x13 This message is number 13");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x14 This message is number 14");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x15 This message is number 15");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x16 This message is number 16");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x17 This message is number 17");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x18 This message is number 18");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x19 This message is number 19");
	g_BATCore.MessagePlayer(0,MyMsg);

	_snprintf(MyMsg,191,"\x20 This message is number 20");
	g_BATCore.MessagePlayer(0,MyMsg);
	*/
}
#endif
#if BAT_DEBUG == 2
extern unsigned int g_ActiveIDChecks;
extern stSQLSettings g_SQLSettings;
extern bool g_IsConnected[MAXPLAYERS+1];
extern bool g_IsConnecting[MAXPLAYERS+1];

void AdminCmd::CmdThreadTest(int admin_id)
{
	int Connected=0;
	int Connecting=0;
	int SteamdNotValid=0;
	int WaitingSetup=0;
	int WaitingSQL=0;
	int Valid=0;
	for(int i=1;i<=g_MaxClients;i++)
	{
		if(g_IsConnecting[i])
			Connecting++;
		if(g_IsConnected[i])
			Connected++;

		if((g_IsConnected[i] || g_IsConnecting[i]) && g_UserInfo[i].SteamIdStatus == STEAM_ID_NOTVALID) 
			SteamdNotValid++;
		if((g_IsConnected[i] || g_IsConnecting[i]) && g_UserInfo[i].SteamIdStatus == STEAM_ID_WAIT_FOR_SETUP) 
			WaitingSetup++;
		if((g_IsConnected[i] || g_IsConnecting[i]) && g_UserInfo[i].SteamIdStatus == STEAM_ID_WAIT_FOR_SQL) 
			WaitingSQL++;
		if((g_IsConnected[i] || g_IsConnecting[i]) && g_UserInfo[i].SteamIdStatus == STEAM_ID_VALID) 
			Valid++;
	}

	g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,"Players - Connected: %d Connecting: %d (ActiveID checks: %d) SQL Thread: %d Errors: %d",Connected,Connecting,(int)g_ActiveIDChecks,g_SQLSettings.ThreadActive,g_SQLSettings.Errors);
	g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,"Steamid status: Valid: %d NotValid: %d  Waiting Setup: %d Waiting SQL: %d",Valid,SteamdNotValid,WaitingSetup,WaitingSQL);
}
#endif
/************************************************* The Functions that actually do the action bellow ************************************************/
void AdminCmd::ReloadSettings(int admin_id)
{
	g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("ReloadConfig"));
	g_BATCore.ReloadSettings();
		
	g_BATCore.AddLogEntry("%s<%s> made the server reload settings and config files",g_BATCore.GetPlayerName(admin_id),g_UserInfo[admin_id].Steamid);
}
void AdminCmd::AdminTalkToAll(int admin_id,const char *pText)
{
	char buffer[192];

	_snprintf(buffer,191,"%s - %s",g_BATCore.GetPlayerName(admin_id),pText);
	g_BATCore.SendCSay(0,buffer);
}
void AdminCmd::PlayerList(int admin_id)
{
	int Connecting=0;
	for(int i=1;i<=g_MaxClients;i++)
	{
		if(g_IsConnecting[i])
			Connecting++;
	}

	if(g_BATCore.HasAccess(admin_id,ADMIN_ANY))
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("PlayerListAdmin"),g_PlayerCount,Connecting,g_ReservedSlotsUsers,m_ReservedSlots->GetFreeNoneAdminSlots());
		for(int i=1;i<=g_MaxClients;i++)
		{
			if(g_IsConnected[i])
			{
				if(g_BATCore.HasAccess(i,ADMIN_ANY))
					g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,"%d) %s<%s> IP: %s UserID: %d AdminName: %s Flags: %s",i,g_BATCore.GetPlayerName(i),g_UserInfo[i].Steamid,g_UserInfo[i].IP,g_UserInfo[i].Userid,g_UserInfo[i].AdminName,g_UserInfo[i].AdminFlags);
				else
					g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,"%d) %s<%s> IP: %s UserID: %d",i,g_BATCore.GetPlayerName(i),g_UserInfo[i].Steamid,g_UserInfo[i].IP,g_UserInfo[i].Userid);
			}
		}
	}
	else
	{
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,m_Translation->GetTranslation("PlayerList"),g_PlayerCount,Connecting);

		for(int i=1;i<=g_MaxClients;i++)
		{
			if(g_IsConnected[i])
			{
				g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,"%d) %s<%s> UserID: %d",i,g_BATCore.GetPlayerName(i),g_UserInfo[i].Steamid,g_UserInfo[i].Userid);
			}
		}
	}
}
void AdminCmd::StartMapVote(int admin_id)
{
	g_BATCore.AddLogEntry("%s<%s> started a map vote",g_BATCore.GetPlayerName(admin_id),g_UserInfo[admin_id].Steamid);

	if( g_ArgCount != 0)
	{
		char *arg1 = GetArg(1);
		if(strcmp(arg1,"nextmap") == 0)
			m_MapVote->StartPublicMapVote(g_BATCore.GetTimeLeft(),false);
		else
			m_MapVote->StartPublicMapVote(g_BATCore.GetTimeLeft(),true);
	}
	else
		m_MapVote->StartPublicMapVote(g_BATCore.GetTimeLeft(),true);
}
void AdminCmd::AdminHelp(int admin_id)
{
	for(unsigned int i=0;i<g_AdminCmdCount;i++) if(g_BATCore.HasAccess(admin_id,g_AdminCommands[i].AccessRequired))
		g_BATCore.GetAdminCmds()->NoticePlayer(admin_id,"[BAT] %s - %s",g_AdminCommands[i].Cmd,m_Translation->GetTranslation(g_AdminCommands[i].Description));
}
void AdminCmd::ChangeName(int target_id,int admin_id)
{
	const char *tName = g_BATCore.GetPlayerName(target_id);
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	if(!g_UserInfo[target_id].IsBot)
		g_BATCore.GetEngine()->ClientCommand(g_UserInfo[target_id].PlayerEdict,"name ChangeName");

	
	if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("ChangeName"),aName,tName);
	else
	{
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("ChangeName"),"A Admin",tName);
		g_BATCore.MessageAdmins(m_Translation->GetTranslation("ChangeName"),aName,tName);
	}

	g_BATCore.AddLogEntry("%s<%s> forced %s<%s> to change his name",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid);
}
void AdminCmd::Changelevel(int admin_id,char *MapName)
{
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("Changelevel"),aName,MapName);
	else
	{
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("Changelevel"),"A Admin",MapName);
		g_BATCore.MessageAdmins(m_Translation->GetTranslation("Changelevel"),aName,MapName);
	}
	
	g_BATCore.AddLogEntry("%s<%s> changed map to %s",aName,g_UserInfo[admin_id].Steamid,MapName);
	g_BATCore.StartChangelevelCountDown(MapName);
}
void AdminCmd::ChangeTeam(int target_id,int admin_id,int Team)
{
	const char *tName = g_BATCore.GetPlayerName(target_id);
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	char *TeamName = g_BATCore.GetTeamName(Team);

	if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("ChangeTeam"),aName,tName,TeamName);
	else
	{
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("ChangeTeam"),"A Admin",tName,TeamName);
		g_BATCore.MessageAdmins(m_Translation->GetTranslation("ChangeTeam"),aName,tName,TeamName);
	}
	
	g_BATCore.AddLogEntry("%s<%s> made %s<%s> change to %s team",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid,TeamName);
	
	/*
	char ClientCmd[24];
	_snprintf(ClientCmd,sizeof(ClientCmd)-1,"jointeam %d",Team);
	g_BATCore.GetEngine()->ClientCommand(g_UserInfo[target_id].PlayerEdict,ClientCmd);
	*/

	IPlayerInfo *PlayerInfo=g_BATCore.GetPlayerInfo()->GetPlayerInfo(g_UserInfo[target_id].PlayerEdict);
	
	if (PlayerInfo && g_UserInfo[target_id].PlayerEdict && !g_UserInfo[target_id].PlayerEdict->IsFree()) 
	{ 
		PlayerInfo->ChangeTeam( Team ); 
	}
}

void AdminCmd::Slap(int target_id,int admin_id,int SlapDmg)
{
	const char *tName = g_BATCore.GetPlayerName(target_id);
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	if(SlapDmg == Slap_SlayAndRulesUser)
	{
		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SlayedUser"),aName,tName);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SlayedUser"),"A admin",tName);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("SlayedUser"),aName,tName);
		}
		
		g_BATCore.AddLogEntry("%s<%s> slayed %s<%s> and showed him the server rules",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid);

		if(!g_UserInfo[target_id].IsBot)
			g_BATCore.GetHelpers()->ClientCommand(g_UserInfo[target_id].PlayerEdict,"kill");

		if(!g_UserInfo[target_id].IsBot)
		{
			if(!g_RulesURLSet) 
				NoticePlayer(admin_id,"No rules url is set, please check the BATConfig.cfg");
			else if(!g_UserInfo[target_id].IsBot)
				ShowMOTD(target_id,g_RulesURL);	
		}
	}
	else if(SlapDmg == Slap_SlayUser)
	{
		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SlayedUser"),aName,tName);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SlayedUser"),"A admin",tName);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("SlayedUser"),aName,tName);
		}

		g_BATCore.AddLogEntry("%s<%s> slayed %s<%s>",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid);

		g_BATCore.GetUtils()->UTIL_KillPlayer(g_UserInfo[target_id].PlayerEdict);
	}
	else if(SlapDmg == 0)
	{
		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SlapDmg"),aName,tName,0);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SlapDmg"),"A admin",tName,0);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("SlapDmg"),aName,tName,0);
		}

		g_BATCore.AddLogEntry("%s<%s> slapped %s<%s> with 0 damage",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid);
	}
	else if(SlapDmg == Slap_SlapTo1Hp)
	{
		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SlappedToHp"),aName,tName);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SlappedToHp"),"A admin",tName);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("SlappedToHp"),aName,tName);
		}

		g_BATCore.AddLogEntry("%s<%s> slapped %s<%s> to 1 hp",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid);
		g_BATCore.GetUtils()->UTIL_SetHealth(g_UserInfo[target_id].PlayerEdict,1);
	}
	else if(SlapDmg > 0)
	{
		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SlapDmg"),aName,tName,SlapDmg);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SlapDmg"),"A admin",tName,SlapDmg);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("SlapDmg"),aName,tName,SlapDmg);
		}

		g_BATCore.AddLogEntry("%s<%s> slapped %s<%s> with %d damage",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid,SlapDmg);

		int Health = g_BATCore.GetUtils()->UTIL_GetHealth(g_UserInfo[target_id].PlayerEdict) - SlapDmg;
		if(Health <= 0)
		{
			if(!g_UserInfo[target_id].IsBot)
				g_BATCore.GetHelpers()->ClientCommand(g_UserInfo[target_id].PlayerEdict,"kill");
		}
		else
			g_BATCore.GetUtils()->UTIL_SetHealth(g_UserInfo[target_id].PlayerEdict,Health);//pBase->SetHealth(Health);

	}
	srand(time(NULL));
	Vector Velocity;
	Velocity.x = (rand()%800) - 400; // sslice is cool
	Velocity.y = (rand()%800) - 400;
	Velocity.z = (rand()%150) + 150;

	g_BATCore.GetUtils()->UTIL_TelePortPlayer(g_UserInfo[target_id].PlayerEdict,NULL,NULL,&Velocity);

	if(!g_UserInfo[target_id].IsBot)
		g_BATCore.EmitSound(0,target_id,SOUND_SLAP);
}
void AdminCmd::Kick(int target_id,int admin_id,char *Reason)
{
	const char *tName = g_BATCore.GetPlayerName(target_id);
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	if(!Reason || Reason == "" || strlen(Reason) < 1)
	{
		g_BATCore.ServerCommand("kickid %d",g_UserInfo[target_id].Userid);
		g_BATCore.AddLogEntry("%s<%s> kicked %s<%s>",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid);

		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("KickedUser"),aName,tName);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("KickedUser"),"A admin",tName);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("KickedUser"),aName,tName);
		}
	}
	else
	{
		Reason = g_BATCore.StrRemoveQuotes(Reason);
		g_BATCore.ServerCommand("kickid %d %s",g_UserInfo[target_id].Userid,Reason);
		g_BATCore.AddLogEntry("%s<%s> kicked %s<%s> (Reason: %s)",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid,Reason);

		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("KickedUserReason"),aName,tName,Reason);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("KickedUserReason"),"A admin",tName,Reason);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("KickedUserReason"),aName,tName,Reason);
		}
	}
}
void AdminCmd::BanId(int admin_id,char *BanID,int BanTime,char *Reason)
{
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	if(Reason == NULL || Reason == "" || strlen(Reason) < 1)
	{
		g_BATCore.ServerCommand("banid %d %s kick",BanTime,BanID);
		g_BATCore.AddLogEntry("%s<%s> banned %s for %d time",aName,g_UserInfo[admin_id].Steamid,BanID,BanTime);

		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedUser"),aName,BanID);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedUser"),"A admin",BanID);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("BannedUser"),aName,BanID);
		}
	}
	else
	{
		Reason = g_BATCore.StrRemoveQuotes(Reason);
		g_BATCore.ServerCommand("banid %d %s",BanTime,BanID);
		g_BATCore.ServerCommand("kickid \"%s\" %s",BanID,Reason);
		g_BATCore.AddLogEntry("%s<%s> banned %s (Reason: %s) for %d time",aName,g_UserInfo[admin_id].Steamid,BanID,Reason,BanTime);

		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedUserReason"),aName,BanID,Reason);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedUserReason"),"A admin",BanID,Reason);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("BannedUserReason"),aName,BanID,Reason);
		}
	}
	g_BATCore.ServerCommand("writeid");
}
void AdminCmd::SetNextmap(int admin_id,char *Nextmap)
{
	g_NextMapIndex = g_BATCore.GetBATMaps()->GetMapIndex(Nextmap);
	_snprintf(g_NextMapName,MAX_MAPNAMELEN,"%s",Nextmap);

	if(m_MapVote->WaitForModEvent())
		VoteInfo.Status = VOTE_DONE_WAITFOREVENT;			// we tell the "task" system, that it should control what the nextmap is
	else
	{
		VoteInfo.Status = VOTE_DONE;
		g_fMapChangeTime = 0.0;
		g_bChangeMapCountDown = true;
	}
	if(m_MapVote->ModHandlesMapChange())
		g_BATCore.GetBATVar().SetNextMap()->SetValue(Nextmap);

	if(admin_id != -1)
	{
		const char *Name = g_BATCore.GetPlayerName(admin_id);
		g_BATCore.AddLogEntry("%s<%s> manually made set %s to the nextmap",Name,g_UserInfo[admin_id].Steamid,Nextmap);

		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SetNextmap"),Name,Nextmap);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("SetNextmap"),"A admin",Nextmap);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("SetNextmap"),Name,Nextmap);
		}
	}
}
void AdminCmd::LoadRandomLevel()
{
	if(g_MapCount == 0) return;
	srand(time(NULL));
	int NewMapIndex = rand()%g_MapCount;
	if(NewMapIndex < 0 || NewMapIndex >= g_MapCount)
		return;

	g_NextMapIndex = NewMapIndex;
	g_BATCore.AddLogEntry("%s was loaded as the random map",g_MapList[NewMapIndex]->MapName);	
	g_BATCore.ServerCommand("changelevel %s",g_MapList[NewMapIndex]->MapName);
}
void AdminCmd::UnBanId(int admin_id,char *BanID)
{
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	g_BATCore.ServerCommand("removeid %s",BanID);
	if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("UnBannedUser"),aName,BanID);
	else
	{
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("UnBannedUser"),"A admin",BanID);
		g_BATCore.MessageAdmins(m_Translation->GetTranslation("UnBannedUser"),aName,BanID);
	}

	g_BATCore.AddLogEntry("%s<%s> unbanned %s",aName,g_UserInfo[admin_id].Steamid,BanID);

	g_BATCore.ServerCommand("writeid");
}
void AdminCmd::UnBanIP(int admin_id,char *BanID)
{
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	g_BATCore.ServerCommand("removeip %s",BanID);

	if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("UnBannedUser"),aName,BanID);
	else
	{
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("UnBannedUser"),"A admin",BanID);
		g_BATCore.MessageAdmins(m_Translation->GetTranslation("UnBannedUser"),aName,BanID);
	}

	g_BATCore.AddLogEntry("%s<%s> unbanned %s",aName,g_UserInfo[admin_id].Steamid,BanID);

	g_BATCore.ServerCommand("writeip");
}
void AdminCmd::Ban(int target_id,int admin_id,int BanTime,char *Reason)
{
	const char *tName = g_BATCore.GetPlayerName(target_id);
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	if(Reason == NULL || Reason == "" || strlen(Reason) < 1)
	{
		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedUser"),aName,tName);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedUser"),"A admin",tName);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("BannedUser"),aName,tName);
		}

		g_BATCore.AddLogEntry("%s<%s> banned %s<%s> for %d time",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid,Reason,BanTime);
		g_BATCore.ServerCommand("banid %d %d kick",BanTime,g_UserInfo[target_id].Userid);
	}
	else
	{
		Reason = g_BATCore.StrRemoveQuotes(Reason);
		g_BATCore.AddLogEntry("%s<%s> banned %s<%s> (Reason: %s) for %d time",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid,Reason,BanTime);

		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedUserReason"),aName,tName,Reason);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedUserReason"),"A admin",tName,Reason);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("BannedUserReason"),aName,tName,Reason);
		}

		g_BATCore.ServerCommand("banid %d %d",BanTime,g_UserInfo[target_id].Userid);
		g_BATCore.ServerCommand("kickid %d %s",g_UserInfo[target_id].Userid,Reason);
	}
	g_BATCore.ServerCommand("writeid");
}
void AdminCmd::BanIP(int admin_id,int BanTime,char *BanIP,char *Reason)
{
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	// We see if any players are using this IP, and if they have immunity. If the admin has immunity this test is pointless
	if(!g_BATCore.HasAccess(admin_id,ADMIN_IMMUNITY))
	{
		for(int i=1;i<=g_MaxClients;i++) if(g_IsConnected[i])
		{
			if(strcmp(g_UserInfo[i].IP,BanIP) == 0 && g_BATCore.HasAccess(i,ADMIN_IMMUNITY))
			{
				NoticePlayer(admin_id,m_Translation->GetTranslation("TargetHasImmunity"),g_BATCore.GetPlayerName(i));
				return;
			}
		}
	}

	if(Reason == NULL || Reason == "" || strlen(Reason) < 1)
	{
		g_BATCore.AddLogEntry("%s<%s> banned ip %s for %d time",aName,g_UserInfo[admin_id].Steamid,BanIP,BanTime);

		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedIP"),aName,BanIP);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedIP"),"A admin",BanIP);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("BannedIP"),aName,BanIP);
		}

		g_BATCore.ServerCommand("addip %d %s",BanTime,BanIP);
	}
	else
	{
		Reason = g_BATCore.StrRemoveQuotes(Reason);	
		if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedIPReason"),aName,BanIP,Reason);
		else
		{
			g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("BannedIPReason"),"A admin",BanIP,Reason);
			g_BATCore.MessageAdmins(m_Translation->GetTranslation("BannedIPReason"),aName,BanIP,Reason);
		}
		
		g_BATCore.AddLogEntry("%s<%s> banned ip %s for %d time (Reason: %s)",aName,g_UserInfo[admin_id].Steamid,BanIP,BanTime,Reason);
		g_BATCore.ServerCommand("addip %d %s",BanTime,BanIP);
	}
	g_BATCore.ServerCommand("writeip");
}
void AdminCmd::Say(int admin_id,char *Msg)
{
	g_BATCore.MessagePlayer(0,"\x04 Admin %s\x01 - %s",g_BATCore.GetPlayerName(admin_id),Msg);
}
void AdminCmd::Gag(int target_id,int admin_id)
{
	const char *tName = g_BATCore.GetPlayerName(target_id);
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	g_BATCore.AddLogEntry("%s<%s> gaged %s<%s>",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid);
	
	if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("GagPlayer"),aName,tName);
	else
	{
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("GagPlayer"),"A admin",tName);
		g_BATCore.MessageAdmins(m_Translation->GetTranslation("GagPlayer"),aName,tName);
	}

	g_UserInfo[target_id].GagStatus = ((int)GAG_TEXTCHAT + GAG_VOICECOMM);
}
void AdminCmd::UnGag(int target_id,int admin_id)
{
	const char *tName = g_BATCore.GetPlayerName(target_id);
	const char *aName = g_BATCore.GetPlayerName(admin_id);

	g_BATCore.AddLogEntry("%s<%s> ungaged %s<%s>",aName,g_UserInfo[admin_id].Steamid,tName,g_UserInfo[target_id].Steamid);

	if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("UnGagPlayer"),aName,tName);
	else
	{
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("UnGagPlayer"),"A admin",tName);
		g_BATCore.MessageAdmins(m_Translation->GetTranslation("UnGagPlayer"),aName,tName);
	}

	g_UserInfo[target_id].GagStatus = GAG_NONE;
}
void AdminCmd::AdminChat(int id,char temp[])
{
	char MyMsg[192];
	_snprintf(MyMsg,191,"\x03 ADMIN Chat - %s\x01: %s",g_BATCore.GetPlayerName(id),temp);

	for(int i=1;i<=g_MaxClients;i++) if(g_IsConnected[i] && g_BATCore.HasAccess(i,ADMIN_CHAT))
	{
		g_BATCore.MessagePlayer(i,MyMsg);
	}
}
void AdminCmd::MenuRconCmd(int admin_id,int MenuRconIndex)
{
	const char *aName = g_BATCore.GetPlayerName(admin_id);
	g_BATCore.AddLogEntry("%s<%s> has executed the rcon menu command: %s",aName,g_UserInfo[admin_id].Steamid,g_MenuRconCmds[MenuRconIndex]->Description);	

	if(g_BATCore.GetBATVar().GetHideAdminName() == 0)
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("MenuRconCmds"),aName,g_MenuRconCmds[MenuRconIndex]->Description);
	else
	{
		g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("MenuRconCmds"),"A admin",g_MenuRconCmds[MenuRconIndex]->Description);
		g_BATCore.MessageAdmins(m_Translation->GetTranslation("MenuRconCmds"),aName,g_MenuRconCmds[MenuRconIndex]->Description);
	}

	g_BATCore.ServerCommand(g_BATCore.GetMenuRconCmds()->ParseRconCmdsString(MenuRconIndex,false));	
}

/* ********************** Util function to get args ******************************/
bool g_IsViaSay=false;

void AdminCmd::NoticePlayer(int admin_id, const char *msg, ...)
{
	char vafmt[192];
	va_list ap;
	va_start(ap, msg);
	int len = _vsnprintf(vafmt, 191, msg, ap);
	va_end(ap);

	if(g_IsViaSay)
		g_BATCore.MessagePlayer(admin_id,vafmt);
	else
		g_BATCore.ConsolePrint(admin_id,vafmt);
}
char *AdminCmd::GetReason(unsigned int ArgStart)
{
	static char Reason[192];
	int len=0;

	if(ArgStart >= g_ArgCount)
		return NULL;

	for(unsigned i=ArgStart;i<g_ArgCount;i++)
	{
		if(i == 1)
			len += _snprintf(&(Reason[len]),191-len,"%s",GetArg(i));
		else
			len += _snprintf(&(Reason[len]),191-len," %s",GetArg(i));
	}
	return Reason;
}
void AdminCmd::ShowMOTD(int target_id,const char *url)
{
	RecipientFilter rf; // the corresponding recipient filter 

	rf.AddPlayer (target_id); 
	rf.MakeReliable();

	bf_write *netmsg = g_BATCore.GetEngine()->UserMessageBegin (static_cast<IRecipientFilter *>(&rf), g_ModSettings.VGUIMenu); 
	netmsg->WriteString("info"); // the HUD message itself 
	netmsg->WriteByte(1);
	netmsg->WriteByte(3); // I don't know yet the purpose of this byte, it can be 1 or 0 
	netmsg->WriteString("type"); // the HUD message itself 
	netmsg->WriteString("2"); // the HUD message itself 
	netmsg->WriteString("title"); // the HUD message itself 
	netmsg->WriteString("Server Rules"); // the HUD message itself 

	netmsg->WriteString("msg"); // the HUD message itself 
	netmsg->WriteString(url); // the HUD message itself 

	g_BATCore.GetEngine()->MessageEnd();
}
char *AdminCmd::GetArg(int ArgNum)
{
	if(!g_IsViaSay)
		return (char *)g_LastCCommand.Arg(ArgNum);
	else
		return GetArgChat(ArgNum);
	return NULL;
}
char *AdminCmd::GetArgChat(unsigned int ArgNum)
{
	if(ArgNum >= g_ArgCount)
		return NULL;
	
	if(ArgNum > 0)
		ArgNum--;
	
	return g_ArgList[ArgNum]->Text;
}