/* ======== Basic Admin tool ========
* Copyright (C) 2004-2006 Erling K. Sæterdal
* No warranties of any kind
*
* License: zlib/libpng
*
* Author(s): Erling K. Sæterdal ( EKS )
* Credits:
*	Menu code based on code from CSDM ( http://www.tcwonline.org/~dvander/cssdm ) Created by BAILOPAN
*	Helping on misc errors/functions: BAILOPAN,LDuke,sslice,devicenull,PMOnoTo,cybermind ( most who idle in #sourcemod on GameSurge realy )
* ============================ */

#include "recipientfilters.h"
#include "utlvector.h"
#include "../BATCore.h"

//extern int g_MaxClients;
//extern bool g_IsConnected[MAXPLAYERS+1];
extern ConstPlayerInfo g_UserInfo[MAXPLAYERS+2];

RecipientFilter::RecipientFilter()
{
	m_InitMessage = false;
	m_Reliable = false;
}

RecipientFilter::~RecipientFilter()
{
}

void RecipientFilter::MakeReliable()
{
	m_Reliable = true;
}

void RecipientFilter::MsgRecipients()
{
	Msg("Displaying recipients:\n");
	for (int i=0; i<GetRecipientCount(); i++)
	{
		Msg("Recipient #%d: %d\n", i, GetRecipientIndex(i));
	}
}

int RecipientFilter::GetRecipientCount() const
{
	return m_Recipients.Size();
}

int RecipientFilter::GetRecipientIndex(int slot) const
{
	if (slot < 0 || slot >= GetRecipientCount())
		return -1;

	return m_Recipients[slot];
}

bool RecipientFilter::IsInitMessage() const
{
	return m_InitMessage;
}

bool RecipientFilter::IsReliable() const
{
	return m_Reliable;
}

void RecipientFilter::AddAllPlayers(int maxClients)
{
	for (int i=1; i<=maxClients; i++)
	{
		if(g_IsConnected[i] == true && !g_UserInfo[i].IsBot)
			m_Recipients.AddToTail(i);
	}
}

/*
void RecipientFilter::AddPlayer(edict_t *e)
{
	int i = g_BATCore.m_Engine->IndexOfEdict(e);
	if(g_IsConnected[i] == true)
		m_Recipients.AddToTail(g_BATCore.m_Engine->IndexOfEdict(e));
}
*/

void RecipientFilter::AddPlayer(int index)
{
	if(g_IsConnected[index] == true && !g_UserInfo[index].IsBot)
		m_Recipients.AddToTail(index);
}
