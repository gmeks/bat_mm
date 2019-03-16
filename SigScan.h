#ifndef SIGSCAN_H
#define SIGSCAN_H

#include <stdio.h>
#include "SigScan.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h> 
#endif

/*
class Ext_SigScan
{
public:
#ifdef WIN32
	static void Init_Sigs();
#endif
};
*/

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return(stricmp(sz1, sz2) == 0);
}

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#endif
#include <sh_vector.h>
using namespace SourceHook;

class SigScan
{
public:
//	~SigScan(void)
	SigScan();
	void* FindFunctionAddresss(const char *SignatureName,unsigned char *WinSig, char *WinMask, size_t WinSigLength, char *NixSymbol);
	void Dispose();
	void FindBaseAddress();

	bool ScannerError;

private:
	/*
	//static unsigned char *base_addr;
	//static size_t base_len;
	unsigned char *sig_str;
	char *sig_mask;
	unsigned long sig_len;

	

	char sig_name[64];
	char is_set;
	void *sig_addr;
	void *linux_addr;
	*/
	
#ifdef WIN32
	//void* FindSignature(void);
	bool GetDllMemInfo(void);
	void* FindSignature(const char *SignatureName, unsigned char *WinSig, char *WinMask, size_t WinSigLength);
#else
	bool GetBaseAddress();
	void* FindSymbolAddress(char *SignatureName, char *symbol);
#endif
};


#endif

 