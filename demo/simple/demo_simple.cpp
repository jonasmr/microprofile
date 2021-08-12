// MIT License
//
// Copyright (c) 2019 Jonas Meyer
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
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

//generate zipped results
#define MICROPROFILE_MINIZ 1
#if MICROPROFILE_MINIZ
#include "miniz.c"
#endif



#define MICROPROFILE_MAX_FRAME_HISTORY (2<<10)
#define MICROPROFILE_IMPL
#include "microprofile.h"

MICROPROFILE_DEFINE(MAIN, "MAIN", "Main", MP_AUTO);

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

void ManyScopes(int c)
{
	MICROPROFILE_SCOPEI("many", "scopes", 0);
	if(c > 0)
	{
		{
			spinsleep(40);
		}
		ManyScopes(c-1);
	}
}

void four_sec()
{
	MicroProfileOnThreadCreate("four_sec");
	for(int i = 0; i < 4; ++i)
	{
		for(int j = 0; j < 100; ++j)
		{
			MICROPROFILE_SCOPEI("four_sec", "four", MP_YELLOW);
			spinsleep(10000);
		}
		printf("step %d\n", i);
	}
	MicroProfileOnThreadExit();
}
int main(int argc, char* argv[])
{
	MicroProfileOnThreadCreate("Main");
	printf("press ctrl-c to quit\n");

	//turn on profiling
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);

#define AUTO_FLIP 1


	#if AUTO_FLIP
	MicroProfileStartAutoFlip(30);
	#endif


	while(!g_nQuit)
	{
		MICROPROFILE_SCOPE(MAIN);
		{
			spinsleep(2000);
			ManyScopes(68);
		}
		static int x = 0;
		if(x++ % 20 < 10)
		{
			MICROPROFILE_SCOPEI("exclusive-test", "stable-hest-1", 0);
			spinsleep(2000);
		}
		else
		{
			spinsleep(4000);
			MICROPROFILE_SCOPEI("exclusive-test", "slut-prut", MP_AUTO);
			spinsleep(2000);
		}

		spinsleep(30000);
		#if 0 == AUTO_FLIP
		MicroProfileFlip(0);
		#endif

		static bool once = false;
		if(!once)
		{
			once = 1;
			printf("open localhost:%d in chrome to capture profile data\n", MicroProfileWebServerPort());
		}
	}
	#if AUTO_FLIP
	MicroProfileStopAutoFlip();
	#endif
	MicroProfileShutdown();

	return 0;
}
