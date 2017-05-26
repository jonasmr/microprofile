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

#ifdef _WIN32
void usleep(__int64);
#endif
uint32_t g_nQuit = 0;

void StartFakeWork();
void StopFakeWork();

extern "C" void C_Test();

#define DUMP_SPIKE_TEST 0

MICROPROFILE_DECLARE_LOCAL_ATOMIC_COUNTER(ThreadsStarted);
MICROPROFILE_DEFINE_LOCAL_ATOMIC_COUNTER(ThreadSpinSleep, "/runtime/spin_sleep");
MICROPROFILE_DECLARE_LOCAL_COUNTER(LocalCounter);
int main(int argc, char* argv[])
{
	MicroProfileOnThreadCreate("Main");
	printf("press ctrl-c to quit\n");

	//turn on profiling
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);

	MicroProfileStartContextSwitchTrace();

	MICROPROFILE_COUNTER_CONFIG("/runtime/localcounter", MICROPROFILE_COUNTER_FORMAT_BYTES, 10000, 0);

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
	MICROPROFILE_COUNTER_ADD("//memory//main5/", 0xff0000000000ll);
	MICROPROFILE_COUNTER_ADD("//memory//main6/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main7/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main8/", 1000);
	MICROPROFILE_COUNTER_ADD("//memory//main9/", 1000);
	MICROPROFILE_COUNTER_ADD("//\\\\///lala////lelel", 1000);

	MICROPROFILE_COUNTER_SET("fisk/geder/", 42);
	MICROPROFILE_COUNTER_SET("fisk/aborre/", -2002);
	MICROPROFILE_COUNTER_SET_LIMIT("fisk/aborre/", 120);

	MICROPROFILE_COUNTER_CONFIG_ONCE("/test/sinus", MICROPROFILE_COUNTER_FORMAT_BYTES, 0, MICROPROFILE_COUNTER_FLAG_DETAILED);
	MICROPROFILE_COUNTER_CONFIG("/test/cosinus", MICROPROFILE_COUNTER_FORMAT_DEFAULT, 0, MICROPROFILE_COUNTER_FLAG_DETAILED);


	#if DUMP_SPIKE_TEST
	MicroProfileDumpFile("spike.html", "spike.csv", 200.f, -1.f);
	#endif	

	MICROPROFILE_TIMELINE_TOKEN(htok_one);
	MICROPROFILE_TIMELINE_TOKEN(htok_two);
	MICROPROFILE_TIMELINE_TOKEN(htok_three);
	MICROPROFILE_TIMELINE_TOKEN(htok_four);
	MICROPROFILE_TIMELINE_TOKEN(htok_five);
	MICROPROFILE_TIMELINE_TOKEN(htok);
	MICROPROFILE_TIMELINE_TOKEN(htok2);

	StartFakeWork();
	while(!g_nQuit)
	{
		MICROPROFILE_SCOPE(MAIN);
		{
			usleep(16000);
		}
		static int xx = 1;
		if(0 == (++xx % 1000))
		{
			MICROPROFILE_SCOPEI("geddehest", "fiszzk", 0xff00ff);
			usleep(201 * 1000);
		} 
		C_Test();

		static int hest = 0;
		hest++;
		MICROPROFILE_TIMELINE_LEAVE(htok_one);
		MICROPROFILE_TIMELINE_ENTERF(htok_one, MP_DARKGOLDENROD, "one");
		if(0 == hest%4)
		{
			MICROPROFILE_TIMELINE_ENTERF(htok_two, MP_DARKGOLDENROD, "two");
		}
		else if(2 == hest%4)
		{
			MICROPROFILE_TIMELINE_LEAVE(htok_two);
		}

		if(0 == hest%12)
		{
			MICROPROFILE_TIMELINE_ENTERF(htok_three, MP_YELLOW, "three %d", hest);
		}
		else if(10 == hest%12)
		{
			MICROPROFILE_TIMELINE_LEAVE(htok_three);
		}
		if(1 == hest%8)
		{
			MICROPROFILE_TIMELINE_ENTERF(htok_four, MP_YELLOW, "four %d", hest);
		}
		else if(7 == hest%8)
		{
			MICROPROFILE_TIMELINE_LEAVE(htok_four);
		}
		if(2 == hest%7)
		{
			MICROPROFILE_TIMELINE_ENTERF(htok_five, MP_RED, "five %d", hest);
		}
		else if(5 == hest%7)
		{
			MICROPROFILE_TIMELINE_LEAVE(htok_five);
		}

		if(1 == (hest%5))
		{
			MICROPROFILE_TIMELINE_ENTERF(htok, MP_PINK3, "hest %s %d", "ged", hest);
			MICROPROFILE_TIMELINE_ENTERF(htok2, MP_CYAN, "CyAN", 1);
		}
		else if(3 == (hest%5))
		{
			MICROPROFILE_TIMELINE_LEAVE(htok);
			MICROPROFILE_TIMELINE_LEAVE(htok2);
		}
		MICROPROFILE_COUNTER_LOCAL_ADD(LocalCounter, 3);
		MICROPROFILE_COUNTER_LOCAL_SUB(LocalCounter, 1);
		MicroProfileFlip(0);
		static bool once = false;
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
				usleep(1000*1000);	
				printf("sleep done, spike.html should be saved in 5 frames\n");
			}
		}
		#endif
		if(0 == (xx % 3000) && 0 != (xx % 1000))
		{
			MICROPROFILE_SCOPEI("hest", "fisk", 0xff00ff);
			printf("sleep 500\n");
			usleep(75 * 1000);
		}

		MICROPROFILE_COUNTER_LOCAL_UPDATE_ADD_ATOMIC(ThreadsStarted);
		MICROPROFILE_COUNTER_LOCAL_UPDATE_SET_ATOMIC(ThreadSpinSleep);
		MICROPROFILE_COUNTER_LOCAL_UPDATE_ADD(LocalCounter);

		static float f = 0;
		f += 0.1f;
		int sinus = (int)(10000000 * (sinf(f)));
		int cosinus = int(cosf(f*1.3f) * 100000 + 50000);
		MICROPROFILE_COUNTER_SET("/test/sinus", sinus);
		MICROPROFILE_COUNTER_SET("/test/cosinus", cosinus);

		

	}

	StopFakeWork();
	
	MicroProfileShutdown();

	return 0;
}

MICROPROFILE_DEFINE_LOCAL_COUNTER(LocalCounter, "/runtime/localcounter");
