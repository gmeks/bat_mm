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

#ifndef _INCLUDE_BATSQL
#define _INCLUDE_BATSQL

#ifdef WIN32
#include <WinSock2.h>
#endif
#include <mysql.h>

//#define LinuxThreading

#ifdef LinuxThreading
/********************/
/*  Linux Threading */
/********************/
#define ThreadReturnType void *
#define ThreadParm void * Garbage

#define ThreadReturn \
	pthread_exit((void *)NULL); \
	return 0;

#define TSleep usleep

#define ThreadKill pthread_cancel(g_pSQLThread);
#else
/*********************/
/* Windows Threading */
/*********************/
#define ThreadReturnType DWORD WINAPI
//#define ThreadParm LPARAM
#define ThreadParm
#define TSleep Sleep

#define ThreadReturn \
	DWORD FalseReturn = NULL; \
	return FalseReturn;

#define SQL_DEBUG 1

#define ThreadKill TerminateThread(g_ThreadHandle,0);
#endif

#if SQL_DEBUG == 1

#define SqlDebugPrint(NewLogMsg, ...) \
	g_BATCore.GetMsgBuffer()->AddMsgToBuffer(NewLogMsg);

#else

#define SqlDebugPrint(NewLogMsg, ...)

#endif \
	

class BATSQL
{
public:
	void KillActiveThread();
	void ResumeSqlThread();
	void StartSQLThread();
	void BufferLogInformation(const char * NewLogInfo);
	void ExitSQLThread();

	static bool CheckIDAgainstSQL(int pIndex);
	static bool MoveLogInfo();

	static bool MakeLocalUsersFile();
	static void ShutdownSQL(); // Called when critical errors happen, and stops using SQL

	static bool GetServerIDSourceBans();

private:	
	static bool AmxBansCheckServerExists(MYSQL *sock);
	static int GetSourceBansModIndex(MYSQL *sock);
};
ThreadReturnType SqlThreadWaitLoop();
#endif // end _INCLUDE_BATSQL