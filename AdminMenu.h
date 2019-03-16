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

#ifndef _INCLUDE_MENUS_ADMINMENU
#define _INCLUDE_MENUS_ADMINMENU

//#include "BATMenu.h"

//extern menuId g_AdminMenu;

class AdminMenu : public IMenu
{
public:
	bool Display(BATMenuBuilder *make, int playerIndex);
	MenuSelectionType MenuChoice(player_t player, int option);

private:
	bool IsMorePlayers(int index);
	bool IsMoreMaps(int index);
	bool IsMoveMenuRconCmds(int index);
	void ChangeSlapMenuSetting(int id);		// This func changes the slap menu via the menu
	void GenPlayerList(int id,BATMenuBuilder *make);
	void GenRconMenuList(int id,BATMenuBuilder *make);
	void GenMapList(int id,BATMenuBuilder *make,int MaxMaps);
	void ResetMenu(int id,bool ReShowMenu);
	void ChangeBanMenuSetting(int id);
	void Get8Option(int id,BATMenuBuilder *make);
	void DoOptionTenorNine(int id,int option);
	void ChangeTeamMenuSetting(int id);
};
#endif