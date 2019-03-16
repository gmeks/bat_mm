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

#ifndef _INCLUDE_TRANSLATION_H
#define _INCLUDE_TRANSLATION_H

//#include <convar.h>

#define MAX_TRANSMAXLEN 192
#define MAX_LANGUAGENAMELEN 40
#define TRANS_UPDATE_TIME 2000

typedef struct 
{
	char LanguageName[MAX_LANGUAGENAMELEN+1]; 	// The "name" of the text, and that we search after
	char LanguageTranslator[MAX_LANGUAGENAMELEN+1]; 	// The "name" of the text, and that we search after
	bool HasBeenChecked;
}TranslationLangNameStruct;
typedef struct 
{
	char ItemText[MAX_TRANSMAXLEN+1]; 	// The "name" of the text, and that we search after
	bool HasText;
}TranslationItemStruct;
typedef struct 
{
	char ItemName[MAX_TRANSMAXLEN+1]; 	// The "name" of the text, and that we search after
	unsigned long ItemNameHash;			// The hash of the TextName Char array
	unsigned int UseCount;				// The amount of times this language string has been gotten. Used to determine the order of the array
	SourceHook::CVector<TranslationItemStruct *>Translations;
}TranslationStruct;


class Translation : private StrUtil
{
public:
	void LoadTranslationFiles(); // Reads the translation file into memory
	static void UpdateCurLanguage(); // Reads the bat_language CVAR, and updates the internal globals, incase a new language was selected
	char *GetTranslation(const char *Trans);
	void SortTranslationList(); // This function resorts the translation list after how much each item is used
	void CheckLanguage(int LangIndex);

	
private:
	int AddNewLanguage(const char *LanguageName);
	int FindTransItemIndex(const char *ItemName);
};
#endif //_INCLUDE_TRANSLATION_H

