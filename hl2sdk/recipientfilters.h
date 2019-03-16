#ifndef _INCLUDE_CRECIPIENTFILTERS_H
#define _INCLUDE_CRECIPIENTFILTERS_H

#include <irecipientfilter.h>
#include <edict.h>
#include <bitvec.h>
#include <utlvector.h>

class RecipientFilter : public IRecipientFilter
{
public:
	RecipientFilter();
	~RecipientFilter();
public:
	bool IsReliable() const;
	bool IsInitMessage() const;
	void MakeReliable();
	void MsgRecipients();
public:
	int GetRecipientCount() const;
	int GetRecipientIndex(int slot) const;
public:
	void AddAllPlayers(int maxClients);
	void AddPlayer(edict_t *pPlayer);
	void AddPlayer(int index);
private:
	bool m_Reliable;
	bool m_InitMessage;
	CUtlVector<int> m_Recipients;
};

#endif //_INCLUDE_CRECIPIENTFILTERS_H
