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

#include "const.h"
#include "eiface.h"

#ifndef _INCLUDE_Modinfo
#define _INCLUDE_Modinfo

enum ModIndex
{
	MOD_UNKNOWN=0,
	MOD_CSTRIKE,
	MOD_DOD,
	MOD_HL2MP,
	MOD_HL2CTF,
	MOD_EMPIRES,
	MOD_DYSTOPIA,
	MOD_SRCFORTS,
	MOD_PVKII,
	MOD_INSURGENCY,
	MOD_FIREARMS2,
	MOD_TF2,
	MOD_ETERNALSILENCE,
};
typedef struct 
{
	int MenuMsg;		// The id of the MenuMsg
	int HintText;		// HintText
	int TextMsg;		// TextMsg
	int HudMsg;			// HudMsg
	int SayText;		// SayMsg
	int VGUIMenu;		// VGUIMenu

	bool SupportsWinlimit;
	bool DefaultRadioMenuSupport;
	bool ValveCenterSay; // These center say are really, short so we have to reshow them
	int GameMod;		// Index of the mod
	char GameModName[32];	// String of the mod
}ModSettingsStruct;
class ModInfo
{
public:
	void GetModInformation(IServerGameDLL *pServerDll,IVEngineServer *pEngine);

private:
	void GetMsgNumberInformation();
	void GetModName();
	int GetMsgNum(const char *MsgName);
};

extern ModSettingsStruct g_ModSettings;
#endif // _INCLUDE_MenuRconCmds.