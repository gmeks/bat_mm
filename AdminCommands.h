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

#ifndef _INCLUDE_ADMINCMDS
#define _INCLUDE_ADMINCMDS

#include "BATCore.h"
#include "const.h"

class AdminCmd
	{
	public:
		static void CmdCSay(int admin_id);
		static void CmdAdminSay(int admin_id);
		static void CmdSay(int admin_id);
		static void CmdAddAdmin(int admin_id);

		static void CmdGag(int admin_id);
		static void CmdUnGag(int admin_id);
		static void CmdChangeName(int admin_id);
		static void CmdKick(int admin_id);
		static void CmdVoteKick(int admin_id);
		static void CmdVoteBan(int admin_id);
		static void CmdBan(int admin_id);
		static void CmdBanIP(int admin_id);
		static void CmdUnBanIP(int admin_id);
		static void CmdUnBan(int admin_id);
		static void CmdTeam(int admin_id);
		static void CmdChangeLevel(int admin_id);
		static void CmdAdminMenu(int admin_id);
		static void CmdRcon(int admin_id);
		static void CmdNextMap(int admin_id);
		static void CmdSlay(int admin_id);
		static void CmdSlap(int admin_id);
		static void CmdShowRules(int admin_id);
		static void CmdEjectComm(int admin_id);
		static void CmdVote(int admin_id);
		static void CmdPrivateSay(int admin_id);
		static void CmdSetMoveType(int admin_id);

#if BAT_DEBUG == 1
		static void CmdTest(int admin_id);
		static void CmdUpdateTrans(int admin_id);
#endif
#if BAT_DEBUG == 2
		static void CmdThreadTest(int admin_id);
#endif

		static void ShowMOTD(int target_id,const char *url);
		static void AdminTalkToAll(int admin_id,const char *pText);
		static void Kick(int target_id,int admin_id,char *Reason);
		static void Ban(int target_id,int admin_id,int BanTime,char *Reason);
		static void BanId(int admin_id,char *BanID,int BanTime,char *Reason);
		static void BanIP(int admin_id,int BanTime,char *BanIP,char *Reason);
		static void UnBanId(int admin_id,char *BanID);
		static void UnBanIP(int admin_id,char *BanIP);
		static void ChangeName(int target_id,int admin_id);
		static void Slap(int target_id,int admin_id,int SlapDmg);
		static void Say(int admin_id,char *Msg);
		static void ChangeTeam(int target_id,int admin_id,int Team);
		static void Changelevel(int admin_id,char *MapName);
		static void SetNextmap(int admin_id,char *Nextmap);
		static void LoadRandomLevel();	// Loads a random map from the map lists
		static void AdminHelp(int admin_id);
		static void StartMapVote(int admin_id);
		static void PlayerList(int admin_id);
		static void ReloadSettings(int admin_id);
		static void AdminChat(int id,char temp[]);
		static void MenuRconCmd(int id,int RconMenuCmdIndex);
		
		static void Gag(int target_id,int admin_id);
		static void UnGag(int target_id,int admin_id);
		
	
		static void NoticePlayer(int admin_id, const char *msg, ...); // This function either console prints or does a say msg depending on if the command was issued via console or Chat

	private:
		static char *GetArgChat(unsigned int ArgNum);
		static char *GetArg(int ArgNum);
		static char *GetReason(unsigned int ArgStart);	// Gets the reason, from the start arg to the end
		


	};
#endif // _INCLUDE_ADMINCMDS
