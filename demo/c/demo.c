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
#include "microprofile.h"

void spinsleep(int64_t nUs)
{

#if MICROPROFILE_ENABLED
	float fToMs = MicroProfileTickToMsMultiplierCpu();
	int64_t nTickStart = MicroProfileTick();
	float fElapsed = 0;
	float fTarget = nUs / 1000000.f;
	do
	{
		int64_t nTickEnd = MicroProfileTick();
		fElapsed = (nTickEnd - nTickStart) * fToMs;

	}while(fElapsed < fTarget);
#endif
}



int g_nQuit = 0;
int main(int argc, char* argv[])
{
	MicroProfileOnThreadCreate("Main");
	printf("press ctrl-c to quit\n");

	//turn on profiling
	MicroProfileSetEnableAllGroups(1);
	MicroProfileSetForceMetaCounters(1);
	MicroProfileStartContextSwitchTrace();
#if 0 //not yet implemented.
	MICROPROFILE_COUNTER_CONFIG("/runtime/localcounter", MICROPROFILE_COUNTER_FORMAT_BYTES, 500, 0);

	MICROPROFILE_COUNTER_ADD("memory/main", 1000);
	MICROPROFILE_COUNTER_ADD("memory/gpu/vertexbuffers", 1000);
	MICROPROFILE_COUNTER_ADD("memory/gpu/indexbuffersxsxsxsxsxsxsxxsxsxsxs", 200);
	MICROPROFILE_COUNTER_ADD("memory//main", 1000);
	MICROPROFILE_COUNTER_ADD("memory//", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//mainx/\\//", 1000);
	MICROPROFILE_COUNTER_ADD("//memoryx//mainx/", 1000);
	MICROPROFILE_COUNTER_ADD("//memoryy//main/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main0/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main1/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main2/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main3/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main4/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main5/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main6/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main7/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main8/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main9/", 1000);
	MICROPROFILE_COUNTER_ADD("//\\\\///lala////lelel", 1000);
#endif


	#if DUMP_SPIKE_TEST
	MicroProfileDumpFile("spike.html", "spike.csv", 200.f, -1.f);
	#endif	

	while(!g_nQuit)
	{
		MICROPROFILE_ENTERI("Main", "Main", 0xff0000ff);
		{
			MICROPROFILE_ENTERI("Main", "Sleep", 0xff0000ff);
			spinsleep(16000);
			MICROPROFILE_LEAVE();
		}
		static int xx = 1;
		if(0 == (++xx % 1000))
		{
			MICROPROFILE_ENTERI("geddehest", "fiszzk", 0xff00ff);
			spinsleep(201 * 1000);
			MICROPROFILE_LEAVE();
		} 

		// MICROPROFILE_COUNTER_LOCAL_ADD(LocalCounter, 3);
		// MICROPROFILE_COUNTER_LOCAL_SUB(LocalCounter, 1);
		MicroProfileFlip(0, MICROPROFILE_FLIP_FLAG_DEFAULT);
		static int once = 0;
		if(!once)
		{
			once = 1;
			printf("open localhost:%d in chrome to capture profile data\n", MicroProfileWebServerPort());
		}

		#if DUMP_SPIKE_TEST
		static int nCounter = 0;
		if(nCounter < 200)
		{
			printf("\r%5d/200", nCounter++);
			fflush(stdout);
			if(nCounter == 200)
			{
				printf("\nsleeping 1s\n");
				MICROPROFILE_SCOPEI("SPIKE_TEST", "Test", 0xff00ff00);
				spinsleep(1000*1000);	
				printf("sleep done, spike.html should be saved in 5 frames\n");
			}
		}
		#endif
		if(0 == (xx % 3000) && 0 != (xx % 1000))
		{
			MICROPROFILE_ENTERI("hest", "fisk", 0xff00ff);
			printf("sleep 500\n");
			spinsleep(75 * 1000);
			MICROPROFILE_LEAVE();
		}
		MICROPROFILE_LEAVE();
	}
	
	MicroProfileShutdown();

	return 0;
}
