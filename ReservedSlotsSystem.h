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

#ifndef _INCLUDE_RESERVEDSLOTSSYSTEM
#define _INCLUDE_RESERVEDSLOTSSYSTEM

//#include "convar.h"
class ReservedSlotsSystem
	{
	public:
		ReservedSlotsSystem();
		~ReservedSlotsSystem();

	public:
		bool HandleReservedSlotsRequest(int id);
		int GetFreeNoneAdminSlots();
		
		static void cbCvarUpdate();
		static void cbTaskKickPlayer(int id);
		void UpdateVisibleSlots();

	private:
		bool AllowServerJoin(int id);
		
		
		void KickOrRedirectPlayer(int id);
		//int IsSlotFree();
		int GetPlayerToKick();
		ConVar *g_VisibleMaxPlayers;
	};

extern int g_ReservedSlotsUsers;
extern bool g_BlockIDCheck;
#endif // _INCLUDE_RESERVEDSLOTSSYSTEM