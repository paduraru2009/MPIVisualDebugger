#pragma once

#include <time.h>
#include <string>
#include <iostream>
#include <sstream>
#define USE_MPI_SYNCRONIZED_TIMER	// Comment this if you want to use system time

#ifdef USE_MPI_SYNCRONIZED_TIMER
#include <mpi.h>
#else
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#include <sys/time.h>
#endif
#endif 

// Precision in nano-seconds
#define MPIDEBUGGER_TIME_PRECISION 1000000

#define COMPILE_TIME_ASSERT(cond) \
	typedef char compile_time_assert##__FILE__[(cond) ? 1 : -1];

// Returns only file name from full path
class Utils
{
public:
	
	static char* getFileName(const char* fullPath);

	// time util functions
	static long long convertWallDoubleTimeToLongInt(const double doubleWallTime);

	static void startTimer();
	static void setTimer(const long long baseTime) { m_sBaseTime = baseTime; }

	// Time of events keeping in sync machines and order
	static long long getDebuggerTime() { return (convertWallDoubleTimeToLongInt(get_wall_time()) - m_sBaseTime); }
	
	static void fillFileNameTextForRank(std::ostringstream& outStr, const int rank)
	{
		outStr<<"MPIDebuggerEvents_"<<rank<<".txt";
	}

	static void fillFileNameBinaryForRank(std::ostringstream& outStr, const int rank)
	{
		outStr<<"MPIDebuggerEvents_"<<rank<<".bin";
	}

#ifdef USE_MPI_SYNCRONIZED_TIMER
	static double get_wall_time() 
	{ 
		return MPI_Wtime();
	}
#else
	#ifdef _WIN32
	static double get_wall_time()
	{
		LARGE_INTEGER time,freq;
		if (!QueryPerformanceFrequency(&freq)){
			//  Handle error
			return 0;
		}
		if (!QueryPerformanceCounter(&time)){
			//  Handle error
			return 0;
		}
		return (double)time.QuadPart / freq.QuadPart;
	}
	static double get_cpu_time()
	{
		FILETIME a,b,c,d;
		if (GetProcessTimes(GetCurrentProcess(),&a,&b,&c,&d) != 0){
			//  Returns total user time.
			//  Can be tweaked to include kernel times as well.
			return
				(double)(d.dwLowDateTime |
				((unsigned long long)d.dwHighDateTime << 32)) * 0.0000001;
		}else{
			//  Handle error
			return 0;
		}
	}

	
	#else//  Posix/Linux
	static double get_wall_time()
	{
		struct timeval time;
		if (gettimeofday(&time,NULL)){
			//  Handle error
			return 0;
		}
		return (double)time.tv_sec + (double)time.tv_usec * .000001;
	}
	static double get_cpu_time()
	{
		return (double)clock() / CLOCKS_PER_SEC;
	}
	#endif // _WIN32
#endif // USE_MPI_SYNCRONIZED_TIMER

	static long long m_sBaseTime;
private:

	
};