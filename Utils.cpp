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

/*
Code based on nice article from: http://wiki.alliedmods.net/Entity_Properties
Sigs: http://forums.alliedmods.net/showthread.php?t=55565
*/
#include <stdio.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h> 
#endif

#include "Utils.h"
#include "BATCore.h"
#include "server_class.h"
#include "SigScan.h"

int g_nHealthOffset;
int g_nVelocityOffset;
int g_nMoveTypeOffset;

void *g_pCBaseTeleport;
void *g_pCommitSuicide;

bool g_UseSignatureScanner=true;

unsigned int strlen(unsigned char *MyArray)
{
	uint length=0;
	while(*MyArray != '\0') 
	{
		MyArray ++;
		length++;
	}
	
	return length;
}

void Utils::UTIL_PrecacheOffsets()
{
	g_nHealthOffset = UTIL_FindOffset("CBasePlayer", "m_iHealth");	
	//g_nVelocityOffset = UTIL_FindOffset("CBasePlayer", "m_vecOrigin");	

#if BAT_DEBUG == 1
	g_BATCore.AddLogEntry("Debug: g_nHealthOffset: %d",g_nHealthOffset);
	UTIL_DumpOffsets();
#endif
//	CFuncFinder::FindPointers();
	SigScan *SigFinder = new SigScan;

#if BAT_ORANGEBOX == 1
	unsigned char *CBaseTeleportSig = (unsigned char *)"\x83\xEC\x18\x53\x56\x8B\xD9\x8B\x0D\x78\xB2\x46\x22\x33\xF6\x33\xC0\x3B\xCE\x7E\x21\x8B\x15\x6C\xB2\x46\x22\xEB\x03\x8D\x49\x00\x39\x1C\x82\x74\x09\x83\xC0\x01\x3B\xC1\x7C\xF4\xEB\x08\x3B\xC6\x0F\x8D\x17\x01\x00\x00\x55\x57\x8D\x44\x24\x10\x50\x51\xB9\x6C\xB2\x46\x22\x89\x5C\x24\x18\xE8\xB4\x88\xF9\xFF\x8D\x4C\x24\x14\x51\x53\x89\x44\x24\x18\x89\x74\x24\x1C\x89\x74\x24\x20\x89\x74\x24\x24\x89\x74\x24\x28\x89\x74\x24\x2C";
	char *CBaseTeleportMask = "xxxxxxx??????xxxxxx?????????????xxx?????xx????xx??????x?????xx?????xxxx?????????xxxxxxxxxxxxxxxxxxxxxxxxxx";
	char *CBaseTeleportNixSymbol = "_ZN11CBaseEntity8TeleportEPK6VectorPK6QAngleS2_";
								   
	
	unsigned char *CBaseCommitSuicideSig = (unsigned char *)"\x83\xEC\x4C\x56\x8B\xF1\x80\xBE\x8C\x00\x00\x00\x00\x0F\x85\x81\x00\x00\x00\x8B\x0D\x2A\x2A\x2A\x2A";
	char *CBaseCommitSuicideMask = "xxxxxxxxxxxxxxxxxxxxx????";
	char *CBaseCommitSuicideNixSymbol = "_ZN9CTFPlayer13CommitSuicideEbb";
#else
	unsigned char *CBaseTeleportSig = (unsigned char *)"\x83\xEC\x18\x53\x56\x8B\xD9\x8B\x0D\x78\xB2\x46\x22\x33\xF6\x33\xC0\x3B\xCE\x7E\x21\x8B\x15\x6C\xB2\x46\x22\xEB\x03\x8D\x49\x00\x39\x1C\x82\x74\x09\x83\xC0\x01\x3B\xC1\x7C\xF4\xEB\x08\x3B\xC6\x0F\x8D\x17\x01\x00\x00\x55\x57\x8D\x44\x24\x10\x50\x51\xB9\x6C\xB2\x46\x22\x89\x5C\x24\x18\xE8\xB4\x88\xF9\xFF\x8D\x4C\x24\x14\x51\x53\x89\x44\x24\x18\x89\x74\x24\x1C\x89\x74\x24\x20\x89\x74\x24\x24\x89\x74\x24\x28\x89\x74\x24\x2C";
	char *CBaseTeleportMask = "xxxxxxx??????xxxxxx?????????????xxx?????xx????xx??????x?????xx?????xxxx?????????xxxxxxxxxxxxxxxxxxxxxxxxxx";
	char *CBaseTeleportNixSymbol = "_ZN11CBaseEntity8TeleportEPK6VectorPK6QAngleS2_"; //"_ZN11CBaseEntity8TeleportEPK6VectorPK6QAngleS";


	unsigned char *CBaseCommitSuicideSig = (unsigned char *)"\x83\xEC\x4C\x56\x8B\xF1\x80\xBE\x8C\x00\x00\x00\x00\x0F\x85\x81\x00\x00\x00\x8B\x0D\x2A\x2A\x2A\x2A";
	char *CBaseCommitSuicideMask = "xxxxxxxxxxxxxxxxxxxxx????";
	char *CBaseCommitSuicideNixSymbol = "_ZN11CBasePlayer13CommitSuicideEv";

#endif
										

	g_pCBaseTeleport = SigFinder->FindFunctionAddresss("CBaseTeleport",CBaseTeleportSig,CBaseTeleportMask,106,CBaseTeleportNixSymbol);
	g_pCommitSuicide = SigFinder->FindFunctionAddresss("CommitSuicide",CBaseCommitSuicideSig,CBaseTeleportMask,25,CBaseCommitSuicideNixSymbol);
	
	SigFinder->Dispose();
	delete SigFinder;

	if(!g_nHealthOffset)
		g_BATCore.AddLogEntry("ERROR: Did not find the offset needed to change the players health, you should update BAT");
	
	if(g_ModSettings.GameMod == MOD_SRCFORTS)
	{
		g_nMoveTypeOffset = UTIL_FindOffset("CBasePlayer", "m_MoveType");

		if(!g_nMoveTypeOffset)
			g_BATCore.AddLogEntry("ERROR: Did not find the offset needed to change the players movement(Used for no clip), you should update BAT");
	}
}


void Utils::UTIL_SetMoveType(edict_t *pEntity, int Flag)
{
	if (!g_nMoveTypeOffset || !g_UseSignatureScanner) return;

	*(int *)(pEntity->GetUnknown() + g_nMoveTypeOffset) = Flag;
	pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}
int Utils::UTIL_GetMoveType(edict_t *pEntity)
{
	if (!g_nMoveTypeOffset || g_UseSignatureScanner ) return 0;

	return (int)*(int *)(pEntity->GetUnknown() + g_nMoveTypeOffset);
}


void Utils::UTIL_TelePortPlayer(edict_t *pEntity,const Vector *pos,const QAngle *ang,const Vector *vel)
{
	// How it looks in SDK
	// Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
	if(!g_pCBaseTeleport || !g_UseSignatureScanner) return; // No valid pointer screw this.

	CBaseEntity *pCBase = pEntity->GetUnknown()->GetBaseEntity();
	if(!pCBase) return;

	union {
		void (EmptyClass::*mfpnew)(const Vector *,  const QAngle *, const Vector *);
		void* addr;
	} u;
	u.addr = g_pCBaseTeleport;


	(reinterpret_cast<EmptyClass*>(pCBase)->*u.mfpnew)(pos, ang, vel);
}



void Utils::UTIL_SetHealth(edict_t *pEntity, int Health)
{
	if (!g_nHealthOffset || !g_UseSignatureScanner) return;

	IServerUnknown *pUnknown = (IServerUnknown *)pEntity->GetUnknown();
	if (!pUnknown)	return;

	CBaseEntity *pCBase = pUnknown->GetBaseEntity();
	*(int *)((char *)pCBase + g_nHealthOffset) = Health;
	//pEntity->m_fStateFlags |= FL_EDICT_CHANGED;
}
int Utils::UTIL_GetHealth(edict_t *pEntity)
{
	if (!g_nHealthOffset || !g_UseSignatureScanner) return -1;

	IServerUnknown *pUnknown = (IServerUnknown *)pEntity->GetUnknown();
	if (!pUnknown) return 0;


	CBaseEntity *pCBase = pUnknown->GetBaseEntity();
	return *(int *)((char *)pCBase + g_nHealthOffset);
}
void Utils::UTIL_KillPlayer(edict_t *pEntity)
{
	if(!g_pCommitSuicide || !g_UseSignatureScanner)
	{
		int id = g_BATCore.GetEngine()->IndexOfEdict(pEntity);
		if(g_UserInfo[id].IsBot) return; // Cant kill bots with clientcommand

		g_BATCore.GetHelpers()->ClientCommand(pEntity,"kill");
		return;
	}

	CBaseEntity *pCBase = pEntity->GetUnknown()->GetBaseEntity();
	if(!pCBase) return;

	union {
		void (EmptyClass::*mfpnew)();
		void* addr;
	} u;
	u.addr = g_pCommitSuicide;

	(reinterpret_cast<EmptyClass*>(pCBase)->*u.mfpnew)();
}

void Utils::CallBackUseSigScannerCvar()
{
	g_UseSignatureScanner = g_BATCore.GetBATVar().GetUseSigScanner();
}

int Utils::UTIL_FindOffset(char *ClassName, char *Property)
{	
	ServerClass *sc = UTIL_FindServerClass(ClassName);
	if(!sc)	return 0;

	SendProp *pProp = UTIL_FindSendProp(sc->m_pTable, Property);
	if(!pProp)	return 0;

	int NewOffset = pProp->GetOffset();
	return NewOffset;
}
void Utils::UTIL_DumpOffsets()
{
	FILE *fp = fopen("offsets.txt","w");
	if (!fp) return;

	ServerClass *sc = g_BATCore.GetServerGameDll()->GetAllServerClasses();
	while (sc)
	{
		int NumProps = sc->m_pTable->GetNumProps();
		fprintf(fp, "%s\n", sc->m_pTable->GetName());
		for (int i = 0; i < NumProps; i++)
		{
			SendProp *prop = sc->m_pTable->GetProp(i);
			if (prop)
				fprintf(fp, "  %5d %s\n", prop->GetOffset() / 4, prop->GetName());
		}
		sc = sc->m_pNext;
	}
	fclose(fp);
} 


/**
* Searches for a named Server Class.
*
* @param name		Name of the top-level server class.
* @return 		Server class matching the name, or NULL if none found.
*/
ServerClass *Utils::UTIL_FindServerClass(const char *name)
{
	ServerClass *pClass = g_BATCore.GetServerGameDll()->GetAllServerClasses();
	while (pClass)
	{
		if (strcmp(pClass->m_pNetworkName, name) == 0)
		{
			return pClass;
		}
		pClass = pClass->m_pNext;
	}
	return NULL;
}
/**
* Recursively looks through a send table for a given named property.
*
* @param pTable	Send table to browse.
* @param name		Property to search for.
* @return 		SendProp pointer on success, NULL on failure.
*/
SendProp *Utils::UTIL_FindSendProp(SendTable *pTable, const char *name)
{
	int count = pTable->GetNumProps();
	SendProp *pProp;
	for (int i=0; i<count; i++)
	{
		pProp = pTable->GetProp(i);
		if (strcmp(pProp->GetName(), name) == 0)
		{
			return pProp;
		}
		if (pProp->GetDataTable())
		{
			if ((pProp=UTIL_FindSendProp(pProp->GetDataTable(), name)) != NULL)
			{
				return pProp;
			}
		}
	}

	return NULL;
}