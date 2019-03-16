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

#ifndef _INCLUDE_LoadAdminAccounts_H
#define _INCLUDE_LoadAdminAccounts_H

#include "BATCore.h"
#include "const.h"

extern int unsigned g_AccountCount;

class LoadAdminAccounts
{
public:
	void ReadAdminFile();
	void WriteAdminFile();
	void ClearAdminList() { g_AccountCount = 0; }
	void AddAdminAccount(const char *SteamIDorIP,const char *Flags,const char *AccountName);

	const char *GetAdminAccountName(int AdminIndex);
	const char* GetAdminAccountFlagsString(int iFlags);

	unsigned int  HashLogin(const char *a);		// Makes a int32 hash of the steamid, so make comparing them faster ( NOT 100% safe, just fast )
	
	int GetAdminAccountFlags(int AdminIndex);
	int ReadFlags(const char *c);		// Converts the user flags letters into a int
	int IsUserAdmin(const char *Steamid,const char *IP);

	int LoadAdminAccounts::FastStrHash(const char *OrgString);

	static int ConvertFromAmxxStyleAdminRights(const char *pFlags);
	static int ConvertFromSourceModAdminRights(const char *pFlags);
	static int ConvertToSoureModAdminRights(int iBATFlags);
private:
	

};
#endif //_INCLUDE_CVARS_H