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

#include "BATMenu.h"
#include "BATCore.h"
#include "keyvalues.h"
#include "hl2sdk/recipientfilters.h"
#include "usermessages.h"
#include "color.h"
#include "const.h"
#include "sh_string.h"
#include <sh_vector.h>
#include <bitbuf.h>

#define PluginBase PluginCore
//#define PluginBase g_ForgiveTKCore

player_t g_UserInfoMenu[MAXPLAYERS+2];
extern BATMenuMngr g_MenuMngr;
BATMenuBuilder *g_MenuBuilder;

#define MAX_MENUOPTIONLENGTH 48
#define MaxMenuSize 1024 // Max is 255, but we dont want to send more then 250
#define MaxPrSend 240	// This is the maximum we can send at once, the total max is 255, but the source engines adds a few bytes after us. So we need to be safe
#define MENU_OPEN_TIME -1 // 30

char g_MenuOption[10][MAX_MENUOPTIONLENGTH+1];
char g_MenuTitle[MAX_MENUOPTIONLENGTH];

char g_OldMenu[MaxMenuSize+1]; // Used to resend the menus
menuId g_ReShowMenuID;
int g_IsReShowOfMenu;
bool g_IsMenuReShow;

int g_MenuKeys;
int g_ActiveOptions=0;

unsigned int g_ActiveMenus=0;
SourceHook::CVector<menu_t *>g_MenuList;

void BATMenuBuilder::AddOption(const char *fmt)
{
	if(g_ActiveOptions >= 10)
	{
		PluginCore.PluginSpesficLogPrint("!ERROR! To many options added to a menu, '%s' was ignored",fmt);
		return;
	}	

	_snprintf(g_MenuOption[g_ActiveOptions],MAX_MENUOPTIONLENGTH,"%s",fmt);
	g_ActiveOptions++;
}
void BATMenuBuilder::SetTitle(const char *title)
{
	_snprintf(g_MenuTitle,MAX_MENUOPTIONLENGTH,"%s",title);
}
void BATMenuBuilder::SetKeys(int keys)
{
	g_MenuKeys = keys;
}
int BATMenuMngr::RegisterMenu(IMenu *menu)
{
	g_MenuList.push_back(new menu_t);

	g_MenuList[g_ActiveMenus]->menu = menu;
	g_ActiveMenus++;
	return (int)(g_ActiveMenus-1);
}
void BATMenuMngr::ShowMenu(int player, menuId menuid)
{
	g_UserInfoMenu[player].index = player;
	g_UserInfoMenu[player].menu = menuid;
	g_HasMenuOpen[player] = true;

	bool MenuGoodChoice = g_MenuList[menuid]->menu->Display(g_MenuBuilder,player);
	if(!MenuGoodChoice)
		return;

	char MenuMsg[MaxMenuSize+1];
	int len=0;

	if(g_MenyType == 1)
	{
		RecipientFilter rf;

		if (player == 0) 
		{
			for(int i=1;i<=g_MaxClients;i++) if(g_IsConnected[i] && !g_UserInfo[i].IsBot)
			{
				g_UserInfoMenu[i].index = i;
				g_UserInfoMenu[i].menu = menuid;
				g_HasMenuOpen[i] = true;
				rf.AddPlayer(i);
			}
		} else {
			rf.AddPlayer(player);
		}
		rf.MakeReliable();

		len += _snprintf(MenuMsg,MaxMenuSize,"%s\n",g_MenuTitle);
		for(int i=0;i<g_ActiveOptions;i++)
		{
			if(strlen(g_MenuOption[i]) == 0 || g_MenuOption[i][0] == '\n')
				len += _snprintf(&(MenuMsg[len]),MaxMenuSize-len,"\n");
			else
				len += _snprintf(&(MenuMsg[len]),MaxMenuSize-len,"%d. %s\n",i+1,g_MenuOption[i]);
		}
		ShowRadioMenu(player,MenuMsg);		
	}
	else
	{
		for(int i=MAX_MENUOPTIONLENGTH-1;i > 0;i--)
		{
			if(g_MenuTitle[i] == ':')
			{
				g_MenuTitle[i] = '\0';
				break;
			}
		}

		KeyValues *kv = new KeyValues( "menu" );
		kv->SetString( "title", g_MenuTitle );
		kv->SetInt( "level", 1 );
		kv->SetColor( "color", Color( 255, 0, 0, 255 ));
		kv->SetInt( "time", 20 );
		kv->SetString( "msg", "Make your selection in the menu" );

		char num[11], msg[MAX_MENUOPTIONLENGTH+1], cmd[MAX_MENUOPTIONLENGTH+1];

		for(int i=0;i<g_ActiveOptions;i++)
		{
			if(strlen(g_MenuOption[i]) == 0 || g_MenuOption[i][0] == '\n') // i >= g_ActiveOptions || 
				continue;
			
			_snprintf( num, 10, "%i", i );
			_snprintf( cmd, MAX_MENUOPTIONLENGTH, "menuselect %i", i+1 );
			_snprintf( msg, MAX_MENUOPTIONLENGTH, "%i. %s", i+1,g_MenuOption[i] );

			KeyValues *item1 = kv->FindKey( num, true );
			item1->SetString( "msg", msg );
			item1->SetString( "command", cmd );
		}
		if(player == 0)
		{
			for(int i=1;i<=g_MaxClients;i++) if(g_IsConnected[i] && !g_UserInfo[i].IsBot)
			{
				g_UserInfoMenu[i].index = i;
				g_UserInfoMenu[i].menu = menuid;
				g_HasMenuOpen[i] = true;
				PluginCore.GetHelpers()->CreateMessage( g_UserInfo[i].PlayerEdict, DIALOG_MENU, kv, this );
			}
		}
		else
			PluginCore.GetHelpers()->CreateMessage( g_UserInfo[player].PlayerEdict, DIALOG_MENU, kv, this );

		kv->deleteThis();
	}
	g_ActiveOptions=0;
}
void BATMenuMngr::MenuChoice(int playerIndex, int choice)
{
	menuId menuid = g_UserInfoMenu[playerIndex].menu;
	g_HasMenuOpen[playerIndex] = false;

	switch(g_MenuList[menuid]->menu->MenuChoice(g_UserInfoMenu[playerIndex], choice))
	{
	case MENUSELECTION_GOOD:
		PluginCore.GetEngine()->ClientCommand(g_UserInfo[playerIndex].PlayerEdict,PLAYSOUND_GOOD);
		break;
	case MENUSELECTION_BAD:
		PluginCore.GetEngine()->ClientCommand(g_UserInfo[playerIndex].PlayerEdict,PLAYSOUND_BAD);
	    break;
	}
}
void BATMenuMngr::ShowRadioMenu(int id,char *MenuString)
{
	RecipientFilter rf;

	if (id == 0) 
	{
		rf.AddAllPlayers(g_MaxClients);
	} else {
		rf.AddPlayer(id);
	}
	rf.MakeReliable();

	char *ptr = MenuString;
	size_t len = strlen(MenuString);
	char save = 0;

	while (true)
	{
		if (len > MaxPrSend)
		{
			save = ptr[MaxPrSend];
			ptr[MaxPrSend] = '\0';
		}
		bf_write *buffer = PluginCore.GetEngine()->UserMessageBegin(static_cast<IRecipientFilter *>(&rf), g_ModSettings.MenuMsg);
		buffer->WriteWord((1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7)| (1<<8) | (1<<9));
		buffer->WriteChar(MENU_OPEN_TIME);
		buffer->WriteByte( (len > MaxPrSend) ? 1 : 0 );
		buffer->WriteString(ptr);
		PluginCore.GetEngine()->MessageEnd();
		if (len > MaxPrSend)
		{
			ptr[MaxPrSend] = save;
			ptr = &(ptr[MaxPrSend]);
			len -= MaxPrSend;
		} else {
			break;
		}
	}
}
void BATMenuMngr::ClearForMapchange()
{
	for(int i=1;i<=g_MaxClients;i++)
	{
		g_HasMenuOpen[i] = false;
	}
}
void BATMenuMngr::ShowCornerMsg(int id,const char *TheMsg)
{
	KeyValues *kv = new KeyValues( "msg" );
	kv->SetString( "title", TheMsg );
	kv->SetColor( "color", Color( 255, 0, 0, 255 ));
	kv->SetInt( "level", 5);
	kv->SetInt( "time", 10);
	g_BATCore.GetHelpers()->CreateMessage( g_UserInfo[id].PlayerEdict, DIALOG_MSG, kv, this );
	kv->deleteThis();
}
#ifndef DIALOG_ASKCONNECT
#define DIALOG_ASKCONNECT 4		// Im not updating all the sdk around so we do this.
#endif

void BATMenuMngr::ShowClientReConnectDialog(int id)
{

	
}
#if BAT_DEBUG == 1
/*
void BATMenuMngr::ShowESCTextBox(int id)
{
	KeyValues *kv = new KeyValues( "menu" );
	kv->SetString( "title", "You've got options, hit ESC" );
	kv->SetInt( "level", 1 );
	kv->SetColor( "color", Color( 255, 0, 0, 255 ));
	kv->SetInt( "time", 20 );
	kv->SetString( "msg", "Pick an option\nOr don't." );

	PluginCore.GetHelpers()->CreateMessage( g_UserInfo[id].PlayerEdict, DIALOG_TEXT, kv, this );
	kv->deleteThis();
}
*/
#endif