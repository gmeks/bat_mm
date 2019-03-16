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

#include "time.h"
#include "BATCore.h"
#include "BATSQL.h"
#include "Translation.h"
#include "BATMaps.h"
#include "AdminCommands.h"
#include "PublicMessages.h"
#include "MenuRconCmds.h"
#include "ReservedSlotsSystem.h"
#include "SourceModInterface.h"
#include "cvars.h"
#include "const.h"
#include <string.h>
#include <bitbuf.h>
#include <ctype.h>
#include "hl2sdk/recipientfilters.h"

#include <Color.h>
#include "hl2sdk/KeyValues.h"
//#include "playerstate.h"
//#include "cbase.h"

extern stSQLSettings g_SQLSettings;
extern stSQLStatus g_SQLStatus;

extern SourceHook::CVector<MapStuct *>g_MapList;
extern char g_CurrentMapName[MAX_MAPNAMELEN+1];
extern int g_MapCount;
extern int g_CurrentMapIndex;
extern char g_NextMapName[MAX_MAPNAMELEN+1];

extern char g_ModName[32];

extern float g_FlTime;
extern float g_GameStartTime;

extern float g_fMapChangeTime;
extern bool g_bChangeMapCountDown;

extern SourceHook::CVector<AdminInterfaceListnerStruct *>g_CallBackList;
extern unsigned int g_CallBackCount;

extern int g_ReservedSlotsSystem;

char g_LogBuffer[MAX_LOGBUFFER][MAX_LOGLEN+1];
unsigned int g_LogBufferCount = 0;

extern PublicMessages *m_PublicMessage;
extern ReservedSlotsSystem *m_ReservedSlots;
extern BATSQL *m_BATSQL;
extern BATMaps *m_BATMaps;
extern Translation *m_Translation;
extern MenuRconCmds *m_MenuRconCmds;
extern BATMenuMngr g_MenuMngr;

FILE *g_LogFileStream;
char g_BasePath[256];
char g_BATConfigFile[64];

time_t g_tLastBATExecTime; // Since we can enter a fun little loop with config files, we store the last time we execed BAT config, so make sure we dont loop.
unsigned int g_ActiveIDChecks = 0;
extern bool g_UseLogBuffer1;

// Used by the Csay system, to reshow old messages, as they stay so low on the orignal time
extern int g_ReShowCSay;
extern char g_LastCSayMsg[192];

extern bool g_SettingsLoaded;

void BATCore::StartChangelevelCountDown(const char *NewMap)
{
	float MapChangeDelay = g_BATVars.GetMapChangeDelay();
	g_fMapChangeTime = GetTimeLeft() - MapChangeDelay;
	g_bChangeMapCountDown = true;

	if(strcmp(g_NextMapName,NewMap) != 0)
		_snprintf(g_NextMapName,MAX_MAPNAMELEN,NewMap);

	MessagePlayer(0,m_Translation->GetTranslation("MapChangeCountDown"),g_NextMapName,MapChangeDelay+1);
}
void BATCore::CheckUsersID()
{
#if BAT_DEBUG == 3
	ServerCommand("echo CheckUsersID: g_ActiveIDChecks %d",g_ActiveIDChecks);
#endif

	g_ActiveIDChecks = 0;
	g_BlockIDCheck = false;
	for(int i=1;i<=g_MaxClients;i++) if((g_IsConnected[i] || g_IsConnecting[i] == true ) && ( g_UserInfo[i].SteamIdStatus == STEAM_ID_NOTVALID || g_UserInfo[i].SteamIdStatus ==  STEAM_ID_WAIT_FOR_SETUP))
	{
		if(g_BlockIDCheck)
		{
			g_ActiveIDChecks ++ ;
			return;
		}
		g_ActiveIDChecks++;
		g_UserInfo[i].PlayerEdict = m_Engine->PEntityOfEntIndex(i);
		_snprintf(g_UserInfo[i].Steamid,MAX_NETWORKID_LENGTH,"%s",m_Engine->GetPlayerNetworkIDString(g_UserInfo[i].PlayerEdict));

#if BAT_DEBUG == 3
		ServerCommand("echo CheckUsersID %d: Authid: %s  Status: %d",i,g_UserInfo[i].Steamid,g_UserInfo[i].SteamidStatus);
#endif

		if(strcmp(g_UserInfo[i].Steamid,"STEAM_ID_PENDING") != 0) 
		{
			if(g_SQLStatus.Enabled == true && g_UserInfo[i].SteamIdStatus == STEAM_ID_NOTVALID) // Player has valid steamid, we check his id against sql
			{
				g_UserInfo[i].SteamIdStatus = STEAM_ID_WAIT_FOR_SQL;
				g_SQLStatus.CueLength++;

				if(g_SQLStatus.ThreadSuspended)
				{
					m_BATSQL->ResumeSqlThread();
				}
				continue;
			}
			SetupPlayerInfo(i);
		}
		else if(g_IsConnected[i] && (g_UserInfo[i].ConnectTime+g_BATVars.GetAuthTime() ) < g_FlTime) // More then the allowed time has passed, and we kick him
		{
			ServerCommand("kickid %d Your STEAMID did not Validate",g_UserInfo[i].Userid);
			MessagePlayer(0,"[BAT] %s was kicked because his steam id never got validated by the steam master servers",GetPlayerName(i));
			AddLogEntry("%s<%s> was kicked because his steamid never validated",GetPlayerName(i),g_UserInfo[i].IP);				
		}
	}
}

char *BATCore::GetTimeleftString()
{
	if(g_BATCore.GetBATVar().GetTimelimitCvar()->GetInt() == 0)
		return "No Time Limit";

	static char temp[192];
	int timelimit = (int)g_BATCore.GetTimeLeft();
	int minsleft = timelimit/60;
	int secleft = timelimit - (minsleft*60);
	if(secleft < 10)
		_snprintf(temp,191,"%i:0%i",minsleft,secleft);
	else
		_snprintf(temp,191,"%i:%i",minsleft,secleft);

	return temp;
}
bool BATCore::IsUserAlive(int id)
{
	/*
	CPlayerState *pPlayerState = m_ServerClients->GetPlayerState(g_UserInfo[id].PlayerEdict);
	if(!pPlayerState)
		return false;

	if(!pPlayerState->deadflag) // This function returns true if dead. so we just reverse it. Why some funtions are IsDead and some are IsAlive beats me
		return true;
*/
	IPlayerInfo *pPlayerInfo = g_BATCore.GetPlayerInfo()->GetPlayerInfo(g_UserInfo[id].PlayerEdict);
	if(!pPlayerInfo)
	{
		g_BATCore.AddLogEntry("ERROR! There was a error trying to get IPlayerInfo when trying to check if a user is alive");
		return false;
	}
	bool IsDead = pPlayerInfo->IsDead();

	if(!IsDead)
		return true;
	return false;
}
void BATCore::SetupPlayerInfo(int id)
{
	edict_t *pEntity = m_Engine->PEntityOfEntIndex(id);
	const char *pSteamid = m_Engine->GetPlayerNetworkIDString(pEntity);
	if(!pEntity || !pSteamid)
	{
		g_BATCore.AddLogEntry("ERROR: All hell is loose, some client with no steamid is here, this normaly happens if a plugin does ServerExecte() server will likely crash now");
		g_BATCore.WriteLogBuffer();
		return;
	}

	g_UserInfo[id].PlayerEdict = pEntity;
    _snprintf(g_UserInfo[id].Steamid,MAX_NETWORKID_LENGTH,"%s",pSteamid);

#if BAT_DEBUG >= 3
	ServerCommand("echo Setting up player %s",g_UserInfo[id].Steamid);
#endif

	g_UserInfo[id].Userid = m_Engine->GetPlayerUserId(pEntity);	
	
	if(strcmp(g_UserInfo[id].Steamid,"BOT") == 0 || strcmp(g_UserInfo[id].Steamid,"HLTV") == 0)
		g_UserInfo[id].IsBot = true;
	else 
		g_UserInfo[id].IsBot = false;

	if(strcmp(g_UserInfo[id].Steamid,"STEAM_ID_PENDING") == 0)		// We check to make sure his steamid is validated.
	{
		g_UserInfo[id].SteamIdStatus = STEAM_ID_NOTVALID;
		g_ActiveIDChecks++;
	}
	else
	{
		if(g_SQLStatus.Enabled == true && g_UserInfo[id].SteamIdStatus != STEAM_ID_WAIT_FOR_SETUP && !g_UserInfo[id].IsBot)
		{
			g_UserInfo[id].SteamIdStatus = STEAM_ID_WAIT_FOR_SQL;
			g_SQLStatus.CueLength++;
		}
		else
			g_UserInfo[id].SteamIdStatus = STEAM_ID_VALID;
	}

	if(g_SQLStatus.Enabled == false && g_UserInfo[id].SteamIdStatus == STEAM_ID_VALID)
	{
		g_UserInfo[id].AdminIndex = m_LoadAdminAccounts->IsUserAdmin(g_UserInfo[id].Steamid,g_UserInfo[id].IP);
		if(g_UserInfo[id].AdminIndex > -1)
		{
			g_UserInfo[id].AdminAccess = m_LoadAdminAccounts->GetAdminAccountFlags(g_UserInfo[id].AdminIndex);
			_snprintf(g_UserInfo[id].AdminName,127,"%s",m_LoadAdminAccounts->GetAdminAccountName(g_UserInfo[id].AdminIndex));
			_snprintf(g_UserInfo[id].AdminFlags,39,"%s",m_LoadAdminAccounts->GetAdminAccountFlagsString(g_UserInfo[id].AdminAccess));
		}
	}

	if(HasAccess(id,ADMIN_ANY,false))
	{
		AddLogEntry("%s<%s><%s> has logged in as a admin (Account name: %s Account flags: %s)",GetPlayerName(id),pSteamid,g_UserInfo[id].IP,g_UserInfo[id].AdminName,g_UserInfo[id].AdminFlags);
		
		if(HasAccess(id,ADMIN_RESERVEDSLOTS))
			g_ReservedSlotsUsers++;

		if(g_BATVars.GetSMInterface() == 2)
			g_SMExt.SetAdminRights(id,g_UserInfo[id].AdminAccess,g_UserInfo[id].AdminName);
	}
	if(g_UserInfo[id].SteamIdStatus == STEAM_ID_VALID)
		m_ReservedSlots->HandleReservedSlotsRequest(id);

	if(g_CallBackCount > 0 && g_BATCore.GetBATVar().GetAdminInterfaceEnabled() != 0 && g_UserInfo[id].SteamIdStatus == STEAM_ID_VALID)
	{
		for(unsigned int i=0;i<g_CallBackCount;i++)
		{
			AdminInterfaceListner *ptr = (AdminInterfaceListner *)g_CallBackList[i]->ptr;
			if(!ptr)
				continue;

			ptr->Client_Authorized(id);
		}		
	}
}
void BATCore::UpdatePlayerInformation()
{
	for (int i=1;i<=g_MaxClients;i++)
	{
		g_IsConnecting[i] = false;
		g_IsConnected[i] = false;
		g_UserInfo[i].SteamIdStatus = STEAM_ID_NOTVALID;
		g_UserInfo[i].AdminAccess = 0;
	}
}
void BATCore::UnloadAdminInterface()
{
	if(g_CallBackCount > 0)
	{
		for(unsigned int i=0;i<g_CallBackCount;i++)
		{
			AdminInterfaceListner *ptr = (AdminInterfaceListner *)g_CallBackList[i]->ptr;
			if(!ptr)
				continue;

			ptr->OnAdminInterfaceUnload();
		}
	}
}
void BATCore::SendHintMsg(int id,char *text)
{
	RecipientFilter mrf;
	char FormatedText[192];
	if(id == 0)
		mrf.AddAllPlayers(g_MaxClients);
	else
		mrf.AddPlayer(id);
	
	_snprintf(FormatedText,191,"%s",text);
	int len = strlen(FormatedText);
	if(len > 30) // Since the client dont automatically add \n when needed, we have to do it.
	{
		int LastAdded=0;
		for(int i=0;i<len;i++)
		{

			int t = GetNextSpaceCount(text,i);
			char b = FormatedText[i];

			if(FormatedText[i] == ' ' && LastAdded > 30 && (len-i) > 10 || (GetNextSpaceCount(text,i+1) + LastAdded)  > 34)
			{
				FormatedText[i] = '\n';
				LastAdded = 0;
			}
			else
				LastAdded++;
		}
	}

	bf_write *buffer = m_Engine->UserMessageBegin(static_cast<IRecipientFilter *>(&mrf), g_ModSettings.HintText);
	buffer->WriteByte(-1);
	buffer->WriteString(FormatedText);
	m_Engine->MessageEnd();	
}
void BATCore::SendCornerMsg(int id,const char *fmt, ...)
{
	char vafmt[240];
	va_list ap;
	va_start(ap, fmt);
	int len = _vsnprintf(vafmt, 239, fmt, ap);
	va_end(ap);
	len += _snprintf(&(vafmt[len]),239-len," \n");


	/*
	len = strlen(vafmt);
	if(len > 50) // Since the client dont automatically add \n when needed, we have to do it.
	{
		int LastAdded=0;
		for(int i=0;i<len;i++)
		{

			int t = GetNextSpaceCount(vafmt,i);
			char b = vafmt[i];

			if(vafmt[i] == ' ' && LastAdded > 40 && (len-i) > 10 || (GetNextSpaceCount(vafmt,i+1) + LastAdded)  > 40)
			{
				vafmt[i] = '\n';
				LastAdded = 0;
			}
			else
				LastAdded++;
		}
	}
	*/
	if(id == 0)
	{
		for(int i=1;i<=g_MaxClients;i++)
		{
			if(g_IsConnected[i] && !g_UserInfo[i].IsBot)
				g_MenuMngr.ShowCornerMsg(i,vafmt);
		}
	}
	else
		g_MenuMngr.ShowCornerMsg(id,vafmt);

/*
	KeyValues *kv = new KeyValues( "msg" );
	kv->SetString( "title", vafmt );
	kv->SetColor( "color", Color( 255, 0, 0, 255 ));
	kv->SetInt( "level", 5);
	kv->SetInt( "time", 10);
	g_BATCore.GetHelpers()->CreateMessage( g_UserInfo[id].PlayerEdict, DIALOG_MSG, kv, g_MenuMngr );
	kv->deleteThis();*/

}
int BATCore::GetNextSpaceCount(char *Text,int CurIndex)
{
	int Count=0;
	int len = strlen(Text);
	for(int i=CurIndex;i<len;i++)
	{
		if(Text[i] == ' ')
			return Count;
		else
			Count++;
	}

	return Count;
}
void BATCore::SendCSay(int id,const char *pText,bool IsReShowMsg) 
{
	if(!IsReShowMsg)
	{
		g_ReShowCSay = 5; 

		int StrLen = strlen(pText) + 1;
		_snprintf(g_LastCSayMsg,StrLen,"%s",pText);
		g_LastCSayMsg[StrLen] = '\n'; // We need a line break so the console message dont get messed up
		g_LastCSayMsg[StrLen+1] = '\0';

		for(int i=1;i<=g_MaxClients;i++)	
		{
			if(g_IsConnected[i] && g_UserInfo[i].IsBot == false )
				m_Engine->ClientPrintf(g_UserInfo[i].PlayerEdict,g_LastCSayMsg);
		}
	}	

	RecipientFilter mrf;

	if(id == 0 )
		mrf.AddAllPlayers(g_MaxClients);
	else
		mrf.AddPlayer(id);


	if(g_ModSettings.ValveCenterSay)
	{
		bf_write *buffer = m_Engine->UserMessageBegin(static_cast<IRecipientFilter *>(&mrf), g_ModSettings.TextMsg);
//FIXME
//		buffer->WriteByte(HUD_PRINTCENTER);
		buffer->WriteString(g_LastCSayMsg);
		m_Engine->MessageEnd();
	}
	else
	{
		bf_write *msg = m_Engine->UserMessageBegin((RecipientFilter *)&mrf,g_ModSettings.HudMsg);
		//msg->WriteByte(4); //4
		//msg->WriteString(temp);
		msg->WriteByte( 0); //textparms.channel
		msg->WriteFloat( -1 ); // textparms.x ( -1 = center )
		msg->WriteFloat( -.25 ); // textparms.y ( -1 = center )
		msg->WriteByte( 0xFF ); //textparms.r1
		msg->WriteByte( 0x00 ); //textparms.g1
		msg->WriteByte( 0x00 ); //textparms.b1
		msg->WriteByte( 0xFF ); //textparms.a2
		msg->WriteByte( 0xFF ); //textparms.r2
		msg->WriteByte( 0xFF ); //textparms.g2
		msg->WriteByte( 0xFF ); //textparms.b2
		msg->WriteByte( 0xFF ); //textparms.a2
		msg->WriteByte( 0x00); //textparms.effect
		msg->WriteFloat( 0 ); //textparms.fadeinTime
		msg->WriteFloat( 0 ); //textparms.fadeoutTime
		msg->WriteFloat( 10 ); //textparms.holdtime
		msg->WriteFloat( 45 ); //textparms.fxtime
		msg->WriteString( g_LastCSayMsg ); //Message
		m_Engine->MessageEnd();
	}
}
float BATCore::GetTimeLeft()
{
	float GameTime = g_FlTime-g_GameStartTime;

	return ((g_BATVars.GetTimelimitCvar()->GetFloat() * 60) - GameTime);
}
bool BATCore::IsValidTarget(int Admin_id,int Target_id,int Option)
{
	if(Admin_id == 0 || Target_id <= 0)	// To prevent problems
		return false;

	if(!(g_UserInfo[Admin_id].AdminAccess & ADMIN_IMMUNITY) && (g_UserInfo[Target_id].AdminAccess & ADMIN_IMMUNITY)) // target is immune
		return false;
	
	if(Option & TARGET_HUMAN && g_UserInfo[Target_id].IsBot == true) // Target is a bot or hltv
		return false;
	
	if(Option & TARGET_ALIVE && IsUserAlive(Target_id) == false) // Target is a bot or hltv
		return false;

	return true;
}
bool BATCore::HasAccess(int id,int Access,bool ShowWarrning)
{
	if(id == ID_SERVER)
		return true;

	if(g_UserInfo[id].AdminAccess & Access)
		return true;

	if(Access == ADMIN_ANY)
	{
		 if(g_UserInfo[id].AdminAccess > 0 && g_UserInfo[id].AdminAccess != ADMIN_RESERVEDSLOTS)
			 return true;
		 else
			 return false;
	}	

	if(!g_UserInfo[id].IsBot && ShowWarrning)
		ConsolePrint(id,"[BAT] You dont have access to that command");
	return false;
}


char *BATCore::GetTeamName(int Team)	// Gets the name of the team
{
	static char TeamName[32];
	if(Team < 1 || Team >= MAX_TEAMSINMENU)
	{
		_snprintf(TeamName,31,"Team %d",Team);
		AddLogEntry("ERROR: Trying to get the name of a invalid team %d",Team);
		return TeamName;
	}
	if(g_ModSettings.GameMod == MOD_CSTRIKE)	
	{
		switch (Team)
		{
		case 1: return "Spectator";
		case 2: return "Terrorist";
		case 3: return "Counter-Terrorist";
		}
	}
	else if(g_ModSettings.GameMod == MOD_DOD)	
	{
		switch (Team)
		{
		case 1: return "Spectator";
		case 2: return "Allied";
		case 3: return "Axis";
		}
	}
	else if(g_ModSettings.GameMod == MOD_EMPIRES)	
	{
		switch (Team)
		{
		case 1: return "Spectator";
		case 2: return "Northern Faction";
		case 3: return "Brenodi Empire";
		}
	}
	else if(g_ModSettings.GameMod == MOD_PVKII)	
	{
		switch (Team)
		{
		case 1: return "Spectator";
		case 2: return "Pirates";
		case 3: return "Vikings";
		case 4: return "Knights";
		}
	}
	else if(g_ModSettings.GameMod == MOD_DYSTOPIA)	
	{
		switch (Team)
		{
		case 1: return "Spectator";
		case 2: return "Punks";
		case 3: return "Cops";
		}
	}
	else if(g_ModSettings.GameMod == MOD_INSURGENCY)	
	{
		switch (Team)
		{
		case 1: return "Marines";
		case 2: return "Iraq";
		case 3: return "Spectator";
		}
	}
	else if(g_ModSettings.GameMod == MOD_TF2)	
	{
		switch (Team)
		{		
		case 4: return "Red";
		case 3: return "Blue";
		case 2: return "Spectator";
		case 1: return "Random";
		}
	}
	
	_snprintf(TeamName,31,"Team %d",Team);
	return TeamName;
}
int BATCore::FindPlayer(const char *pTargetInfo)	// This function tries to find the correct index of a player based steamid
{
	bool SearchForUserid = false;
	int UserID = 0;
	if(pTargetInfo[0] == '#')
	{
		SearchForUserid = true;
		UserID = atoi(&pTargetInfo[1]);
	}
	if(SearchForUserid == true)
	{
		for(int i=1;i<=g_MaxClients;i++)
		{
			if(g_IsConnected[i] == true)
			{
				if(UserID == g_UserInfo[i].Userid )	// Name or steamid matches TargetInfo so we return the index of the player
				{
					return i;
				}
			}
		}
	}
	else
	{
		for(int i=1;i<=g_MaxClients;i++)		// We check if steam id matches or his name ( Case sensitive search on name)
		{
			if(g_IsConnected[i] == true)
			{
				const char *Name = GetPlayerName(i);

				if(stricmp(pTargetInfo,g_UserInfo[i].Steamid) == 0 || strcmp(pTargetInfo,Name) == 0 )	// Name or steamid matches TargetInfo so we return the index of the player
					return i;
			}
		} 
		for(int i=1;i<=g_MaxClients;i++)	// We check if the name matches the TargetInfo, we use a case insensitive check. 
		{
			if(g_IsConnected[i] == true)
			{
				const char *Name = GetPlayerName(i);

				if(stricmp(pTargetInfo,Name) == 0 )	// Name matches TargetInfo so we return the index of the player
					return i;
			}
		}

		// Check for partial name match
		char TempTarget[64];
		_snprintf(TempTarget,63,"%s",pTargetInfo);
		char *pTargetText = TempTarget;

		pTargetText = strlwr(pTargetText);

		int found = 0;
		int foundID;
		for(int i=1;i<=g_MaxClients;i++)
		{
			if(g_IsConnected[i] == true)
			{
				char tName[64];
				_snprintf(tName,63,"%s",GetPlayerName(i));
				const char *pName = strlwr(tName); //convert to lowercase for insensitive search

				if(strstr(pName,pTargetText) != NULL) // Check for a match, if found, 
				{
					found++;  //count it and record ID
					foundID = i;

					if(found > 1) // check to see if there have been previous matches
					{
						break; // found more than one ID, break the loop
					}
				}
			}
		}
		if(found == 1)
		{
			return foundID;
		}
		else if(found > 1)
		{
			// Found more than one, dont take action on ID. Perhaps inform admin of non-unique string
			return -2;
		}
	}
	return -1;
}

void BATCore::ServerCommand(const char *fmt, ...)
{
	static char buffer[512]; // Made it static, for evil sake. This way when going back in the call browser the information is correct
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(buffer, sizeof(buffer)-2, fmt, ap);
	va_end(ap);
	strcat(buffer,"\n");
	m_Engine->ServerCommand(buffer);
}
void BATCore::ConsolePrint(int index, const char *msg, ...)
{
	char vafmt[192];
	va_list ap;
	va_start(ap, msg);
	int len = _vsnprintf(vafmt, 191, msg, ap);
	va_end(ap);
	len += _snprintf(&(vafmt[len]),191-len," \n");

	if(index == ID_SERVER)
	{
		META_CONPRINTF(vafmt);
		return;
	}
	else if(index == 0)
	{
		for(int i=1;i<=g_MaxClients;i++) if(g_IsConnected[i] && !g_UserInfo[i].IsBot)
			m_Engine->ClientPrintf(g_UserInfo[i].PlayerEdict,vafmt);
	}
	else
		m_Engine->ClientPrintf(g_UserInfo[index].PlayerEdict,vafmt);
}
void BATCore::MessagePlayer(int index, const char *msg, ...)
{
	 RecipientFilter rf;
	 if (index > g_MaxClients || index < 0)
		 return;

	 if (index == 0)
	 {
		 rf.AddAllPlayers(g_MaxClients);
	 } else {
		 rf.AddPlayer(index);
	 }
	 if(rf.GetRecipientCount() == 0)
		 return;

	 char vafmt[240];
	 va_list ap;
	 va_start(ap, msg);
	 int len = _vsnprintf(vafmt, 239, msg, ap);
	 va_end(ap);
	 len += _snprintf(&(vafmt[len]),239-len," \n");

	 bf_write *buffer = m_Engine->UserMessageBegin(static_cast<IRecipientFilter *>(&rf), g_ModSettings.SayText);
	 buffer->WriteByte(0);
	 buffer->WriteString(vafmt);
	 buffer->WriteByte(0);		// 0 = seems to mean it dont parse the text for colors , it does not parse for text
	 m_Engine->MessageEnd();
}
void BATCore::MessageAdmins(const char *msg, ...)
{
	RecipientFilter rf;
	for(int i=1;i<=g_MaxClients;i++)
	{
		if(g_IsConnected[i] && HasAccess(i,ADMIN_CHAT,false))
			rf.AddPlayer(i);
	}
	if(rf.GetRecipientCount() == 0)
		return;
	
	char vafmt[240];
	va_list ap;
	va_start(ap, msg);
	int len = _vsnprintf(vafmt, 239, msg, ap);
	va_end(ap);
	len += _snprintf(&(vafmt[len]),239-len," \n");

	bf_write *buffer = m_Engine->UserMessageBegin(static_cast<IRecipientFilter *>(&rf), g_ModSettings.SayText);
	buffer->WriteByte(0);
	buffer->WriteString(vafmt);
	buffer->WriteByte(0);		// 0 = seems to mean it dont parse the text for colors , it does not parse for text
	m_Engine->MessageEnd();
}
void BATCore::EmitSound(int SoundRecivers,int SoundStartPoint,const char *SoundName)
{
	RecipientFilter rf;
	if (SoundRecivers > g_MaxClients || SoundRecivers < 0)
		return;

	if (SoundRecivers == 0)
	{
		rf.AddAllPlayers(g_MaxClients);
	} else {
		rf.AddPlayer(SoundRecivers);
	}
	const int SoundChannel = 1; // Who knows, magic perhaps.
	m_Sound->EmitSound(rf,SoundStartPoint,SoundChannel,SoundName,1.0,0);
}

void BATCore::GetLogFileName()		// This function gets the actual mod name, and not the full pathname
{
	char FileName[256],date[32];

	time_t td; time(&td);
	strftime(date, 31, "%m-%d-%Y", localtime(&td));

	_snprintf(FileName,255,"%slogs/adminlog_%s.log",g_BasePath,date);

#ifdef  WIN32_LEAN_AND_MEAN
	StrReplace(FileName,"/","\\",255);
#endif
	//_snprintf(FileName,len,"%s/addons/bat/logs/adminlog.log",path);
	
	if(g_LogFileStream)		// We close the old file stream, just to be safe
		fclose(g_LogFileStream);

	g_LogFileStream = fopen(FileName, "at"); // "wt" clears the file each time

#if BAT_DEBUG  == 1
	g_BATCore.AddLogEntry("BAT Debug: New log file is: %s",FileName);
#endif
}

void BATCore::ExecBATCfg()
{
	//ServerCommand("exec bat/config.cfg game/addons\n ");
	time_t seconds;
	seconds = time (NULL);
	if((g_tLastBATExecTime+3) > seconds)
		return;

	ServerCommand("exec %s",g_BATConfigFile);
	g_tLastBATExecTime = seconds;
}
void BATCore::ExecMapCfg()
{
	char temp[786], tdir[512];	
	g_BATCore.GetEngine()->GetGameDir(tdir,512);
	_snprintf(temp,785,"%s/cfg/mapconfig/%s.cfg",tdir,g_CurrentMapName);
	
	FILE *in = fopen(temp,"rt");
#if BAT_DEBUG == 1
	if (in == NULL)
	{
		AddLogEntry("ERROR: Unable to find map config file for %s ( should be in %s )",g_CurrentMapName,temp);
		return;
	}
#endif
	if(in != NULL)
	{
			META_LOG(g_PLAPI,"Loading config file for %s",g_CurrentMapName);
			fclose(in);
			ServerCommand("exec mapconfig/%s.cfg",g_CurrentMapName);
	}
}
const char *BATCore::GetPlayerName(int id)
{
	if(id == ID_SERVER)
		return g_BATVars.GetHostName();

	return m_Engine->GetClientConVarValue(id,"name");
}
int BATCore::GetDaysSince1970()
{
	time_t seconds;
	seconds = time (NULL);

	return seconds/86400;
}

void BATCore::ReloadSettings()
{
	ExecMapCfg();
	ExecBATCfg();
	//m_Engine->ServerExecute();

	m_Translation->UpdateCurLanguage();
	g_BATVars.GetSQLInfo();

	// We now start or stop the SQL thread as needed, no point having it spinning if its not used or waiting for SQL querys that dont do anything
	if(g_SQLStatus.Enabled && !g_SQLStatus.ThreadRunning)
	{
		m_BATSQL->StartSQLThread();
	}else if(!g_SQLStatus.Enabled && g_SQLStatus.ThreadRunning)
		m_BATSQL->ExitSQLThread();

	if(g_SQLStatus.Enabled == false)
		m_LoadAdminAccounts->ReadAdminFile();
	else
	{
		switch(g_SQLSettings.DataBaseType)		// Getting server id if needed
		{
		case AmxBans:
			
			break;

		case SourceBans:
			BATSQL::GetServerIDSourceBans();
			break;
		}
	}

	m_BATMaps->LoadMapFiles();
	m_MenuRconCmds->ReadMenuCmdsFile();
	m_PublicMessage->ReadPublicMessagesFile();

	for(int i=1;i<=g_MaxClients;i++)
	{
		if(g_IsConnected[i] || g_IsConnecting[i])
		{
			g_UserInfo[i].SteamIdStatus = STEAM_ID_WAIT_FOR_SETUP;		
			g_ActiveIDChecks++;
		}
	}
}
void BATCore::AddLogEntry(const char * LogInfo, ...)
{
	if(g_LogBufferCount >= MAX_LOGBUFFER-1)
		WriteLogBuffer();

	char vafmt[MAX_LOGLEN+1];
	va_list ap;
	va_start(ap, LogInfo);
	_vsnprintf(vafmt,MAX_LOGLEN,LogInfo,ap);
	va_end(ap);

	char date[32];
	time_t td; time(&td);
	strftime(date, 31, "%m/%d/%Y - %H:%M:%S", localtime(&td));
	
	if(g_SQLStatus.Enabled == true)
	{
		m_BATSQL->BufferLogInformation(vafmt);
	}
	_snprintf(g_LogBuffer[g_LogBufferCount],MAX_LOGLEN,"%s - %s\n",date,vafmt);
	//ServerCommand("echo %s",g_LogBuffer[g_LogBufferCount]);
	META_CONPRINTF(g_LogBuffer[g_LogBufferCount]);
	g_LogBufferCount++;
}

void BATCore::WriteLogBuffer()
{
	if(g_LogBufferCount == 0)
		return;

	if (!g_LogFileStream)
	{
		META_CONPRINTF("[BAT]!ERROR! When trying to write log buffer to log file, information will be lost");
		return;
	}
	
	if(g_SQLStatus.Enabled && !g_SQLStatus.MoveLogInfo)
	{
		if(g_UseLogBuffer1) // We start using the other SQL log buffer
			g_UseLogBuffer1 = false;
		else
			g_UseLogBuffer1 = true;

		g_SQLStatus.MoveLogInfo = true;

		if(g_SQLStatus.ThreadSuspended)
		{
			m_BATSQL->ResumeSqlThread();
		}
	}

	for(unsigned int i=0;i<g_LogBufferCount;i++)
		fputs(g_LogBuffer[i],g_LogFileStream);

	g_LogBufferCount = 0;
}
void BATCore::SetupBasePath()
{
	char tdir[256];
	g_BATCore.GetEngine()->GetGameDir(tdir,255);
	//int StrLen = _snprintf(g_BasePath,255,"%s",tdir);

	const char *ChangedStartArgs = strstr(Plat_GetCommandLine(),"admin_basepath");

	if(ChangedStartArgs != NULL)
	{
		char TempArray[256];
		ChangedStartArgs += 15;

		int StrLen = strlen(ChangedStartArgs);
		int IOT = GetFirstIndexOfChar((char *)ChangedStartArgs,StrLen-1,' ');
		if(IOT == -1)
			_snprintf(TempArray,255,"%s",ChangedStartArgs);
		else
		{
			_snprintf(TempArray,IOT,"%s",ChangedStartArgs);
			TempArray[IOT] = '\0';
		}

		StrTrim(TempArray);
		if(TempArray[0] != '\\' && TempArray[0] != '/')
		{
			_snprintf(g_BasePath,255,"\\%s",TempArray);
			_snprintf(TempArray,255,"%s",g_BasePath);
		}

		StrLen = strlen(TempArray);
		if(TempArray[StrLen] != '\\' && TempArray[StrLen] != '/')
		{
			TempArray[StrLen] = '\\';
			TempArray[StrLen + 1] = '\0';
		}

		_snprintf(g_BasePath,255,"%s%s",tdir,TempArray);
	}
	else
	{
		_snprintf(g_BasePath,255,"%s/addons/bat/",tdir);
	}

#if defined WIN32_LEAN_AND_MEAN
	StrReplace(g_BasePath,"/","\\",255);
#else
	StrReplace(g_BasePath,"\\","/",255);
#endif
	// Now we check if we need to change the config file to
	ChangedStartArgs = strstr(Plat_GetCommandLine(),"admin_configfile");
	if(ChangedStartArgs != NULL)
	{
		ChangedStartArgs += 17;

		int StrLen = strlen(ChangedStartArgs);
		int IOT = GetFirstIndexOfChar((char *)ChangedStartArgs,StrLen,' ');

		if(IOT == -1)
			_snprintf(g_BATConfigFile,63,"%s",ChangedStartArgs);
		else
			_snprintf(g_BATConfigFile,IOT,"%s",ChangedStartArgs);

		StrTrim(g_BATConfigFile);
	}
	else
		_snprintf(g_BATConfigFile,63,"BATConfig.cfg");
}

void BATCore::GetFilePath(const char *FileName,char *NewFilePath)
{
	//char tdir[256];
	//g_BATCore.GetEngine()->GetGameDir(tdir,255);
	
	//int StrLen = 
	_snprintf(NewFilePath,255,"%s%s",g_BasePath,FileName);

	//strcpy(FileName,tdir);
}

/***************************************** Bellow here comes the String util functions ***************************************************/
char* StrUtil::StrRemoveQuotes(char *text)
{
	int len = strlen(text);

	for(int i=0;i<len;i++)
	{
		if(text[i] == '\"')
			text[i] = NULL;
	}
	return text;
}
int StrUtil::GetFirstIndexOfChar(char *Text,int MaxLen,unsigned char t)
{
	int TextIndex = 0;
	bool WSS = false; // WhiteSpaceSearch
	if(StrIsSpace(t))
		WSS = true;

	if(!WSS)
	{
		while(TextIndex <= MaxLen)
		{
			if(Text[TextIndex] == t)
				return TextIndex;
			
			TextIndex++;
		}
	}
	else
	{
		while(TextIndex <= MaxLen)
		{
			if(StrIsSpace(Text[TextIndex]))
				return TextIndex;

			TextIndex++;
		}
	}

	return -1;
}
int StrUtil::StrReplace(char *str, const char *from, const char *to, int maxlen) // Function from sslice 
{
	char  *pstr   = str;
	int   fromlen = Q_strlen(from);
	int   tolen   = Q_strlen(to);
	int	  RC=0;		// Removed count

	while (*pstr != '\0' && pstr - str < maxlen) {
		if (Q_strncmp(pstr, from, fromlen) != 0) {
			*pstr++;
			continue;
		}
		Q_memmove(pstr + tolen, pstr + fromlen, maxlen - ((pstr + tolen) - str) - 1);
		Q_memcpy(pstr, to, tolen);
		pstr += tolen;
		RC++;
	}
	return RC;
}
void StrUtil::StrTrim(char *buffer)
{
	StrTrimLeft(buffer);
	StrTrimRight(buffer);	
}
void StrUtil::StrTrimRight(char *buffer)
{
	// Make sure buffer isn't null
	if (buffer)
	{
		// Loop through buffer backwards while replacing whitespace chars with null chars
		for (int i = (int)strlen(buffer) - 1; i >= 0; i--)
		{
			if (StrIsSpace(buffer[i]))
				buffer[i] = '\0';
			else
				break;
		}
	}
}
bool StrUtil::StrIsSpace(unsigned char b)
{
	switch (b)
	{
	case ' ': 
		return true;
	case '\n':
		return true;
	case '\r':
		return true;
	case '\0': 
		return true;
	}
	return false;
}
/*
More info can be found here: Hashes
http://en.wikipedia.org/wiki/Duff's_device
*/
unsigned long StrUtil::StrHash(register const char *str, register int len) 
{
	register unsigned long n = 0;

#define HASHC	n = *str++ + 65587 * n  // The other number is better somehow, magic i guess ( Old num: 65599 )

	if (len > 0) {
		register int loop = (len + 8 - 1) >> 3;

		switch(len & (8 - 1)) {
		case 0:	do {
			HASHC;	case 7:	HASHC;
		case 6:	HASHC;	case 5:	HASHC;
		case 4:	HASHC;	case 3:	HASHC;
		case 2:	HASHC;	case 1:	HASHC;
				} while (--loop);
		}
	}
	return n;
}
void StrUtil::StrTrimLeft(char *buffer)
{
	// Let's think of this as our iterator
	char *i = buffer;

	// Make sure the buffer isn't null
	if (i && *i)
	{
		// Add up number of whitespace characters
		while(StrIsSpace(*i))
			i++;

		// If whitespace chars in buffer then adjust string so first non-whitespace char is at start of buffer
		// :TODO: change this to not use memcpy()!
		if (i != buffer)
			memcpy(buffer, i, (strlen(i) + 1) * sizeof(char));
	}
}
/*
// A class that enables you to start strings alphabetically

void StrUtilAlphaSort::AddString(char *NewStr)
{
	g_StrList.push_back(new StrAlphaSortStuct);	
	g_StrList[g_StrListCount]->TheStr = NewStr;
	g_StrList[g_StrListCount]->Index = -1;
	g_StrListCount++;
}
void StrUtilAlphaSort::SortList()
{
	for(int i=0;i<g_StrListCount;i++)
	{
		g_StrList[g_StrListCount]->Index = -1;
	}
}
int StrUtilAlphaSort::GetStringIndex(char *NewStr)
{
	for(int i=0;i<g_StrListCount;i++)
	{
		if(strcmp(NewStr,g_StrList[i]->TheStr) == 0)
		{
			//g_StrList[i]
		}
	}
	return 0;
}
*/