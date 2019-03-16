/*
From:
http://wiki.alliedmods.net/Signature_Scanning
and PimpinJuice
*/

#include "BATCore.h"
#include "SigScan.h"

#ifdef WIN32
unsigned char* g_BaseAddr;
size_t g_BaseAddrLength;
#else
void* g_BaseAddr;
#endif
/*
SigScan::~SigScan(void)
{
	//delete[] sig_str;
	//delete[] sig_mask;
}
*/
SigScan::SigScan()
{
#ifdef WIN32
	bool GotBase = GetDllMemInfo();
#else
	bool GotBase = GetBaseAddress();
#endif
	if(!GotBase)
	{
		ScannerError = true;
		g_BATCore.AddLogEntry("ERROR: There was a error getting base address (More details should be above log entry)");
	}
	else
		ScannerError = false;
}
void* SigScan:: FindFunctionAddresss(const char *SignatureName,unsigned char *WinSig, char *WinMask, size_t WinSigLength, char *NixSymbol)
{
	if(ScannerError) 
		return NULL;

#ifdef WIN32
	void *FunctionPointer = FindSignature(SignatureName,WinSig,WinMask,WinSigLength);
#else
	void *FunctionPointer = FindSymbolAddress(SignatureName,NixSymbol);
#endif

	if(!FunctionPointer) g_BATCore.WriteLogBuffer(); // Something has gone wrong, we force the logs to written in case we are gonna crash soon
	return FunctionPointer;
}
void SigScan::Dispose()
{
#ifdef WIN32
	// Nothing to do really
#else
	if(g_BaseAddr) dlclose(g_BaseAddr);
	g_BaseAddr = NULL;
#endif
}

#ifdef WIN32
unsigned int istrcmp(char *str1, char *str2)
{
	unsigned int i, len1 = strlen(str1), len2 = strlen(str2);
	if(len1 != len2)
		return 0;
	for(i = 0;i < len1;i++)
	{
		if(tolower(str1[i]) != tolower(str2[i]))
			return 0;
	}
	return 1;
}

void* SigScan::FindSignature(const char *SignatureName, unsigned char *WinSig, char *WinMask, size_t WinSigLength)
{
	unsigned char *pBasePtr = g_BaseAddr;
	unsigned char *pEndPtr = g_BaseAddr+g_BaseAddrLength;
	size_t i, height = 0;
	while(pBasePtr < pEndPtr)
	{
		for(i = 0;i < WinSigLength;i++)
		{
			if(i > height)
				height++;
			if((WinMask[i] != '?') && (WinSig[i] != pBasePtr[i]))
				break;
		}
		if(i == WinSigLength)
			return (void*)pBasePtr;
		pBasePtr++;
	}

	g_BATCore.AddLogEntry("ERROR: %s - Signature finding failure: Sig Failed at Height: %d", SignatureName,height);
	return NULL;
}
bool SigScan::GetDllMemInfo(void)
{
	char binpath[1024];
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 modent;
	g_BaseAddr = 0;
	g_BaseAddrLength = 0;
	g_BATCore.GetEngine()->GetGameDir(binpath, 512);
	sprintf(binpath, "%s\\bin\\server.dll", binpath);
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if(hModuleSnap == INVALID_HANDLE_VALUE)
	{
		g_BATCore.AddLogEntry("ERROR: Signature finding failure: CreateToolhelp32Snapshot failed");
		return false;
	}
	modent.dwSize = sizeof(MODULEENTRY32);
	if(!Module32First(hModuleSnap, &modent))
	{
		g_BATCore.AddLogEntry("ERROR: Signature finding failure: Module32First failed.");
		CloseHandle(hModuleSnap);
		return false;
	}
	do
	{
		if(istrcmp(modent.szExePath, binpath))
		{
			g_BaseAddr = modent.modBaseAddr;
			g_BaseAddrLength = modent.modBaseSize;
			CloseHandle(hModuleSnap);
			return true;
		}
	} while(Module32Next(hModuleSnap, &modent));
	CloseHandle(hModuleSnap);

	g_BATCore.AddLogEntry("ERROR: Signature finding failure: Failed to find server module in module list");
	return false;
}

#else
bool SigScan::GetBaseAddress()
{
	char binpath[512];
	g_BATCore.GetEngine()->GetGameDir(binpath,511);
	sprintf(binpath, "%s/bin/server_i486.so",binpath);
	g_BaseAddr = dlopen(binpath, RTLD_NOW);
	if(g_BaseAddr == NULL)
	{
		g_BATCore.AddLogEntry("ERROR: dlopen() failed with error: \"%s\", This means signature scanning all failed",dlerror());
		return false;
	}
	return true;
	//find_sym_addr(handle,funcname,linuxstr,&newSignature->linux_addr);
}

void* SigScan::FindSymbolAddress(char *name, char *symbol) 
{
	void *addr;
	if(g_BaseAddr == NULL)
		return NULL;

	addr = dlsym(g_BaseAddr, symbol);
	if(addr == NULL) {
		g_BATCore.AddLogEntry("ERROR: dlsym() failed for function \"%s\" with error: \"%s\", plugin will not function properly.",name, dlerror());
		return NULL;
	}

#if BAT_DEBUG == 1
	g_BATCore.AddLogEntry("Found linux symbol function: %s, address: %X\n", name, addr);
#endif

	return addr;
}
#endif