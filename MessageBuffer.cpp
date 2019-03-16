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
#include "BATSQL.h"
#include "time.h"
#include "const.h"
#include "MessageBuffer.h"

SourceHook::CVector<stMessageBuffer *>g_MessageBuffer;
bool g_HasMessagesInBuffer;
bool g_ListLocked;
uint g_BufferSize=0;

void MessageBuffer::AddMsgToBuffer(const char *Msg, ...)
{
	if(g_ListLocked)
	{
		int CPS = 0;
		while(g_ListLocked && CPS <= 500)
		{
			TSleep(1);
		}

		if(CPS >= 500)
		{
			g_BATCore.AddLogEntry("ERROR! Critical error with message buffer, waited 500ms for it to get unlocked but it did not, logged message lost");
			return;
		}
	}	

	g_ListLocked = true;
	g_HasMessagesInBuffer = true;

	int ItemIndex = -1;
	for(uint i=0;i<g_BufferSize;i++)
	{
		if(g_MessageBuffer[i] != NULL && !g_MessageBuffer[i]->InUse)
		{
			ItemIndex = i;
		}
	}
	if(ItemIndex == -1)
		ItemIndex = g_BufferSize;

	char vafmt[MAX_LOGLEN+1];
	va_list ap;
	va_start(ap, Msg);
	_vsnprintf(vafmt,MAX_LOGLEN,Msg,ap);
	va_end(ap);
	

	g_MessageBuffer.push_back(new stMessageBuffer);
	snprintf(g_MessageBuffer[g_BufferSize]->Message,MAX_LOGLEN,"%s",vafmt);
	g_MessageBuffer[g_BufferSize]->InUse = true;
	g_BufferSize++;
	g_ListLocked = false;
}
const char *MessageBuffer::GetNextBufferedMsg()
{
	if(g_ListLocked)
		return NULL;

	g_ListLocked = true;

	for(uint i=0;i<g_BufferSize;i++)
	{
		if(g_MessageBuffer[i] != NULL && g_MessageBuffer[i]->InUse)
		{
			g_ListLocked = false;
			g_MessageBuffer[i]->InUse = false;
			return g_MessageBuffer[i]->Message;
		}
	}

	g_MessageBuffer.clear();
	g_ListLocked = false;
	g_HasMessagesInBuffer = false;
	return NULL;
}

bool MessageBuffer::HasBufferedMessages() // Checks if there is a message in the buffer, and if its safe to get it. Will not return true if its currently not threadsafe to do so
{
	if(g_ListLocked)
		return false;

	if(g_BufferSize != 0 && !g_HasMessagesInBuffer)
		ClearList();

	return g_HasMessagesInBuffer;
}

void MessageBuffer::ClearList()
{
	g_MessageBuffer.clear();
	g_BufferSize = 0;
}
