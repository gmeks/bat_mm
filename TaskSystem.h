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

#ifndef _INCLUDE_TASKSYSTEM_H
#define _INCLUDE_TASKSYSTEM_H

//#include <convar.h>
typedef struct 
{
	bool TaskActive;
	bool HasArgs;
	float ExecuteTime;
	int IntArg;
	CmdFuncIntArg FunctionPointer;
}stTask;


class TaskSystem
{
public:
	void CreateTask(CmdFuncIntArg FunctionPointer,int Delay,int IntArg); // Adds a task thats based on a client id
	void RemoveClientTasks(int ClientID); // Removes a task thats based on client id

	void CheckTasks(float CurTime);
	void ClearTaskList();
	
private:
	int GetTaskID(); // Find a task id to use
	//int GetIndexTranslation(char *Text);
	//unsigned int GetNextInArray(unsigned int Bellow);	 // Used with SortTranslationList() to find what the next item to use is.

};
#endif //_INCLUDE_TASKSYSTEM_H
