/* ======== Basic Admin tool ========
* Copyright (C) 2004-2007 Erling K. Sæterdal
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Erling K. Sæterdal ( EKS )
* Credits:
*	Menu code based on code from CSDM ( http://www.tcwonline.org/~dvander/cssdm ) Created by BAILOPAN
* Helping on misc errors/functions: BAILOPAN,karma,LDuke,sslice,devicenull,PMOnoTo,cybermind ( most who idle in #sourcemod on GameSurge realy )
* ============================ */

#include <time.h>
#include "BATCore.h"
#include "MapVote.h"
#include "const.h"


extern int g_MaxClients;
extern int g_MapVoteMenu;

extern float g_ChangeMapTime;

VoteInfoStruct VoteInfo;

extern ModSettingsStruct g_ModSettings;
extern BATMenuMngr g_MenuMngr;

extern SourceHook::CVector<MapStuct *>g_MapList;
extern SourceHook::CVector<MapsVotesStruct *>g_VotesOnMap;

extern char g_CurrentMapName[MAX_MAPNAMELEN+1];
extern int g_CurrentMapIndex;
extern int g_MapCount;

extern float g_fMapChangeTime;
extern bool g_bChangeMapCountDown;

char g_MapNameInVote[MapsInPublicVote][MAX_MAPNAMELEN+1];
char g_SayMapList[192];					// Contains the list of maps when ppl do a say map list
				
VoteCmdInfo g_VoteCmdInfo;

int g_PlayersVote[MAXPLAYERS+1] = {MAPVOTE_NOVOTE};
int g_MapIndexInVote[MapsInPublicVote];
int g_CurrentMapExtendTimes=0;		// The amount of times the current map has been extended
int g_OrgTimelimit = 0;
int g_OrgWinLimit=0;
int g_CurTimeLimit = -5000;				// What the last time limit set by the plugin is
int g_CurWinLimit = 0;				// What the last time limit set by the plugin is
int g_MaxExtendTimes = 0;
int g_MapExtendOption = 1;

int g_MapVotePercentageRequired;				// Percentage of map votes required
int g_PlayerVotePercentageRequired;				// Percentage of player votes like vote kick/ban votes required


void MapVote::StartCmdCustomVote(const char *Question,const char *SrvCmd)
{
	ResetArrays();
	VoteInfo.Method = VOTEMETHOD_MENU;
	VoteInfo.VoteEndTime = g_BATCore.GetTimeLeft() - PublicVoteTime;
	VoteInfo.Status = VOTE_VOTERUNNING;
	VoteInfo.Type = VOTETYPE_CMDCUSTOM;
	_snprintf(g_VoteCmdInfo.Question,MAX_CUSTOMCMDVOTELEN,"%s",Question);
	if(SrvCmd != NULL)
	{
		g_VoteCmdInfo.ExecuteSrvCmd = true;
		_snprintf(g_VoteCmdInfo.SrvCmd,MAX_CMDSIZE,"%s",SrvCmd);
	}
	else
		g_VoteCmdInfo.ExecuteSrvCmd =false;

	for(int i=1;i<=g_MaxClients;i++)
	{
		g_PlayersVote[i] = MAPVOTE_NOVOTE;
		if(g_IsConnected[i] && g_UserInfo[i].IsBot == false)
		{
			VoteInfo.PlayersInVote++;
		}
	}	
	g_MenuMngr.ShowMenu(0,g_MapVoteMenu);
}
void MapVote::StartVoteKick(int PlayerIndex)
{
	ResetArrays();
	VoteInfo.VoteEndTime = g_BATCore.GetTimeLeft() - PublicVoteTime;
	
	VoteInfo.Method = VOTEMETHOD_MENU;	
	VoteInfo.Status = VOTE_VOTERUNNING;
	VoteInfo.Type = VOTETYPE_KICKPLAYER;
	_snprintf(g_VoteCmdInfo.Question,MAX_CUSTOMCMDVOTELEN,"Kick %s?",g_BATCore.GetPlayerName(PlayerIndex));
    _snprintf(g_VoteCmdInfo.SrvCmd,MAX_CMDSIZE,"%d",PlayerIndex);

	for(int i=1;i<=g_MaxClients;i++)
	{
		g_PlayersVote[i] = MAPVOTE_NOVOTE;
		if(g_IsConnected[i] && g_UserInfo[i].IsBot == false)
		{
			VoteInfo.PlayersInVote++;
		}
	}	
	g_MenuMngr.ShowMenu(0,g_MapVoteMenu);
}
void MapVote::StartVoteBan(int PlayerIndex)
{
	ResetArrays();
	VoteInfo.VoteEndTime = g_BATCore.GetTimeLeft() - PublicVoteTime;

	VoteInfo.Method = VOTEMETHOD_MENU;	
	VoteInfo.Status = VOTE_VOTERUNNING;
	VoteInfo.Type = VOTETYPE_BANPLAYER;

	_snprintf(g_VoteCmdInfo.Question,MAX_CUSTOMCMDVOTELEN,"Ban %s?",g_BATCore.GetPlayerName(PlayerIndex));
	_snprintf(g_VoteCmdInfo.SrvCmd,MAX_CMDSIZE,"%d",PlayerIndex);

	for(int i=1;i<=g_MaxClients;i++)
	{
		g_PlayersVote[i] = MAPVOTE_NOVOTE;
		if(g_IsConnected[i] && g_UserInfo[i].IsBot == false)
		{
			VoteInfo.PlayersInVote++;
		}
	}	
	g_MenuMngr.ShowMenu(0,g_MapVoteMenu);
}
bool MapVote::StartPublicMapVote(float TimeLeft,bool ChangeAfterDone)
{
	ResetArrays();
	if(g_BATCore.GetBATVar().GetMapExtendOption() == 2)
	{
		VoteInfo.Method = VOTEMETHOD_CHAT;
		VoteInfo.MapsInVote = g_MapCount;
		VoteInfo.VoteEndTime = TimeLeft - PublicVoteTimeChat;
	}
	else	// Menu Powered menus will be used
	{
		VoteInfo.Method = VOTEMETHOD_MENU;

		if(g_MapCount > MapsInPublicVote)
			VoteInfo.MapsInVote = MapsInPublicVote;
		else
			VoteInfo.MapsInVote = g_MapCount-1;		// We remove one since we cant vote for currentmap ( Its shown as extend)

		VoteInfo.VoteEndTime = TimeLeft - PublicVoteTime;
		
		if(g_MapCount < 2)
		{
			g_BATCore.AddLogEntry("ERROR! To few maps in mapcycle to generate vote, 2 is required to run a vote");
			VoteInfo.Status = VOTE_ERROR;
			return false;
		}
	}
	VoteInfo.Status = VOTE_VOTERUNNING;
	GenListOfMapsForVote();

	
	if(ChangeAfterDone)
		VoteInfo.Type = VOTETYPE_PUBLICVOTEENDFAST;
	else
		VoteInfo.Type = VOTETYPE_PUBLICVOTE;
	
	g_MaxExtendTimes = g_BATCore.GetBATVar().GetMapExtendTimes();

	for(int i=1;i<=g_MaxClients;i++)
	{
		g_PlayersVote[i] = MAPVOTE_NOVOTE;
		if(g_IsConnected[i] && g_UserInfo[i].IsBot == false)
		{
				VoteInfo.PlayersInVote++;
		}
	}
	if(VoteInfo.Method == VOTEMETHOD_MENU)
		g_MenuMngr.ShowMenu(0,g_MapVoteMenu);

	else if(VoteInfo.Method == VOTEMETHOD_CHAT)
	{
		g_BATCore.MessagePlayer(0,"[BAT] A vote is now enabled, please vote for what map you want next");
		g_BATCore.MessagePlayer(0,"[BAT] To vote for a map say 'vote mapname' (Example: 'vote %s')",g_MapList[0]->MapName);
	}
	return true;
}
void MapVote::StartMultiCustomMapVote(int MapIndex[MapsInPublicVote],int MapsInVote,int WhenToChangeMap )
{
	ResetArrays();

	VoteInfo.Method = VOTEMETHOD_MENU;
	VoteInfo.MapsInVote = MapsInVote;
	VoteInfo.VoteEndTime = g_BATCore.GetTimeLeft() - PublicVoteTime;
	VoteInfo.Status = VOTE_VOTERUNNING;

	for(int i=0;i<MapsInVote;i++)
	{
		g_MapIndexInVote[i] = MapIndex[i];
		_snprintf(g_MapNameInVote[i],MAX_MAPNAMELEN,g_MapList[MapIndex[i]]->MapName);
	}

	if(WhenToChangeMap == CHANGEMAP_NEXTMAP)
		VoteInfo.Type = VOTETYPE_PUBLICVOTE; // VOTETYPE_PUBLICVOTE
	else
		VoteInfo.Type = VOTETYPE_PUBLICVOTEENDFAST ;
	
	for(int i=1;i<=g_MaxClients;i++)
	{
		g_PlayersVote[i] = MAPVOTE_NOVOTE;
		if(g_IsConnected[i] && g_UserInfo[i].IsBot == false)
		{
				VoteInfo.PlayersInVote++;
		}
	}
	if(VoteInfo.Method == VOTEMETHOD_MENU)
		g_MenuMngr.ShowMenu(0,g_MapVoteMenu);
}
void MapVote::StartCustomMapVote(int MapIndex,int WhenToChangeMap )
{
	ResetArrays();
	if(WhenToChangeMap == CHANGEMAP_NEXTMAP)
		VoteInfo.Type = VOTETYPE_CUSTOMNEXTMAP;
	else
		VoteInfo.Type = VOTETYPE_CUSTOM;

	if(g_BATCore.GetBATVar().GetMapExtendOption() == 2)
		VoteInfo.Method = VOTEMETHOD_CHAT;
	else 
		VoteInfo.Method = VOTEMETHOD_MENU;

	VoteInfo.VoteEndTime = g_BATCore.GetTimeLeft() - PublicVoteTime;
	VoteInfo.Status = VOTE_VOTERUNNING;
	VoteInfo.MapsInVote = 1;
	g_MapIndexInVote[0] = MapIndex;

	_snprintf(g_MapNameInVote[0],MAX_MAPNAMELEN,g_MapList[MapIndex]->MapName);
	
	for(int i=1;i<=g_MaxClients;i++)
	{
		g_PlayersVote[i] = MAPVOTE_NOVOTE;
		if(g_IsConnected[i] && g_UserInfo[i].IsBot == false)
		{
			VoteInfo.PlayersInVote++;
		}
	}
	if(VoteInfo.Method == VOTEMETHOD_MENU)
		g_MenuMngr.ShowMenu(0,g_MapVoteMenu);
	else if(VoteInfo.Method == VOTEMETHOD_CHAT)
	{
		g_BATCore.MessagePlayer(0,"[BAT] A vote is now enabled, please vote for what map you want next");
		g_BATCore.MessagePlayer(0,"[BAT] To vote for a map say 'vote mapname' (Example: 'vote %s')",g_MapList[0]->MapName);
	}
}
void MapVote::AddRTVVote(int id )
{
	if(VoteInfo.Status != VOTE_ROCKTHEVOTESTARTED)
		StartRockTheVote();

	if(g_PlayersVote[id] == MAPVOTE_NOVOTE)
	{
		g_PlayersVote[id] = MAPVOTE_ROCKTHEVOTE;
		VoteInfo.PlayersThatHasVoted++;
		g_BATCore.MessagePlayer(0,"%s wants to rockthevote ( %d% of required %d% )",g_BATCore.GetPlayerName(id),GetPrecentageOfPlayers(VoteInfo.PlayersThatHasVoted,VoteInfo.PlayersInVote),g_MapVotePercentageRequired);
	}
	//if(VoteInfo.PlayersThatHasVoted >= VoteInfo.PlayersInVote)
	if(GetPrecentageOfPlayers(VoteInfo.PlayersThatHasVoted,VoteInfo.PlayersInVote) >= g_MapVotePercentageRequired)
		StartPublicMapVote(g_BATCore.GetTimeLeft(),true);
}
void MapVote::StartRockTheVote()
{
	ResetArrays();

	VoteInfo.Status = VOTE_ROCKTHEVOTESTARTED;
	VoteInfo.Type = VOTETYPE_ROCKTHEVOTE;
	VoteInfo.VoteEndTime = g_BATCore.GetTimeLeft() - RTVTime;

	for(int i=1;i<=g_MaxClients;i++)
	{
		g_PlayersVote[i] = MAPVOTE_NOVOTE;
		if(g_IsConnected[i] && g_UserInfo[i].IsBot == false)
		{
				VoteInfo.PlayersInVote++;
		}
	}
	g_BATCore.MessagePlayer(0,"[BAT] Rockthevote is now enabled, type 'say rockthevote' if you want to change map");
}

void MapVote::EndMapVote()
{
	if(VoteInfo.Type == VOTETYPE_PUBLICVOTE || VoteInfo.Type == VOTETYPE_PUBLICVOTEENDFAST)
		EndMapVotePublic();
	else if(VoteInfo.Type == VOTETYPE_CUSTOM || VoteInfo.Type == VOTETYPE_CUSTOMNEXTMAP )
		EndMapVoteCustom();
	else if(VoteInfo.Type == VOTETYPE_ROCKTHEVOTE)
		EndRockthevote();
	else if(VoteInfo.Type == VOTETYPE_CMDCUSTOM || VoteInfo.Type == VOTETYPE_KICKPLAYER || VoteInfo.Type == VOTETYPE_BANPLAYER)
		EndCmdVoteCustom();

	VoteInfo.Method = VOTEMETHOD_NOVOTE;
}
void MapVote::EndCmdVoteCustom()
{
	int Yes=0;
	int No=0;
	int TotalVotes=0;

	for(int i=1;i<=g_MaxClients;i++)
	{
		if(g_PlayersVote[i] == MAPVOTE_YES)
		{
			Yes++;
			TotalVotes++;
		}
		else if(g_PlayersVote[i] == MAPVOTE_NO)
		{
			No++;
			TotalVotes++;
		}
	}
	int YesPrecentageVotes = GetPrecentageOfPlayers(Yes,TotalVotes);

	if(VoteInfo.Type == VOTETYPE_CMDCUSTOM)
	{
		if(YesPrecentageVotes >= g_PlayerVotePercentageRequired)
		{
			g_BATCore.MessagePlayer(0,"[BAT] Vote success for '%s' (%d votes)",g_VoteCmdInfo.Question,Yes);
			if(g_VoteCmdInfo.ExecuteSrvCmd)
				g_BATCore.ServerCommand(g_VoteCmdInfo.SrvCmd);

			VoteInfo.Status = VOTE_WAITING;
		}
		else
		{
			g_BATCore.MessagePlayer(0,"[BAT] Vote failed with %d no votes (%d yes) ",No,Yes);
			VoteInfo.Status = VOTE_WAITING;
		}
	}
	else if(VoteInfo.Type == VOTETYPE_KICKPLAYER) // It was a votekick
	{
		int TargetIndex = atoi(g_VoteCmdInfo.SrvCmd);

		if(YesPrecentageVotes >= g_PlayerVotePercentageRequired)
		{
			g_BATCore.MessagePlayer(0,"[BAT] Vote kick success for '%s' (%d votes)",g_BATCore.GetPlayerName(TargetIndex),Yes);
			g_BATCore.ServerCommand("kickid %d Vote kick by players",g_UserInfo[TargetIndex].Userid);

			VoteInfo.Status = VOTE_WAITING;
		}
		else
		{
			g_BATCore.MessagePlayer(0,"[BAT] Vote kick failed with %d no votes (%d yes) ",No,Yes);
			VoteInfo.Status = VOTE_WAITING;
		}
	}
	else if(VoteInfo.Type == VOTETYPE_BANPLAYER) // It was a vote ban
	{
		int TargetIndex = atoi(g_VoteCmdInfo.SrvCmd);

		if(YesPrecentageVotes >= g_PlayerVotePercentageRequired)
		{
			g_BATCore.MessagePlayer(0,"[BAT] vote ban success for '%s' (%d votes)",g_BATCore.GetPlayerName(TargetIndex),Yes);
			g_BATCore.ServerCommand("banid %d.0 %d",g_BATCore.GetBATVar().GetVoteBanTime(),g_UserInfo[TargetIndex].Userid);
			g_BATCore.ServerCommand("kickid %d Vote banned by players",g_UserInfo[TargetIndex].Userid);

			VoteInfo.Status = VOTE_WAITING;
		}
		else
		{
			g_BATCore.MessagePlayer(0,"[BAT] vote ban failed with %d no votes (%d yes) ",No,Yes);
			VoteInfo.Status = VOTE_WAITING;
		}
	}
}
void MapVote::EndRockthevote()
{
	int Yes=0;
	int No=0;
	int TotalVotes=0;

	for(int i=1;i<=g_MaxClients;i++) if(g_IsConnected[i])
	{
		if(g_PlayersVote[i] == MAPVOTE_ROCKTHEVOTE)
		{
			Yes++;
			TotalVotes++;
		} 
		else if(g_PlayersVote[i] == MAPVOTE_NOVOTE)
		{
			No++;
			TotalVotes++;
		}
	}
	
	int YesPrecentageVotes = GetPrecentageOfPlayers(Yes,TotalVotes);
	
	if(YesPrecentageVotes >= g_MapVotePercentageRequired)
	{
		g_BATCore.MessagePlayer(0,"[BAT] Rockthevote was successful, starting vote (with %d votes) ",Yes);
		StartPublicMapVote(g_BATCore.GetTimeLeft(),true);
	}
	else
	{
		g_BATCore.MessagePlayer(0,"[BAT] Rockthevote failed with %d no votes (%d yes) ",No,Yes);
		VoteInfo.Status = VOTE_WAITING;
	}
}
void MapVote::EndMapVoteCustom()
{
	int Yes=0;
	int No=0;
	int TotalVotes=0;

	for(int i=1;i<=g_MaxClients;i++)
	{
		if(g_PlayersVote[i] == MAPVOTE_YES)
		{
			Yes++;
			TotalVotes++;
		}
		else if(g_PlayersVote[i] == MAPVOTE_NO)
		{
			No++;
			TotalVotes++;
		}
	}
	int YesPrecentageVotes = GetPrecentageOfPlayers(Yes,TotalVotes);
	
	if(YesPrecentageVotes >= g_MapVotePercentageRequired)
	{
		g_BATCore.MessagePlayer(0,"[BAT] %s won the map vote changing map(with %d votes) ",g_MapNameInVote[0],Yes);
		VoteInfo.Status = VOTE_DONE;

		if(VoteInfo.Type == VOTETYPE_CUSTOM)
			g_BATCore.StartChangelevelCountDown(g_MapNameInVote[0]);
		else
		{
			g_BATCore.GetAdminCmds()->SetNextmap(-1,g_MapNameInVote[0]);

			if(WaitForModEvent())
				VoteInfo.Status = VOTE_DONE_WAITFOREVENT;			// we tell the "task" system, that it should control what the nextmap is
			else if(ModHandlesMapChange())
				VoteInfo.Status = VOTE_DONE;
			else
			{
				g_fMapChangeTime = 0.0;
				g_bChangeMapCountDown = true;
			}
			g_BATCore.MessagePlayer(0,"[BAT] Nextmap is set to %s",g_MapNameInVote[0]);
		}
	}
	else
	{
		g_BATCore.MessagePlayer(0,"[BAT] Map vote failed with %d no votes (%d yes) ",No,Yes);
		VoteInfo.Status = VOTE_WAITING;
	}
}
void MapVote::EndMapVotePublic()
{
	/*
	for(unsigned int i=0;i<g_VotesOnMap.size();i++)
		g_VotesOnMap[i]->MapVotes = 0;
*/

	int ExtendVotes=0;
	int TotalVotes = 0;
	int NextMapIndex = 0;
	int NextMapVotes = 0;

	for(int i=1;i<=g_MaxClients;i++) if(g_IsConnected[i])
	{
		if(g_PlayersVote[i] == MAPVOTE_EXTEND)
			ExtendVotes++;
		else if(g_PlayersVote[i] != MAPVOTE_NOVOTE)
		{
			g_VotesOnMap[g_PlayersVote[i]]->MapVotes++;
			TotalVotes++;
		}
	}

	if((TotalVotes + ExtendVotes) == 0)		// Nobody voted for anything. We stop stop here
	{
		VoteInfo.Status = VOTE_DONE_NOVOTES;
		g_BATCore.MessagePlayer(0,"[BAT] Nobody voted in nextmap vote, nextmap will remain %s ",g_NextMapName);
		return;
	}
	//for(int i=0;i<VoteInfo.MapsInVote;i++)
	for(int i=0;i<g_MapCount;i++)
	{
		if(NextMapVotes < g_VotesOnMap[i]->MapVotes)
		{
			NextMapIndex = i;
			NextMapVotes = g_VotesOnMap[i]->MapVotes;
		}
	}
	if(VoteInfo.Method == VOTEMETHOD_CHAT)
	{
		_snprintf(g_MapNameInVote[NextMapIndex],MAX_MAPNAMELEN,g_MapList[NextMapIndex]->MapName);
		g_MapIndexInVote[NextMapIndex] = NextMapIndex;
	}
	
	if(ExtendVotes > NextMapVotes)
	{
		if(g_CurrentMapExtendTimes == 0)
		{
			g_OrgTimelimit = g_BATCore.GetBATVar().GetTimelimitCvar()->GetInt();
			
			if(g_ModSettings.SupportsWinlimit)
				g_OrgWinLimit = g_BATCore.GetBATVar().GetWinlimitCvar()->GetInt();
		}

		VoteInfo.Status = VOTE_WAITING;
		g_CurrentMapExtendTimes++;

		g_CurTimeLimit = g_BATCore.GetBATVar().GetTimelimitCvar()->GetInt() + g_OrgTimelimit;
		g_BATCore.GetBATVar().GetTimelimitCvar()->SetValue(g_CurTimeLimit);

		if(g_ModSettings.SupportsWinlimit)
		{
			g_CurWinLimit =  g_BATCore.GetBATVar().GetWinlimitCvar()->GetInt() + g_OrgWinLimit;
			g_BATCore.GetBATVar().GetWinlimitCvar()->SetValue(g_CurWinLimit);
		}
		g_BATCore.MessagePlayer(0,"[BAT] Extending current map");
	}
	else
	{
		if(VoteInfo.Type == VOTETYPE_PUBLICVOTE)
		{
			g_BATCore.MessagePlayer(0,"[BAT] %s won with %d votes, and is set to nextmap ",g_MapNameInVote[NextMapIndex],NextMapVotes);

			if(WaitForModEvent())
				VoteInfo.Status = VOTE_DONE_WAITFOREVENT;
			else if(ModHandlesMapChange())
				VoteInfo.Status = VOTE_DONE;
			else	// The running mod has no event, so we just do it manually
			{
				VoteInfo.Status = VOTE_DONE;
				g_fMapChangeTime = 0.0;
				g_bChangeMapCountDown=true;
			}
		}
		else
		{
			VoteInfo.Status = VOTE_DONE;
			g_ChangeMapTime = g_BATCore.GetTimeLeft() - g_BATCore.GetBATVar().GetMapChangeDelay();
			g_BATCore.MessagePlayer(0,"[BAT] Changing to %s ( Won with %d votes ) ",g_MapNameInVote[NextMapIndex],NextMapVotes);
			g_BATCore.StartChangelevelCountDown(g_MapNameInVote[NextMapIndex]);
		}
		g_BATCore.GetAdminCmds()->SetNextmap(-1,g_MapNameInVote[NextMapIndex]);
	}
}
void MapVote::ResetTimeLimit()
{
	if(g_CurTimeLimit == -5000)
		return; // This is the first time the map loads, so nothing to do realy

	if(g_CurTimeLimit == g_BATCore.GetBATVar().GetTimelimitCvar()->GetInt())
	{
		g_BATCore.ConsolePrint(ID_SERVER,"[BAT] Reseting mp_timelimit back to %d",g_OrgTimelimit);
		g_BATCore.GetBATVar().GetTimelimitCvar()->SetValue(g_OrgTimelimit);
	}
}
/**************************************************** Menu function ********************************************/
bool MapVote::Display(BATMenuBuilder *make, int playerIndex)
{
	if(playerIndex != 0)
	{
		if(!g_UserInfo[playerIndex].IsBot)
			g_BATCore.GetEngine()->ClientCommand(g_UserInfo[playerIndex].PlayerEdict,SOUND_SHOWVOTE);
	}
	else
	{
		for(int i=1;i<=g_MaxClients;i++)
		{
			if(!g_UserInfo[i].IsBot && g_IsConnected[i])
				g_BATCore.GetEngine()->ClientCommand(g_UserInfo[i].PlayerEdict,SOUND_SHOWVOTE);
		}
	}

	if(g_MenuMngr.GetMenuType() != 1)
		g_BATCore.MessagePlayer(playerIndex,"You need to press ESC to see the map vote menu");

	if(VoteInfo.Type == VOTETYPE_PUBLICVOTE || VoteInfo.Type == VOTETYPE_PUBLICVOTEENDFAST)
	{
		make->SetTitle("Vote for nextmap:");

		for(int i=0;i<MapsInPublicVote;i++)
		{
			if(g_MapIndexInVote[i] == -1)	// The menu option is blank.
				make->AddOption("");
			else
				make->AddOption(g_MapNameInVote[i]);
		}

		if(g_CurrentMapExtendTimes < g_MaxExtendTimes)
			make->AddOption("Extend currentmap");

		make->SetKeys( (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7)| (1<<8) | (1<<9));
		return true;
	}
	else if(VoteInfo.Type == VOTETYPE_CUSTOM || VoteInfo.Type == VOTETYPE_CUSTOMNEXTMAP )
	{
		char MenuTitle[128];
		if(VoteInfo.Type == VOTETYPE_CUSTOM)
			_snprintf(MenuTitle,127,"Change to %s",g_MapNameInVote[0]);
		else
			_snprintf(MenuTitle,127,"Set nextmap to %s",g_MapNameInVote[0]);

		make->SetTitle(MenuTitle);

		make->AddOption("Yes");
		make->AddOption("No");
	
		make->SetKeys( (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7)| (1<<8) | (1<<9));
		return true;
	}
	else if(VoteInfo.Type == VOTETYPE_CMDCUSTOM || VoteInfo.Type == VOTETYPE_KICKPLAYER || VoteInfo.Type == VOTETYPE_BANPLAYER)
	{
		make->SetTitle(g_VoteCmdInfo.Question);

		make->AddOption("Yes");
		make->AddOption("No");

		make->SetKeys( (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7)| (1<<8) | (1<<9));
		return true;
	}
	return false;
}
MenuSelectionType MapVote::MenuChoice(player_t player, int option)
{
	if(VoteInfo.Method == VOTEMETHOD_NOVOTE)
		return MENUSELECTION_NOSOUND;

	int id = player.index;		
	const char *PlayerName = g_BATCore.GetPlayerName(id);
	
	if(VoteInfo.Type == VOTETYPE_PUBLICVOTE || VoteInfo.Type == VOTETYPE_PUBLICVOTEENDFAST)
	{
		int MapIndex = option - 1;

		if(option <= VoteInfo.MapsInVote && g_MapIndexInVote[MapIndex] != -1)	// Means the user voted for map
		{
			g_BATCore.MessagePlayer(0,"[BAT] %s voted for %s",PlayerName,g_MapNameInVote[MapIndex]);
			g_PlayersVote[id] = MapIndex;
		}
		else if(option == 5 && g_CurrentMapExtendTimes < g_MaxExtendTimes)	// Means its a map extend
		{
			g_BATCore.MessagePlayer(0,"[BAT] %s voted to extend (%s)",PlayerName,g_CurrentMapName);
			g_PlayersVote[id] = MAPVOTE_EXTEND;
		}
		else
		{
			g_MenuMngr.ShowMenu(id,g_MapVoteMenu);
			return MENUSELECTION_BAD;
		}
		
		VoteInfo.PlayersThatHasVoted++;
	}
	else if(VoteInfo.Type == VOTETYPE_CUSTOM || VoteInfo.Type == VOTETYPE_CUSTOMNEXTMAP)
	{
		if(option == MAPVOTE_YES)	// Means the user voted for map
		{
			g_BATCore.MessagePlayer(0,"[BAT] %s voted yes",PlayerName);
			g_PlayersVote[id] = MAPVOTE_YES;
		}
		else if(option == MAPVOTE_NO)	// Means its a map extend
		{
			g_BATCore.MessagePlayer(0,"[BAT] %s voted no",PlayerName);
			g_PlayersVote[id] = MAPVOTE_NO;
		}
		else
		{
			g_MenuMngr.ShowMenu(id,g_MapVoteMenu);
			return MENUSELECTION_BAD;
		}
		
		VoteInfo.PlayersThatHasVoted++;
	}
	else if(VoteInfo.Type == VOTETYPE_CMDCUSTOM || VOTETYPE_KICKPLAYER)
	{
		if(option == MAPVOTE_YES)	// Means the user voted for map
		{
			g_BATCore.MessagePlayer(0,"[BAT] %s voted yes",PlayerName);
			g_PlayersVote[id] = MAPVOTE_YES;
		}
		else if(option == MAPVOTE_NO)	// Means its a map extend
		{
			g_BATCore.MessagePlayer(0,"[BAT] %s voted no",PlayerName);
			g_PlayersVote[id] = MAPVOTE_NO;
		}
		else
		{
			g_MenuMngr.ShowMenu(id,g_MapVoteMenu);
			return MENUSELECTION_BAD;
		}

		VoteInfo.PlayersThatHasVoted++;
	}
	if(VoteInfo.PlayersThatHasVoted >= VoteInfo.PlayersInVote)
		EndMapVote();
	
	return MENUSELECTION_GOOD;
}
/**************************************************** Text Based Voting Functions ********************************************/
void MapVote::PrintMapList(int id)
{
	g_BATCore.MessagePlayer(id,g_SayMapList);
}
void MapVote::CheckSayVote(int id,char *MapName)
{
	int MapIndex = GetMapIndexInVote(MapName);
	//g_BATCore.ServerCommand("echo Map: '%s' MapIndex: %d (%s) CurMap: %d",MapName,MapIndex,g_MapName[MapIndex],g_CurrentMapIndex);

	if(MapIndex != MAPVOTE_BADMAP)
	{
		if(g_PlayersVote[id] != MAPVOTE_NOVOTE)
		{
			int OldMapIndex = g_PlayersVote[id];
			g_VotesOnMap[OldMapIndex]->MapVotes--;
			g_PlayersVote[id] = MapIndex;
			g_VotesOnMap[MapIndex]->MapVotes++;

			g_BATCore.MessagePlayer(0,"[BAT] %s changed his vote from %s to %s ( %d votes)",g_BATCore.GetPlayerName(id),g_MapList[OldMapIndex]->MapName,g_MapList[MapIndex]->MapName,g_VotesOnMap[MapIndex]->MapVotes);
		}
		else
		{	
			g_PlayersVote[id] = MapIndex;
			g_VotesOnMap[MapIndex]->MapVotes++;
			VoteInfo.PlayersThatHasVoted++;

			g_BATCore.MessagePlayer(0,"[BAT] %s vote %s ( %d votes)",g_BATCore.GetPlayerName(id),g_MapList[MapIndex]->MapName,g_VotesOnMap[MapIndex]->MapVotes);			
		}
		if(g_PlayersVote[id] == g_CurrentMapIndex)	
            g_PlayersVote[id] = MAPVOTE_EXTEND;
		
		if(VoteInfo.PlayersThatHasVoted >= VoteInfo.PlayersInVote)
			EndMapVote();
	}
	else
		g_BATCore.MessagePlayer(0,"[BAT] %s is not a valid map, 'say maplist' to show maps in vote",MapName);
}
int MapVote::GetMapIndexInVote(char *MapName)
{
	int MapIndex = MAPVOTE_BADMAP;
 	if(stricmp(MapName,"extend") == 0 && g_CurrentMapIndex >= 0)
		return g_CurrentMapIndex;

	for(int i=0;i<g_MapCount;i++)
	{
		if(g_MapList[i]->MapType != MAPTYPE_ADMINMAP && stricmp(MapName,g_MapList[i]->MapName) == 0)
			MapIndex = i;
	}
	return MapIndex;
}
/**************************************************** Shared function ********************************************/
void MapVote::ResetArrays()
{
	VoteInfo.PlayersThatHasVoted = 0;
	VoteInfo.PlayersInVote = 0;
	VoteInfo.MapsAddedToVote = 0;
	for(int i=0;i<MapsInPublicVote;i++)
		g_MapIndexInVote[i] = -1;

	for(int i=0;i<g_MaxClients;i++)
		g_PlayersVote[i] = MAPVOTE_NOVOTE;

	for(unsigned int i=0;i<g_VotesOnMap.size();i++)
		g_VotesOnMap[i]->MapVotes = 0;
}
void MapVote::GenListOfMapsForVote()
{
	if(VoteInfo.Method == VOTEMETHOD_MENU)
	{
		int CurMapsInVote=0;
		int MapIndex=-1;
		srand(time(NULL));

		g_MapIndexInVote[0] = g_NextMapIndex;
		_snprintf(g_MapNameInVote[0],MAX_MAPNAMELEN,g_MapList[g_NextMapIndex]->MapName);
		VoteInfo.MapsAddedToVote++;

		for(int i=1;i<VoteInfo.MapsInVote;i++)
		{
			g_MapIndexInVote[i] = -1;
			MapIndex = rand()%(g_MapCount-1);
			int CPS = 0;

			while(IsMapOrginal(MapIndex) == false && CPS <= 500)
			{
				CPS++;
				MapIndex = rand()%g_MapCount;	
				//META_LOG(g_PLAPI,"%d) MapIndex: %d Mapcount: %d Mapsinvote: %d",VoteInfo.MapsAddedToVote,MapIndex,g_MapCount,VoteInfo.MapsInVote);
			}
			if(CPS >= 500)
				g_BATCore.AddLogEntry("There was a critical error trying to generate a map vote, please contact BAT Author ( send your mapcycle.txt,votemaps.ini & adminmaps.ini )");
		
			g_MapIndexInVote[i] = MapIndex;
			VoteInfo.MapsAddedToVote++;
			_snprintf(g_MapNameInVote[i],MAX_MAPNAMELEN,g_MapList[MapIndex]->MapName);
		}
	}
	else
	{
		_snprintf(g_SayMapList,191,"Maps in vote: ");
		for(int i=0;i<VoteInfo.MapsInVote;i++)
		{
			if(g_MapList[i]->MapType == MAPTYPE_ADMINMAP)
				continue;

			if( i != 0)
			{
				strcat(g_SayMapList,",");	
				strcat(g_SayMapList,g_MapList[i]->MapName);		//_snprintf(g_MapList,",%s",g_MapName[i]);
			}
			else
				strcat(g_SayMapList,g_MapList[i]->MapName);
		}
	}
}
bool MapVote::IsMapOrginal(int MapIndex)
{
	if(g_MapList[MapIndex]->MapType == MAPTYPE_ADMINMAP)
		return false;

	if(MapIndex == g_CurrentMapIndex)
		return false;

	for(int i=0;i<VoteInfo.MapsAddedToVote;i++)
	{
		if(g_MapIndexInVote[i] == MapIndex)
			return false;
	}
	return true;
}
bool MapVote::WaitForModEvent()
{
	if(g_ModSettings.GameMod == MOD_CSTRIKE || g_ModSettings.GameMod == MOD_DOD || g_ModSettings.GameMod == MOD_EMPIRES || g_ModSettings.GameMod == MOD_TF2) //|| g_ModSettings.GameMod == MOD_INSURGENCY)
		return true;
	
	return false; // Not a known mod, so we cant wait for a event can we.
}
bool MapVote::ModHandlesMapChange()
{
	if(g_ModSettings.GameMod == MOD_INSURGENCY && g_BATCore.GetBATVar().SetNextMap())
		return true;

	return false; // Not a known mod, so we cant wait for a event can we.
}
bool MapVote::MapVoteOnStartOfMap()
{
	if(g_ModSettings.GameMod == MOD_INSURGENCY || g_ModSettings.GameMod == MOD_EMPIRES || g_ModSettings.GameMod == MOD_PVKII  || g_ModSettings.GameMod == MOD_TF2)
		return true;

	return false; // Not a known mod, so we cant wait for a event can we.
}
int MapVote::GetPrecentageOfPlayers(int Players,int PlayerCount)
{
	return int((Players * 100) / PlayerCount);
}
void MapVote::CallbackMapVoteCvar()
{
	g_MapExtendOption = g_BATCore.GetBATVar().GetMapExtendOption();
}
