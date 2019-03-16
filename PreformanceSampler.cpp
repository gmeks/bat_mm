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

#include "PreformanceSampler.h"

#if PREFORMANCE_SAMPLER == 1
#include <Windows.h>
#include "stdio.h"
#endif

#ifdef WIN32
#else
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define _snprintf snprintf	
#define _vsnprintf vsnprintf	
#endif


static __int64 g_QuickSampleList[MAX_SAMPLEITEMS];
static LARGE_INTEGER g_TempSample;
static __int64 g_Freq;

static int g_SampleCount;
static stPreSample g_SampleList[MAX_SAMPLEITEMS];
static bool g_HasBeenConstructed  = false;

PreformanceSampler::PreformanceSampler()
{
	QPF(&g_TempSample);

	g_Freq = ((__int64)g_TempSample.HighPart << 32) + (__int64)g_TempSample.LowPart;
	g_HasBeenConstructed = true;
}
PreformanceSampler::~PreformanceSampler()
{
	
}
int PreformanceSampler::AddPreformaceName(const char *NameOfTimer)
{
	if(!g_HasBeenConstructed)
		PreformanceSampler();

	strcpy(g_SampleList[g_SampleCount].Name,NameOfTimer);

	g_SampleList[g_SampleCount].TotalTime = 0;
	g_SampleList[g_SampleCount].CalledTimes = 0;
	g_SampleList[g_SampleCount].ShortestTime = ULONG_MAX;
	g_SampleList[g_SampleCount].LongestTime = 0;

	g_SampleCount++;
	return g_SampleCount - 1;
}

void PreformanceSampler::SampleStart(int SamplerIndex)
{
	QPC(&g_TempSample);

	g_QuickSampleList[SamplerIndex] = ((__int64)g_TempSample.HighPart << 32) + (__int64)g_TempSample.LowPart;
	(g_QuickSampleList[SamplerIndex] *= 1000000) /= g_Freq;
}
void PreformanceSampler::SampleEnd(int SamplerIndex)
{
	QPC(&g_TempSample);

	__int64 EndTime = ((__int64)g_TempSample.HighPart << 32) + (__int64)g_TempSample.LowPart;
	(EndTime *= 1000000) /= g_Freq;

	EndTime -= g_QuickSampleList[SamplerIndex];

	g_SampleList[SamplerIndex].CalledTimes++;
	g_SampleList[SamplerIndex].TotalTime += EndTime;	

	if(EndTime > g_SampleList[SamplerIndex].LongestTime)
		g_SampleList[SamplerIndex].LongestTime = EndTime;

	if(EndTime < g_SampleList[SamplerIndex].ShortestTime)
		g_SampleList[SamplerIndex].ShortestTime = EndTime;
}
void PreformanceSampler::WriteLogFile(const char *FileName)
{
	FILE *UsersFileStream = fopen(FileName, "wt"); // "wt" clears the file each time
	if(!UsersFileStream)
		return;

	char sTempLine[512];
	char MinTime[256];
	char MaxTime[256];
	char TotalTime[256];
	char AvrTime[256];

	fputs("<html><head><title>Preformance information</title></head>",UsersFileStream);	
	fputs("<table border=\"1\">",UsersFileStream);

		
	_snprintf(sTempLine,sizeof(sTempLine),"<tr><td>Preformance item name</td><td>Called times</td><td>Longest time</td><td>Shortest time</td><td>Total time</td><td>Average time</td></b>");
	fputs(sTempLine,UsersFileStream);

	for(int i=0;i<g_SampleCount;i++)
	{
		_snprintf(MinTime,sizeof(MinTime),"%s",GetTimeInformationString(g_SampleList[i].ShortestTime));
		_snprintf(MaxTime,sizeof(MaxTime),"%s",GetTimeInformationString(g_SampleList[i].LongestTime));
		_snprintf(TotalTime,sizeof(TotalTime),"%s",GetTimeInformationString(g_SampleList[i].TotalTime));
		_snprintf(AvrTime,sizeof(AvrTime),"%s",GetTimeInformationString(g_SampleList[i].TotalTime / g_SampleList[i].CalledTimes));
		

		_snprintf(sTempLine,sizeof(sTempLine),"<div><tr><td>%s</td><td>%d</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr></div>",g_SampleList[i].Name,g_SampleList[i].CalledTimes,MaxTime,MinTime,TotalTime,AvrTime);
		fputs(sTempLine,UsersFileStream);
	}

	fputs("</table></body></html>",UsersFileStream);	
	fclose(UsersFileStream);
}
const char *PreformanceSampler::GetTimeInformationString(__int64 Time)
{
	static char sTempString[256];
	long long Seconds = GetSeconds(Time);
	long long MiliSeconds = GetMiliseconds(Time);

	if(Seconds != 0)
	{
		_snprintf(sTempString,sizeof(sTempString),"%lld microseconds | %lld milliseconds | %lld seconds",Time,MiliSeconds,Seconds);
		return sTempString;
	}
	else if(MiliSeconds != 0)
	{
		_snprintf(sTempString,sizeof(sTempString),"%lld microseconds | %lld milliseconds",Time,MiliSeconds);
		return sTempString;
	}
	else
	{
		_snprintf(sTempString,sizeof(sTempString),"%lld microsecond",Time);
		return sTempString;
	}
}
long long PreformanceSampler::GetSeconds(__int64 Time)
{
	return Time / 1000000;
}

long long PreformanceSampler::GetMiliseconds(__int64 Time)
{
	return Time / 1000;
}