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
* ============================
Todo:
	Support silent move to spectator
	Support for reshowing menus in tf2
	check for duplicate maps
	menu rcon with typ2 menu
	make votekick voteban use adminsay when someone activates it?
	Can you override one votekick with anohter?


	Use _beginthread() on windows instead?
	remove hardcoded g_LogBuffer:
	With X amount of SQL errors disable the SQL system and revert to config files
	Add a system to allow other plugins to register commands.

Hardcoded things:
	Team names ( If unkown mod or unexpected number, its just gonna show the number as the team name) When adding new update GetTeamName()
	Team max count of 3
	Id of ShowMenu is hardcoded pr mod

	Changes
	votekick/voteban can no longer target yourself
	bat_interfacesourcemod  "0 = Disabled\n1=Gets admin rights from sourcemod\n2=Sets admin rights in sourcemod
*/

#include "BATCore.h"
#include "BATSQL.h"
#include "BATMaps.h"
#include "BATMenu.h"
#include "TaskSystem.h"
#include "PublicMessages.h"
#include "AdminCommands.h"
#include "Translation.h"
#include "AdminCmdManager.h"
#include "ReservedSlotsSystem.h"
#include "BATInterface.h"
#include "cvars.h"
#include "AdminMenu.h"
#include "MapVote.h"
#include "const.h"
#include "shareddefs.h"
#include <time.h>
#include "MenuRconCmds.h"
#include "MessageBuffer.h"
#include "SourceModInterface.h"

#include "hl2sdk/recipientfilters.h"
#include <bitbuf.h>

//#include <inetchannelinfo.h>
//This has all of the necessary hook declarations.  Read it!

BATCore g_BATCore;
BATVars g_BATVars;
BATMenuMngr g_MenuMngr;

AdminCmd *m_AdminCmds;
MapVote *m_MapVote; //	*MapVote = new MapVoteMenu;
PublicMessages *m_PublicMessage;
ReservedSlotsSystem *m_ReservedSlots;
AdminCmdManager *m_AdminCmdMngr;
BATMaps *m_BATMaps;
BATSQL *m_BATSQL;
MenuRconCmds *m_MenuRconCmds;
Translation *m_Translation;
TaskSystem *m_TaskSystem;
AdminMenu *m_AdminMenu;
MapVote *m_MapVoteMenu;
MessageBuffer *m_MessageBuffer;


int g_MaxClients = 0;
int g_AdminMenu = 0;
int g_MapVoteMenu = 0;
bool g_SettingsLoaded = false;
bool g_HasPrecachedSounds;

// CSS Specific stuff: 
float g_RoundEndTime;		// The time the round ended last

int g_IndexOfLastUserCmd = 0;
CCommand g_LastCCommand;

// Since i dident make a proper task system, we use a few floats to remember a few times
float g_FlTime = 0.0f;
float g_GameStartTime = 0.0f;
float g_TimeLeft = 0.0f;
float g_ChangeMapTime = 0.0f;
float g_NextPublicMsgTime = 0.0f;

float g_VoteStartTime; // Used by mods that start their vote on a custom time, instead of at the end of the map

float g_fMapChangeTime;
bool g_bChangeMapCountDown=false;
int g_PublicVoteCmdsMode = 0;

extern SourceHook::CVector<AdminInterfaceListnerStruct *>g_CallBackList;
extern unsigned int g_CallBackCount;

extern int g_CurrentMapIndex;
extern SourceHook::CVector<MapStuct *>g_MapList;
extern char g_CurrentMapName[MAX_MAPNAMELEN+1];
extern char g_NextMapName[MAX_MAPNAMELEN+1]; // Used if the map is out of map cycle
extern int g_CurrentMapExtendTimes;
extern int g_MapExtendOption;

extern int g_PubMsgStatus;
extern unsigned int g_LogBufferCount;
extern unsigned int g_ActiveIDChecks;

// Used by Reserved slots system
extern int g_PlayerCount;
extern int g_ReservedSlotsSystem;

extern bool g_IsViaSay; // Used by the AdminManager to know if it should report errors via chat or console

extern FILE *g_LogFileStream;

bool g_IsConnected[MAXPLAYERS+1];
bool g_IsConnecting[MAXPLAYERS+1];
char g_ModName[32];

int g_ReShowCSay=0; // Used by the Csay system, to reshow old messages, as they stay so low on the orignal time
char g_LastCSayMsg[255];

extern int g_MenuPos[MAXPLAYERS+1];
bool g_RunVoteOnStartOfMap;

int g_TeamWinCount[MAX_TEAMS];

AdminCommandStruct g_AdminCommands[MAX_ADMINCOMMANDS+1];
ConstPlayerInfo g_UserInfo[MAXPLAYERS+2];

// SQL Settings & status 
stSQLSettings g_SQLSettings;
stSQLStatus g_SQLStatus;
int g_LastUsersIniGeneration; // Stores the number of days since 1970 something.

extern VoteInfoStruct VoteInfo;
extern MyListener g_Listener;

#if BAT_ORANGEBOX == 1
ICvar *g_pCVar = NULL;
#else
ICvar *g_pCVar = NULL;
#endif

/*********************************** declare our sourcemm hooks*************************************/
SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);
SH_DECL_HOOK2_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, edict_t *, bool);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *, int);
SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent *, bool);

#if BAT_ORANGEBOX == 1
SH_DECL_HOOK2_void(IServerGameClients, NetworkIDValidated, SH_NOATTRIB, 0, const char *, const char *);
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);
SH_DECL_HOOK1_void(ConCommand, Dispatch, SH_NOATTRIB, 0,const CCommand &);
#else
SH_DECL_HOOK1_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *);
//SH_DECL_HOOK2_void(IServerGameClients, NetworkIDValidated, SH_NOATTRIB, 0, const char *, const char *);
SH_DECL_HOOK0_void(ConCommand, Dispatch, SH_NOATTRIB, 0);
#endif

PLUGIN_EXPOSE(BATCore, g_BATCore);

#if BAT_SOURCEMM_16 == 1
ICvar *GetICVar()
{
#if BAT_ORANGEBOX == 1
	return (ICvar *)((g_SMAPI->GetEngineFactory())(CVAR_INTERFACE_VERSION, NULL));
#else
	return (ICvar *)((g_SMAPI->GetEngineFactory())(VENGINE_CVAR_INTERFACE_VERSION, NULL));
#endif
}
#endif

bool BATCore::LevelInit(const char *pMapName, const char *pMapEntities, const char *pOldLevel, const char *pLandmarkName, bool loadGame, bool background)
{
	_snprintf(g_CurrentMapName,sizeof(g_CurrentMapName)-1,"%s",pMapName);
	g_CurrentMapIndex = GetBATMaps()->GetMapIndex(pMapName);
	g_NextMapIndex = GetBATMaps()->GetNextmap();

	if(!g_MapList.empty() && g_NextMapIndex != -1)
		_snprintf(g_NextMapName,MAX_MAPNAMELEN,"%s",g_MapList[g_NextMapIndex]->MapName);

	LoadBATSettings(false);
	RETURN_META_VALUE(MRES_IGNORED, true);
}
void BATCore::LevelShutdown( void )
{
	if(g_LogBufferCount > 0 && g_SettingsLoaded)
		WriteLogBuffer();

	m_MapVote->ResetTimeLimit();
	m_TaskSystem->ClearTaskList();
	RETURN_META(MRES_IGNORED);
}
void BATCore::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	if(!g_HasPrecachedSounds)
	{
		g_HasPrecachedSounds = true;
		m_Sound->PrecacheSound(SOUND_SLAP,true);
		m_Sound->PrefetchSound(SOUND_SLAP);

		m_Sound->PrecacheSound(SOUND_SHOWVOTE,true);
		m_Sound->PrefetchSound(SOUND_SHOWVOTE);

		m_Sound->PrecacheSound(SOUND_INVALID,true);
		m_Sound->PrefetchSound(SOUND_INVALID);

		m_Sound->PrecacheSound(SOUND_CHOICEMADE,true);
		m_Sound->PrefetchSound(SOUND_CHOICEMADE);
	}

	g_MaxClients = clientMax;
	RETURN_META(MRES_IGNORED);
}
void BATCore::ClientDisconnect(edict_t *pEntity)
{
	int id = m_Engine->IndexOfEdict(pEntity);
	m_TaskSystem->RemoveClientTasks(id);

	//META_LOG(g_PLAPI, "ClientDisconnect called: pEntity=%d", pEntity ? m_Engine->IndexOfEdict(pEntity) : 0);
	if(HasAccess(id,ADMIN_ANY))
	{
		const char *Name = GetPlayerName(id);
		const char *Steamid =  m_Engine->GetPlayerNetworkIDString(pEntity);

		AddLogEntry("%s<%s><%s> has disconnected (Account name: %s Account flags: %s)",Name,Steamid,g_UserInfo[id].IP,g_UserInfo[id].AdminName,g_UserInfo[id].AdminFlags);
		
		if(HasAccess(id,ADMIN_RESERVEDSLOTS))
		{
			if(g_BATVars.GetReservedSlots() == 2)
				m_ReservedSlots->UpdateVisibleSlots();

			g_ReservedSlotsUsers--;
		}
	}
	g_UserInfo[id].AdminAccess = 0;
	g_UserInfo[id].SteamIdStatus = STEAM_ID_NOTVALID;

	g_IsConnected[id] = false;
	g_IsConnecting[id] = false;
	g_PlayerCount--;

	RETURN_META(MRES_IGNORED);
}

bool BATCore::ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
	//META_LOG(g_PLAPI, "ClientConnect called: pEntity=%d, pszName=%s, pszAddress=%s", pEntity ? m_Engine->IndexOfEdict(pEntity) : 0, pszName, pszAddress);
	g_PlayerCount++;
	int id = m_Engine->IndexOfEdict(pEntity);
	int len = strlen(pszAddress);

	g_IsConnecting[id] = true;
	g_ActiveIDChecks++;
	g_UserInfo[id].SteamIdStatus = STEAM_ID_NOTVALID;
	g_UserInfo[id].GagStatus = GAG_NONE;
	g_UserInfo[id].AdminAccess = -1;
	g_UserInfo[id].AdminAccess = 0;
	g_UserInfo[id].AdminFlags[0] = '\0'; 

	_snprintf(g_UserInfo[id].IP,39,pszAddress);
	
	for(int i=0;i<len;i++)
	{
		if(pszAddress[i] == ':')
		{
			g_UserInfo[id].IP[i] = '\0';
			break;
		}
	}
//	META_LOG(g_PLAPI, "ClientConnect %s",g_g_UserInfo[id].IP);
	RETURN_META_VALUE(MRES_IGNORED, true);
}
void BATCore::ClientPutInServer(edict_t *pEntity, char const *playername)
{
	 if(!pEntity || pEntity->IsFree()) 
		 return;

	int id = m_Engine->IndexOfEdict(pEntity);
	g_IsConnected[id] = true;
	g_IsConnecting[id] = false;
	if(g_UserInfo[id].SteamIdStatus == STEAM_ID_NOTVALID)	// The player does not have a valid steamid, so his info has not been saved by the Steamid validation system. So we do it now
		SetupPlayerInfo(id);

	if(g_UserInfo[id].IsBot)		// Evil hack: ClientConnect is not called on bots ( This could get all messy if there a hltv around )
		g_PlayerCount++;
	
	g_UserInfo[id].ConnectTime = g_FlTime;	
	g_UserInfo[id].AntiFlood = g_FlTime + WAITTIME_CHATCMDS;	
	g_UserInfo[id].LastPublicVoteStart = g_FlTime + WAITTIME_PUBVOTE;	

#if BAT_DEBUG == 1
	META_LOG(g_PLAPI, "ClientPutInServer called: pEntity=%d, playername=%s UserID=%d AdminAccess=%d (Is Admin: %d) IsNoneHumanPlayer=%d", pEntity ? m_Engine->IndexOfEdict(pEntity) : 0, playername,g_UserInfo[id].Userid,g_UserInfo[id].AdminAccess,g_UserInfo[id].AdminIndex,g_UserInfo[id].IsBot);
#endif
	RETURN_META(MRES_IGNORED);
}


#if BAT_ORANGEBOX == 1
void BATCore::ClientCommand(edict_t *pEntity, const CCommand &args)
{
#else
void BATCore::ClientCommand(edict_t *pEntity)
{
	CCommand args;
#endif
	int id = m_Engine->IndexOfEdict(pEntity);
	g_LastCCommand = args;

	const char *command = g_LastCCommand.Arg(0);

	if(VoteInfo.Method != VOTEMETHOD_MENU && !HasAccess(id,ADMIN_ANY) )
		RETURN_META(MRES_IGNORED);

	if (strcmp(command,"menuselect") == 0)
	{
		if(!g_MenuMngr.HasMenuOpen(id))
			RETURN_META(MRES_IGNORED);

		const char *arg1 = g_LastCCommand.Arg(1);
		//catch menu commands
		if (arg1)
		{
			int arg = atoi(arg1);
			if(arg < 1 || arg > 10) // Make sure makes no invalid selection.
				return;

			g_MenuMngr.MenuChoice(id, arg);
			RETURN_META(MRES_SUPERCEDE);
		}
		RETURN_META(MRES_IGNORED);
	}

	if(!HasAccess(id,ADMIN_ANY))		// If the user does no have admin, no point in the rest of the checks.
		RETURN_META(MRES_IGNORED);

	if(m_AdminCmdMngr->CheckCmd(id,command))
		RETURN_META(MRES_SUPERCEDE);
	else
		RETURN_META(MRES_IGNORED);

	//META_LOG(g_PLAPI, "ClientCommand called: pEntity=%d (commandString=%s)", pEntity ? m_Engine->IndexOfEdict(pEntity) : 0, m_Engine->Cmd_Args() ? m_Engine->Cmd_Args() : "");
	RETURN_META(MRES_IGNORED);
}

#if BAT_ORANGEBOX == 1
void BATCore::ClientSay(const CCommand &args)
{
#else
void BATCore::ClientSay()
{
	CCommand args;
#endif

	int id = g_IndexOfLastUserCmd;
	g_LastCCommand = args;

	if(g_UserInfo[id].GagStatus & GAG_TEXTCHAT) // If the guy is muted, we block it
	{
		g_BATCore.MessagePlayer(id,m_Translation->GetTranslation("GagedWarrning"));
		RETURN_META(MRES_SUPERCEDE); 
	}

	bool isTeamSay = false;
	char FirstWord[192];
	_snprintf(FirstWord,191,"%s",g_LastCCommand.Arg(1));
	char *pFirstWord = (char *)FirstWord;
	int TextLen = strlen(pFirstWord);

	const char *SayCmd = g_LastCCommand.Arg(0);
	if(strcmp(SayCmd,"say_team") == 0)
		isTeamSay = true;

	int StartArg = 1;
	if(g_ModSettings.GameMod == MOD_EMPIRES && isTeamSay)
	{
		FirstWord[0] = ' ';
		FirstWord[1] = ' ';
		FirstWord[2] = ' ';
		FirstWord[3] = ' ';
		g_BATCore.StrTrim(FirstWord);
	}	
	else if(g_ModSettings.GameMod == MOD_INSURGENCY)
	{
		/*
		(02:11:08) ([INS]Louti) Q_snprintf( szSendText, sizeof( szSendText ), "say2 %i %i %s", iType, bThirdPerson ? 1 : 0, pszHandledText );
		(02:11:36) ([INS]Louti) enum SayTypes_t
		(02:11:36) ([INS]Louti) {
		(02:11:36) ([INS]Louti) 	SAYTYPE_INVALID = -1,
		(02:11:36) ([INS]Louti) 	SAYTYPE_SERVER = 0,
		(02:11:36) ([INS]Louti) 	SAYTYPE_GLOBAL,
		(02:11:36) ([INS]Louti) 	SAYTYPE_TEAM,
		(02:11:38) ([INS]Louti) 	SAYTYPE_SQUAD,
		(02:11:41) ([INS]Louti) 	SAYTYPE_COUNT
		(02:11:43) ([INS]Louti) };

		
		Arg0 say2 Arg1 1 Arg2 0 Arg3 MarineRegular
		Arg0 say2 Arg1 2 Arg2 0 Arg3 MarineTeamSay

		Arg0 say2 Arg1 1 Arg2 0 Arg3 IraqRegularSay
		Arg0 say2 Arg1 2 Arg2 0 Arg3 IraqTeamSAy
		

		const char *Arg0 = g_LastCCommand.Arg(0);
		const char *arg1 = g_LastCCommand.Arg(1);
		const char *arg2 = g_LastCCommand.Arg(2);

		const char *arg3 = g_LastCCommand.Arg(3);
		const char *arg4 = g_LastCCommand.Arg(4);
		const char *arg5 = g_LastCCommand.Arg(5);
		
		ServerCommand("echo Arg0 %s Arg1 %s Arg2 %s Arg3 %s",Arg0,arg1,arg2,arg3);
		*/
		int SayType = atoi(g_LastCCommand.Arg(1));

		if(SayType != 1 && SayType != 2)
			return; // This was some odd say type like squad / server say or whatever we dont care but we dont handle it.

		_snprintf(FirstWord,191,"%s",g_LastCCommand.Arg(3));
		pFirstWord = (char *)FirstWord;
		TextLen = strlen(pFirstWord);
		StartArg = 3;
	}
	

	if(g_PublicVoteCmdsMode != 0)
	{
		//WAITTIME_CHATCMDS

		if (TextLen > 8 && strncmp("votekick",pFirstWord+1,8) == 0)
		{
			if(g_BATCore.HasAccess(id,ADMIN_KICK))
				m_AdminCmdMngr->CheckSayCmd(id,"votekick");
			else
			{
				if(g_PublicVoteCmdsMode != 1 && g_PublicVoteCmdsMode != 3) 
					return; // votekick is not enabled, so none admins cannot use it.

				if(g_FlTime < g_UserInfo[id].LastPublicVoteStart)
					return; // Block to prevent Spam

				g_UserInfo[id].AdminAccess += ADMIN_KICK;
				m_AdminCmdMngr->CheckSayCmd(id,"votekick");
				g_UserInfo[id].AdminAccess -= ADMIN_KICK;
				
				g_UserInfo[id].LastPublicVoteStart = g_FlTime + WAITTIME_PUBVOTE;
				MessageAdmins("%s<%s> started a vote kick",GetPlayerName(id),g_UserInfo[id].Steamid);
			}
			RETURN_META(MRES_SUPERCEDE);
		}
		else if (TextLen > 7 && strncmp("voteban",pFirstWord+1,7) == 0)
		{
			if(g_BATCore.HasAccess(id,ADMIN_BAN))
				m_AdminCmdMngr->CheckSayCmd(id,"voteban");
			else
			{
				if(g_PublicVoteCmdsMode != 2 && g_PublicVoteCmdsMode != 3) 
					return; // voteban is not enabled, so none admins cannot use it.

				if(g_FlTime < g_UserInfo[id].LastPublicVoteStart)
					return; // Block to prevent Spam

				g_UserInfo[id].AdminAccess += ADMIN_BAN;
				m_AdminCmdMngr->CheckSayCmd(id,"voteban");
				g_UserInfo[id].AdminAccess -= ADMIN_BAN;

				g_UserInfo[id].LastPublicVoteStart = g_FlTime + WAITTIME_PUBVOTE;
				MessageAdmins("%s<%s> started a vote ban",GetPlayerName(id),g_UserInfo[id].Steamid);
			}
			RETURN_META(MRES_SUPERCEDE);
		}
	}

	
	if(pFirstWord[0] == '/')
		pFirstWord = pFirstWord +1;
	
	// int test = strncmp(firstword,"/votekick",9+32);
	//g_BATCore.ServerCommand("echo Firstword: %s sec: %s (Arg count: %d",firstword,g_LastCCommand.Arg(2),g_LastCCommand.ArgC()());
	if (FirstWord[0] == '#')
	{
		if(!g_BATCore.HasAccess(id,ADMIN_ANY))
			RETURN_META(MRES_SUPERCEDE);
	
		for(int i=0;i<TextLen;i++)
		{
			if(g_BATCore.StrIsSpace(FirstWord[i]))
			{
				FirstWord[i] = '\0';
				break;
			}
		}
		FirstWord[0] = ' '; // remove the #
		g_BATCore.StrTrimLeft(FirstWord);
		if(strlen(FirstWord)>= MAX_CMDSIZE)		// Dont want to pass a to big command do we.
			RETURN_META(MRES_SUPERCEDE);

		m_AdminCmdMngr->CheckSayCmd(id,FirstWord);
		RETURN_META(MRES_SUPERCEDE);
	}
	else if (FirstWord[0] == '@')
	{
		if(g_BATVars.AllowNonAdminPlayersToSendAdminSay() && !isTeamSay && !g_BATCore.HasAccess(id,ADMIN_CHAT))
			RETURN_META(MRES_SUPERCEDE);

		int ArgCount = g_LastCCommand.ArgC();
		int len = 0;
		char SayMsg[192];

		if(ArgCount >= 2)
		{
			for(int i=StartArg;i<ArgCount;i++)
			{
				if(i == StartArg)
					len += _snprintf(&(SayMsg[len]),191-len,"%s",g_LastCCommand.Arg(i));
				else
					len += _snprintf(&(SayMsg[len]),191-len," %s",g_LastCCommand.Arg(i));
			}
		}
		else 
			_snprintf(SayMsg,191,"%s",g_LastCCommand.Arg(1));

		int ACount = g_BATCore.StrReplace(SayMsg,"@"," ",191);
		g_BATCore.StrReplace(SayMsg,"\"","",191);
		g_BATCore.StrTrim(SayMsg);
	
		if(isTeamSay == false)
		{
			if(ACount == 1)		// We do a normal say instead
				g_BATCore.GetAdminCmds()->Say(id,SayMsg);
			else
				g_BATCore.GetAdminCmds()->AdminTalkToAll(id,SayMsg);
		}
		else
			g_BATCore.GetAdminCmds()->AdminChat(id,SayMsg);

		RETURN_META(MRES_SUPERCEDE);            
	}
	// ---------------------------------------------- Your common functions related to none admin players like nextmap and so on
	if( !g_BATCore.HasAccess(id,ADMIN_ANY) && g_FlTime < g_UserInfo[id].AntiFlood)
		return; // Block to prevent Spam
	g_UserInfo[id].AntiFlood = g_FlTime + WAITTIME_CHATCMDS;
	
	if (TextLen > 7 && TextLen < 10 && stricmp(pFirstWord,"timeleft") == 0) 
	{
		if(!g_BATVars.PubTimeleftEnabled())
			return;

		g_BATCore.MessagePlayer(0,"Time Left: %s",g_BATCore.GetTimeleftString());
	}
	else if (TextLen > 6 && TextLen < 9 && stricmp(pFirstWord,"nextmap") == 0 )
	{
		if(g_BATVars.GetMapExtendOption() == 0)
			return;

		g_BATCore.MessagePlayer(0,"The nextmap: %s",g_NextMapName);
	}
	else if(TextLen > 6 && TextLen < 9 && stricmp(pFirstWord,"thetime") == 0 ) 
	{
		if(!g_BATVars.PubTimeleftEnabled())
			return;

		char date[32];
		time_t td; time(&td);
		strftime(date, 31, "%H:%M:%S", localtime(&td));
		g_BATCore.MessagePlayer(0,"The time: %s",date);
	}
	else if(TextLen > 4 && TextLen < 6 && stricmp(pFirstWord,"rules") == 0 ) 
	{
		if(!g_RulesURLSet)
			return; // There is to rules url set, we cannot show it to anyone then

		AdminCmd::ShowMOTD(id,g_RulesURL);
	}
	// ---------------------------------------------- Functions related to map voting via chat
	else if(VoteInfo.Method == VOTEMETHOD_CHAT)
	{
		if(strncmp(FirstWord,"vote",2) >= 0)
		{
			char MapName[MAX_MAPNAMELEN+1] = "";
			snprintf(MapName,MAX_MAPNAMELEN,"%s",FirstWord);

			if(g_BATCore.StrReplace(MapName,"vote","",MAX_MAPNAMELEN) == 0)
				return;
			
			g_BATCore.StrTrim(MapName);
			//g_BATCore.ServerCommand("echo firstword: '%s' MapName: '%s'",firstword,MapName);
		
			if(g_LastCCommand.ArgC() < 2)	
			{
				g_BATCore.MessagePlayer(id,m_Translation->GetTranslation("MapVoteSay"),g_MapList[0]->MapName);
				return;
			}
			
			g_BATCore.GetMapVote()->CheckSayVote(id,MapName);
			RETURN_META(MRES_SUPERCEDE);
		}
		else if(stricmp(pFirstWord,"maplist") == 0)
		{
			g_BATCore.GetMapVote()->PrintMapList(id);
			RETURN_META(MRES_SUPERCEDE);
		}
	}
	else if (g_BATCore.GetBATVar().GetMapRockTheVote() == 1 && ( stricmp(pFirstWord,"rockthevote") == 0 || stricmp(FirstWord,"rtv") == 0 ) )
	{
		if(VoteInfo.Status != VOTE_ROCKTHEVOTESTARTED && VoteInfo.Status != VOTE_WAITING && VoteInfo.Status != VOTE_DONE_NOVOTES)
		{
			g_BATCore.MessagePlayer(id,m_Translation->GetTranslation("CannotRockTheVote"));
			return;
		}
		g_BATCore.GetMapVote()->AddRTVVote(id);
		RETURN_META(MRES_SUPERCEDE);
	}
/*
	else
		g_BATCore.ServerCommand("echo Goes unhanded firstword: '%s'  VoteInfo.Method: %d / %d",firstword,VoteInfo.Method,strncmp(firstword,"vote",2));
*/
}

void BATCore::SetCommandClient(int index)
{
	//META_LOG(g_PLAPI, "SetCommandClient() called: index=%d", index);
	g_IndexOfLastUserCmd = index +1;
	//RETURN_META(MRES_IGNORED);
}


void BATCore::GameFrame(bool simulating)
{
	float CurTime = GetGlobals()->curtime;

	if(CurTime - g_FlTime >= TASK_CHECKTIME)
	{
		PreSamplerStart("GameFrame")

		g_FlTime = CurTime;
		g_TimeLeft = GetTimeLeft();

		if(g_ActiveIDChecks > 0)	// If we should check the players to make sure their steamid is validated ( IE some player has is STEAM_ID_PENDING as their id )
			CheckUsersID();

		m_TaskSystem->CheckTasks(CurTime);

		if(m_MessageBuffer->HasBufferedMessages())
		{
			const char * pMessageBuffer = m_MessageBuffer->GetNextBufferedMsg();

			if(pMessageBuffer != NULL)
			{
				g_BATCore.AddLogEntry(pMessageBuffer);
				MessageAdmins(pMessageBuffer);
			}
		}

		if(g_bChangeMapCountDown && g_TimeLeft <= g_fMapChangeTime)
		{
			g_bChangeMapCountDown = false;
			ServerCommand("changelevel %s",g_NextMapName);		
		}

		if(g_PubMsgStatus == PM_ENABLED && CurTime >= g_NextPublicMsgTime)
			m_PublicMessage->ShowRandomPublicMessage();

		if(g_ReShowCSay > 0) // Since Center say messages disappears really fast in CS we reshow it a few times
		{
			g_ReShowCSay--;
			SendCSay(0,g_LastCSayMsg,true);
		}

		/********************** DONT ADD CODE BELLOW THIS, VOTE CODE CAN STOP GAMEFRAME FUNCTION WHEN IT SO CHOOSES, ONLY CODE RELATED TO MapVote BELLOW HERE*/
		if(VoteInfo.Status == VOTE_WAITING  || VoteInfo.Status == VOTE_ROCKTHEVOTESTARTED)
		{
			if(g_MapExtendOption <= 0 || g_MapExtendOption >= 3)
				return; // Map voting is disabled, so no vote will be started automatically

			if(!g_RunVoteOnStartOfMap)
			{
				if(g_BATVars.GetTimelimitCvar()->GetInt() == 0)
					return; // No time limit no vote.

				// The vote should be run at the end of the map
				if(g_TimeLeft <= TimeToStartMapVote )
					m_MapVote->StartPublicMapVote(g_TimeLeft,false);
			} 
			else
			{
				// The vote should be run at the start of the map.
				if(g_FlTime >= g_VoteStartTime)
				{
					int Timelimit = g_BATVars.GetTimelimitCvar()->GetInt();
					if(Timelimit == 0) Timelimit = 30; // No timelimit, we will show the vote in 30min instead
					
					g_VoteStartTime += Timelimit * 60; // make it into seconds

					m_MapVote->StartPublicMapVote(g_TimeLeft,false);
				}
			}
		}		
		else if((VoteInfo.Status == VOTE_VOTERUNNING || VoteInfo.Status == VOTE_ROCKTHEVOTESTARTED) && g_TimeLeft <= VoteInfo.VoteEndTime)
			m_MapVote->EndMapVote();

		PreSamplerEnd("GameFrame")
		//ServerCommand("echo GameFrame() Timeleft:%.2f (g_ChangeMapTime: %.0f  VoteInfo.Status: %d)",g_TimeLeft,g_ChangeMapTime,VoteInfo.Status);
	}
	RETURN_META(MRES_IGNORED);
}
bool BATCore::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

#if BAT_SOURCEMM_16 == 1
	GET_V_IFACE_ANY(GetServerFactory, m_ServerDll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetServerFactory, m_ServerClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetServerFactory, m_InfoMngr, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);

	GET_V_IFACE_CURRENT(GetEngineFactory, m_Engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, m_GameEventManager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(GetEngineFactory, m_Helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_CURRENT(GetEngineFactory, m_Sound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);


	m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, LevelInit, m_ServerDll, SH_MEMBER(this, &BATCore::LevelInit), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, ServerActivate, m_ServerDll, SH_MEMBER(this, &BATCore::ServerActivate), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, GameFrame, m_ServerDll, SH_MEMBER(this, &BATCore::GameFrame), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameDLL, LevelShutdown, m_ServerDll, SH_MEMBER(this, &BATCore::LevelShutdown), false));
	//m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientActive, gameclients, SH_MEMBER(this, &g_BATCore::Hook_ClientActive), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientDisconnect, m_ServerClients, SH_MEMBER(this, &BATCore::ClientDisconnect), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientPutInServer, m_ServerClients, SH_MEMBER(this, &BATCore::ClientPutInServer), true));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, SetCommandClient, m_ServerClients, SH_MEMBER(this, &BATCore::SetCommandClient), true));
	//m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientSettingsChanged, gameclients, SH_MEMBER(this, &g_BATCore::Hook_ClientSettingsChanged), false));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientConnect, m_ServerClients, SH_MEMBER(this, &BATCore::ClientConnect), false));
	m_hooks.push_back(SH_ADD_HOOK(IServerGameClients, ClientCommand, m_ServerClients, SH_MEMBER(this, &BATCore::ClientCommand), false));
#else
	//GET_V_IFACE_ANY(serverFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);

	GET_V_IFACE_ANY(serverFactory, m_ServerDll, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(serverFactory, m_ServerClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(serverFactory, m_InfoMngr, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);

	GET_V_IFACE_CURRENT(engineFactory, m_Engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	GET_V_IFACE_CURRENT(engineFactory, m_GameEventManager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
	GET_V_IFACE_CURRENT(engineFactory, m_Helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_CURRENT(engineFactory, m_Sound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);

	//Hook LevelInit to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, m_ServerDll, &g_BATCore, &BATCore::LevelInit, true);				//Hook ServerActivate to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, m_ServerDll, &g_BATCore, &BATCore::ServerActivate, true);		//Hook GameFrame to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, m_ServerDll, &g_BATCore, &BATCore::GameFrame, true);				//Hook LevelShutdown to our function -- this makes more sense as pre I guess

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, m_ServerDll, &g_BATCore, &BATCore::LevelShutdown, false);		//Hook ClientActivate to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, m_ServerClients, &g_BATCore, &BATCore::ClientDisconnect, true);		//Hook ClientPutInServer to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, m_ServerClients, &g_BATCore, &BATCore::ClientPutInServer, true);	//Hook SetCommandClient to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, m_ServerClients, &g_BATCore, &BATCore::SetCommandClient, true);		//Hook ClientSettingsChanged to our function

	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, m_ServerClients, &g_BATCore, &BATCore::ClientConnect, false);	//Hook ClientCommand to our function
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, m_ServerClients, &g_BATCore, &BATCore::ClientCommand, false);	//This hook is a static hook, no member function
#endif


	m_Engine_CC = SH_GET_CALLCLASS(m_Engine); // invoking their hooks (when needed).
	SH_CALL(m_Engine_CC, &IVEngineServer::LogPrint)("[BAT] All hooks started!\n");


#if BAT_SOURCEMM_16 == 1
	g_SMAPI->AddListener(this,&g_Listener);

	g_pCVar = GetICVar();
#else
	g_SMAPI->AddListener(g_PLAPI, this);

	GET_V_IFACE_CURRENT(engineFactory, g_pCVar, ICvar, VENGINE_CVAR_INTERFACE_VERSION);
#endif	

#if BAT_ORANGEBOX == 1
	ConVar_Register(0,this);
#else
	ConCommandBaseMgr::OneTimeInit(this);
#endif
	

	if(late)
	{		
		_snprintf(g_CurrentMapName,MAX_MAPNAMELEN,"%s",GetGlobals()->mapname.ToCStr());
		LoadBATSettings(true);
		UpdatePlayerInformation();
		AddLogEntry("Warning: The plugin was late loaded this is unsafe, expect minor bugs until a map change has happens (Translation CHANGE MAP NOW)");
	}	return true;
}
bool BATCore::Unload(char *error, size_t maxlen)
{
	if(g_SQLStatus.ThreadRunning)
		m_BATSQL->ExitSQLThread();	

	while(g_SQLStatus.ThreadRunning)
	{
		// We now wait for the SQL thread to properbly exit, or we get nasty crashes well outside BAT/srcds well into the OS kernel
		g_BATCore.ServerCommand("echo BAT: Waiting for SQL thread to complete its task");
		TSleep(250);
	}

	UnloadAdminInterface();

	if(g_SMExt.IsConnected())
		g_SMExt.Disconnect();
	
	m_GameEventManager->RemoveListener(this);

	delete m_AdminMenu;
	delete m_MapVoteMenu;

	//m_GameEventManager->Reset();

	WriteLogBuffer();		// We dont any info in the Log buffer if present
	if(g_LogFileStream)	fclose(g_LogFileStream); // We close the file stream

#if BAT_SOURCEMM_16 == 1
	for (size_t i = 0; i < m_hooks.size(); i++)
	{
		if (m_hooks[i] != 0)
		{
			SH_REMOVE_HOOK_ID(m_hooks[i]);
		}
	}
	m_hooks.clear();
#else
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, m_ServerDll, &g_BATCore, &BATCore::LevelInit, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, m_ServerDll, &g_BATCore, &BATCore::ServerActivate, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, m_ServerDll, &g_BATCore, &BATCore::GameFrame, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, m_ServerDll, &g_BATCore, &BATCore::LevelShutdown, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, m_ServerClients, &g_BATCore, &BATCore::ClientDisconnect, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, m_ServerClients, &g_BATCore, &BATCore::ClientPutInServer, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, m_ServerClients, &g_BATCore, &BATCore::SetCommandClient, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, m_ServerClients, &g_BATCore, &BATCore::ClientConnect, false);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, m_ServerClients, &g_BATCore, &BATCore::ClientCommand, false);
#endif

	//this, sourcehook does not keep track of.  we must do this.
	SH_RELEASE_CALLCLASS(m_Engine_CC);

	return true;
}
bool BATCore::RegisterConCommandBase(ConCommandBase *pVar)
{
	return META_REGCVAR(pVar);;
}
void BATCore::AllPluginsLoaded()
{
	if(g_SMExt.IsConnected() && g_BATVars.GetSMInterface() != 0)
		g_SMExt.Connect();
}

void BATCore::FireGameEvent(IGameEvent *event)
{
	if (!event || !event->GetName() )
		return;

	const char *name = event->GetName();
#if BAT_DEBUG == 1
	g_BATCore.ServerCommand("echo Event: %s called",name);
#endif

	if(g_ModSettings.GameMod == MOD_CSTRIKE)
	{
		if(strcmp(name,"round_end") != 0)
			return;

		g_RoundEndTime = GetGlobals()->curtime;
		int EndReason = event->GetInt("reason");
		if(EndReason == 16)
		{
			g_GameStartTime = GetGlobals()->curtime;
		}
		
		// We now check mp_winlimit
		int TempWinLimit = g_BATVars.GetWinLimit();
		if(TempWinLimit > 0)
		{
			int WinnerTeam = event->GetInt("winner");
			if(WinnerTeam >= MAX_TEAMS || WinnerTeam <= 0)
				return; // the winning team num is wrong, stop it here or we crash and burn

			g_TeamWinCount[WinnerTeam]++;

			if(VoteInfo.Status == VOTE_DONE_WAITFOREVENT && (g_TimeLeft <= 2 || g_TeamWinCount[WinnerTeam] >= TempWinLimit))
			{
				StartChangelevelCountDown(g_NextMapName);
				VoteInfo.Status = VOTE_DONE;
			}
			else if(g_TeamWinCount[WinnerTeam] >= (TempWinLimit-2))
			{
				if((VoteInfo.Status == VOTE_WAITING  || VoteInfo.Status == VOTE_ROCKTHEVOTESTARTED) && (g_BATVars.GetMapExtendOption() > 0 && g_BATVars.GetMapExtendOption() != 3))
					m_MapVote->StartPublicMapVote(g_TimeLeft,false);
			}
		}
		else
		{
			if(VoteInfo.Status == VOTE_DONE_WAITFOREVENT && g_TimeLeft <= 2)
			{
				StartChangelevelCountDown(g_NextMapName);
				VoteInfo.Status = VOTE_DONE;
			}
		}
	}
	else if(g_ModSettings.GameMod == MOD_DOD)
	{
		if(strcmp(name,"dod_game_over") != 0)
			return;

		if(VoteInfo.Status == VOTE_DONE_WAITFOREVENT && g_TimeLeft <= 2)
		{
			StartChangelevelCountDown(g_NextMapName);
			VoteInfo.Status = VOTE_DONE;
		}
	}
	else if(g_ModSettings.GameMod == MOD_EMPIRES )
	{
		if(strcmp(name,"game_end") != 0)
			return;

		StartChangelevelCountDown(g_NextMapName);
		VoteInfo.Status = VOTE_DONE;
	}
	else if(g_ModSettings.GameMod == MOD_PVKII)
	{
		if(strcmp(name,"round_end") == 0)
			return;

		g_TeamWinCount[0]++;
		if(g_BATVars.GetRoundLimit() == g_TeamWinCount[0])
		{
			StartChangelevelCountDown(g_NextMapName);
			VoteInfo.Status = VOTE_DONE;
		}
	}
	else if(g_ModSettings.GameMod == MOD_TF2)
	{
		if(strcmp(name,"tf_game_over") == 0 || strcmp(name,"teamplay_game_over") == 0)
		{
			if(VoteInfo.Status != VOTE_DONE_WAITFOREVENT)
				return;

			const char * EndReason = event->GetString("reason");

			if(strcmp(EndReason,"Reached Time Limit") != 0 && strcmp(EndReason,"Reached Win Limit") != 0 )
			{
				g_BATCore.AddLogEntry("TF Debug: Map ended with unknown reason: %s",EndReason);
				return;
			}
			StartChangelevelCountDown(g_NextMapName);
			VoteInfo.Status = VOTE_DONE;
		}
		else
		{
			const char * EndReason = event->GetString("reason");
			if(EndReason)
				g_BATCore.AddLogEntry("TF Debug: Name: %s  Reason: %s",event->GetName(),EndReason);
			else
				g_BATCore.AddLogEntry("TF Debug: Name: %s",event->GetName());
		}

	}
}
ConCommand *BATCore::HookConsoleCmd(const char *CmdName)
{
	ConCommandBase *pCmd = g_pCVar->GetCommands();
	ConCommand* pTemp;

	while (pCmd)
	{
		if (pCmd->IsCommand() && stricmp(pCmd->GetName(),CmdName) == 0) 
		{
			pTemp = (ConCommand *)pCmd;

#if BAT_SOURCEMM_16 == 1
			m_hooks.push_back(SH_ADD_HOOK(ConCommand, Dispatch, pTemp, SH_MEMBER(this, &BATCore::ClientSay), false));
#else
			SH_ADD_HOOK_MEMFUNC(ConCommand, Dispatch, pTemp, &g_BATCore, &BATCore::ClientSay, false);
#endif
			return pTemp;
		}

		pCmd = const_cast<ConCommandBase *>(pCmd->GetNext());
	}
	return NULL;
}
void BATCore::OneTimeInitBAT()
{
	ModInfo *pModInfo = new ModInfo;
	pModInfo->GetModInformation(m_ServerDll,m_Engine);
	delete pModInfo;
	SetupBasePath();

	g_SettingsLoaded = true;
	GetLogFileName();

	g_BATVars.HookCvars();


	if(g_ModSettings.GameMod == MOD_INSURGENCY)
		pSayCmd = HookConsoleCmd("say2");	
	else
		pSayCmd = HookConsoleCmd("say");

	pSayTeamCmd = HookConsoleCmd("say_team");

	if(!g_ModSettings.DefaultRadioMenuSupport && g_BATCore.GetBATVar().GetMenuType() == 1)
	{
		ServerCommand("bat_menutype 2");
		g_BATCore.AddLogEntry("ERROR: This mod does not support Radio style menus, switching to ESC based menus ( bat_menutype 2)");
	}

	g_UserInfo[ID_SERVER].AdminAccess = ADMIN_IMMUNITY;
	_snprintf(g_UserInfo[ID_SERVER].AdminName,10,"Server");
	_snprintf(g_UserInfo[ID_SERVER].IP,39,"Localhost");
	_snprintf(g_UserInfo[ID_SERVER].Steamid,39,"Localhost");

	m_AdminMenu = new AdminMenu;
	m_MapVoteMenu = new MapVote;		
	m_ReservedSlots = new ReservedSlotsSystem;
	m_AdminCmdMngr = new AdminCmdManager;
	m_AdminCmds = new AdminCmd;
	m_BATMaps = new BATMaps;
	m_Translation = new Translation;
	m_MenuRconCmds = new MenuRconCmds;
	m_Utils = new Utils;
	m_TaskSystem = new TaskSystem;
	m_MessageBuffer = new MessageBuffer;

	m_Utils->UTIL_PrecacheOffsets();
	m_MenuRconCmds->ReadMenuCmdsFile();

	g_AdminMenu = g_MenuMngr.RegisterMenu(m_AdminMenu);
	g_MapVoteMenu = g_MenuMngr.RegisterMenu(m_MapVoteMenu);

	m_AdminCmdMngr->RegisterCommand("admin_help","CmdHelpHow","CmdHelpDisc",(CmdFuncIntArg)&AdminCmd::AdminHelp,ADMIN_ANY,0);
	m_AdminCmdMngr->RegisterCommand("admin_csay","CmdCSayHow","CmdCSayDisc",(CmdFuncIntArg)&AdminCmd::CmdCSay,ADMIN_CHAT,1);
	m_AdminCmdMngr->RegisterCommand("admin_say","CmdSayHow","CmdSayDisc",(CmdFuncIntArg)&AdminCmd::CmdSay,ADMIN_CHAT,1);
	m_AdminCmdMngr->RegisterCommand("admin_chat","CmdChatHow","CmdChatDisc",(CmdFuncIntArg)&AdminCmd::CmdAdminSay,ADMIN_CHAT,1);
	m_AdminCmdMngr->RegisterCommand("admin_name","CmdNameHow","CmdNameDisc",(CmdFuncIntArg)&AdminCmd::CmdChangeName,ADMIN_KICK,1);
	m_AdminCmdMngr->RegisterCommand("admin_kick","CmdKickHow","CmdKickDisc",(CmdFuncIntArg)&AdminCmd::CmdKick,ADMIN_KICK,1);
	m_AdminCmdMngr->RegisterCommand("admin_ban","CmdBanHow","CmdBanDisc",(CmdFuncIntArg)&AdminCmd::CmdBan,ADMIN_BAN,2);
	m_AdminCmdMngr->RegisterCommand("admin_unban","CmdUnBanHow","CmdUnBanDisc",(CmdFuncIntArg)&AdminCmd::CmdUnBan,ADMIN_BAN,1);
	m_AdminCmdMngr->RegisterCommand("admin_banip","CmdBanHow","CmdBanDisc",(CmdFuncIntArg)&AdminCmd::CmdBanIP,ADMIN_BANIP,2);
	m_AdminCmdMngr->RegisterCommand("admin_unbanip","CmdUnBanHow","CmdUnBanDisc",(CmdFuncIntArg)&AdminCmd::CmdUnBanIP,ADMIN_BANIP,1);
	m_AdminCmdMngr->RegisterCommand("admin_slay","CmdSlayHow","CmdSlayDisc",(CmdFuncIntArg)&AdminCmd::CmdSlay,ADMIN_KICK,1);
	m_AdminCmdMngr->RegisterCommand("admin_slap","CmdSlapHow","CmdSlapDisc",(CmdFuncIntArg)&AdminCmd::CmdSlap,ADMIN_KICK,2);

	m_AdminCmdMngr->RegisterCommand("admin_team","CmdTeamHow","CmdTeamDisc",(CmdFuncIntArg)&AdminCmd::CmdTeam,ADMIN_KICK,2);
	m_AdminCmdMngr->RegisterCommand("admin_rules","CmdRulesHow","CmdRulesDisc",(CmdFuncIntArg)&AdminCmd::CmdShowRules,ADMIN_KICK,1);
	m_AdminCmdMngr->RegisterCommand("admin_map","CmdMapHow","CmdMapDisc",(CmdFuncIntArg)&AdminCmd::CmdChangeLevel,ADMIN_MAP,1);
	m_AdminCmdMngr->RegisterCommand("admin_rcon","CmdRconHow","CmdRconDisc",(CmdFuncIntArg)&AdminCmd::CmdRcon,ADMIN_RCON,1);
	m_AdminCmdMngr->RegisterCommand("admin_menu","CmdMenuHow","CmdMenuDisc",(CmdFuncIntArg)&AdminCmd::CmdAdminMenu,ADMIN_ANY,0);
	m_AdminCmdMngr->RegisterCommand("admin","CmdMenuHow","CmdMenuDisc",(CmdFuncIntArg)&AdminCmd::CmdAdminMenu,ADMIN_ANY,0);
	m_AdminCmdMngr->RegisterCommand("admin_startmapvote","CmdStartMapVoteHow","CmdStartMapVoteDisc",(CmdFuncIntArg)&AdminCmd::StartMapVote,ADMIN_MAP,0);
	m_AdminCmdMngr->RegisterCommand("admin_list","CmdListHow","CmdListDisc",(CmdFuncIntArg)&AdminCmd::PlayerList,ADMIN_ANY,0);
	m_AdminCmdMngr->RegisterCommand("admin_reload","CmdReloadHow","CmdReloadDisc",(CmdFuncIntArg)&AdminCmd::ReloadSettings,ADMIN_RCON,0);
	m_AdminCmdMngr->RegisterCommand("admin_nextmap","CmdNextMapHow","CmdNextMapDisc",(CmdFuncIntArg)&AdminCmd::CmdNextMap,ADMIN_MAP,1);
	m_AdminCmdMngr->RegisterCommand("admin_vote","CmdVoteHow","CmdVoteDisc",(CmdFuncIntArg)&AdminCmd::CmdVote,ADMIN_MAP,1);
	m_AdminCmdMngr->RegisterCommand("admin_psay","CmdpSayHow","CmdpSayDisc",(CmdFuncIntArg)&AdminCmd::CmdPrivateSay,ADMIN_CHAT,2);
	m_AdminCmdMngr->RegisterCommand("admin_votekick","CmdVoteKickHow","CmdVoteKickDisc",(CmdFuncIntArg)&AdminCmd::CmdVoteKick,ADMIN_KICK,1);
	m_AdminCmdMngr->RegisterCommand("admin_voteban","CmdVoteKickHow","CmdVoteKickDisc",(CmdFuncIntArg)&AdminCmd::CmdVoteBan,ADMIN_BAN,1);
	m_AdminCmdMngr->RegisterCommand("admin_gag","CmdGagHow","CmdGagDisc",(CmdFuncIntArg)&AdminCmd::CmdGag,ADMIN_KICK,1);
	m_AdminCmdMngr->RegisterCommand("admin_ungag","CmdUnGagHow","CmdUnGagDisc",(CmdFuncIntArg)&AdminCmd::CmdUnGag,ADMIN_KICK,1);

	// No translation yet
	m_AdminCmdMngr->RegisterCommand("admin_loadrandomlevel","CmdLoadRanomLevelHow","CmdLoadRanomLevelDisc",(CmdFuncIntArg)&AdminCmd::LoadRandomLevel,ADMIN_MAP,0);
	m_AdminCmdMngr->RegisterCommand("admin_addadmin","CmdAddAdminHow","CmdAddAdminDisc",(CmdFuncIntArg)&AdminCmd::CmdAddAdmin,ADMIN_RCON,3);

	if(g_ModSettings.GameMod == MOD_SRCFORTS)
		m_AdminCmdMngr->RegisterCommand("admin_noclip","CmdMoveTypeHow","CmdMoveTypeDisc",(CmdFuncIntArg)&AdminCmd::CmdSetMoveType,ADMIN_NOCLIP,0);

	if(g_ModSettings.GameMod == MOD_EMPIRES)
		m_AdminCmdMngr->RegisterCommand("admin_eject","CmdEjectHow","CmdEjectDisc",(CmdFuncIntArg)&AdminCmd::CmdEjectComm,ADMIN_KICK,1);

#if BAT_DEBUG == 1
	m_AdminCmdMngr->RegisterCommand("admin_transupdate","admin_msglist in console","Displays a list of messages on the server: admin_msglist <Display to number>",(CmdFuncIntArg)&AdminCmd::CmdUpdateTrans,ADMIN_RCON,0);	
	m_AdminCmdMngr->RegisterCommand("admin_test","Run this and you will die","An evil test command was entered wrong",(CmdFuncIntArg)&AdminCmd::CmdTest,ADMIN_RCON,0);	
#endif
#if BAT_DEBUG == 2

	m_AdminCmdMngr->RegisterCommand("admin_debug","Correct usage of admin_debug is: admin_debug","This debug command prints debug info in console",(CmdFuncIntArg)&AdminCmd::CmdThreadTest,ADMIN_KICK,0);
#endif


	if(g_ModSettings.GameMod == MOD_CSTRIKE)
	{
		m_GameEventManager->AddListener(this,"round_end",true);
	}
	else if(g_ModSettings.GameMod == MOD_DOD)
		m_GameEventManager->AddListener(this,"dod_game_over",true);		
	else if(g_ModSettings.GameMod == MOD_EMPIRES)
	{
		m_GameEventManager->AddListener(this,"game_end",true);	
	}
	else if(g_ModSettings.GameMod == MOD_INSURGENCY)
	{
		//g_VoteStartTime = (g_pTimeLimit->GetFloat() * 60) - TimeToStartMapVote;
		m_GameEventManager->AddListener(this,"game_newmap",true);				
	}
	//else if(g_ModSettings.GameMod == MOD_DYSTOPIA)
	//	m_GameEventManager->AddListener(this,"round_restart",true);	
	else if (g_ModSettings.GameMod == MOD_PVKII)
	{
		m_GameEventManager->AddListener(this,"round_end",true);
	}
	else if(g_ModSettings.GameMod == MOD_TF2)
	{
		m_GameEventManager->AddListener(this,"tf_game_over",true);
		m_GameEventManager->AddListener(this,"teamplay_game_over",true);
		m_GameEventManager->AddListener(this,"teamplay_suddendeath_end",true);
		m_GameEventManager->AddListener(this,"teamplay_overtime_end",true);
		m_GameEventManager->AddListener(this,"teamplay_waiting_abouttoend",true);
	}
	g_SQLStatus.ThreadRunning = false; // It cant be running, but we make sure the value is invited to the right thing
	g_LastUsersIniGeneration = 0;
	g_MenuMngr.SetMenuType(1);

	g_BATVars.SetupCallBacks();
	m_Translation->LoadTranslationFiles();
	ReloadSettings();

	// We do it once, later we trust the cvar callback on this
	if(g_BATVars.GetPubMsgTime() > 0 && g_BATCore.GetBATVar().GetPubMsgMode() > 0)
	{
		g_PubMsgStatus = PM_ENABLED;
		g_NextPublicMsgTime = g_BATVars.GetPubMsgTime();
	}
	else 
		g_PubMsgStatus = PM_DISABLED;

	// Now we decide if we want the mapvote on the start or end of the map, this is depends on what mod is running
	if(m_MapVote->MapVoteOnStartOfMap())
		g_RunVoteOnStartOfMap = true;
	else
		g_RunVoteOnStartOfMap = false;

	// We check if admin_loadrandomlevel was in the startup args, if it was we change to random map
	const char *StartARgs  = Plat_GetCommandLine();
	if(strstr(StartARgs,"admin_loadrandomlevel") != NULL)
		AdminCmd::LoadRandomLevel();
}
void BATCore::LoadBATSettings(bool LateLoad)
{
#if BAT_DEBUG == 1
	PreSamplerStart("Sleep test");
	Sleep(5200);
	PreSamplerEnd("Sleep test");
#endif

	if(g_SettingsLoaded == false)
		OneTimeInitBAT();
	else
	{
		GetLogFileName();

		if(g_BATVars.ReloadSettingsOnMapChange())
			ReloadSettings();
	}
	g_MenuMngr.ClearForMapchange();

	g_GameStartTime = GetGlobals()->curtime;
	g_FlTime = GetGlobals()->curtime;

	g_TimeLeft = GetTimeLeft();
	g_CurrentMapIndex = GetBATMaps()->GetMapIndex(g_CurrentMapName);
	g_NextMapIndex = GetBATMaps()->GetNextmap();

	VoteInfo.Status = VOTE_WAITING;
	VoteInfo.Method = VOTEMETHOD_NOVOTE;
	for(int i=1;i<=g_MaxClients;i++)
	{
		g_IsConnecting[i] = false;
		g_IsConnected[i] = false;
	}

	if(m_MapVote->MapVoteOnStartOfMap())
	{
		g_VoteStartTime = g_GameStartTime + TimeToStartMapVote;
	}

	// We check the status on the sourcemod interface
	if(g_BATVars.GetSMInterface() >= 1 && g_BATVars.GetSMInterface() <= 2 && !g_SMExt.IsConnected())
		g_SMExt.Connect();
	else if(g_SMExt.IsConnected() && g_BATVars.GetSMInterface() == 0)
		g_SMExt.Disconnect();

	g_CurrentMapExtendTimes = 0;
	g_ReservedSlotsUsers = 0;
	g_PlayerCount = 0;

	if(g_ModSettings.SupportsWinlimit) // Clearly arrays thats not used hurts baby jesus
	{
		for(int i=0;i<MAX_TEAMS;i++)
			g_TeamWinCount[i] = 0;
	}
	if(LateLoad)
	{
		UpdatePlayerInformation();	
	}
	if(g_SQLStatus.Enabled) // We update the users ini once pr day. Simple stuff i know, but whatever works
	{
		int tCurDay = GetDaysSince1970();
		if(tCurDay != g_LastUsersIniGeneration)
		{
			g_SQLStatus.MakeLocalUsersFile = true;

			if(g_SQLStatus.ThreadSuspended)
				m_BATSQL->ResumeSqlThread();

			tCurDay = g_LastUsersIniGeneration;
		}
	}	

	WriteLogBuffer();
}
