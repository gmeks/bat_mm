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

#ifndef _INCLUDE_SAMPLEPLUGIN_H
#define _INCLUDE_SAMPLEPLUGIN_H

class StrUtil
{
public:
	int StrReplace(char *str, const char *from, const char *to, int maxlen); // Replaces part of a string with something else
	char* StrRemoveQuotes(char *text);			// Removes Quotes from the string
	int GetFirstIndexOfChar(char *Text,int MaxLen,unsigned char t);	// Gets index of the instance of t in a char array
	void StrTrimLeft(char *buffer);	// Trim from the left ( Removes white spaces )
	void StrTrimRight(char *buffer); // Trim from the right ( Removes white spaces )
	void StrTrim(char *buffer);		// Trims the string ( Removes white spaces )
	bool StrIsSpace(unsigned char b);	// If the char is a space or whitespace
	unsigned long StrHash(register const char *str, register int len); // Hashes the string  ( Uses dbm hash ( Duff's device version )
};
struct StrAlphaSortStuct
{
	char *TheStr; // Pointer to the actual text.
	int Index; // What order this string has after all the strings have been alfabeticly sorted
};

#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>
#include <voice_gamemgr.h>
#include "hl2sdk/convar.h"
#include "MapVote.h"
#include "BATMaps.h"
#include "ModInfo.h"
#include "BATSQL.h"
#include "AdminCmdManager.h"
#include "AdminCommands.h"
#include "TaskSystem.h"
#include "LoadAdminAccounts.h"
#include "ReservedSlotsSystem.h"
#include "MenuRconCmds.h"
#include "ienginesound.h"
#include <igameevents.h>
#include <iplayerinfo.h>
#include "Utils.h"
#include "cvars.h"
#include "const.h"
#include <sh_vector.h>
#include "Translation.h"
#include "MessageBuffer.h"
#include "PreformanceSampler.h"


extern BATMaps *m_BATMaps;
extern BATSQL *m_BATSQL;
extern ReservedSlotsSystem *m_ReservedSlots;
extern TaskSystem *m_TaskSystem;
extern MenuRconCmds *m_MenuRconCmds;
extern MessageBuffer *m_MessageBuffer;

class BATCore : 
	public ISmmPlugin, 
	public IMetamodListener,
	public IGameEventListener2,
	public IConCommandBaseAccessor,
	public StrUtil
{
public:
	bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	bool Unload(char *error, size_t maxlen);
	void AllPluginsLoaded();
	bool Pause(char *error, size_t maxlen)
	{
		AddLogEntry("BAT was paused, AdminInterface was unloaded and will properly not function until after a map change or server restart");
		UnloadAdminInterface();
		WriteLogBuffer();
		return true;
	}
	bool Unpause(char *error, size_t maxlen)
	{
		AddLogEntry("BAT was unpaused, expect all sorts of minor issues, you should change map now");
		UpdatePlayerInformation();
		return true;
	}

	const char *GetAuthor()
	{
		return "EKS";
	}
	const char *GetName()
	{
		return "Basic Admin Tool";
	}
	const char *GetDescription()
	{
		return "A Basic Admin Tool, that provides admin command and a menu";
	}
	const char *GetURL()
	{
		return "http://www.TheXSoft.com";
	}
	const char *GetLicense()
	{
		return "zlib/libpng";
	}
	const char *GetVersion()
	{
		return BAT_VERSION;
	}
	const char *GetDate()
	{
		return __DATE__;
	}
	const char *GetLogTag()
	{
		return "BAT";
	}

#if BAT_ORANGEBOX == 1
	int GetApiVersion() { return METAMOD_PLAPI_VERSION; }
#else

#endif

	//These functions are from IServerPluginCallbacks
	//Note, the parameters might be a little different to match the actual calls!

	//Called on LevelInit.  Server plugins only have pMapName
	bool LevelInit(const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background);

	//Called on ServerActivate.  Same definition as server plugins
	void ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);

	//Called on level shutdown.  Same definition as server plugins 
	void LevelShutdown(void);

	//Called on a game tick.  Same definition as server plugins
	void GameFrame(bool simulating);

	//Client is activate (whatever that means).  We get an extra parameter...
	// "If bLoadGame is true, don't spawn the player because its state is already setup."
	void ClientActive(edict_t *pEntity, bool bLoadGame);

	//Client disconnects - same as server plugins
	void ClientDisconnect(edict_t *pEntity);

	
	void ClientPutInServer(edict_t *pEntity, char const *playername);//Client is put in server - same as server plugins	
	void SetCommandClient(int index);//Sets the client index - same as server plugins


	//Called on client connect.  Unlike server plugins, we return whether the 
	// connection is allowed rather than set it through a pointer in the first parameter.
	// You can still supercede the GameDLL through RETURN_META_VALUE(MRES_SUPERCEDE, true/false)
	bool ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
	virtual bool RegisterConCommandBase(ConCommandBase *pVar);

#if BAT_ORANGEBOX == 1
	void ClientCommand(edict_t *pEntity, const CCommand &args);
	void ClientSay( const CCommand &command ); // This is where we get have the client say hooked.	
#else
	void ClientCommand(edict_t *pEntity);
	void ClientSay();
#endif

	/** End of SourceMM Functions */
	
	//bool CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker);

	IVEngineServer *GetEngine() { return m_Engine; }
	BATVars  GetBATVar() { return g_BATVars; }		//FIXME: Make it return a pointer instead.
	AdminCmd *GetAdminCmds() { return m_AdminCmds; }
	BATSQL *GetBATSql() { return m_BATSQL; }
	ReservedSlotsSystem *GetReservedSlotsSystem() { return m_ReservedSlots; }
	IPlayerInfoManager *GetPlayerInfo() { return m_InfoMngr; }
	IServerGameDLL *GetServerGameDll(){ return m_ServerDll; }
	IServerPluginHelpers *GetHelpers(){ return m_Helpers; }
	IEngineSound *GetSoundInterface(){ return m_Sound; }
	Utils *GetUtils(){ return m_Utils; }
	MapVote *GetMapVote(){ return m_MapVote; }
	BATMaps *GetBATMaps(){ return m_BATMaps; }
	TaskSystem *GetTaskSystem(){ return m_TaskSystem; }	
	LoadAdminAccounts* GetAdminLoader() { return m_LoadAdminAccounts; }
	MenuRconCmds* GetMenuRconCmds() { return m_MenuRconCmds; }
	MessageBuffer* GetMsgBuffer() {return m_MessageBuffer; }

	void StartChangelevelCountDown(const char *NewMap); // Used to start the countdown for the nextmap, so we get a nice little timer
	int FindPlayer(const char *PlayerID);
	void EmitSound(int SoundRecivers,int SoundStartPoint,const char *SoundName);

	float GetTimeLeft();
	char *GetTimeleftString();

	void ConsolePrint(int index, const char *msg, ...);
	void SendHintMsg(int id,char *temp);
	void SendCornerMsg(int id,const char *fmt, ...);
	void ServerCommand(const char *fmt, ...);
	void SendCSay(int id,const char *pText,bool IsReShowMsg = false);
	void MessageAdmins(const char *msg, ...);
	void MessagePlayer(int index, const char *msg, ...);
	
	char *GetTeamName(int Team);	// Returns the name of the current team
	void ReloadSettings();			// Reloads settings and files, this only happens when the server loads for the first time or via admin commands
	void GetFilePath(const char * FileName,char *NewFilePath);
	void CheckUsersID();			// Checks if users steamdid is validated ( from STEAM_ID_PENDING to a real one )

	void AddLogEntry(const char * LogInfo, ...);		// Adds a log entry to the log buffer
	void WriteLogBuffer();						// Forcefully writes the log buffer to file
	
	bool IsValidTarget(int Admin_id,int Target_id,int Option);
	bool HasAccess(int id,int Access,bool ShowWarrning = true);
	bool IsUserAlive(int id);
	const char *GetPlayerName(int id);

	
	void FireGameEvent(IGameEvent *event);
	void UpdatePlayerInformation(); // This function gets called after the plugin gets unpaused or late loaded, it updates the player list
	void SetupPlayerInfo(int id);

private:
	IGameEventManager2 *m_GameEventManager;	
	IVEngineServer *m_Engine;
	IServerPluginHelpers *m_Helpers;
	IServerGameDLL *m_ServerDll;
	IServerGameClients *m_ServerClients;
	IPlayerInfoManager *m_InfoMngr;
	IEngineSound *m_Sound;
	Utils *m_Utils;

	MapVote *m_MapVote;
	AdminCmd *m_AdminCmds;	
	LoadAdminAccounts *m_LoadAdminAccounts;

	ConCommand *pSayCmd;
	ConCommand *pSayTeamCmd;
	SourceHook::CallClass<IVEngineServer> *m_Engine_CC;
	SourceHook::CVector<int> m_hooks;

	void LoadBATSettings(bool LateLoad);
	void OneTimeInitBAT();

	void GetLogFileName();
	void ExecMapCfg();
	void ExecBATCfg();
	void SetupBasePath();
	void CheckReservedSlots(int id);
	void SetupModSpesficInfo(); // Gets the menu message id, for the running mod
	int GetDaysSince1970();

	int GetNextSpaceCount(char *Text,int CurIndex);
	void UnloadAdminInterface(); // Used to inform plugins using the admin interface that its no longer usable

	ConCommand *HookConsoleCmd(const char *CmdName);
	void RemoveHookConsoleCmd(ConCommand *pTheCmd);
};

extern BATCore g_BATCore;
extern BATVars g_BATVars;
extern BATMenuMngr g_MenuMngr;

extern ConstPlayerInfo g_UserInfo[MAXPLAYERS+2];
extern int g_MaxClients;
extern bool g_IsConnected[MAXPLAYERS+1];
extern bool g_IsConnecting[MAXPLAYERS+1];
extern Translation *m_Translation;

extern char g_BasePath[256];

PLUGIN_GLOBALVARS();


#if BAT_ORANGEBOX == 0
#define GetGlobals g_SMAPI->pGlobals

class CCommand
{
public:
	const char *ArgS()
	{
		return g_BATCore.GetEngine()->Cmd_Args();
	}
	int ArgC()
	{
		return g_BATCore.GetEngine()->Cmd_Argc();
	}

	const char *Arg(int index)
	{
		return g_BATCore.GetEngine()->Cmd_Argv(index);
	}
};
extern ICvar *g_pCVar;
#else
#define GetGlobals g_SMAPI->GetCGlobals
#endif

extern CCommand g_LastCCommand;

#endif //_INCLUDE_SAMPLEPLUGIN_H
