/* ======== Basic Admin tool ========
* Copyright (C) 2004-2007 Erling K. Sæterdal
* No warranties of any kind
*
*
* Author(s): Erling K. Sæterdal ( EKS )
* ============================ */

#ifndef _INCLUDE_PREFORMANCESAMLER
#define _INCLUDE_PREFORMANCESAMLER

#define MAX_SAMPLENAME_LEN 40
#define MAX_SAMPLEITEMS 25

#ifdef WIN32
#define PREFORMANCE_SAMPLER 1
#else
#define __int64 long
#define ULONG_MAX 45000
typedef struct 
{
	long HighPart;
	long LowPart;
}LARGE_INTEGER;
#define PREFORMANCE_SAMPLER 0
#endif

struct stPreSample 
{
	long long TotalTime;
	long long LongestTime;
	long long ShortestTime;
	long CalledTimes;
	char Name[MAX_SAMPLENAME_LEN];
};
class PreformanceSampler
{
public:
	PreformanceSampler();
	~PreformanceSampler();

public:
	static int AddPreformaceName(const char *NameOfTimer);
	static void SampleStart(int SamplerIndex);
	static void SampleEnd(int SamplerIndex);
	static void WriteLogFile(const char *FileName);

private:
	static long long GetSeconds(__int64 Time);
	static long long GetMiliseconds(__int64 Time);
	static const char *GetTimeInformationString(__int64 Time);
};

#if PREFORMANCE_SAMPLER == 1

#define PreSamplerStart(SampleName) \
	static int iSampleIndex = -1; \
	if(iSampleIndex == -1) \
		iSampleIndex = PreformanceSampler::AddPreformaceName(SampleName); \
	\
	PreformanceSampler::SampleStart(iSampleIndex);

#define PreSamplerEnd(SampleName) \
	PreformanceSampler::SampleEnd(iSampleIndex);

#define QPF(Sample) \
	QueryPerformanceFrequency(Sample);

#define QPC(Sample) \
	QueryPerformanceCounter(Sample);
#else
#define PreSamplerStart(SampleIndex)

#define PreSamplerEnd(SampleName)

#define QPF(Sample)

#define QPC(Sample)
#endif

#endif // _INCLUDE_PREFORMANCESAMLER