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

#ifndef _INCLUDE_BATMENU_H
#define _INCLUDE_BATMENU_H

#include <iserverplugin.h>
#include "const.h"

#define TASKID_MENURESHOW 500

// The Sounds played in the menus
#define SOUND_CHOICEMADE "buttons/button14.wav"
#define SOUND_INVALID "buttons/button18.wav"
#define SOUND_SLAP "player/pl_pain7.wav"
#define SOUND_SHOWVOTE "ambient/buttons/button5.wav"

#define PLAYSOUND  "play "
#define PLAYSOUND_GOOD PLAYSOUND SOUND_CHOICEMADE
#define PLAYSOUND_BAD PLAYSOUND SOUND_INVALID
#define PLAYSOUND_SLAP PLAYSOUND SOUND_SLAP
#define PLAYSOUND_SHOWVOTE PLAYSOUND SOUND_SHOWVOTE

typedef	unsigned int	menuId;
struct player_t
{
	menuId menu;
	int index;
	int data;
};

class BATMenuBuilder
{
public:
	void SetTitle(const char *title);
	void AddOption(const char *fmt);
	void SetKeys(int keys);

private:

};
enum MenuSelectionType
{
	MENUSELECTION_GOOD,
	MENUSELECTION_BAD,
	MENUSELECTION_NOSOUND,
};

class IMenu
{
public:
	virtual ~IMenu() { };
public:
	virtual bool Display(BATMenuBuilder *make, int playerIndex) =0;
	virtual MenuSelectionType MenuChoice(player_t player, int option) =0;
};

struct menu_t
{
	IMenu *menu;
	int id;
};

class BATMenuMngr : public IServerPluginCallbacks
{
public:
	int RegisterMenu(IMenu *menu);
	void ShowMenu(int player, menuId menuid);
	void MenuChoice(int playerIndex, int choice);
	bool HasMenuOpen(int id) { return g_HasMenuOpen[id]; }
	
	int GetMenuType() { return g_MenyType; }
	void SetMenuType(int MenuType) {g_MenyType = MenuType; };
	
	void ClearForMapchange();

	void ShowCornerMsg(int id,const char *TheMsg);
	void ShowClientReConnectDialog(int id);

	static void ReShowOldMenu(int TaskID);

#if BAT_DEBUG == 1
	void ShowESCMenu(int id);
#endif

private:
	static void ShowRadioMenu(int id,char *MenuString);

	// Stuff required as we do interface with the fun valve interface
	virtual bool			Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameGetServerFactory ) {return true;};
	virtual void			Unload( void ) {};
	virtual void			Pause( void ) {};
	virtual void			UnPause( void ) {};
	virtual const char     *GetPluginDescription( void ) {return "FakeInfo"; };      
	virtual void			LevelInit( char const *pMapName ) {};
	virtual void			ServerActivate( edict_t *pEdictList, int edictCount, int clientMax ) {};
	virtual void			GameFrame( bool simulating ) {};
	virtual void			LevelShutdown( void ) {};
	virtual void			ClientActive( edict_t *pEntity ) {};
	virtual void			ClientDisconnect( edict_t *pEntity ) {};
	virtual void			ClientPutInServer( edict_t *pEntity, char const *playername ) {};
	virtual void			SetCommandClient( int index ) {};
	virtual void			ClientSettingsChanged( edict_t *pEdict ) {};
	virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )  {return PLUGIN_CONTINUE; };
	virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName, const char *pszNetworkID ) {return PLUGIN_CONTINUE; };

#if BAT_ORANGEBOX == 1
	virtual PLUGIN_RESULT	ClientCommand(edict_t *pEdict, const CCommand &args)  {return PLUGIN_CONTINUE; }; 
	virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue ) {};
#else
	virtual PLUGIN_RESULT	ClientCommand(edict_t *pEdict)  {return PLUGIN_CONTINUE; }; 
	//virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue ) {};
#endif

	bool g_HasMenuOpen[MAXPLAYERS+1];
	int g_MenyType;
};

#endif //_INCLUDE_CVARS_H