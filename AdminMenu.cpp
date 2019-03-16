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
#include "AdminCommands.h"
#include "AdminMenu.h"
#include "Translation.h"
#include "BATMenu.h"
#include "MenuRconCmds.h"


#define NO_MAP -1
extern int g_AdminMenu;
extern int g_MaxClients;

extern bool g_IsConnected[MAXPLAYERS+1];
extern VoteInfoStruct VoteInfo;

extern SourceHook::CVector<MapStuct *>g_MapList;
extern BATMenuMngr g_MenuMngr;
extern Translation *m_Translation;
extern int g_MapCount;

extern SourceHook::CVector<RconMenuCmdsStruct *>g_MenuRconCmds;
extern int unsigned g_MenuRconCmdsCount;
int g_MaxMenuRconPages;

int g_MenuPos[MAXPLAYERS+1];		// What menu the player is in
int g_MenuPage[MAXPLAYERS+1];		// What page the player is in the menu ( like a kick menu, where there may be multiple pages of players
int g_MenuLastPlayer[MAXPLAYERS+1][MAXPLAYERS+1];
int g_MenuTarget[MAXPLAYERS+1][MAXPLAYERS+1];

int g_MenuBanSelection[MAXPLAYERS+1];
int g_MenuSlapSelection[MAXPLAYERS+1];
int g_MenuMapSelection[MAXPLAYERS+1];
int g_MenuTeamSelection[MAXPLAYERS+1] = {1,1};
int g_MenuLastMap[MAXPLAYERS+1];

int g_MapInCustomVote[MAXPLAYERS+1]  = {0,0};
int g_MapInMapVote[MAXPLAYERS+1][MapsInPublicVote]  = {0,-1};

/*
kick player
ban player
slap/slay player

Change name
Change team

Vote map
Change map
*/
/*
AdminMenu::~AdminMenu()
{
}
*/

bool AdminMenu::Display(BATMenuBuilder *make, int playerIndex)
{
	make->SetKeys( (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7)| (1<<8) | (1<<9));

	if(g_MenuPos[playerIndex] == MENU_ROOT)
	{
		make->SetTitle(m_Translation->GetTranslation("MenuRoot"));

		make->AddOption(m_Translation->GetTranslation("MenuKick"));
		make->AddOption(m_Translation->GetTranslation("MenuBan"));
		make->AddOption(m_Translation->GetTranslation("MenuSlap"));

		make->AddOption(m_Translation->GetTranslation("MenuName"));
		make->AddOption(m_Translation->GetTranslation("MenuTeam"));

		make->AddOption(m_Translation->GetTranslation("MenuVoteMap"));
		make->AddOption(m_Translation->GetTranslation("MenuChangeMap"));
		make->AddOption(m_Translation->GetTranslation("MenuRconMenu"));
		make->AddOption("");
		make->AddOption(m_Translation->GetTranslation("MenuExit"));

		if(g_MenuMngr.GetMenuType() != 1)
			g_BATCore.MessagePlayer(playerIndex,m_Translation->GetTranslation("MenuEscSeeMenu"));

		return true;
	}
	else if(g_MenuPos[playerIndex] == MENU_KICK)
	{
		if(!g_BATCore.HasAccess(playerIndex,ADMIN_KICK) || g_BATCore.HasAccess(playerIndex,ADMIN_VOTEONLY))
		{
			g_MenuPos[playerIndex] = MENU_ROOT;
			return false;
		}

		make->SetTitle("BAT - Kick menu:");
		GenPlayerList(playerIndex,make);
		return true;
	}
	else if(g_MenuPos[playerIndex] == MENU_BAN)
	{
		if(!g_BATCore.HasAccess(playerIndex,ADMIN_BAN) || g_BATCore.HasAccess(playerIndex,ADMIN_VOTEONLY))
		{
			g_MenuPos[playerIndex] = MENU_ROOT;
			return false;
		}

		make->SetTitle("BAT - Ban menu:");

		GenPlayerList(playerIndex,make);
		return true;
	}
	else if(g_MenuPos[playerIndex] == MENU_SLAP)
	{
		if(!g_BATCore.HasAccess(playerIndex,ADMIN_KICK))
		{
			g_MenuPos[playerIndex] = MENU_ROOT;
			return false;
		}

		make->SetTitle("BAT - Slap/Slay menu:");

		GenPlayerList(playerIndex,make);
		return true;
	}
	else if(g_MenuPos[playerIndex] == MENU_NAME)
	{
		if(!g_BATCore.HasAccess(playerIndex,ADMIN_KICK))
		{
			g_MenuPos[playerIndex] = MENU_ROOT;
			return false;
		}

		make->SetTitle("BAT - Name menu:");

		GenPlayerList(playerIndex,make);
		return true;
	}
	else if(g_MenuPos[playerIndex] == MENU_TEAM)
	{
		if(!g_BATCore.HasAccess(playerIndex,ADMIN_KICK))
		{
			g_MenuPos[playerIndex] = MENU_ROOT;
			return false;
		}

		make->SetTitle("BAT - Change team menu:");

		GenPlayerList(playerIndex,make);
		return true;
	}
	else if(g_MenuPos[playerIndex] == MENU_MAPVOTE)
	{
		if(!g_BATCore.HasAccess(playerIndex,ADMIN_MAP))
		{
			g_MenuPos[playerIndex] = MENU_ROOT;
			return false;
		}

		make->SetTitle("BAT - Map vote:");
		
		if(g_MenuMngr.GetMenuType() == 1)
			GenMapList(playerIndex,make,6);
		else
			GenMapList(playerIndex,make,4);
		return true;
	}
	else if(g_MenuPos[playerIndex] == MENU_MAP)
	{
		if(!g_BATCore.HasAccess(playerIndex,ADMIN_MAP) || g_BATCore.HasAccess(playerIndex,ADMIN_VOTEONLY))
		{
			g_MenuPos[playerIndex] = MENU_ROOT;
			return false;
		}

		make->SetTitle("BAT - Change map:");

		if(g_MenuMngr.GetMenuType() == 1)
			GenMapList(playerIndex,make,7);
		else
			GenMapList(playerIndex,make,5);
		return true;
	}
	else if(g_MenuPos[playerIndex] = MENU_MENURCON)
	{
		if(!g_BATCore.HasAccess(playerIndex,ADMIN_MENURCON))
		{
			g_MenuPos[playerIndex] = MENU_ROOT;
			return false;
		}

		make->SetTitle("BAT - Menu Rcon:");
		GenRconMenuList(playerIndex,make);	
		return true;
	}
	return false;
}
MenuSelectionType AdminMenu::MenuChoice(player_t player, int option)
{
	int id = player.index;
	int target = g_MenuTarget[id][option];

	if(g_MenuMngr.GetMenuType() != 1)	// Since ESC menus only have 8 options we had to change the option in case its a higher number
	{
		int Menu = g_MenuPos[id];
		if(Menu == MENU_KICK  || Menu == MENU_NAME || Menu == MENU_MENURCON)
		{
			if(option >= 7)
				option += 2;
		}
		else if(Menu == MENU_BAN || Menu == MENU_MAPVOTE || Menu == MENU_TEAM )
		{
			if(option >= 5)
				option += 2;
		}
		else if(Menu == MENU_MAP || Menu == MENU_SLAP)
		{
			if(option >= 6)
				option += 2;
		}
		target = g_MenuTarget[id][option];
	}
#if BAT_DEBUG == 1
	//g_BATCore.MessagePlayer(0,"selected %d in the menu",option);
#endif
	if(g_MenuPos[id] == MENU_RESET)		// His menu needs to be reset, so we do that.
		ResetMenu(id,true);
	else if(g_MenuPos[id] == MENU_ROOT) 
	{
		switch(option)
		{
			case 10:
				return MENUSELECTION_NOSOUND;

			case 9:
				g_MenuMngr.ShowMenu(id,g_AdminMenu);
				return MENUSELECTION_BAD;

			case MENU_MENURCON: // 8
				g_MenuPos[id] = MENU_MENURCON;
				break;

			case MENU_MAP: // 7
				g_MenuPos[id] = MENU_MAP;
				break;

			case MENU_MAPVOTE: // 6

				g_MapInCustomVote[id] = 0;
				for(int i=0;i<MapsInPublicVote;i++)
					g_MapInMapVote[id][i] = NO_MAP;	

				g_MenuPos[id] = MENU_MAPVOTE;
				break;

			case MENU_TEAM: // 5
				g_MenuPos[id] = MENU_TEAM;
				break;

			case MENU_NAME: // 4
				g_MenuPos[id] = MENU_NAME;
				break;

			case MENU_SLAP: // 3
				g_MenuPos[id] = MENU_SLAP;
				break;

			case MENU_BAN: // 2
				g_MenuPos[id] = MENU_BAN;
				break;

			case MENU_KICK: // 1
				g_MenuPos[id] = MENU_KICK;
				break;

		}
		g_MenuMngr.ShowMenu(id,g_AdminMenu);
		return MENUSELECTION_GOOD;
	}
	else if(g_MenuPos[id] == MENU_KICK && g_BATCore.HasAccess(id,ADMIN_KICK)) 
	{
		if(option == 10 || option == 9)
		{
			DoOptionTenorNine(id,option);
			return MENUSELECTION_GOOD;
		}
		else if(option == 8)		// Since the kick menu dont have a 8 option, we just reshow the menu if it was used.
		{
			g_MenuMngr.ShowMenu(id,g_AdminMenu);			
			return MENUSELECTION_GOOD;
		}
		else if(!g_BATCore.IsValidTarget(id,target,TARGET_NORULES))
			return MENUSELECTION_BAD;

		g_BATCore.GetAdminCmds()->Kick(target,id,"");
		ResetMenu(id,false);
		return MENUSELECTION_GOOD;
	}
	else if(g_MenuPos[id] == MENU_BAN && g_BATCore.HasAccess(id,ADMIN_BAN)) 
	{
		if(option == 10 || option == 9)
		{
			DoOptionTenorNine(id,option);
			return MENUSELECTION_GOOD;
		}
		else if(option == 8)		// Since the kick menu dont have a 8 option, we just reshow the menu if it was used.
		{
			ChangeBanMenuSetting(id);
			g_MenuMngr.ShowMenu(id,g_AdminMenu);
			return MENUSELECTION_GOOD;
		}
		else if(!g_BATCore.IsValidTarget(id,target,TARGET_HUMAN))		// Since the kick menu dont have a 8 option, we just reshow the menu if it was used.
		{
			g_MenuMngr.ShowMenu(id,g_AdminMenu);			
			return MENUSELECTION_BAD;
		}

		g_BATCore.GetAdminCmds()->Ban(target,id,g_MenuBanSelection[id],"");
		
		ResetMenu(id,false);
		return MENUSELECTION_GOOD;
	}
	else if(g_MenuPos[id] == MENU_SLAP && g_BATCore.HasAccess(id,ADMIN_KICK)) 
	{
		if(option == 10 || option == 9)
		{
			DoOptionTenorNine(id,option);
			return MENUSELECTION_GOOD;
		}
		else if(option == 8)		// Since the kick menu dont have a 8 option, we just reshow the menu if it was used.
		{
			ChangeSlapMenuSetting(id);
			g_MenuMngr.ShowMenu(id,g_AdminMenu);
			return MENUSELECTION_GOOD;
		}
		else if(!g_BATCore.IsValidTarget(id,target,TARGET_ALIVE))
		{
			g_MenuMngr.ShowMenu(id,g_AdminMenu);			
			return MENUSELECTION_BAD;
		}

		g_BATCore.GetAdminCmds()->Slap(target,id,g_MenuSlapSelection[id]);
		
		if(g_BATCore.IsUserAlive(target))
			g_MenuMngr.ShowMenu(id,g_AdminMenu);
		else
			ResetMenu(id,false);
		return MENUSELECTION_GOOD;
	}
	else if(g_MenuPos[id] == MENU_NAME && g_BATCore.HasAccess(id,ADMIN_KICK)) 
	{
		if(option == 10 || option == 9)
		{
			DoOptionTenorNine(id,option);
			return MENUSELECTION_GOOD;
		}
		else if(!g_BATCore.IsValidTarget(id,target,TARGET_HUMAN))
		{
			g_MenuMngr.ShowMenu(id,g_AdminMenu);			
			return MENUSELECTION_BAD;
		}
		
		g_BATCore.GetAdminCmds()->ChangeName(target,id);

		ResetMenu(id,false);
		return MENUSELECTION_GOOD;
	}
	else if(g_MenuPos[id] == MENU_TEAM && g_BATCore.HasAccess(id,ADMIN_KICK)) 
	{
		if(option == 10 || option == 9)
		{
			DoOptionTenorNine(id,option);
			return MENUSELECTION_GOOD;
		}
		else if(option == 8)		// Since the kick menu dont have a 8 option, we just reshow the menu if it was used.
		{
			ChangeTeamMenuSetting(id);
			g_MenuMngr.ShowMenu(id,g_AdminMenu);
			return MENUSELECTION_GOOD;
		}
		else if(!g_BATCore.IsValidTarget(id,target,TARGET_HUMAN))
		{
			g_MenuMngr.ShowMenu(id,g_AdminMenu);	
			return MENUSELECTION_BAD;
		}

		g_BATCore.GetAdminCmds()->ChangeTeam(target,id,g_MenuTeamSelection[id]);

		ResetMenu(id,false);
		return MENUSELECTION_GOOD;
	}
	else if(g_MenuPos[id] == MENU_MAPVOTE && g_BATCore.HasAccess(id,ADMIN_MAP)) 
	{
		if(option == 10 || option == 9)
		{
			DoOptionTenorNine(id,option);
			return MENUSELECTION_GOOD;
		}
		else if(option == 8)
		{
			if(g_MenuMapSelection[id] == CHANGEMAP_NEXTMAP)
				g_MenuMapSelection[id] = CHANGEMAP_NOW;
			else
				g_MenuMapSelection[id] = CHANGEMAP_NEXTMAP;

			g_MenuMngr.ShowMenu(id,g_AdminMenu);
			return MENUSELECTION_GOOD;
		}
		else if(option == 7)
		{
			if(g_MapInCustomVote[id] == 0)
			{
				g_MenuMngr.ShowMenu(id,g_AdminMenu);
				return MENUSELECTION_BAD;
			}
			else if(g_MapInCustomVote[id] > 1)
			{
				#define MapListLen MapsInPublicVote*MAX_MAPNAMELEN+1

				char MapList[MapListLen];
				int len=0;
				for(int i=0;i<g_MapInCustomVote[id];i++)
				{
					int MI = g_MapInMapVote[id][i];
					if(i != 0)
						len += _snprintf(&(MapList[len]),MapListLen-len,",%s",g_MapList[MI]->MapName);
					else
						len += _snprintf(&(MapList[len]),MapListLen-len,"%s",g_MapList[MI]->MapName);
				}
				
				g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("MenuStartCustomMapVote"),MapList);
				g_BATCore.GetMapVote()->StartMultiCustomMapVote(g_MapInMapVote[id],g_MapInCustomVote[id],g_MenuMapSelection[id]);
				ResetMenu(id,false);
			}
			else if(g_MapInCustomVote[id] == 1)
			{
				g_BATCore.MessagePlayer(0,m_Translation->GetTranslation("MenuStartCustomMapVote"),g_MapList[g_MenuMapSelection[id]]->MapName);
				g_BATCore.GetMapVote()->StartCustomMapVote(g_MapInMapVote[id][0],g_MenuMapSelection[id]);
				ResetMenu(id,false);
			}
			return MENUSELECTION_GOOD;
		}
		else if(target == NO_MAP)
		{
			g_MenuMngr.ShowMenu(id,g_AdminMenu);
			return MENUSELECTION_BAD;
		}
		
		if(g_MapInCustomVote[id] < MapsInPublicVote)
		{
			g_MapInMapVote[id][g_MapInCustomVote[id]] = target;
			g_MapInCustomVote[id]++;
			g_BATCore.MessagePlayer(id,m_Translation->GetTranslation("MenuMapAdded"),g_MapList[target]->MapName,g_MapInCustomVote[id],MapsInPublicVote);
		}
		else
			g_BATCore.MessagePlayer(id,m_Translation->GetTranslation("MenuMapCannotAdd"));


		g_MenuMngr.ShowMenu(id,g_AdminMenu);
		return MENUSELECTION_GOOD;
	}
	else if(g_MenuPos[id] == MENU_MAP && g_BATCore.HasAccess(id,ADMIN_MAP) && !g_BATCore.HasAccess(id,ADMIN_VOTEONLY)) 
	{
		if(option == 10 || option == 9)
		{
			DoOptionTenorNine(id,option);
			return MENUSELECTION_GOOD;
		}
		else if(option == 8)
		{
			if(g_MenuMapSelection[id] == CHANGEMAP_NEXTMAP)
				g_MenuMapSelection[id] = CHANGEMAP_NOW;
			else
				g_MenuMapSelection[id] = CHANGEMAP_NEXTMAP;

			g_MenuMngr.ShowMenu(id,g_AdminMenu);
			return MENUSELECTION_GOOD;
		}
		else if(target == NO_MAP)
		{
			g_MenuMngr.ShowMenu(id,g_AdminMenu);
			return MENUSELECTION_GOOD;
		}

		if(g_MenuMapSelection[id] == CHANGEMAP_NEXTMAP)
			g_BATCore.GetAdminCmds()->SetNextmap(id,g_MapList[target]->MapName);
		else
			g_BATCore.GetAdminCmds()->Changelevel(id,g_MapList[target]->MapName);

		ResetMenu(id,false);
	}
	else if(g_MenuPos[id] == MENU_MENURCON && g_BATCore.HasAccess(id,ADMIN_MENURCON)) 
	{
		if(option == 10 || option == 9)
		{
			DoOptionTenorNine(id,option);
			return MENUSELECTION_GOOD;
		}
		else if(target == NO_MAP)
		{
			g_MenuMngr.ShowMenu(id,g_AdminMenu);
			return MENUSELECTION_GOOD;
		}
		
		AdminCmd::MenuRconCmd(id,target);
		return MENUSELECTION_GOOD;
	}
	else
		ResetMenu(id,false);

	return MENUSELECTION_BAD;
}
void AdminMenu::GenRconMenuList(int id,BATMenuBuilder *make)
{
	int PlayersToAdd = 0;
	if(g_MenuMngr.GetMenuType() == 1)
		PlayersToAdd = 8;
	else
		PlayersToAdd = 5;

	int MenuPage = g_MenuPage[id];
	int PlayersAdded=0;
	uint i=0;

	if(MenuPage > 0)	// If we are generating menu thats not the first page, we know where to start
		i = g_MenuLastPlayer[id][MenuPage-1];

	while(i < g_MenuRconCmdsCount)
	{
		PlayersAdded++;
		make->AddOption(g_BATCore.GetMenuRconCmds()->ParseRconCmdsString(i,true));
		g_MenuTarget[id][PlayersAdded] = i;

		if(PlayersAdded == PlayersToAdd)
			break;

		i++;
	}
	while(PlayersAdded < PlayersToAdd)		// in case we dident add all server players we add magic to the menu, so we get it all filled up
	{
		PlayersAdded++;
		g_MenuTarget[id][PlayersAdded] = NO_MAP;
		make->AddOption("");
	}
	g_MenuLastPlayer[id][MenuPage] = i + 1;

	if(IsMoveMenuRconCmds(id))
	{
		make->AddOption(m_Translation->GetTranslation("MenuNext"));
	}
	else
		make->AddOption("");

	if(MenuPage > 1 || g_MenuPos[id] != MENU_ROOT)
		make->AddOption(m_Translation->GetTranslation("MenuBack"));
	else
		make->AddOption(m_Translation->GetTranslation("MenuExit"));
}
void AdminMenu::GenMapList(int id,BATMenuBuilder *make,int MaxMaps)
{
	int i = 0;
	int MenuPage = g_MenuPage[id];
	
	if(MenuPage > 0)	// If we are generating menu thats not the first page, we know where to start
		i = g_MenuLastPlayer[id][MenuPage-1];
	
	int MapsAdded = 1;
	while(MapsAdded <= MaxMaps && i < g_MapCount)
	{
		g_MenuTarget[id][MapsAdded] = i;
		make->AddOption(g_MapList[i]->MapName);
		i++;
		MapsAdded++;
	}
	g_MenuLastMap[id] = i;
	g_MenuLastPlayer[id][MenuPage] = i;

	while(MapsAdded <= MaxMaps)
	{
		make->AddOption("");
		g_MenuTarget[id][MapsAdded] = NO_MAP;
		MapsAdded++;
	}
	if(g_MenuPos[id] == MENU_MAPVOTE)
	{
		char TextBuffer[28];
		_snprintf(TextBuffer,27,m_Translation->GetTranslation("MenuMapStart"),g_MapInCustomVote[id]);
		make->AddOption(TextBuffer);
	}
	if(g_MenuMapSelection[id] == CHANGEMAP_NOW)
	{
		if(g_MenuPos[id] == MENU_MAPVOTE)
			make->AddOption(m_Translation->GetTranslation("MenuMapChangeAfter"));
		else
			make->AddOption(m_Translation->GetTranslation("MenuMapChangeNow"));
	}
	else if(g_MenuMapSelection[id] == CHANGEMAP_NEXTMAP)
		make->AddOption(m_Translation->GetTranslation("MenuMapChangeNextmap"));
		
	if(IsMoreMaps(i))
		make->AddOption(m_Translation->GetTranslation("MenuNext"));
	else
		make->AddOption("");

	make->AddOption(m_Translation->GetTranslation("MenuBack"));
}
void AdminMenu::GenPlayerList(int id,BATMenuBuilder *make)		// Add players to the menu
{
	int MenuPage = g_MenuPage[id];
	int PlayersAdded=0;
	int i=1;
	int PlayersToAdd;
	if(g_MenuMngr.GetMenuType() == 1)
		PlayersToAdd = 7;
	else
		PlayersToAdd = 5;

	if(MenuPage > 0)	// If we are generating menu thats not the first page, we know where to start
		i = g_MenuLastPlayer[id][MenuPage-1];

	while(i<=g_MaxClients)
	{
		if(g_IsConnected[i])
		{
			PlayersAdded++;

			make->AddOption(g_BATCore.GetPlayerName(i));
			g_MenuTarget[id][PlayersAdded] = i;

			if(PlayersAdded == PlayersToAdd)
				break;
		}
		i++;
	}
	while(PlayersAdded < PlayersToAdd)		// in case we dident add all server players we add magic to the menu, so we get it all filled up
	{
		PlayersAdded++;
		g_MenuTarget[id][PlayersAdded] = 0;
		make->AddOption("");
	}
	Get8Option(id,make);

	if(IsMorePlayers(i+1))
		make->AddOption(m_Translation->GetTranslation("MenuNext"));
	else
		make->AddOption("");

	if(MenuPage > 1 || g_MenuPos[id] != MENU_ROOT)
		make->AddOption(m_Translation->GetTranslation("MenuBack"));
	else
		make->AddOption(m_Translation->GetTranslation("MenuExit"));
	g_MenuLastPlayer[id][MenuPage] = i + 1;
}
void AdminMenu::DoOptionTenorNine(int id,int option)	// Since this can be handled generic in all menus, we have it in a separate function to make the MenuChoice() simpler to read. Returns true if the menu should be reshown
{
	if(option == 10)
	{
		if(g_MenuPage[id] > 0)
			g_MenuPage[id]--;
		else
			ResetMenu(id,false);
	}
	else if(option == 9)	// tried to change to the next menu
	{
		if(g_MenuPos[id] == MENU_MENURCON)
		{
			if(IsMoveMenuRconCmds(id))
				g_MenuPage[id]++;
		}

		else if(g_MenuPos[id] == MENU_MAP ||  g_MenuPos[id] == MENU_MAPVOTE)
		{
			if(IsMoreMaps(g_MenuLastMap[id]))
				g_MenuPage[id]++;
		}
		else if(IsMorePlayers(g_MenuLastPlayer[id][g_MenuPage[id]]))
			g_MenuPage[id]++;
	}
	g_MenuMngr.ShowMenu(id,g_AdminMenu);
}
bool AdminMenu::IsMoveMenuRconCmds(int index)	// This function is used to check if there is more players dont fit in the menu.
{
	int MenuPage = g_MenuPage[index];
	int CurrentIndex = g_MenuLastPlayer[index][MenuPage];
	if(CurrentIndex < (int)g_MenuRconCmdsCount)
		return true;

	return false;
}
bool AdminMenu::IsMoreMaps(int index)	// This function is used to check if there is more players dont fit in the menu.
{
	if(index < g_MapCount)
		return true;
	
	return false;
}
bool AdminMenu::IsMorePlayers(int index)	// This function is used to check if there is more players dont fit in the menu.
{
	for(int i=index;i<=g_MaxClients;i++)
	{
		if(g_IsConnected[i])
			return true;
	}
	return false;
}
void AdminMenu::ChangeBanMenuSetting(int id)		// This func changes the ban time via the menu
{
	switch (g_MenuBanSelection[id])
	{
		case 0 : 
			g_MenuBanSelection[id] = 5;
			break;
		case 5 : 
			g_MenuBanSelection[id] = 60;
			break;
		case 60 : 
			if(g_BATCore.HasAccess(id,ADMIN_TEMPBAN))
				g_MenuBanSelection[id] = 5;
			else
				g_MenuBanSelection[id] = 0;
			break;
	}
}
void AdminMenu::ChangeTeamMenuSetting(int id)		// This func changes the slap menu via the menu
{
	g_MenuTeamSelection[id]++;
	if(g_MenuTeamSelection[id] >= MAX_TEAMSINMENU)
		g_MenuTeamSelection[id] = 1;
}
void AdminMenu::ChangeSlapMenuSetting(int id)		// This func changes the slap menu via the menu
{
	switch (g_MenuSlapSelection[id])
	{
		case -10: 
			g_MenuSlapSelection[id] = -5;
			break;
		case -5 : 
			g_MenuSlapSelection[id] = -1;
			break;
		case -1 : 
			g_MenuSlapSelection[id] = 0;
			break;
		case 0 : 
			g_MenuSlapSelection[id] = 1;
		case 1 : 
			g_MenuSlapSelection[id] = 25;
			break;
		case 25 : 
			g_MenuSlapSelection[id] = -10;
			break;
	}
}
void AdminMenu::Get8Option(int id,BATMenuBuilder *make)	// This function gets the 8 th option in menu
{
	if(g_MenuPos[id] == MENU_KICK || g_MenuPos[id] == MENU_NAME)	// These menus dont have a 8th option
		make->AddOption("");
	
	else if(g_MenuPos[id] == MENU_TEAM)
	{
		if(g_MenuTeamSelection[id] == 0)
			ChangeTeamMenuSetting(id);

		make->AddOption(g_BATCore.GetTeamName(g_MenuTeamSelection[id]));
	}
	else if(g_MenuPos[id] == MENU_BAN)
	{
		if(g_MenuBanSelection[id] == 0 && g_BATCore.HasAccess(id,ADMIN_TEMPBAN))
			g_MenuBanSelection[id] = 5;

		if(g_MenuBanSelection[id] == 0)
			make->AddOption(m_Translation->GetTranslation("MenuBanPerm"));
		else if(g_MenuBanSelection[id] == 5)
			make->AddOption(m_Translation->GetTranslation("MenuBan5"));
		else if(g_MenuBanSelection[id] == 60)
			make->AddOption(m_Translation->GetTranslation("MenuBan60"));
	}
	else if(g_MenuPos[id] == MENU_SLAP)
	{
		if(g_MenuSlapSelection[id] == 0)
			make->AddOption(m_Translation->GetTranslation("MenuSlapNoDmg"));
		else if(g_MenuSlapSelection[id] == 1)
			make->AddOption(m_Translation->GetTranslation("MenuSlap1Dmg"));
		else if(g_MenuSlapSelection[id] == 25)
			make->AddOption(m_Translation->GetTranslation("MenuSlap25Dmg"));
		else if(g_MenuSlapSelection[id] == -1)
			make->AddOption(m_Translation->GetTranslation("MenuSlapTo1"));
		else if(g_MenuSlapSelection[id] == -5)
			make->AddOption(m_Translation->GetTranslation("MenuSlay"));
		else if(g_MenuSlapSelection[id] == -10)
			make->AddOption(m_Translation->GetTranslation("MenuSlayRules"));
	}
}

void AdminMenu::ResetMenu(int id,bool ReShowMenu)	// This function resets a players menu position, and can reshow the menu if need be
{
	g_MenuPage[id] = 0;
	g_MenuPos[id] = MENU_ROOT;
	g_MenuLastMap[id] = 0;

	if(ReShowMenu == true)
		g_MenuMngr.ShowMenu(id,g_AdminMenu);

	RETURN_META(MRES_SUPERCEDE);
}
