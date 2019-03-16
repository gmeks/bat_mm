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

#ifndef _INCLUDE_SMInterface_H
#define _INCLUDE_SMInterface_H

#include "BATCore.h"
#include "const.h"
#include "IExtensionSys.h"
#include "IPlayerHelpers.h"
#include <IAdminSystem.h>
#include <IPlayerHelpers.h>
#include <stdio.h>
#include "smsdk_ext.h"
#include "IPlayerHelpers.h"

using namespace SourceMod;

class SourceModInterface : public IExtensionInterface
{
public:
	SourceModInterface::SourceModInterface();

	virtual bool OnExtensionLoad(IExtension *me,IShareSys *sys,char *error,	size_t maxlength,bool late);
	virtual void OnExtensionUnload();
	virtual void OnExtensionsAllLoaded();
	virtual void OnExtensionPauseChange(bool pause);
	virtual bool QueryRunning(char *error, size_t maxlength);
	virtual bool IsMetamodExtension() {return false; }
	virtual const char *GetExtensionName() { return g_BATCore.GetName(); }
	virtual const char *GetExtensionURL() { return g_BATCore.GetURL(); }
	virtual const char *GetExtensionTag() { return g_BATCore.GetLogTag(); }
	virtual const char *GetExtensionAuthor() { return g_BATCore.GetAuthor(); }
	virtual const char *GetExtensionVerString() { return BAT_VERSION; }
	virtual const char *GetExtensionDescription() { return g_BATCore.GetDescription(); }
	virtual const char *GetExtensionDateString() { return __DATE__; }

	bool Connect();
	void Disconnect();
	bool IsConnected() { return g_InterfaceConnected; }
	void SetAdminRights(int iClient,int iBATFlags,const char *AdminName);

private:
	/*
	IShareSys *g_pShareSys;
	IExtension *myself;
	IAdminSystem *g_pAdmins;
	IPlayerManager *g_pPlayers;
	*/

	bool g_InterfaceConnected;
};

class SourceModInterfaceCallbacks : public IClientListener
{
public:
	void OnClientPostAdminCheck(int client);
};

/*
extern SourceModInterface g_SMExt;
extern IShareSys *g_pShareSys;
extern IExtension *myself;
extern IAdminSystem *g_pAdmins;
extern IPlayerManager *g_pPlayers;
*/

extern SourceModInterface g_SMExt;

#endif // _INCLUDE_SMInterface_H