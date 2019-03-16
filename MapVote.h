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

#ifndef _INCLUDE_MENUS_MAPVOTE
#define _INCLUDE_MENUS_MAPVOTE

#include "BATMenu.h"
#include "const.h"

class MapVote : public IMenu
{
public:
	bool StartPublicMapVote(float GameTime,bool ChangeAfterDone);
	void StartCustomMapVote(int MapIndex,int WhenToChangeMap);
	void StartVoteKick(int PlayerIndex); // Start a votekick against a player
	void StartVoteBan(int PlayerIndex); // Start a voteban against a player
	void StartCmdCustomVote(const char *Question,const char *SrvCmd = NULL);
	void StartMultiCustomMapVote(int MapIndex[MapsInPublicVote],int MapsInVote,int WhenToChangeMap );
	void EndMapVote();
	bool Display(BATMenuBuilder *make, int playerIndex);
	MenuSelectionType MenuChoice(player_t player, int option);
	void AddRTVVote(int id );
	void PrintMapList(int id);
	void ResetTimeLimit();
	void CheckSayVote(int id,char *MapName); // Used to check a map vote via chat, returns number of votes in succesfull. And MAPVOTE_BADMAP if the map was bad or not found
	
	bool WaitForModEvent();	// Used to check if current mod fires a event before the map change, or if we check the timeleft
	bool ModHandlesMapChange();
	bool MapVoteOnStartOfMap();
	
	static void CallbackMapVoteCvar();

private:
	void GenListOfMapsForVote();
	void EndRockthevote();
	void EndMapVotePublic();
	void EndMapVoteCustom();
	void EndCmdVoteCustom();
	void StartRockTheVote();
	void ResetArrays();
	bool IsMapOrginal(int MapIndex);

	// Chat based voting
	int GetMapIndexInVote(char *MapName); // Checks if map is in internal g_MapName array, return index
	int GetPrecentageOfPlayers(int Players,int PlayerCount);	
	
};
#endif // _INCLUDE_MENUS_MAPVOTE