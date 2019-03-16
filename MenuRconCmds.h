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

#ifndef _INCLUDE_MenuRconCmds
#define _INCLUDE_MenuRconCmds

#include "BATMenu.h"


class MenuRconCmds
{
public:
	void ReadMenuCmdsFile();
	const char *ParseRconCmdsString(int Index,bool UseDisc);

private:

};
#endif // _INCLUDE_MenuRconCmds