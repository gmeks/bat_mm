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

#ifndef _INCLUDE_MESSAGEBUFFER
#define _INCLUDE_MESSAGEBUFFER

typedef struct 
{
	char Message[MAX_LOGLEN+1];
	bool InUse;
}stMessageBuffer;

class MessageBuffer
{
public:
	void AddMsgToBuffer(const char *Msg, ...);
	bool HasBufferedMessages();
	const char *GetNextBufferedMsg();

private:	
	void ClearList();
	//static int ConvertFromAmxxStyleAdminRights(char *Flags);

};
#endif // end _INCLUDE_MESSAGEBUFFER

