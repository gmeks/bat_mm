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
#include "BATMaps.h"
#include "const.h"
//#include "convar.h"
#include "sh_string.h"
#include <sh_vector.h>
#include "Translation.h"

#ifndef WIN32
#define UINT_MAX      0xffffffff    /* maximum unsigned int value - This define is stolen from M$ wonder if i gett sued :)*/ 
#endif

SourceHook::CVector<TranslationStruct *>g_TranslationList;
SourceHook::CVector<TranslationLangNameStruct *>g_LanguageList;
char g_LanguageVersion[20];
unsigned int g_TranslationItemCount=0;
short g_LastLanguageIndex=0;			// Used so we know what the index of the last language we added
int g_CurrentLanguage = -1;
int g_MainLanguage = 0;

#define DEBUG_TRANSLATION 0

void Translation::UpdateCurLanguage()
{
	if(g_TranslationItemCount == 0)
		return;
	
	const char *Lang = g_BATCore.GetBATVar().GetBATLanguage();
	int NewLangauge = -1;

	for(short i=0;i < g_LastLanguageIndex;i++)
	{
		if(strcmp(g_LanguageList[i]->LanguageName,Lang) == 0)
		{
			NewLangauge = i;
			break;
		}
	}
	if(NewLangauge == -1)
	{
		char LangList[512];
		LangList[0] = '\0';

		for(int i=0;i<g_LastLanguageIndex;i++)
		{
			if( i != 0)
			{
				strcat(LangList,",");	
				strcat(LangList,g_LanguageList[i]->LanguageName);		//_snprintf(g_MapList,",%s",g_MapName[i]);
			}
			else
				strcat(LangList,g_LanguageList[i]->LanguageName);
		}
		if(g_CurrentLanguage == -1 || g_CurrentLanguage >= g_LastLanguageIndex)
			g_CurrentLanguage = 0;

		g_BATCore.ServerCommand("echo BAT supports the following languages: %s",LangList);
		return;
	}
	g_CurrentLanguage = NewLangauge;
	m_Translation->CheckLanguage(g_CurrentLanguage);
}
char *Translation::GetTranslation(const char *ItemName)
{
	int ItemNameLen = strlen(ItemName);
	unsigned long ItemNameHash = g_BATCore.StrHash(ItemName,ItemNameLen);

	for(uint i=0;i<g_TranslationItemCount;i++)
	{
		if(g_TranslationList[i]->ItemNameHash == ItemNameHash && strcmp(g_TranslationList[i]->ItemName,ItemName) == 0)
		{
			if(g_TranslationList[i]->UseCount != UINT_MAX)
				g_TranslationList[i]->UseCount++;

			if(g_CurrentLanguage != g_MainLanguage && g_TranslationList[i]->Translations[g_CurrentLanguage]->HasText)
				return g_TranslationList[i]->Translations[g_CurrentLanguage]->ItemText;
			else
				return g_TranslationList[i]->Translations[g_MainLanguage]->ItemText;
		}
	}

	return (char *)ItemName;
}
void Translation::LoadTranslationFiles()
{
	char TempTransFile[256];
	g_BATCore.GetFilePath(FILE_TRANSLATION,TempTransFile);
	FILE *in = fopen(TempTransFile,"rt");

	char line[256];
	int LineLen=0;
	int LineNum=0;

	if(!in)
	{
		g_BATCore.AddLogEntry("ERROR: The translation file is not present should be in '%s/%s' , this is a fatal error. Your server will likely crash soon or print garbage text all around",g_BasePath,TempTransFile);
		g_BATCore.WriteLogBuffer();
		return;
	}
	bool BadLanguage = false;
	int CurLangIndex = -1;
	int CurTranslationIndex=-1;
	int IOT=-1;
	char TempItemName[MAX_LANGUAGENAMELEN];

	g_TranslationList.clear();
	g_TranslationItemCount = 0;

	while (!feof(in)) 
	{
		line[0] = '\0';
		LineNum++;

		fgets(line,255,in);
		LineLen = strlen(line);
		StrTrim(line);

		if (!line || line[0] == '\0' ||line[0] == '/' || line[0] == ';' || LineLen <= 2) 
			continue;

		if(line[0] == '[') // Its a new language happy days
		{
			char Language[MAX_LANGUAGENAMELEN];
			IOT = g_BATCore.GetFirstIndexOfChar(line,LineLen,']');

			if(IOT == -1)
			{
				g_BATCore.AddLogEntry("Warning! Badly formated translation file, around line %d",LineNum);
				BadLanguage = true;
				continue;
			}

			_snprintf(Language,IOT,"%s",line+1);
			Language[IOT-1] = '\0';
			if(stricmp("version",Language) == 0)
			{
				fgets(line,255,in);
				LineLen = strlen(line);
				IOT = g_BATCore.GetFirstIndexOfChar(line,LineLen,'=');
				if(IOT == -1)
				{
					g_BATCore.AddLogEntry("Warning! Badly formated translation file, around line %d (Getting version)",LineNum);
					continue;
				}
				_snprintf(g_LanguageVersion,19,"%s",line+IOT+1);
				continue;
			}


			CurLangIndex = AddNewLanguage(Language);
			BadLanguage = false;
			continue;
		}
		else if(CurLangIndex != -1 && !BadLanguage)
		{
			IOT = g_BATCore.GetFirstIndexOfChar(line,LineLen,'=');
			if(IOT == -1)
			{
				g_BATCore.AddLogEntry("Warning! Badly formated translation file, around line %d (Failed to find = symbol)",LineNum);
				BadLanguage = true;
				continue;
			}
			IOT++;
			_snprintf(TempItemName,IOT,"%s",line);
			TempItemName[IOT-1] = '\0';
			g_BATCore.StrTrim(TempItemName);
			CurTranslationIndex = FindTransItemIndex(TempItemName);

			// Now we get the actual text
			IOT++;
			_snprintf(g_TranslationList[CurTranslationIndex]->Translations[CurLangIndex]->ItemText,LineLen-IOT,"%s",line+IOT);			
			g_TranslationList[CurTranslationIndex]->Translations[CurLangIndex]->HasText = true;
		}
		else
		{
			g_BATCore.AddLogEntry("Warning! Badly formated translation file, around line %d (No language set)",LineNum);
			continue;
		}
	}
#if DEBUG_TRANSLATION == 1
	g_BATCore.AddLogEntry("Language Debug: Done reading translation file, found %d languages",g_LastLanguageIndex+1);
#endif

	fclose(in);
}
int Translation::FindTransItemIndex(const char *ItemName)
{
	int TransItemIndex = 0;
	int ItemNameLen = strlen(ItemName);
	unsigned long ItemNameHash = g_BATCore.StrHash(ItemName,ItemNameLen);
	for(uint i=0;i<g_TranslationItemCount;i++)
	{
		if(g_TranslationList[i]->ItemNameHash == ItemNameHash && strcmp(g_TranslationList[i]->ItemName,ItemName) == 0)
			return i;
	}
	
	// We need to add the item
	g_TranslationList.push_back(new TranslationStruct);
	g_TranslationList[g_TranslationItemCount]->ItemNameHash = ItemNameHash;
	g_TranslationList[g_TranslationItemCount]->UseCount = 0;
	_snprintf(g_TranslationList[g_TranslationItemCount]->ItemName,ItemNameLen+1,"%s",ItemName);

	// We now need to add the different items for all languages that are already in the list	
	for(short i=0;i < g_LastLanguageIndex;i++)
	{
		g_TranslationList[g_TranslationItemCount]->Translations.push_back(new TranslationItemStruct);
		g_TranslationList[g_TranslationItemCount]->Translations[i]->HasText = false;
		g_TranslationList[g_TranslationItemCount]->Translations[i]->ItemText[0] = '\0';
	}
	g_TranslationItemCount++;

#if DEBUG_TRANSLATION == 1
	g_BATCore.AddLogEntry("Language Debug: Added new item: '%s'",ItemName);
#endif
	return (int)g_TranslationItemCount - 1;
}

int Translation::AddNewLanguage(const char *LanguageName)
{
	g_LanguageList.push_back(new TranslationLangNameStruct);
	g_LanguageList[g_LastLanguageIndex]->HasBeenChecked = false;

	int IOT = g_BATCore.GetFirstIndexOfChar((char *)LanguageName,MAX_LANGUAGENAMELEN,'=');
	if(IOT == -1)
	{
		_snprintf(g_LanguageList[g_LastLanguageIndex]->LanguageName,MAX_LANGUAGENAMELEN,"%s",LanguageName);
		g_LanguageList[g_LastLanguageIndex]->LanguageTranslator[0] = '\0';

#if DEBUG_TRANSLATION == 1
		g_BATCore.AddLogEntry("Language Debug: Added Language: '%s'",g_LanguageList[g_LastLanguageIndex]->LanguageName);
#endif
	}
	else
	{
		_snprintf(g_LanguageList[g_LastLanguageIndex]->LanguageName,IOT+1,"%s",LanguageName);
		g_LanguageList[g_LastLanguageIndex]->LanguageName[IOT] = '\0';
		
		_snprintf(g_LanguageList[g_LastLanguageIndex]->LanguageTranslator,MAX_LANGUAGENAMELEN-IOT,"%s",LanguageName+IOT+1);

#if DEBUG_TRANSLATION == 1
		g_BATCore.AddLogEntry("Language Debug: Added Language: '%s' Credits %s",g_LanguageList[g_LastLanguageIndex]->LanguageName,g_LanguageList[g_LastLanguageIndex]->LanguageTranslator);
#endif
	}

	for(uint i=0;i<g_TranslationItemCount;i++)
	{
		g_TranslationList[i]->Translations.push_back(new TranslationItemStruct);
		g_TranslationList[i]->Translations[g_LastLanguageIndex]->HasText = false;
		g_TranslationList[i]->Translations[g_LastLanguageIndex]->ItemText[0] = '\0';
	}
	g_LastLanguageIndex++;
	return g_LastLanguageIndex - 1;
}
void Translation::CheckLanguage(int LangIndex)
{
	if(g_MainLanguage == LangIndex)
		return;

	if(g_LanguageList[LangIndex]->HasBeenChecked)
		return;

	for(uint i=0;i<g_TranslationItemCount;i++)
	{
		if(!g_TranslationList[i]->Translations[LangIndex]->HasText)
		{
			g_BATCore.AddLogEntry("Warning: Missing translation for '%s' will default to %s",g_TranslationList[i]->ItemName,g_LanguageList[g_MainLanguage]);
		}		
	}
	g_LanguageList[LangIndex]->HasBeenChecked = true;
}
void Translation::SortTranslationList()
{

}