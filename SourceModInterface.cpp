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

#include "SourceModInterface.h"

SourceModInterface g_SMExt;
SourceModInterfaceCallbacks *g_SMCallbacks;
IExtensionManager *smexts;

IShareSys *g_pShareSys;
IExtension *myself;
IAdminSystem *g_pAdmins;
IPlayerManager *g_pPlayers;

SourceModInterface::SourceModInterface()
{
	g_pShareSys = NULL;
	myself = NULL;
	g_pAdmins = NULL;
	g_pPlayers = NULL;
	g_InterfaceConnected = false;
}

void SourceModInterface::Disconnect()
{
	if(g_pPlayers)
		g_pPlayers->RemoveClientListener(g_SMCallbacks);
	
	if(myself)
		smexts->UnloadExtension(myself);

	g_pAdmins = NULL;
	g_pShareSys = NULL;
	myself = NULL;
	g_pAdmins = NULL;
	g_pPlayers = NULL;

	g_InterfaceConnected = false;
}
bool SourceModInterface::Connect()
{
	char error[256];
	char path[256];
	int maxlength = sizeof(error);

	g_SMCallbacks = new SourceModInterfaceCallbacks;

	if ((smexts = (IExtensionManager *)g_SMAPI->MetaFactory(SOURCEMOD_INTERFACE_EXTENSIONS, NULL, NULL)) == NULL)
	{
		g_BATCore.AddLogEntry("ERROR! Failed to get %s interface, this means any integration with sourcemod will not happen",SOURCEMOD_INTERFACE_EXTENSIONS);
		
		if (error)
			g_BATCore.AddLogEntry("ERROR! Sourcemod returned the error: %s",error);

		Disconnect();
		return false;
	}

	/* This could be more dynamic */
#if defined __linux__
	g_SMAPI->PathFormat(path,sizeof(path),"addons/addons/bin/bat_mm_i486.so");
#else
	g_SMAPI->PathFormat(path,sizeof(path),"addons/addons/bin/bat_mm.dll");
#endif


	if ((myself = smexts->LoadExternal(&g_SMExt,path,"bat_mm.ext",error,maxlength))	== NULL)
	{
		g_BATCore.AddLogEntry("ERROR! Failed to load as sourcemod external, sourcemod integration is impossible");
		Disconnect();
		return false;
	}

	bool GotInterface = g_pShareSys->RequestInterface(SMINTERFACE_ADMINSYS_NAME,SMINTERFACE_ADMINSYS_VERSION, myself, (SMInterface **)&g_pAdmins);

	if(!GotInterface)
	{
		g_BATCore.AddLogEntry("ERROR! Failed to get the sourcemod admin interface ( %s Version: %d )",SMINTERFACE_ADMINSYS_NAME,SMINTERFACE_ADMINSYS_VERSION);
		Disconnect();
		return false;
	}

	GotInterface = g_pShareSys->RequestInterface(SMINTERFACE_PLAYERMANAGER_NAME,SMINTERFACE_PLAYERMANAGER_VERSION, myself, (SMInterface **)&g_pPlayers);
	if(!GotInterface)
	{
		g_BATCore.AddLogEntry("ERROR! Failed to get the sourcemod admin interface ( %s Version: %d )",SMINTERFACE_PLAYERMANAGER_NAME,SMINTERFACE_PLAYERMANAGER_VERSION);
		Disconnect();
		return false;
	}
	
	g_InterfaceConnected = true;
	g_pPlayers->AddClientListener(g_SMCallbacks);
	return true;
}
bool SourceModInterface::OnExtensionLoad(IExtension *me, IShareSys *sys,  char *error,  size_t maxlength,  bool late)
{
	g_pShareSys = sys;
	myself = me;
	return true;
}

void SourceModInterface::OnExtensionUnload()
{
	/* Clean up any resources here, and more importantly, make sure 
	* any listeners/hooks into SourceMod are totally removed, as well 
	* as data structures like handle types and forwards.
	*/

	//...
}

void SourceModInterface::OnExtensionsAllLoaded()
{
	/* Called once all extensions are marked as loaded.
	* This always called, and always called only once.
	*/
}

void SourceModInterface::OnExtensionPauseChange(bool pause)
{
}

bool SourceModInterface::QueryRunning(char *error, size_t maxlength)
{
	/* if something is required that can't be determined during the initial 
	* load process, print a message here will show a helpful message to 
	* users when they view the extension's info.
	*/
	return true;
}
void SourceModInterface::SetAdminRights(int id,int iBATFlags,const char *AdminName)
{
	IGamePlayer *pPlayer = g_pPlayers->GetGamePlayer(g_UserInfo[id].PlayerEdict);	
	int iFlags = g_BATCore.GetAdminLoader()->ConvertToSoureModAdminRights(iBATFlags);
	AdminId AdminID = pPlayer->GetAdminId();

	if (AdminID == INVALID_ADMIN_ID)
	{
		AdminID = g_pAdmins->CreateAdmin(AdminName);
		pPlayer->SetAdminId(AdminID,true);
	}

	g_pAdmins->SetAdminFlags(AdminID,Access_Effective,iFlags);
}
void SourceModInterfaceCallbacks::OnClientPostAdminCheck(int client)
{
	IGamePlayer *pPlayer = g_pPlayers->GetGamePlayer(g_UserInfo[client].PlayerEdict);

	AdminId admin = pPlayer->GetAdminId();
	const char * AuthID = pPlayer->GetAuthString();

	if(g_BATVars.GetSMInterface() == 1)
	{
		if (admin == INVALID_ADMIN_ID)
			return;

		uint bits = g_pAdmins->GetAdminFlags(admin, Access_Effective);
		const char *SMAdminString = g_BATCore.GetAdminLoader()->GetAdminAccountFlagsString(bits);

		int NewAdminRights = g_BATCore.GetAdminLoader()->ConvertFromSourceModAdminRights(SMAdminString);
		const char *BATAdminString = g_BATCore.GetAdminLoader()->GetAdminAccountFlagsString(bits);

		g_UserInfo[client].AdminAccess = NewAdminRights;
		_snprintf(g_UserInfo[client].AdminFlags,39,"%s",g_BATCore.GetAdminLoader()->GetAdminAccountFlagsString(NewAdminRights));
	}
	else if(g_BATVars.GetSMInterface() == 2)
	{		
		g_SMExt.SetAdminRights(client,g_UserInfo[client].AdminAccess,g_UserInfo[client].AdminName);
	}
}
