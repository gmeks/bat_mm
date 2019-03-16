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
#include "TaskSystem.h"

SourceHook::CVector<stTask *>g_TaskList;
unsigned int g_TaskListSize=0;

/*
Things to time:
	StartChangelevelCountDown()
*/

#define TASK_SYSTEM_DEBUG 0

void TaskSystem::CreateTask(CmdFuncIntArg FunctionPointer,int Delay,int ClientID) // ClientID should match a clients index
{
	// We try to find a task id
	int NewTaskID = GetTaskID();
	g_TaskList[NewTaskID]->TaskActive = true; 
	g_TaskList[NewTaskID]->FunctionPointer = FunctionPointer;
	g_TaskList[NewTaskID]->HasArgs = true;
	g_TaskList[NewTaskID]->IntArg = ClientID;
	g_TaskList[NewTaskID]->ExecuteTime = g_SMAPI->pGlobals()->curtime + Delay;

#if TASK_SYSTEM_DEBUG == 1
	g_BATCore.AddLogEntry("Task System Debug: TaskID: %d) Client Task added for %d time %f(%d New delay) ( Curtime %f )",NewTaskID,ClientID,g_TaskList[NewTaskID]->ExecuteTime,Delay,g_SMAPI->GetCGlobals()->curtime);
#endif
}

void TaskSystem::CheckTasks(float CurTime)
{
	if(g_TaskListSize == 0)
		return;

	int ActiveTasks=0;

	for (unsigned int i=0; i<g_TaskListSize;i++)
	{
		if(g_TaskList[i] == NULL || !g_TaskList[i]->TaskActive )
			continue;

		ActiveTasks++;

		if (CurTime >= g_TaskList[i]->ExecuteTime)
		{
#if TASK_SYSTEM_DEBUG == 1
			g_BATCore.AddLogEntry("Task System Debug:  TaskID: %d exetued  %f ( Curtime %f )",g_TaskList[i]->IntArg,g_TaskList[i]->ExecuteTime,g_SMAPI->GetCGlobals()->curtime);
#endif

			if(g_TaskList[i]->HasArgs)
				g_TaskList[i]->FunctionPointer(g_TaskList[i]->IntArg);
			//else
			//	g_TaskList[i]->FunctionPointer();
			// We now delete the task.
			g_TaskList[i]->TaskActive = false;
		}
	}

	if(ActiveTasks == 0)
	{
		ClearTaskList();
	}
}
void TaskSystem::RemoveClientTasks(int ClientID)
{
	if(g_TaskListSize == 0)
		return;
	
	for (unsigned int i=0; i<g_TaskListSize;i++)
	{
		if (g_TaskList[i] != NULL && g_TaskList[i]->IntArg == ClientID)
		{
			g_TaskList[i]->TaskActive = false;
		}
	}
}
void TaskSystem::ClearTaskList()
{
	g_TaskList.clear();
	g_TaskListSize = 0;
}
int TaskSystem::GetTaskID()
{
	int ActiveTasks=0;

	for (unsigned int i=0; i<g_TaskListSize;i++)
	{
		if(g_TaskList[i] != NULL && !g_TaskList[i]->TaskActive)
		{
#if TASK_SYSTEM_DEBUG == 1
			g_BATCore.AddLogEntry("Task System Debug: Finding TaskID found only unused index at %d",i);
#endif
			return i;
		}
	}

	// We failed to find a old task, we crate a new one
	g_TaskList.push_back(new stTask);
	g_TaskListSize++;

#if TASK_SYSTEM_DEBUG == 1
	g_BATCore.AddLogEntry("Task System Debug: Increased the size of the task array its now %d in size",g_TaskListSize);
#endif
	return (g_TaskListSize - 1 );
}