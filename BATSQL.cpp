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

//#define LinuxThreading
//#include <WinSock2.h>
/*
Add some function to check if a thread is still alive?
*/

#define DEBUG_SQL 0

#if WIN32
#include <WinSock2.h>
#else
#include <pthread.h>
#endif

#include "BATCore.h"
#include "BATSQL.h"
#include "time.h"
#include "const.h"
#include <mysql.h>

#ifdef LinuxThreading
pthread_t g_pSQLThread;
pthread_mutex_t g_mRunning;
bool g_SqlThreadShouldSleep;
#else
HANDLE g_ThreadHandle;
#endif


#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

extern stSQLSettings g_SQLSettings;
extern stSQLStatus g_SQLStatus;
extern BATSQL *m_BATSQL;

extern int g_MaxClients;
extern unsigned int g_ActiveIDChecks;
extern bool g_IsConnected[MAXPLAYERS+1];
extern bool g_IsConnecting[MAXPLAYERS+1];

bool g_UseLogBuffer1 = true; // If we should use log buffer 1;
uint g_SqlLogBuffer1Count=0;
uint g_SqlLogBuffer2Count=0;
SourceHook::CVector<eSQLAdminLog *>g_SqlLogBuffer1;
SourceHook::CVector<eSQLAdminLog *>g_SqlLogBuffer2;

bool g_ExitSQLThread; // If the SQL thread should exit
int g_AmxBansAddServerTries;

/*
This function is where the SQL Thread lives, it will loop and check if players needs to be authed or log info moved. When it has nothing to do , it will suspend itself, so the main srcds thread wakes it back up again when needed
*/
ThreadReturnType SqlThreadWaitLoop(ThreadParm)
{
	bool WasSuccess;
	while(!g_ExitSQLThread)
	{
		WasSuccess = true;

		if(g_SQLStatus.CueLength > 0)
		{
			g_SQLStatus.CueLength =0;
			bool FoundWork =false;
			int id = 0;

			for(int i=1;i<=g_MaxClients;i++) if(g_UserInfo[i].SteamIdStatus == STEAM_ID_WAIT_FOR_SQL && ( g_IsConnected[i] || g_IsConnecting[i]))
			{
				g_SQLStatus.CueLength++;

				if(FoundWork == false)
				{
					id = i;
					FoundWork = true;
					break;
				}
			}
			
			if(g_SQLStatus.CueLength > 0)
			{
				WasSuccess = BATSQL::CheckIDAgainstSQL(id);
			}
		}
		if(g_SQLStatus.MoveLogInfo)
			WasSuccess = BATSQL::MoveLogInfo();

		if(g_SQLStatus.MakeLocalUsersFile)
			WasSuccess = BATSQL::MakeLocalUsersFile();
		//g_SQLStatus.MakeLocalUsersFile = false;
		
		if(!WasSuccess) // We failed, we make this thread sleep for 5 sec, and we try again. Most likely its the SQL server thats down, so we dont hammer it
		{
			SqlDebugPrint("SQL DEBUG: There was some sort of error checking a players id or moving the logs, sleeping for 5 seconds");

			if(g_SQLStatus.Errors > 5)
			{

				SqlDebugPrint("SQL DEBUG: There was more then 5 SQL errors, we kill the SQL thread");
				BATSQL::ShutdownSQL();
				
			}else{
				TSleep(5000);
			}
		}
		if(g_SQLStatus.CueLength == 0 && !g_SQLStatus.MoveLogInfo && !g_ExitSQLThread && !g_SQLStatus.MakeLocalUsersFile)
		{
			SqlDebugPrint("SQL DEBUG: Thread is suspending itself, dident find any work");

			g_SQLStatus.ThreadSuspended = true;

#if WIN32
			SuspendThread(g_ThreadHandle);
#else
			g_SqlThreadShouldSleep = true;
			while(g_SqlThreadShouldSleep)
			{

				SqlDebugPrint("SQL DEBUG: Thread is sleeping 50 ms");
				TSleep(50);
			}
#endif
			g_SQLStatus.ThreadSuspended = false;
		}
	}
	g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL Thread gracefully quit");
	g_SQLStatus.ThreadRunning = false;
	ThreadReturn
}
void BATSQL::ResumeSqlThread()
{
	SqlDebugPrint("SQL DEBUG: Thread asked to stop sleeping");

#if WIN32
	ResumeThread(g_ThreadHandle);
#else
	g_SqlThreadShouldSleep = false;
#endif
}
void BATSQL::KillActiveThread() // Kills the thread, no mather whats going on
{
	g_SQLStatus.ThreadRunning = false;
	ThreadKill
}
void BATSQL::ExitSQLThread() // Allows the SQL thread to finnish what its doing
{
	SqlDebugPrint("SQL DEBUG: Thread asked to exit");
	g_ExitSQLThread = true;

	if(g_SQLStatus.ThreadSuspended)
		ResumeSqlThread(); // We unpause it so it can die
}
void BATSQL::StartSQLThread() // Starts the SQL thread, and it automaticly starts working or resting
{
	if(g_SQLStatus.ThreadRunning)
	{
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("For some reason the SQL Thread was asked to start up when it was already running, this is bad.");
		return;
	}
	g_SQLStatus.ThreadRunning = true;

	SqlDebugPrint("SQL DEBUG: Thread started");

	g_ExitSQLThread = false;
	g_SQLStatus.ThreadSuspended = false;
	g_SQLStatus.Errors = 0;

#if WIN32
	DWORD threadID=0; 
	LPVOID Garbage = (void*)1;
	g_ThreadHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)SqlThreadWaitLoop,Garbage,0,&threadID);
#else
	//pthread_mutex_init(&g_mRunning,NULL);
	//pthread_create(&g_pSQLThread,NULL,&SqlThreadWaitLoop,NULL);

	int PlayerID = 1;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	pthread_create(&g_pSQLThread,&attr, SqlThreadWaitLoop,(void*)PlayerID);
#endif
}
bool BATSQL::CheckIDAgainstSQL(int index)
{
	g_ActiveIDChecks++;

	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;
	char	qbuf[256];

	MYSQL *pSock = mysql_init(NULL);

	if (mysql_real_connect(pSock,g_SQLSettings.ServerIP,g_SQLSettings.UserName,g_SQLSettings.Password,g_SQLSettings.DataBase,0,NULL,0) == NULL)
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Couldn't connect to engine MySQL Server - %s",mysql_error(pSock));
		g_BATCore.MessageAdmins("ERROR: Couldn't connect to engine MySQL Server - %s",mysql_error(pSock));

		mysql_close(pSock);
		return false;
	}

	switch(g_SQLSettings.DataBaseType)
	{
	case BAT_V2:
		_snprintf(qbuf,255,"SELECT * FROM %s WHERE PlayerID = '%s' or PlayerID = '%s'",g_SQLSettings.AdminTable,g_UserInfo[index].Steamid,g_UserInfo[index].IP);
		break;

	case SourceBans:
		//_snprintf(qbuf,255,"SELECT * FROM %s WHERE server_id =%i or server_id IS NULL",g_SQLSettings.AdminTable,g_SQLSettings.AmxBansServerID);	
		_snprintf(qbuf,255,"SELECT * FROM %s WHERE (identity = '%s' or identity = '%s') and (server_id =%i or server_id IS NULL)",g_SQLSettings.AdminTable,g_UserInfo[index].Steamid,g_UserInfo[index].IP,g_SQLSettings.AmxBansServerID);		
		break;

	case AmxBans:
		if (!AmxBansCheckServerExists(pSock))
			return false;

		_snprintf(qbuf,255,"SELECT * FROM %s WHERE (PlayerID = '%s' or PlayerID = '%s') and srvid=%i",g_SQLSettings.AdminTable,g_UserInfo[index].Steamid,g_UserInfo[index].IP,g_SQLSettings.AmxBansServerID);		
		break;
	}

	SqlDebugPrint("SQL DEBUG: Query logged - %s",qbuf);

	if(mysql_query(pSock,qbuf))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Query failed - trying to check if player '%s' had any access rights - %s",g_UserInfo[index].Steamid,mysql_error(pSock));
		mysql_close(pSock);
		return false;
	}
	if (!(res=mysql_store_result(pSock)))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Query failed - trying to check if player '%s' had any access rights - %s",g_UserInfo[index].Steamid,mysql_error(pSock));
		mysql_close(pSock);
		return false;
	}
	row = mysql_fetch_row(res);	

	//g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Query logged - Rows returned for %s (%i)",g_UserInfo[index].Steamid,res->row_count);
	if(res->row_count == 0) // The user was not in the SQL DB, and we are done with him
	{

		SqlDebugPrint("SQL DEBUG: Player was not in DB: %s",g_UserInfo[index].Steamid);

		mysql_free_result(res);
		mysql_close(pSock);
		g_UserInfo[index].AdminAccess = 0; // Just for safety
		g_UserInfo[index].SteamIdStatus = STEAM_ID_WAIT_FOR_SETUP;		
		return true;
	}
	int tFlags = 0;

	switch(g_SQLSettings.DataBaseType)
	{
		case AmxBans:
			_snprintf(g_UserInfo[index].AdminName,127,"%s",row[3]);
			_snprintf(g_UserInfo[index].AdminFlags,39,"%s",g_BATCore.GetAdminLoader()->GetAdminAccountFlagsString(g_UserInfo[index].AdminAccess));
			g_UserInfo[index].AdminAccess = g_BATCore.GetAdminLoader()->ConvertFromAmxxStyleAdminRights(row[2]);
			break;

		case SourceBans:
			char AdminID[64],AdminFlags[32],AdminName[64];

			_snprintf(AdminID,63,"%s",row[0]);
			_snprintf(AdminFlags,31,"%s",row[1]);
			_snprintf(AdminName,63,"%s",row[2]);

			if(strcmp(AdminID,"(null)") == 0 || strcmp(AdminFlags,"(null)") == 0 || strcmp(AdminName,"(null)") == 0 )
			{
				g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: There was a error with the BAT view in sourcebans, it returned null something needs to be updated");
				g_SQLStatus.Errors++;

				mysql_free_result(res);
				mysql_close(pSock);
				return false;
			}

			tFlags = g_BATCore.GetAdminLoader()->ConvertFromSourceModAdminRights(AdminFlags);
			_snprintf(AdminFlags,31,"%s",g_BATCore.GetAdminLoader()->GetAdminAccountFlagsString(tFlags));						

			g_UserInfo[index].AdminAccess = tFlags;
			_snprintf(g_UserInfo[index].AdminName,127,"%s",AdminName);
			_snprintf(g_UserInfo[index].AdminFlags,39,"%s",AdminFlags);
			break;

		case BAT_V2:
			g_UserInfo[index].AdminAccess = g_BATCore.GetAdminLoader()->ReadFlags(row[2]);
			_snprintf(g_UserInfo[index].AdminName,127,"%s",row[3]);
			_snprintf(g_UserInfo[index].AdminFlags,39,"%s",g_BATCore.GetAdminLoader()->GetAdminAccountFlagsString(g_UserInfo[index].AdminAccess));
			break;
	}
		
	g_UserInfo[index].SteamIdStatus = STEAM_ID_WAIT_FOR_SETUP;

	SqlDebugPrint("SQL DEBUG: %s was in the MySQL Database, flags %s",g_BATCore.GetPlayerName(index),row[2]);

	mysql_free_result(res);
	mysql_close(pSock);
	return true;
}
bool BATSQL::MoveLogInfo()
{	
	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;
	char	qbuf[512]; // Size should be relative to: MAX_LOGLEN + 48 (log time size) and hostname size thats 128
	MYSQL *pSock = mysql_init(NULL);

	if (mysql_real_connect(pSock,g_SQLSettings.ServerIP,g_SQLSettings.UserName,g_SQLSettings.Password,g_SQLSettings.DataBase,0,NULL,0) == NULL)
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Couldn't connect to engine MySQL Server - %s",mysql_error(pSock));
		g_BATCore.MessageAdmins("ERROR: Couldn't connect to engine MySQL Server - %s",mysql_error(pSock));
		
		mysql_close(pSock);
		return false;
	}
	char ThisServerIP[128];
	_snprintf(ThisServerIP,127,"%s:%s",g_BATCore.GetBATVar().GetCvarString("ip"),g_BATCore.GetBATVar().GetCvarString("hostport"));

	if(g_UseLogBuffer1)
	{
		for(unsigned int i=0;i<g_SqlLogBuffer1Count;i++)
		{
			if(strlen(g_SqlLogBuffer1[i]->Info) == 0)
				continue;

			_snprintf(qbuf,511,"INSERT INTO %s (ServerIP,AdminID,LogEntry,LogTime) Values('%s','nada','%s','%s')",g_SQLSettings.LogTable,ThisServerIP,g_SqlLogBuffer1[i]->Info,g_SqlLogBuffer1[i]->LogTime);
			SqlDebugPrint("SQL DEBUG: Moving logs SQL Query: %s",qbuf);

			if(mysql_query(pSock,qbuf))
			{
				g_SQLStatus.Errors++;
				g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Moving logs to SQL failed Query failed - %s",mysql_error(pSock));
				mysql_close(pSock);
				return false;
			}
		}
		g_SqlLogBuffer1Count = 0;
		g_SqlLogBuffer1.clear();
	}
	else
	{
		for(unsigned int i=0;i<g_SqlLogBuffer2Count;i++)
		{
			if(strlen(g_SqlLogBuffer2[i]->Info) == 0)
				continue;

			_snprintf(qbuf,511,"INSERT INTO %s (ServerIP,AdminID,LogEntry,LogTime) Values('%s','nada','%s','%s')",g_SQLSettings.LogTable,ThisServerIP,g_SqlLogBuffer2[i]->Info,g_SqlLogBuffer2[i]->LogTime);
			SqlDebugPrint("SQL DEBUG: Moving logs SQL Query: %s",qbuf);

			if(mysql_query(pSock,qbuf))
			{
				g_SQLStatus.Errors++;
				g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Moving logs to SQL failed Query failed: %s",mysql_error(pSock));
				mysql_close(pSock);
				return false;
			}
		}
		g_SqlLogBuffer2Count = 0;
		g_SqlLogBuffer2.clear();
	}
	g_SQLStatus.MoveLogInfo = false;
	mysql_close(pSock);
	return true;
}


/*
 The point of this function is to temporary store log information in a buffer until the point where it needs to be moved into the SQL DB
 When this points happens, we start storing new log information in another array in case new log info comes while we are moving the data
*/
void BATSQL::BufferLogInformation(const char * NewLogInfo)
{
	time_t td; time(&td);
	char SqlDate[48];
	strftime(SqlDate, 47, "%Y-%m-%d %H:%M:%S", localtime(&td));	

	if(g_UseLogBuffer1)
	{
		if(g_SqlLogBuffer1Count >= g_SqlLogBuffer1.size()-1 || g_SqlLogBuffer1.size() == 0)
			g_SqlLogBuffer1.push_back(new eSQLAdminLog);

		mysql_escape_string(g_SqlLogBuffer1[g_SqlLogBuffer1Count]->Info,NewLogInfo,strlen(NewLogInfo));
		_snprintf(g_SqlLogBuffer1[g_SqlLogBuffer1Count]->LogTime,47,"%s",SqlDate);
		g_SqlLogBuffer1Count++;
	}
	else
	{
		if(g_SqlLogBuffer2Count >= g_SqlLogBuffer2.size()-1 || g_SqlLogBuffer2.size() == 0)
			g_SqlLogBuffer2.push_back(new eSQLAdminLog);

		mysql_escape_string(g_SqlLogBuffer2[g_SqlLogBuffer2Count]->Info,NewLogInfo,strlen(NewLogInfo));
		_snprintf(g_SqlLogBuffer2[g_SqlLogBuffer2Count]->LogTime,47,"%s",SqlDate);
		g_SqlLogBuffer2Count++;
	}
}
int BATSQL::GetSourceBansModIndex(MYSQL *pSock)
{
	// We now get the mod id.
	char	qbuf[2048];
	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;

	_snprintf(qbuf,sizeof(qbuf),"SELECT `mid` FROM sb_mods WHERE `name` = '%s'",g_BATCore.GetServerGameDll()->GetGameDescription());

	if(mysql_query(pSock,qbuf))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Query failed (%s)\n",mysql_error(pSock));
		mysql_close(pSock);
		return -1;
	}
	if (!(res=mysql_store_result(pSock)))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Query failed, trying to get mod index");
		mysql_close(pSock);
		return -1;
	}
	//g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Query logged - Rows returned for %s (%i)",g_UserInfo[index].Steamid,res->row_count);

	row = mysql_fetch_row(res);	
	if(res->row_count == 0)
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: The '%s' is not in the sourcebans list, perhaps a update of sourcebans is in order",g_BATCore.GetServerGameDll()->GetGameDescription());
		mysql_close(pSock);
		return -1;
	}
	int ModID = atoi(row[0]);
	if(ModID < 0 || ModID >= 2000)
	{
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: The mod id returned is bad, it was %d , something is wrong shutting down sql",ModID);
		mysql_free_result(res);
		mysql_close(pSock);
		ShutdownSQL();
		return -1;
	}

	return ModID;
}
bool BATSQL::GetServerIDSourceBans()
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;
	MYSQL *pSock = mysql_init(NULL);
	char	qbuf[2048];

	if (mysql_real_connect(pSock,g_SQLSettings.ServerIP,g_SQLSettings.UserName,g_SQLSettings.Password,g_SQLSettings.DataBase,0,NULL,0) == NULL)
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Couldn't connect to engine MySQL Server - %s",mysql_error(pSock));
		g_BATCore.MessageAdmins("ERROR: Couldn't connect to engine MySQL Server - %s",mysql_error(pSock));

		mysql_close(pSock);
		return false;
	}	

	//const char *pHostName = g_BATCore.GetBATVar().GetCvarString("hostname");
	const char *pIPAddress = g_BATCore.GetBATVar().GetCvarString("ip");
	const char *pHostPort = g_BATCore.GetBATVar().GetCvarString("hostport");
	const char *pRconPassword = g_BATCore.GetBATVar().GetCvarString("rcon_password");
	
	char RconPassword[128];
	mysql_escape_string(RconPassword,pRconPassword,strlen(pRconPassword));

//	char HostName[128];
//	mysql_escape_string(HostName,pHostName,strlen(pHostName));

	if(stricmp(pIPAddress,"localhost") == 0)
	{
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: A critical error has happend, the cvar:'ip' returns localhost, BAT will now disable SQL support");
		ShutdownSQL();
		return false;
	}

	int ModID = GetSourceBansModIndex(pSock);
	if(ModID == -1)
		return false;

	//Determine if this server is already in the database
	// SELECT * FROM %s WHERE (PlayerID = '%s' or PlayerID = '%s') and srvid=%i
	_snprintf(qbuf,sizeof(qbuf),"SELECT * FROM sb_servers WHERE ip='%s' and port='%s' and modid='%d'",pIPAddress,pHostPort,ModID);

	if(mysql_query(pSock,qbuf))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Query failed (%s)\n",mysql_error(pSock));
		mysql_close(pSock);
		return false;
	}
	if (!(res=mysql_store_result(pSock)))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Couldn't get result from %s\n",mysql_error(pSock));
		mysql_close(pSock);
		return false;
	}

	row = mysql_fetch_row(res);	

	if(res->row_count == 0) // The server is not there.
	{
		// ip  port  rcon  modid  
		_snprintf(qbuf,sizeof(qbuf),"INSERT INTO sb_servers(ip,port,rcon,modid) VALUES('%s',%s,'%s',%d)"
			,pIPAddress
			,pHostPort
			,RconPassword
			,ModID);

		if(mysql_query(pSock,qbuf))
		{
			g_SQLStatus.Errors++;
			g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Failed to insert server into DB (%s)\n",mysql_error(pSock));
			mysql_close(pSock);
			ShutdownSQL();
			return false;
		}

		// We get the server id again.
		_snprintf(qbuf,sizeof(qbuf),"SELECT * FROM sb_servers WHERE ip='%s' and port='%s' and modid='%d'",pIPAddress,pHostPort,ModID);
		if(mysql_query(pSock,qbuf))
		{
			g_SQLStatus.Errors++;
			g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Query failed (%s)\n",mysql_error(pSock));
			mysql_close(pSock);
			return false;
		}
		if (!(res=mysql_store_result(pSock)))
		{
			g_SQLStatus.Errors++;
			g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Couldn't get result from %s\n",mysql_error(pSock));
			mysql_close(pSock);
			return false;
		}
		row = mysql_fetch_row(res);	
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("This server was not in the sourcebans sb_servers table, so it was added. This probably means there are no admins assigned to the server either");

		if(res->row_count == 0) // The server is not there.
		{
			g_SQLStatus.Errors++;
			g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Failed to get the server_id after adding it. %s\n",mysql_error(pSock));
			mysql_close(pSock);
			mysql_free_result(res);		//Hrm, server isn't in the database
			return false;
		}
	}
	g_SQLSettings.AmxBansServerID = atoi(row[0]);

	// We now update the server information, basicly its just the rcon PW
	_snprintf(qbuf,sizeof(qbuf),"UPDATE sb_servers SET ip='%s', port='%s', rcon='%s', modid='%d' WHERE sid=%i;"
		,pIPAddress
		,pHostPort
		,RconPassword
		,ModID
		,g_SQLSettings.AmxBansServerID);

	if(mysql_query(pSock,qbuf))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Query failed (%s)\n",mysql_error(pSock));
		mysql_close(pSock);
		return false;
	}

	mysql_free_result(res);		//Hrm, server isn't in the database
	mysql_close(pSock);
	return true;
}
bool BATSQL::AmxBansCheckServerExists(MYSQL *sock)
{
	if (g_SQLSettings.DataBaseType != AmxBans ) return true;
	if (g_SQLSettings.AmxBansServerID != -1) return true;

	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;
	char	qbuf[2048];

	const char *pHostName = g_BATCore.GetBATVar().GetCvarString("hostname");
	const char *pIPAddress = g_BATCore.GetBATVar().GetCvarString("ip");
	const char *pHostPort = g_BATCore.GetBATVar().GetCvarString("hostport");
	const char *pRconPassword = g_BATCore.GetBATVar().GetCvarString("rcon_password");
	
	char HostName[128],RconPassword[128];
	mysql_escape_string(HostName,pHostName,strlen(pHostName));
	mysql_escape_string(RconPassword,pRconPassword,strlen(pRconPassword));

	if(stricmp(pIPAddress,"localhost") == 0)
	{
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: A critical error has happend, the cvar:'ip' returns localhost, BAT will now disable SQL support");
		ShutdownSQL();
		return false;
	}

	//Determine if this server is already in the database
	_snprintf(qbuf,sizeof(qbuf),"SELECT * FROM %s WHERE address='%s:%s'"
		,g_SQLSettings.AmxBansServerTable,pIPAddress,pHostPort);

	if(mysql_query(sock,qbuf))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Query failed (%s)\n",mysql_error(sock));
		mysql_close(sock);
		return false;
	}
	if (!(res=mysql_store_result(sock)))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Couldn't get result from %s\n",mysql_error(sock));
		mysql_close(sock);
		return false;
	}

	row = mysql_fetch_row(res);	

	if(res->row_count == 0)
	{
		mysql_free_result(res);		//Hrm, server isn't in the database

		memset(qbuf,0,sizeof(qbuf));
		_snprintf(qbuf,sizeof(qbuf),"INSERT INTO %s(timestamp,hostname,address,gametype) VALUES(unix_timestamp(),'%s', '%s:%s','%s')"
			,g_SQLSettings.AmxBansServerTable
			,HostName
			,pIPAddress
			,pHostPort
			,g_BATCore.GetServerGameDll()->GetGameDescription());


		if(mysql_query(sock,qbuf))
		{
			g_SQLStatus.Errors++;
			g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Query failed (%s)\n",mysql_error(sock));
			mysql_close(sock);
			return false;
		}
		g_AmxBansAddServerTries++;
		if(g_AmxBansAddServerTries > 5)
		{
			g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Trying to add this server to the amxbans database, failed after %i tries",g_AmxBansAddServerTries);
			ShutdownSQL();
			return false;
		}
		return AmxBansCheckServerExists(sock);
	}
	else
	{
		int srvid = atoi(row[0]);
		g_SQLSettings.AmxBansServerID = srvid;
		mysql_free_result(res);

		//Update to ensure nothing is incorrect
		_snprintf(qbuf,sizeof(qbuf),"UPDATE %s SET timestamp=unix_timestamp(), hostname='%s', gametype='%s', rcon='%s' WHERE id=%i;"
			,g_SQLSettings.AmxBansServerTable,HostName,g_BATCore.GetServerGameDll()->GetGameDescription(),RconPassword,srvid);

		if(mysql_query(sock,qbuf))
		{
			g_SQLStatus.Errors++;
			g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: Query failed (%s)\n",mysql_error(sock));
			mysql_close(sock);
			return false;
		}
		return true;
	}
	return true;
}
bool BATSQL::MakeLocalUsersFile()
{
	MYSQL_RES *res = NULL;
	MYSQL_ROW row = NULL;
	char	qbuf[256];

	MYSQL *pSock = mysql_init(NULL);
	g_SQLStatus.MakeLocalUsersFile = false;

	if (mysql_real_connect(pSock,g_SQLSettings.ServerIP,g_SQLSettings.UserName,g_SQLSettings.Password,g_SQLSettings.DataBase,0,NULL,0) == NULL)
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Couldn't connect to engine MySQL Server - %s",mysql_error(pSock));
		mysql_close(pSock);
		return false;
	}

	switch(g_SQLSettings.DataBaseType)
	{
	case AmxBans:
		if (!AmxBansCheckServerExists(pSock))
			return false;
		// SELECT BanReason,BanID FROM {0} WHERE PlayerID = '{1}'
		//_snprintf(qbuf,255,"SELECT * FROM %s WHERE PlayerID = '%s' or PlayerID = '%s' and srvid=%i",g_SQLSettings.AdminTable,g_UserInfo[index].Steamid,g_UserInfo[index].IP,g_SQLSettings.AmxBansServerID);
		_snprintf(qbuf,255,"SELECT PlayerID,AdminFlags,AdminName FROM %s WHERE srvid=%i",g_SQLSettings.AdminTable,g_SQLSettings.AmxBansServerID);		
		break;

	case SourceBans:
		//SELECT * FROM bat_admin WHERE server_id =-1 or server_id IS NULL
		_snprintf(qbuf,255,"SELECT * FROM %s WHERE server_id =%i or server_id IS NULL",g_SQLSettings.AdminTable,g_SQLSettings.AmxBansServerID);		
		break;

	case BAT_V2:
		_snprintf(qbuf,255,"SELECT * FROM %s",g_SQLSettings.AdminTable);
		break;
	}

#if DEBUG_SQL == 1
	g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL DEBUG: Query logged - %s",qbuf);
#endif

	if(mysql_query(pSock,qbuf))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Query failed - Trying to get all users from DB for updating users.ini file (%s)",mysql_error(pSock));
		mysql_close(pSock);
		return false;
	}
	if (!(res=mysql_store_result(pSock)))
	{
		g_SQLStatus.Errors++;
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Query failed - Trying to get all users from DB for updating users.ini file");
		mysql_close(pSock);
		return false;
	}
	//g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: Query logged - Rows returned for %s (%i)",g_UserInfo[index].Steamid,res->row_count);
	if(res->row_count == 0) 
	{
		// The DB dident return any users, this should not be posible, as we are asking for all users in the DB
		g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: When trying to get a users list from the SQL server, it dident return any players");

		mysql_free_result(res);
		mysql_close(pSock);
		return true;
	}

	g_BATCore.GetMsgBuffer()->AddMsgToBuffer("BAT is now updating your users.ini file based on the content on the SQL server");
	g_BATCore.GetAdminLoader()->ClearAdminList();
	//for(int i=0;i<res->row_count;i++)
	int tFlags = 0;

	while((row = mysql_fetch_row(res)) != NULL)
	{
		switch(g_SQLSettings.DataBaseType)
		{
		case SourceBans:
			//ad.id,ad.identity,gr.flags,ad.name,asg.server_id
			char AdminID[64],AdminFlags[32],AdminName[64];
			
			_snprintf(AdminID,63,"%s",row[0]);
			_snprintf(AdminFlags,31,"%s",row[1]);
			_snprintf(AdminName,63,"%s",row[2]);

			if(strcmp(AdminID,"(null)") == 0 || strcmp(AdminFlags,"(null)") == 0 || strcmp(AdminName,"(null)") == 0 )
			{
				g_BATCore.GetMsgBuffer()->AddMsgToBuffer("SQL ERROR: There was a error with the BAT view in sourcebans, it returned null something needs to be updated");
				continue;
			}
			
			tFlags = g_BATCore.GetAdminLoader()->ConvertFromSourceModAdminRights(AdminFlags);
			_snprintf(AdminFlags,31,"%s",g_BATCore.GetAdminLoader()->GetAdminAccountFlagsString(tFlags));						

			g_BATCore.GetAdminLoader()->AddAdminAccount(AdminID,AdminFlags,AdminName);
			break;

		case AmxBans:
			tFlags = g_BATCore.GetAdminLoader()->ConvertFromAmxxStyleAdminRights(row[1]);
			//g_BATCore.GetMsgBuffer()->AddMsgToBuffer("Debug on user: %s - Amxx Access: %s , new %s ( %d)",row[0],row[1],g_BATCore.GetAdminAccountFlagsString(tFlags),tFlags);
			//_snprintf(sTempLine,255,"%s,%s,%s\r\n",row[0],g_BATCore.GetAdminLoader()->GetAdminAccountFlagsString(tFlags),row[2]);
			g_BATCore.GetAdminLoader()->AddAdminAccount(row[0],g_BATCore.GetAdminLoader()->GetAdminAccountFlagsString(tFlags),row[2]);
			break;

		case BAT_V2:
			g_BATCore.GetAdminLoader()->AddAdminAccount(row[1],row[2],row[3]);
			break;
		}
	}
	g_BATCore.GetAdminLoader()->WriteAdminFile();

#if DEBUG_SQL == 1
	g_BATCore.GetMsgBuffer()->AddMsgToBuffer("Users.ini was updated based on the content of your SQL server, added %i admins",res->row_count);
#endif

	mysql_free_result(res);
	mysql_close(pSock);
	return true;
}
void BATSQL::ShutdownSQL()
{
	g_BATCore.ServerCommand("bat_mysql 0");
	g_SQLStatus.Enabled = false;
	g_SQLStatus.Errors += 6;
	g_BATCore.GetBATSql()->ExitSQLThread();
	
	g_BATCore.GetMsgBuffer()->AddMsgToBuffer("ERROR: BAT has disabled SQL, and is reverting to users.ini for admin access");
	g_BATCore.MessageAdmins("ERROR: BAT has disabled SQL, and is reverting to users.ini for admin access");
	g_BATCore.GetAdminLoader()->ReadAdminFile();

	// Incase someone thats already on the server should have admin, and does not. We check them all based on users.ini ( due to the fact that SQL failure takes up to 5 * 5 sec to stop SQL functions )
	for (int i=1;i<=g_MaxClients;i++) if(g_IsConnected[i] || g_IsConnecting[i])
	{
		g_UserInfo[i].SteamIdStatus = STEAM_ID_NOTVALID;
		g_UserInfo[i].AdminAccess = 0;
		g_BATCore.SetupPlayerInfo(i);
	}
}
