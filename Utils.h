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

#ifndef _INCLUDE_UTILS_H
#define _INCLUDE_UTILS_H

//#include <convar.h>
#include <ISmmPlugin.h>
#include <sourcehook/sourcehook.h>
#include <dt_send.h>

class Utils
{
public:
	void UTIL_PrecacheOffsets();

	void UTIL_SetHealth(edict_t *pEntity, int hp);
	void UTIL_SetMoveType(edict_t *pEntity, int Flag);

	int UTIL_GetHealth(edict_t *pEntity);
	int UTIL_GetMoveType(edict_t *pEntity);

	void UTIL_TelePortPlayer(edict_t *pEntity,const Vector *pos,const QAngle *ang,const Vector *vel);
	void UTIL_KillPlayer(edict_t *pEntity);

	static void CallBackUseSigScannerCvar();

private:
	int UTIL_FindOffset(char *ClassName, char *Property);
	SendProp *UTIL_FindSendProp(SendTable *pTable, const char *name);
	ServerClass *UTIL_FindServerClass(const char *name);

	void UTIL_DumpOffsets();

};

#endif //_INCLUDE_CVARS_H
