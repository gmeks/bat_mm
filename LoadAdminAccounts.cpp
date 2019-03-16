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
#include "LoadAdminAccounts.h"
#include "SourceModInterface.h"
#include "const.h"

#include <sourcehook/sourcehook.h>
#include <sh_vector.h>

SourceHook::CVector<AdminAccountInfo *>g_AdminAccount;
int unsigned g_AccountCount;

void LoadAdminAccounts::AddAdminAccount(const char *SteamIDorIP,const char *Flags,const char *AccountName)
{
	if(g_AccountCount >= g_AdminAccount.size()-1 || g_AdminAccount.size() == 0)
		g_AdminAccount.push_back(new AdminAccountInfo);

	_snprintf(g_AdminAccount[g_AccountCount]->Name,127,"%s",AccountName);
	_snprintf(g_AdminAccount[g_AccountCount]->LoginID,MAX_NETWORKID_LENGTH,"%s",SteamIDorIP);

	g_AdminAccount[g_AccountCount]->Access = ReadFlags(Flags);
	g_AdminAccount[g_AccountCount]->LoginIDHash = HashLogin(Flags);
	g_AccountCount++;
}

void LoadAdminAccounts::WriteAdminFile() // Writes out admina account in memory to a file
{
	char TempTransFile[256],sTempLine[256];	//This gets the actual path to the users.ini file.
	g_BATCore.GetFilePath(FILE_USERS,TempTransFile);
	//g_BATCore.AddLogEntry("BAT is now updating your users.ini file based on the content on the SQL server");

	FILE *UsersFileStream = fopen(TempTransFile, "wt"); // "wt" clears the file each time
	if(!UsersFileStream)
	{
		g_BATCore.AddLogEntry("ERROR: There was a error opening the users file for reading/writing check your setup, local copy cannot be made");
		return;
	}

	// Add the commen thats on top of the file	
	fputs("// Basic Admin tool users ini file\r\n",UsersFileStream);
	fputs("\r\n",UsersFileStream);
	fputs("// Format is:\r\n",UsersFileStream);
	fputs("// STEAMID,access,name\r\n",UsersFileStream);
	fputs("// IP,access,name\r\n",UsersFileStream);
	fputs("\r\n",UsersFileStream);
	fputs("// a = Access to kick,slap,slay,gag\r\n",UsersFileStream);
	fputs("// b = Access to ban\r\n",UsersFileStream);
	fputs("// c = Access to admin chat, and csay\r\n",UsersFileStream);
	fputs("// d = Access to execute commands directly to server console ( rcon )\r\n",UsersFileStream);
	fputs("// e = Access to change the map on the server\r\n",UsersFileStream);
	fputs("// f = Access to reserved slots\r\n",UsersFileStream);
	fputs("// g = Immunity to admin commands ( But only against admins that DONT have the immunity flag )\r\n",UsersFileStream);
	fputs("// h = Access to admin_noclip ( Only works on sourceforts )\r\n",UsersFileStream);
	fputs("// i = Access to admin_banip\r\n",UsersFileStream);
	fputs("// j = Access to Menu Rcon ( Predefined server side commands, in menucmds.ini\r\n",UsersFileStream);
	fputs("// k = Temp bans only ( Still requires b flag, this prevents perm bans or bans over 60min)\r\n",UsersFileStream);
	fputs("// l = Votes only ( Limits user to only be able to start votes for the admin action, like map change or kick/ban )\r\n",UsersFileStream);
	fputs("//\r\n",UsersFileStream);
	fputs("// YOU CANNOT USE LOCAL CHARACTERS IN ANYTHING, STICK TO THE ENGLISH ALFABET.\r\n\r\n",UsersFileStream);

	for (unsigned int i=0; i<g_AccountCount;i++)
	{
		_snprintf(sTempLine,255,"%s,%s,%s\r\n",g_AdminAccount[i]->LoginID,GetAdminAccountFlagsString(g_AdminAccount[i]->Access),g_AdminAccount[i]->Name);
		fputs(sTempLine,UsersFileStream);
	}

	fclose(UsersFileStream); // We close the file stream
}

void LoadAdminAccounts::ReadAdminFile() 
{
	//Begin loading admins
	char TempFile[256];	//This gets the actual path to the users.ini file.
	g_BATCore.GetFilePath(FILE_USERS,TempFile);
	FILE *is = fopen(TempFile,"rt");

	char line[512];
	int iLineLen=0;
	int unsigned LastAdminCount= g_AccountCount;

	g_AccountCount = 0;

	if (!is)
	{
		g_BATCore.AddLogEntry("ERROR! Unable to find admin users.ini file, should be in ( %s/%s )",g_BasePath,FILE_USERS);
		return;
	}

	char TmpChar[64];
	char *TempBuffer;
	int TextIndex=0;
	int LineNum=0;
	while (!feof(is)) 
	{
		if(g_AccountCount >= g_AdminAccount.size()-1 || g_AdminAccount.size() == 0)
			g_AdminAccount.push_back(new AdminAccountInfo);

		fgets(line,512,is);
		iLineLen = strlen(line);
		LineNum++;
		if (line[0] == '/' || line[0] == ';' || iLineLen <= 2) continue;

		//TempBuffer = strchr(line,',');
		for(int i=0;i<2;i++)
		{
			TempBuffer = strpbrk(line,",");
			if(!TempBuffer)		// If the line for some reason is incorrectly formated we abort.
			{
				g_BATCore.AddLogEntry("There was a error parsing the users.ini file on on line %d",LineNum);
				TextIndex = -1;
				break;
			}

			TextIndex = TempBuffer-line+1;
			if(TextIndex  <= 1) // There was no steamid in the text, its mailformed
			{
				g_BATCore.AddLogEntry("ERROR: There was a error in users.ini, on line %i",LineNum);
				continue;
			}

			_snprintf(TmpChar,TextIndex,"%s",line);
			TmpChar[TextIndex-1] = '\0'; // We replace the comma with a end of string symbol

			g_BATCore.StrReplace(line,TmpChar,"",iLineLen);
			line[0] = ' ';
			g_BATCore.StrTrim(TmpChar);
			if(i == 0)
			{
				_snprintf(g_AdminAccount[g_AccountCount]->LoginID,MAX_NETWORKID_LENGTH,TmpChar);
				g_AdminAccount[g_AccountCount]->LoginIDHash = HashLogin(TmpChar);
				//g_AdminAccount[g_AccountCount]->LoginIDHash = FastStrHash(TmpChar);
			}
			else
				g_AdminAccount[g_AccountCount]->Access = ReadFlags(TmpChar);
		}
		if(TextIndex != -1)
		{
			g_BATCore.StrTrim(line);
			_snprintf(g_AdminAccount[g_AccountCount]->Name,127,"%s",line);

#if BAT_DEBUG == 1
			g_BATCore.ServerCommand("echo %d) Admin Name: %s LoginID: %s (Hash: %d) Access %d",g_AccountCount,g_AdminAccount[g_AccountCount]->Name,g_AdminAccount[g_AccountCount]->LoginID,g_AdminAccount[g_AccountCount]->LoginIDHash,g_AdminAccount[g_AccountCount]->Access);
#endif
			g_AccountCount++;
		}
		line[0] = '\0';
	}
	fclose(is);
	if(LastAdminCount > g_AccountCount)	//This means the number of admin accounts has drooped, we should really clear memory
	{
		for(unsigned int i=g_AccountCount;i<LastAdminCount;i++)
			g_AdminAccount[i] = NULL;

		//g_AdminAccount.clear();
		g_BATCore.AddLogEntry("Loaded a total of %d admin accounts (%d less then last time)",g_AccountCount,LastAdminCount-g_AccountCount);
	}
	else
		g_BATCore.AddLogEntry("Loaded a total of %d admin accounts",g_AccountCount);
}

int LoadAdminAccounts::IsUserAdmin(const char *Steamid,const char *IP) 
{
	unsigned int QHashSteamID = HashLogin(Steamid);;
	unsigned int QHashIP = HashLogin(IP);

	for (unsigned int i=0; i<g_AccountCount;i++)
	{
		if (g_AdminAccount[i]->LoginIDHash == QHashSteamID && strcmp(g_AdminAccount[i]->LoginID,Steamid) == 0)
			return i;

		if (g_AdminAccount[i]->LoginIDHash == QHashIP && strcmp(g_AdminAccount[i]->LoginID,IP) == 0)
			return i;
	}
	return -1;
}

int LoadAdminAccounts::GetAdminAccountFlags(int AdminIndex) 
{
	return g_AdminAccount[AdminIndex]->Access;
}
int LoadAdminAccounts::FastStrHash(const char *OrgString) 
{
	int iFlags = 0;
	int iStrLength = strlen(OrgString);
	char TempChar = ' ';

	for(int i=0;i<iStrLength;i++)
	{
		TempChar = OrgString[i];
		if(isalpha(TempChar) && isupper(TempChar))
			TempChar = tolower(TempChar);

		switch (TempChar)
		{
		case 'a': 
			iFlags += (iFlags & (1<<0)) ? (1<<0) : -(1<<0);
			break;

		case 'b': 
			iFlags += (iFlags & (1<<1)) ? (1<<1) : -(1<<1);
			break;
			break;

		case 'c': 
			iFlags += (iFlags & (1<<2)) ? (1<<2) : -(1<<2);
			break;

		case 'd': 
			iFlags += (iFlags & (1<<3)) ? (1<<3) : -(1<<3);
			break;

		case 'e': 
			iFlags += (iFlags & (1<<4)) ? (1<<4) : -(1<<4);
			break;

		case 'f': 
			iFlags += (iFlags & (1<<5)) ? (1<<5) : -(1<<5);
			break;

		case 'g': 
			iFlags += (iFlags & (1<<6)) ? (1<<6) : -(1<<6);
			break;

		case 'h': 
			iFlags += (iFlags & (1<<7)) ? (1<<7) : -(1<<7);
			break;

		case 'i': 
			iFlags += (iFlags & (1<<8)) ? (1<<8) : -(1<<8);
			break;

		case 'j': 
			iFlags += (iFlags & (1<<9)) ? (1<<9) : -(1<<9);
			break;

		case 'k': 
			iFlags += (iFlags & (1<<10)) ? (1<<10) : -(1<<10);
			break;
		case 'l': 
			iFlags += (iFlags & (1<<11)) ? (1<<11) : -(1<<11);
			break;

		case 'm': 
			iFlags += (iFlags & (1<<12)) ? (1<<12) : -(1<<12);
			break;

		case 'n': 
			iFlags += (iFlags & (1<<13)) ? (1<<13) : -(1<<13);
			break;
		case 'o': 
			iFlags += (iFlags & (1<<14)) ? (1<<14) : -(1<<14);
			break;


		case 'p': 
			iFlags += (iFlags & (1<<15)) ? (1<<15) : -(1<<15);
			break;

		case '1':
		case 'q': 
			iFlags += (iFlags & (1<<16)) ? (1<<16) : -(1<<16);
			break;

		case '2':
		case 'r': 
			iFlags += (iFlags & (1<<17)) ? (1<<17) : -(1<<17);
			break;

		case '3':
		case 's': 
			iFlags += (iFlags & (1<<18)) ? (1<<18) : -(1<<18);
			break;

		case '4':
		case 't': 
			iFlags += (iFlags & (1<<19)) ? (1<<0) : -(1<<19);
			break;

		case '5':
		case 'u': 
			iFlags += (iFlags & (1<<20)) ? (1<<20) : -(1<<20);
			break;

		case '6':
		case 'v': 
			iFlags += (iFlags & (1<<21)) ? (1<<21) : -(1<<21);
			break;

		case '7':
		case 'w': 
			iFlags += (iFlags & (1<<22)) ? (1<<22) : -(1<<22);
			break;

		case '8':
		case 'x': 
			iFlags += (iFlags & (1<<23)) ? (1<<23) : -(1<<23);
			break;

		case '9':
		case 'y': 
			iFlags += (iFlags & (1<<24)) ? (1<<24) : -(1<<24);
			break;

		case '0':
		case 'z': 
			iFlags += (iFlags & (1<<25)) ? (1<<25) : -(1<<25);
			break;
		}
	}

	return iFlags;
}
int LoadAdminAccounts::ReadFlags(const char *c) 
{
	int flags = 0;
	int length = strlen(c);
	char TempChar = ' ';

	for(int i=0;i<length;i++)
	{
		TempChar = c[i];
		if(isalpha(TempChar) && isupper(TempChar))
			TempChar = tolower(TempChar);

		switch (TempChar)
		{
		case 'a': 
			if( !(flags & (1<<0)))
				flags += (1<<0);
			break;
		case 'b': 
			if( !(flags & (1<<1)))
				flags += (1<<1);
			break;
		case 'c': 
			if( !(flags & (1<<2)))
				flags += (1<<2);
			break;
		case 'd': 
			if( !(flags & (1<<3)))
				flags += (1<<3);
			break;
		case 'e': 
			if( !(flags & (1<<4)))
				flags += (1<<4);
			break;
		case 'f': 
			if( !(flags & (1<<5)))
				flags += (1<<5);
			break;
		case 'g': 
			if( !(flags & (1<<6)))
				flags += (1<<6);
			break;
		case 'h': 
			if( !(flags & (1<<7)))
				flags += (1<<7);
			break;
		case 'i': 
			if( !(flags & (1<<8)))
				flags += (1<<8);
			break;
		case 'j': 
			if( !(flags & (1<<9)))
				flags += (1<<9);
			break;
		case 'k': 
			if( !(flags & (1<<10)))
				flags += (1<<10);
			break;
		case 'l': 
			if( !(flags & (1<<11)))
				flags += (1<<11);
			break;
		case 'm': 
			if( !(flags & (1<<12)))
				flags += (1<<12);
			break;
		case 'n': 
			if( !(flags & (1<<13)))
				flags += (1<<13);
			break;
		case 'o': 
			if( !(flags & (1<<14)))
				flags += (1<<14);
			break;
		case 'p': 
			if( !(flags & (1<<15)))
				flags += (1<<15);
			break;
		case 'q': 
			if( !(flags & (1<<16)))
				flags += (1<<16);
			break;
		case 'r': 
			if( !(flags & (1<<17)))
				flags += (1<<17);
			break;
		case 's': 
			if( !(flags & (1<<18)))
				flags += (1<<18);
			break;
		case 't': 
			if( !(flags & (1<<19)))
				flags += (1<<19);
			break;
		case 'u': 
			if( !(flags & (1<<20)))
				flags += (1<<20);
			break;
		case 'v': 
			if( !(flags & (1<<21)))
				flags += (1<<21);
			break;
		case 'w': 
			if( !(flags & (1<<22)))
				flags += (1<<22);
			break;
		case 'x': 
			if( !(flags & (1<<23)))
				flags += (1<<23);
			break;
		case 'y': 
			if( !(flags & (1<<24)))
				flags += (1<<24);
			break;
		case 'z': 
			if( !(flags & (1<<25)))
				flags += (1<<25);
			break;
		}
	}
	return flags;
}


unsigned int LoadAdminAccounts::HashLogin(const char *a) 
{
	unsigned int hash = 0;
	int length = strlen(a);
	char c;

	for(int i=0;i<length;i++)
	{
		c = a[i];
		if(isalpha(c) && isupper(c))
			c = tolower(c);
		if(isdigit(c))
		{
			hash += (unsigned int)c;
			continue;
		}

		switch (c)
		{
		case '0': 
			hash += 0;
			break;
		case 'a': 
			hash += 1;
			break;
		case 'b': 
			hash += 2;
			break;
		case 'c': 
			hash += 3;
			break;
		case 'd': 
			hash += 4;
			break;
		case 'e': 
			hash += 5;
			break;
		case 'f': 
			hash += 6;
			break;
		case 'g': 
			hash += 7;
			break;
		case 'h': 
			hash += 8;
			break;
		case 'i': 
			hash += 9;
			break;
		case 'j': 
			hash += 10;
			break;
		case 'k': 
			hash += 11;
			break;
		case 'l': 
			hash += 12;
			break;
		case 'm': 
			hash += 13;
			break;
		case 'n': 
			hash += 14;
			break;
		case 'o': 
			hash += 15;
			break;
		case 'p': 
			hash += 16;
			break;
		case 'q': 
			hash += 17;
			break;
		case 'r': 
			hash += 18;
			break;
		case 's': 
			hash += 19;
			break;
		case 't': 
			hash += 20;
			break;
		case 'u': 
			hash += 21;
			break;
		case 'v': 
			hash += 22;
			break;
		case 'w': 
			hash += 23;
			break;
		case 'x': 
			hash += 24;
			break;
		case 'y': 
			hash += 25;
			break;
		case 'z': 
			hash += 26;
			break;
		case '_': 
			hash *= 27;
			break;
		case ':': 
			hash *= 28;
			break;
		case '.': 
			hash *= 29;
			break;
		}
	}
	return hash;
}

const char *LoadAdminAccounts::GetAdminAccountName(int AdminIndex) 
{
	return g_AdminAccount[AdminIndex]->Name;
}
const char* LoadAdminAccounts::GetAdminAccountFlagsString(int iFlags) // Converts from AMXXmod admin rights to BAT admin flags
{
	static char sFlags[30];
	sFlags[0] = '\0';

	/* BAT Access flags
	// a = Access to kick,slap,slay
	// b = Access to ban
	// c = Access to admin chat, and csay
	// d = Access to execute commands directly to server console ( rcon )
	// e = Access to change the map on the server
	// f = Access to reserved slots
	// g = Immunity to admin commands ( But only against admins that DONT have the immunity flag )
	// h = Access to admin_noclip ( Only works on sourceforts )
	// i = Access to admin_banip
	// j = Access to Menu rcon
	// k = Temp bans only ( Still requires b flag, this prevents perm bans or bans over 60min)
	*/

	if(iFlags & (1<<0))
		strncat(sFlags,"a",1);

	if(iFlags & (1<<1))
		strncat(sFlags,"b",1);

	if(iFlags & (1<<2))
		strncat(sFlags,"c",1);

	if(iFlags & (1<<3))
		strncat(sFlags,"d",1);

	if(iFlags & (1<<4))
		strncat(sFlags,"e",1);

	if(iFlags & (1<<5))
		strncat(sFlags,"f",1);
	
	if(iFlags & (1<<6))
		strncat(sFlags,"g",1);

	if(iFlags & (1<<7))
		strncat(sFlags,"h",1);

	if(iFlags & (1<<8))
		strncat(sFlags,"i",1);

	if(iFlags & (1<<9))
		strncat(sFlags,"j",1);

	if(iFlags & (1<<10))
		strncat(sFlags,"k",1);

	if(iFlags & (1<<11))
		strncat(sFlags,"l",1);

	if(iFlags & (1<<12))
		strncat(sFlags,"m",1);

	if(iFlags & (1<<13))
		strncat(sFlags,"n",1);

	if(iFlags & (1<<14))
		strncat(sFlags,"o",1);

	if(iFlags & (1<<15))
		strncat(sFlags,"p",1);

	if(iFlags & (1<<16))
		strncat(sFlags,"q",1);

	if(iFlags & (1<<17))
		strncat(sFlags,"r",1);

	if(iFlags & (1<<18))
		strncat(sFlags,"s",1);

	if(iFlags & (1<<19))
		strncat(sFlags,"t",1);
	
	if(iFlags & (1<<20))
		strncat(sFlags,"u",1);

	if(iFlags & (1<<21))
		strncat(sFlags,"v",1);

	if(iFlags & (1<<22))
		strncat(sFlags,"w",1);

	if(iFlags & (1<<23))
		strncat(sFlags,"x",1);
	
	if(iFlags & (1<<24))
		strncat(sFlags,"y",1);

	if(iFlags & (1<<25))
		strncat(sFlags,"z",1);

	return (char *)sFlags;
}
int LoadAdminAccounts::ConvertFromAmxxStyleAdminRights(const char *pFlags)
{
	/* AMXX Access rights
	; Access flags:
	; a - immunity (can't be kicked/baned/slayed/slaped and affected by other commmands)
	; b - reservation (can join on reserved slots)
	; c - amx_kick command
	; d - amx_ban and amx_unban commands
	; e - amx_slay and amx_slap commands
	; f - amx_map command
	; g - amx_cvar command (not all cvars will be available)
	; h - amx_cfg command
	; i - amx_chat and other chat commands
	; j - amx_vote and other vote commands
	; k - access to sv_password cvar (by amx_cvar command)
	; l - access to amx_rcon command and rcon_password cvar (by amx_cvar command)
	; m - custom level A (for additional plugins)
	; n - custom level B
	; o - custom level C
	; p - custom level D
	; q - custom level E
	; r - custom level F
	; s - custom level G
	; t - custom level H
	; u - menu access
	; z - user (no admin)
	*/
	/* BAT Access flags
	// a = Access to kick,slap,slay
	// b = Access to ban
	// c = Access to admin chat, and csay
	// d = Access to execute commands directly to server console ( rcon )
	// e = Access to change the map on the server
	// f = Access to reserved slots
	// g = Immunity to admin commands ( But only against admins that DONT have the immunity flag )
	// h = Access to admin_noclip ( Only works on sourceforts )
	// i = Access to admin_banip
	*/
	int flags = 0;
	int length = strlen(pFlags);
	char TempChar;

	for(int i=0;i<length;i++)
	{
		if(!isalpha(pFlags[i]))
			continue;

		if(isupper(pFlags[i]))
			TempChar = tolower(pFlags[i]);

		switch (TempChar)
		{
		case 'a':  // admin immunity
			flags += ADMIN_IMMUNITY; 
			break;
		case 'b':  // Reserved slots
			flags += ADMIN_RESERVEDSLOTS;
			break;
		case 'c':  // kick access
			if(!(flags & ADMIN_KICK)) // We only add the flag once
				flags += ADMIN_KICK;
			break;
		case 'd': // ban
			flags += ADMIN_BAN;
			break;
		case 'e':  // Slay
			if(!(flags & ADMIN_KICK)) // We only add the flag once
				flags += ADMIN_KICK;
			break;
		case 'f':  // Map change
			flags += ADMIN_MAP;
			break;
		case 'g':  // Nothing compares: admin_cvar
			break;
		case 'h':   // Nothing compares: admin_cfg
			break;
		case 'i':  // Chat access
			flags += ADMIN_CHAT;
			break;
		case 'j':  // Vote access
			flags += ADMIN_MENURCON;
			break;
		case 'k':  // Nothing compares
			break;
		case 'l':  // Rcon
			flags += ADMIN_RCON;
			break;
		case 'm': 
			flags += (1<<12);
			break;
		case 'n': 
			flags += (1<<13);
			break;
		case 'o': 
			flags += (1<<14);
			break;
		case 'p': 
			flags += (1<<15);
			break;
		case 'q': 
			flags += (1<<16);
			break;
		case 'r': 
			flags += (1<<17);
			break;
		case 's': 
			flags += (1<<18);
			break;
		case 't': 
			flags += (1<<19);
			break;
		case 'u':  // Menu access - Nothing compares
			flags += (1<<20);
			break;
		case 'v': 
			flags += (1<<21);
			break;
		case 'w': 
			flags += (1<<22);
			break;
		case 'x': 
			flags += (1<<23);
			break;
		case 'y': 
			flags += (1<<24);
			break;
		case 'z': 
			flags += (1<<25);
			break;
		}
	}
	return flags;
}
int LoadAdminAccounts::ConvertFromSourceModAdminRights(const char *pFlags)
{
	/* AMXX Access rights
	reservation  a  Reserved slot access.  
	generic  b  Generic admin; required for admins.  
	kick  c  Kick other players.  
	ban  d  Ban other players.  
	unban  e  Remove bans.  
	slay  f  Slay/harm other players.  
	map  g  Change the map or major gameplay features.  
	cvar  h  Change most cvars.  
	config  i  Execute config files.  
	chat  j  Special chat privileges.  
	vote  k  Start or create votes.  
	password  l  Set a password on the server.  
	rcon  m  Use RCON commands.  
	cheats  m  Change sv_cheats or use cheating commands.  
	root  z  Magically enables all flags.  
	*/
	/* BAT Access flags
	// a = Access to kick,slap,slay
	// b = Access to ban
	// c = Access to admin chat, and csay
	// d = Access to execute commands directly to server console ( rcon )
	// e = Access to change the map on the server
	// f = Access to reserved slots
	// g = Immunity to admin commands ( But only against admins that DONT have the immunity flag )
	// h = Access to admin_noclip ( Only works on sourceforts )
	// i = Access to admin_banip
	*/
	int flags = 0;
	int length = strlen(pFlags);

	for(int i=0;i<length;i++)
	{
		if(!isalpha(pFlags[i]))
			continue;

		char TempChar = pFlags[i];

		if(isupper(TempChar))
			TempChar = tolower(TempChar);

		switch (TempChar)
		{
		case 'a':  
			if(!(flags & ADMIN_RESERVEDSLOTS)) // We only add the flag once
				flags += ADMIN_RESERVEDSLOTS; 
			break;
		case 'b': // generic admin? we dont have those around in these parts
			//flags += ADMIN_RESERVEDSLOTS;
			break;
		case 'c':  // kick access
			if(!(flags & ADMIN_KICK)) // We only add the flag once
				flags += ADMIN_KICK;
			break;
		case 'd': // ban
			if(!(flags & ADMIN_BAN)) // We only add the flag once
				flags += ADMIN_BAN;
			break;
		case 'e':  // Remove bans
			//flags += (1<<5);
			break;
		case 'f':  // Map change
			if(!(flags & ADMIN_KICK)) // We only add the flag once
				flags += ADMIN_KICK;
			break;
		case 'g':  
			if(!(flags & ADMIN_MAP)) 
				flags += ADMIN_MAP;
			break;
		case 'h':   // Nothing compares: admin_cfgw
			//if(!(flags & (1<<7)))
			//	flags += (1<<7);
			break;
		case 'i':  // Chat access
			//if(!(flags & (1<<8)))
			//	flags += (1<<8);
			break;
		case 'j':  // Vote access
			if(!(flags & ADMIN_CHAT)) // We only add the flag once
				flags += ADMIN_CHAT;
			break;
		case 'k':  // K flag in BAT means limited banning abilities, so we dont add that.
			if(!(flags & 1<<10))
				flags += (1<<10);
			break;
		case 'l':  // Rcon
			if(!(flags & ADMIN_RCON))
				flags += ADMIN_RCON;
			break;
		case 'm': 
			if(!(flags & ADMIN_RCON)) // We only add the flag once
				flags += ADMIN_RCON;
			break;
		case 'n': 
			//flags += (1<<13);
			break;
		case 'o': 
			//flags += (1<<14);
			break;
		case 'p': 
			//flags += (1<<15);
			break;
		case 'q': 
			//flags += (1<<16);
			break;
		case 'r': 
			//flags += (1<<17);
			break;
		case 's': 
			//flags += (1<<18);
			break;
		case 't': 
			//flags += (1<<19);
			break;
		case 'u':  // Menu access - Nothing compares
			break;
		case 'v': 
			//flags += (1<<21);
			break;
		case 'w': 
			//flags += (1<<22);
			break;
		case 'x': 
			//flags += (1<<23);
			break;
		case 'y': 
			//flags += (1<<24);
			break;
		case 'z': 
			//flags += (1<<25);
			break;
		}
	}
	return flags;
}


int LoadAdminAccounts::ConvertToSoureModAdminRights(int iBATFlags)
{
	/* Sourcemod
	//#define ADMFLAG_GENERIC				(1<<1)		/**< Convenience macro for Admin_Generic as a FlagBit */
	//#define ADMFLAG_KICK				(1<<2)		/**< Convenience macro for Admin_Kick as a FlagBit */
	//#define ADMFLAG_BAN					(1<<3)		/**< Convenience macro for Admin_Ban as a FlagBit */
	//#define ADMFLAG_UNBAN				(1<<4)		/**< Convenience macro for Admin_Unban as a FlagBit */
	//#define ADMFLAG_SLAY				(1<<5)		/**< Convenience macro for Admin_Slay as a FlagBit */
	//#define ADMFLAG_CHANGEMAP			(1<<6)		/**< Convenience macro for Admin_Changemap as a FlagBit */
	//#define ADMFLAG_CONVARS				(1<<7)		/**< Convenience macro for Admin_Convars as a FlagBit */
	//#define ADMFLAG_CONFIG				(1<<8)		/**< Convenience macro for Admin_Config as a FlagBit */
	//#define ADMFLAG_CHAT				(1<<9)		/**< Convenience macro for Admin_Chat as a FlagBit */
	//#define ADMFLAG_VOTE				(1<<10)		/**< Convenience macro for Admin_Vote as a FlagBit */
	//#define ADMFLAG_PASSWORD			(1<<11)		/**< Convenience macro for Admin_Password as a FlagBit */
	//#define ADMFLAG_RCON				(1<<12)		/**< Convenience macro for Admin_RCON as a FlagBit */
	//#define ADMFLAG_CHEATS				(1<<13)		/**< Convenience macro for Admin_Cheats as a FlagBit */
	//#define ADMFLAG_ROOT				(1<<14)		/**< Convenience macro for Admin_Root as a FlagBit */
	//#define ADMFLAG_CUSTOM1				(1<<15)		/**< Convenience macro for Admin_Custom1 as a FlagBit */
	//#define ADMFLAG_CUSTOM2				(1<<16)		/**< Convenience macro for Admin_Custom2 as a FlagBit */
	//#define ADMFLAG_CUSTOM3				(1<<17)		/**< Convenience macro for Admin_Custom3 as a FlagBit */
	//#define ADMFLAG_CUSTOM4				(1<<18)		/**< Convenience macro for Admin_Custom4 as a FlagBit */
	//#define ADMFLAG_CUSTOM5				(1<<19)		/**< Convenience macro for Admin_Custom5 as a FlagBit */
	//#define ADMFLAG_CUSTOM6				(1<<20)		/**< Convenience macro for Admin_Custom6 as a FlagBit */

	int SMFlags = 0;

	if(iBATFlags & ADMIN_BAN)
	{
		SMFlags += ADMFLAG_BAN;

		if(!(iBATFlags & ADMIN_TEMPBAN))
			SMFlags += ADMFLAG_UNBAN;
	}

	if(iBATFlags & ADMIN_KICK)
	{
		SMFlags += ADMFLAG_KICK;
		SMFlags += ADMFLAG_SLAY;
	}

	if(iBATFlags & ADMIN_MAP)
		SMFlags += ADMFLAG_CHANGEMAP;

	if(iBATFlags & ADMIN_CHAT)
		SMFlags += ADMFLAG_CHAT;

	if(iBATFlags & ADMIN_RCON)
		SMFlags += ADMFLAG_RCON;

	if(iBATFlags & ADMIN_MENURCON)
		SMFlags += ADMFLAG_CONFIG;

	return SMFlags;
}

