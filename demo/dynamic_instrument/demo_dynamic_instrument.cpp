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
void LongTest()
{
	while(!g_nQuit)
	{
		{
			MICROPROFILE_SCOPEI("exclusive-test", "long-100ms", MP_WHEAT);
			spinsleep(250000);
		}
		{
			spinsleep(250000);
		}
	}
}

void ExclusiveTest()
{
	static int hest = 0;
	int NEGLAST = -1;
	while(!g_nQuit)
	{
		MICROPROFILE_SCOPEI("exclusive-test", "outer", MP_PINK);
		spinsleep(5000);
		int NEG = hest++ % 100 > 50?1:0; 
		if(NEG != NEGLAST)
		{
			NEGLAST = NEG;
		}
		{
			MICROPROFILE_SCOPEI("exclusive-test", "inner", MP_GREEN);
			spinsleep(2500);
			if(NEG)
			{
				MICROPROFILE_ENTER_NEGATIVE();
			}
			spinsleep(2500);
			if(NEG)
			{
				MICROPROFILE_LEAVE_NEGATIVE();
			}
			{
				MICROPROFILE_SCOPEI("exclusive-test-2", "inner-2", MP_CYAN);
				spinsleep(2500);
			}
			if(NEG)
			{
				MICROPROFILE_ENTER_NEGATIVE();
			}
			spinsleep(2500);
			if(NEG)
			{
				MICROPROFILE_LEAVE_NEGATIVE();
			}
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

	std::thread T(0?LongTest:ExclusiveTest);


	// std::thread T(ExclusiveTest);


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
			spinsleep(2000);
		}
		else
		{
			spinsleep(4000);
			MICROPROFILE_SCOPEI("exclusive-test", "stable1", MP_GREEN3);
			spinsleep(2000);
		}
		// spinsleep(2000);


// #ifdef _WIN32
// 		Sleep(10)
// #else
		spinsleep(30000);
// #endif
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
