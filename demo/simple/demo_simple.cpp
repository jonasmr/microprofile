// This is free and unencumbered software released into the public domain.
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// For more information, please refer to <http://unlicense.org/>

#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <thread>
#include <atomic>
#include "math.h"

#ifndef _WIN32
#include <unistd.h>
#endif

//generate zipped results
#define MICROPROFILE_MINIZ 1
#if MICROPROFILE_MINIZ
#include "miniz.c"
#endif



#define MICROPROFILE_MAX_FRAME_HISTORY (2<<10)
#define MICROPROFILE_IMPL
#include "microprofile.h"

MICROPROFILE_DEFINE(MAIN, "MAIN", "Main", 0xff0000);

uint32_t g_nQuit = 0;

void spinsleep(int64_t nUs)
{
	float fToMs = MicroProfileTickToMsMultiplierCpu();
	int64_t nTickStart = MicroProfileTick();
	float fElapsed = 0;
	float fTarget = nUs / 1000.f;
	do
	{
		int64_t nTickEnd = MicroProfileTick();
		fElapsed = (nTickEnd - nTickStart) * fToMs;

	}while(fElapsed < fTarget);
}


void ExclusiveTest()
{
	while(!g_nQuit)
	{
		MICROPROFILE_SCOPEI("exclusive-test", "outer", MP_PINK);
		spinsleep(2000);
		// uint64_t nUs = 2000;
		// // MICROPROFILE_COUNTER_LOCAL_ADD_ATOMIC(ThreadSpinSleep, 1);
		// MICROPROFILE_SCOPEI("spin","sleep", 0xffff);
		// #if MICROPROFILE_ENABLED
		// float fToMs = MicroProfileTickToMsMultiplierCpu();
		// int64_t nTickStart = MicroProfileTick();
		// float fElapsed = 0;
		// float fTarget = nUs / 1000000.f;
		// do
		// {
		// 	int64_t nTickEnd = MicroProfileTick();
		// 	fElapsed = (nTickEnd - nTickStart) * fToMs;

		// }while(fElapsed < fTarget);
		// #endif	

		{
					MICROPROFILE_SCOPEI("exclusive-test", "inner", MP_GREEN);
					spinsleep(2000);
					MICROPROFILE_SCOPEI("exclusive-test-2", "inner-2", MP_CYAN);
					spinsleep(2000);

		// 	nUs = 2000;
		// 	MICROPROFILE_SCOPEI("spin","sleep", 0xffff);
		// #if MICROPROFILE_ENABLED
		// 	float fToMs = MicroProfileTickToMsMultiplierCpu();
		// 	int64_t nTickStart = MicroProfileTick();
		// 	float fElapsed = 0;
		// 	float fTarget = nUs / 1000000.f;
		// 	do
		// 	{
		// 		int64_t nTickEnd = MicroProfileTick();
		// 		fElapsed = (nTickEnd - nTickStart) * fToMs;

		// 	}while(fElapsed < fTarget);
		// #endif	


		}
	}
}

int main(int argc, char* argv[])
{
	MicroProfileOnThreadCreate("Main");
	printf("press ctrl-c to quit\n");

	//turn on profiling
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);

	std::thread T(ExclusiveTest);


	while(!g_nQuit)
	{
		MICROPROFILE_SCOPE(MAIN);
		{
			MICROPROFILE_SCOPEI("exclusive-test", "stable0", MP_WHEAT);
			spinsleep(2000);
		}
		static int x = 0;
		if(x++ % 20 < 10)
		{
			MICROPROFILE_SCOPEI("exclusive-test", "stable1", MP_GREEN3);
			spinsleep(50000 + rand()%2000);
		}
		else
		{
			spinsleep(40000);
			MICROPROFILE_SCOPEI("exclusive-test", "stable1", MP_GREEN3);
			spinsleep(10000 + rand()%2000);
		}


#ifdef _WIN32
		Sleep(10)
#else
		usleep(30000);
#endif
		MicroProfileFlip(0);
		static bool once = false;
		if(!once)
		{
			once = 1;
			printf("open localhost:%d in chrome to capture profile data\n", MicroProfileWebServerPort());
		}
	}

	T.join();
	MicroProfileShutdown();

	return 0;
}
