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

#ifndef _INCLUDE_CONST_H
#define _INCLUDE_CONST_H

#define GAME_DLL

#include <metamod_oslink.h>
#include "edict.h"
#include "hl2sdk/strtools.h"
#include "BATInterface.h"
#include "ModInfo.h"
#include <sh_vector.h>

#define FILE_USERS "users.ini"
#define FILE_MENUCMDS "menucmds.ini"
#define FILE_TRANSLATION "translation.dat"
#define FILE_ADMINMAPS "adminmaps.ini"
#define FILE_VOTEMAP "votemaps.ini"
#define FILE_PUBMSG "publicmessages.ini"

#ifdef WIN32
#else
#define LinuxThreading // We activate linux style threading. Can be used on windows with right dll files
#endif

#define BAT_VERSION	"1.6.0 B11"
#define BAT_DEBUG 0

#define BAT_ORANGEBOX 0
#define BAT_SOURCEMM_16 0

#define PluginCore g_BATCore
#define PluginSpesficLogPrint AddLogEntry

#define MAXPLAYERS ABSOLUTE_PLAYER_LIMIT +1
#define MAX_PLAYERS MAXPLAYERS
#define MAX_NETWORKID_LENGTH 64
#define MAX_REDIRECTTIME 10.0

#if BAT_ORANGEBOX == 1
#define MAX_TEAMSINMENU 5
#else
#define FnChangeCallback_t FnChangeCallback
#define MAX_TEAMSINMENU 4
#endif

#define MAX_LOGBUFFER 40
#define MAX_LOGLEN	256
#define MAX_MAPNAMELEN 30	// The maximum length of a mapname
#define MapsInPublicVote 4

#define MESSAGE_NOTFOUND -1
#define ID_SERVER 256

// From PublicMessages.h
#define MESSAGE_MAXLEN 192
#define MESSAGE_MAXCOUNT 20

// ---------- GameFrame()
#define TASK_CHECKTIME 1.0			// This is how often the plugin checks if any tasks should be executed
#define TimeToStartMapVote 180
#define PublicVoteTime 30			// The time a public vote has when menu powered
#define PublicVoteTimeChat 180		// The time a public vote has when chat powered ( IE: say vote de_dust )
#define RTVTime 180					// How longer players have to type "say rockthevote" after the first person says it

// Flood protection
#define WAITTIME_CHATCMDS 1
#define WAITTIME_PUBVOTE 120

// What the 8 th selection in the map menu does
#define CHANGEMAP_NOW 0
#define CHANGEMAP_NEXTMAP 1

// Admin command settings
#define MAX_CMDDESCSIZE 192
#define MAX_CMDSIZE 40
#define MAX_ADMINCOMMANDS 48
#define MAX_CUSTOMCMDVOTELEN 192

enum MapVoteSystem
{
	MAPVOTE_BADMAP=-1,		// Map was not found in mapcycle
	MAPVOTE_CURMAP=-2,		// Cannot vote for current map
	
	MAPVOTE_NOVOTE=-2,
	MAPVOTE_EXTEND=-1,
	MAPVOTE_ROCKTHEVOTE=-3,
	MAPVOTE_YES=1,
	MAPVOTE_NO=2,
};
enum _MapType
{
	MAPTYPE_MAPCYCLEMAP,
	MAPTYPE_VOTEMAP,
	MAPTYPE_ADMINMAP,
};
enum VoteMethod
{
	VOTEMETHOD_MENU=0,
	VOTEMETHOD_CHAT=1,
	VOTEMETHOD_NOVOTE=2,
};
/*
When adding a new language things that need updating:
SortTranslationList() - Where it writes the file
LoadTranslationFiles() - Where it finds what language its reading, and where it reads the info into the array
GetTranslation() - Where it find what string to return in the switch()
UpdateCurLanguage() - The function that determine what language to activate
*/
enum eGagStatus
{
	GAG_NONE=0,
	GAG_TEXTCHAT=(1<<0),
	GAG_VOICECOMM=(1<<1),
};
enum MenuPages
{
	MENU_RESET=-1,
	MENU_ROOT=0,
	MENU_KICK,
	MENU_BAN,
	MENU_SLAP,
	MENU_NAME,
	MENU_TEAM,
	MENU_MAPVOTE,
	MENU_MAP,
	MENU_MENURCON,
};
enum SlapDamage
{
	Slap_SlayAndRulesUser=-10,
	Slap_SlayUser=-5,
	Slap_SlapTo1Hp=-1,
	Slap_NoDmg=0,
};
enum AdminAccessFlags
{
	ADMIN_ANY=0,
	ADMIN_KICK=(1<<0),
	ADMIN_BAN=(1<<1),
	ADMIN_CHAT=(1<<2),
	ADMIN_RCON=(1<<3),
	ADMIN_MAP=(1<<4),
	ADMIN_RESERVEDSLOTS=(1<<5),
	ADMIN_IMMUNITY=(1<<6),
	ADMIN_NOCLIP=(1<<7),
	ADMIN_BANIP=(1<<8),
	ADMIN_MENURCON=(1<<9),
	ADMIN_TEMPBAN=(1<<10),
	ADMIN_VOTEONLY=(1<<11),
};
enum VoteType
{
	VOTETYPE_CUSTOM,					// Custom map vote, changes map after vote is done
	VOTETYPE_CUSTOMNEXTMAP,				// custom map vote, changes map on map end
	VOTETYPE_PUBLICVOTE,				// Normal end of map vote, changes on end of map
	VOTETYPE_PUBLICVOTEENDFAST,			// This type public vote map change right after the vote is over
	VOTETYPE_ROCKTHEVOTE,				// Same as public vote, but started by players and changes map at end of vote
	VOTETYPE_CMDCUSTOM,					// A custom vote started by a Cmd
	VOTETYPE_KICKPLAYER,				// Vote kick on a player
	VOTETYPE_BANPLAYER,					// Vote ban on a player
};
enum PublicMapVoteStatus
{
	VOTE_ERROR=-1,						// Some error happen, user should check console for more details. But plugin will not run any votes
	VOTE_DONE_NOVOTES,					// Happens when nobody votes in the public vote.
	VOTE_WAITING,						// Waiting for the time to show the map vote
	VOTE_VOTERUNNING,					// Showing map vote
	VOTE_DONE_WAITFOREVENT,				// The vote was done, we now wait for a specific event, before we start checking the timeleft
	//VOTE_WAITXSECTHENCHANGEMAP,			// The time limit has expired, now we wait a few sec before we change the map ( so ppl can see scoreboard info)
	VOTE_ROCKTHEVOTESTARTED,			// Someone has started a rockthevote, and we now wait the 3min to see if sufficient others join in )
	VOTE_DONE,							// Vote is done, we are waiting for the mapchange
};
enum TargetOptions		// What options are required for a valid target with the IsValidTarget() function
{
	TARGET_NORULES=(1<<0),
	TARGET_HUMAN=(1<<1),
	TARGET_ALIVE=(1<<2),
};
enum eDataBaseType 
{
	BAT_V2=1,
	AmxBans,
	SourceBans,
};
enum STEAM_ID_Status		// What the status of the steamid player
{
	STEAM_ID_NOTVALID,
	STEAM_ID_WAIT_FOR_SQL,
	STEAM_ID_WAIT_FOR_SETUP,
	STEAM_ID_VALID,
};
enum PublicMessageStatus		// What options are required for a valid target with the IsValidTarget() function
{
	PM_ENABLED=0,
	PM_DISABLED=1,
};
typedef struct 
{
	int Status;			// What status we of the current vote is
	int MapsInVote;		// The number of maps in the current vote
	int Type;			// The type of vote ( Public vote, Rockthevote or custom )
	int Method;			// What style vote we have ( Chat or Menu ) from the VoteMethod enum
	int PlayersThatHasVoted;	// How many players that have already voted
	int PlayersInVote;		// The amount of players that have seen the vote
	int MapsAddedToVote;		// How many maps that already has been added to the vote
	float VoteEndTime;		// When the vote ends
}VoteInfoStruct;
typedef struct 
{
	char Question[MAX_CUSTOMCMDVOTELEN+1]; 	// The question asked in a vote via admin_vote
	char SrvCmd[MAX_CMDSIZE+1];// What command to run if any.
	bool ExecuteSrvCmd;
}VoteCmdInfo;
typedef struct 
{
	char Info[MAX_LOGLEN+1]; // The actual logged info
	char LogTime[48];		 // When it was logged
	//char AdminIndex[MAX_NETWORKID_LENGTH+1];	// The id of the admin
}eSQLAdminLog;
typedef struct 
{
	char AccessText[ADMININTERFACE_MAXACCESSLENGTHTEXT+1]; // The actual text, that the other plugin has registered as a flag
	int InternalFlag;		// This is the actual flag that BAT uses internaly
}CustomAccessFlagStruct;

typedef void(*CmdFuncIntArg)(int);
typedef void(*CmdFuncNoArg)(int);

typedef struct 
{
	char Command[128];
	char Description[64];
}RconMenuCmdsStruct;

typedef struct 
{
	int AccessRequired;						// What status we of the current vote is
	int CmdArgsRequired;					// How many admin command args are required.
	char Description[MAX_CMDDESCSIZE+1];	// Explains what the command does
	char CmdUsage[MAX_CMDDESCSIZE+1];		// The command description, shown to client in admin_help or if the command is entered wrong
	char Cmd[MAX_CMDSIZE+1];				// The actual command, to be entered in either client console or server console
	char SayCmd[MAX_CMDSIZE+1];				// The version of the command thats checked against chat
	CmdFuncIntArg pFunc;							// Pointer to the actual function used.
}AdminCommandStruct;
typedef struct 
{
	char Text[MAX_CMDSIZE+1];
}ArgListStruct;
typedef struct 
{
	int MapIndex;
}MapsInVoteStruct;
typedef struct 
{
	int MapVotes;
}MapsVotesStruct;
typedef struct 
{
	AdminInterfaceListner *ptr;
}AdminInterfaceListnerStruct;
typedef struct 
{
	char LoginID[MAX_NETWORKID_LENGTH+1];
	unsigned int LoginIDHash;						// Contains a hash of the steamid, for fast checking.
	int Access;
	char Name[128];
}AdminAccountInfo;
typedef struct 
{
	char MapName[MAX_MAPNAMELEN];	// the name of the map
	_MapType MapType;			// If the map is in the regular map cycle
	int TextLen;
}MapStuct;
typedef struct 
{
	char Text[MESSAGE_MAXLEN+1];		// The actual text.
	int TextLen;
	int PubMsgColor;						// Index of what color to show
}PublicMessageConst;
typedef struct 
{
	char UserName[128];
	char Password[128];
	char DataBase[128];
	char ServerIP[128];

	char AdminTable[128];
	char LogTable[128];

	char AmxBansServerTable[128];
	int AmxBansServerID;
	eDataBaseType DataBaseType;
}stSQLSettings;
typedef struct 
{
	bool Enabled;			// If we are currently using SQL for admin/logs and so on
	bool ThreadSuspended;		// If the SQL thread is currently working
	int Errors;				// How many errors we have had this map trying to reach the SQL server.
	unsigned int CueLength;			// How many players are in the cue waiting to be checked against SQL DB
	bool MoveLogInfo;		// If we should move information from the log buffers to SQL
	bool MakeLocalUsersFile; // If we should make a new users.ini file based on content of the SQL server
	bool ThreadRunning;		// If the thread is running ( Or has been started mroe accuratly)
}stSQLStatus;
typedef struct 
{
	char Steamid[MAX_NETWORKID_LENGTH+1];
	char IP[40];
	int AdminAccess;
	int AdminIndex;					// The index of the players admin account in AdminAccountInfo 0 based i think
	char AdminFlags[40];			// A string of admin access rights
	int Userid;
	bool IsBot;						// True if its a BOT or a hltv
	edict_t *PlayerEdict;
	float ConnectTime;				// When ClientPutInServer was called on this client.
	float AntiFlood;				// When the user last did something we anti flood protect against
	float LastPublicVoteStart;		// When the user last started a public vote
	STEAM_ID_Status SteamIdStatus;	// The status of the users steamid
	char AdminName[128];			// The name of the admin account
	int GagStatus;			// The status if the player is gaged or not
}ConstPlayerInfo;
/******************************* Menu Related things *******************************/
#endif //_INCLUDE_CONST_H


