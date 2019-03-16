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
#include "BATInterface.h"

SourceHook::CVector<AdminInterfaceListnerStruct *>g_CallBackList;
unsigned int g_CallBackCount;
MyListener g_Listener;
BATAdminInterface g_BATInterface;


extern bool g_IsConnected[MAXPLAYERS+1];
extern int g_MaxClients;

SourceHook::CVector<CustomAccessFlagStruct *>g_CustomAccessFlags;
unsigned int g_CustomFlagsCount=0;
int g_LastUsedFlag = 26;


bool BATAdminInterface::RegisterFlag(const char *Class,const char *Flag,const char *Description)
{	
	int FlagLen = strlen(Flag);
	if(FlagLen >= ADMININTERFACE_MAXACCESSLENGTHTEXT || FlagLen == 0)
	{
		g_BATCore.AddLogEntry("ERROR: AdminInterface - a plugin has tried to register a Flag the wrong text length, was %d . Value should be between 1 - %d",Flag,ADMININTERFACE_REGFLAGDESCRIPTIONLEN);
		return false;
	}

	// BAT only handles letters as flags, so we check if Flag is longer then 1 letter, if it is then we return false.
	if(FlagLen == 1)
	{
		g_BATCore.AddLogEntry("A plugin has registered to use %s as a access flag, and can be used in users.ini directly",Flag);
	}
	else
	{
		if(CustomAccessExistence(Flag))
		{
			g_BATCore.AddLogEntry("A plugin has registered register the same custom access flags twice ( %s )",Flag);
			return true;
		}
		if(g_CustomFlagsCount >= g_CustomAccessFlags.size()-1 || g_CustomAccessFlags.size() == 0)
			g_CustomAccessFlags.push_back(new CustomAccessFlagStruct);

		_snprintf(g_CustomAccessFlags[g_CustomFlagsCount]->AccessText,ADMININTERFACE_MAXACCESSLENGTHTEXT,"%s",Flag);
		g_CustomAccessFlags[g_CustomFlagsCount]->InternalFlag = 1<<(g_LastUsedFlag-1) ;
		g_BATCore.AddLogEntry("A plugin has registered %s as a custom admin access to use this, use %c in users.ini",Flag,GetFlagFromInt(g_LastUsedFlag));
		g_LastUsedFlag--;
		g_CustomFlagsCount++;
	}
	return true;	
}
bool BATAdminInterface::CustomAccessExistence(const char *Flag)
{
	if(g_CustomFlagsCount > 0)
	{
		for(unsigned int i = 0;i<g_CustomFlagsCount;i++)
		{
			if(strcmp(Flag,g_CustomAccessFlags[i]->AccessText) == 0)
				return true;
		}
	}
	return false;
}
bool BATAdminInterface::HasFlag(int id, const char *Class, const char *Flag)
{
	return HasFlag(id,Flag);
}
bool BATAdminInterface::HasFlag(int id,const char *Flag)
{
	if(!IsClient(id))
		return false;

	if(strlen(Flag) == 1)
	{
		int iFlag = (int)g_BATCore.GetAdminLoader()->HashLogin((char *)Flag);
		return g_BATCore.HasAccess(id,iFlag);
	}
	if(g_CustomFlagsCount > 0)
	{
		for(unsigned int i = 0;i<g_CustomFlagsCount;i++)
		{
			if(strcmp(Flag,g_CustomAccessFlags[i]->AccessText) == 0)
			{
				 return g_BATCore.HasAccess(id,g_CustomAccessFlags[i]->InternalFlag);
			}
		}
	}
	else if(strcmp(Flag,"kick") == 0 || strcmp(Flag,"slap") == 0 || strcmp(Flag,"slay") == 0)
	{
		if(g_BATCore.HasAccess(id,ADMIN_KICK))
			return true;
	}
	else if(strcmp(Flag,"ban") == 0)
	{
		if(g_BATCore.HasAccess(id,ADMIN_BAN))
			return true;
	}
	else if(strcmp(Flag,"chat") == 0 || strcmp(Flag,"say") == 0)
	{
		if(g_BATCore.HasAccess(id,ADMIN_CHAT))
			return true;
	}
	else if(strcmp(Flag,"rcon") == 0)
	{
		if(g_BATCore.HasAccess(id,ADMIN_RCON))
			return true;
	}
	else if(strcmp(Flag,"map") == 0)
	{
		if(g_BATCore.HasAccess(id,ADMIN_MAP))
			return true;
	}
	else if(strcmp(Flag,"reservedslots") == 0)
	{
		if(g_BATCore.HasAccess(id,ADMIN_RESERVEDSLOTS))
			return true;
	}
	else if(strcmp(Flag,"immunity") == 0)
	{
		if(g_BATCore.HasAccess(id,ADMIN_IMMUNITY))
			return true;
	}
	else if(strcmp(Flag,"any") == 0)
	{
		if(g_BATCore.HasAccess(id,ADMIN_ANY))
			return true;
	}
	return false;
}
bool BATAdminInterface::IsClient(int id)
{
	if(id > g_MaxClients || !g_IsConnected[id] || g_UserInfo[id].IsBot || g_UserInfo[id].SteamIdStatus != STEAM_ID_VALID)
		return false;
	
	return true;
}
void BATAdminInterface::RemoveListner(AdminInterfaceListner *ptr)
{
	if(g_CallBackCount >= g_CallBackList.size()-1 || g_CallBackList.size() == 0)
		g_CallBackList.push_back(new AdminInterfaceListnerStruct);

	for(unsigned i=0;i<g_CallBackCount;i++)
	{
		if(g_CallBackList[i]->ptr == ptr) // We already have the pointer in the list, so we dont want to save it again
		{
			g_CallBackList[i]->ptr = NULL;			
			break;
		}
	}
}
void BATAdminInterface::AddEventListner(AdminInterfaceListner *ptr)
{
	if(g_CallBackCount >= g_CallBackList.size()-1 || g_CallBackList.size() == 0)
		g_CallBackList.push_back(new AdminInterfaceListnerStruct);

	bool IsOrginal=true;
	for(unsigned i=0;i<g_CallBackCount;i++)
	{
		if(g_CallBackList[i]->ptr == ptr) // We already have the pointer in the list, so we dont want to save it again
		{
			g_BATCore.AddLogEntry("ERROR: A plugin has tried to register the same interface 2 times (Pointer: %p)",ptr);

			IsOrginal = false;
			break;
		}
	}
	if(IsOrginal)
	{
#if BAT_DEBUG == 1
		g_BATCore.ServerCommand("echo [BAT Debug] A plugin has found the AdminInterface (%p)",ptr);
#endif
		g_CallBackList[g_CallBackCount]->ptr = ptr;
		g_CallBackCount++;
	}
	else
	{
		g_BATCore.AddLogEntry("A plugin has registered itself without providing a callback pointer, this will produce crashes if BAT gets unloaded");
	}
}
void *MyListener::OnMetamodQuery(const char *iface, int *ret)
{
	if(g_BATCore.GetBATVar().GetAdminInterfaceEnabled() == 0)
		return NULL;

	if (strcmp(iface, "AdminInterface")==0)
	{
		if (ret)
			*ret = IFACE_OK;

		return (void *)(&g_BATInterface);
	}

	if (ret)
		*ret = IFACE_FAILED;
	return NULL;
}
char BATAdminInterface::GetFlagFromInt(int CharIndex) 
{
	const char Alfabet[] = "abcdefghijklmnopqrstuvwxyz";	
	return Alfabet[CharIndex-1];
}