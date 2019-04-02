// MIT License

// Copyright (c) 2019 Jonas Meyer

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ***********************************************************************

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
