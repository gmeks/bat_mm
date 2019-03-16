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

#ifndef _INCLUDE_CMDMNGR
#define _INCLUDE_CMDMNGR

#include "BATCore.h"
#include "const.h"

class AdminCmdManager
	{
	public:
		void RegisterCommand(char *Cmd,char *CmdUsage,char *CmdDesc,CmdFuncIntArg pFunc,int CmdFlags,int ArgCountReq);
		bool CheckCmd(int id,const char *Cmd);
		bool CheckSayCmd(int id,const char *Cmd);

#if BAT_ORANGEBOX == 1
		static void CheckSrvCmd( const CCommand &args);
#else
		static void CheckSrvCmd( );
#endif


	private:
		int GetArgCount();


	};
#endif // _INCLUDE_CMDMNGR
