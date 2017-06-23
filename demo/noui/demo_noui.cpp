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
#define TEST_MANY_GROUPS 0
#if TEST_MANY_GROUPS
void RegisterGroups()
{
	MICROPROFILE_REGISTER_GROUP("G0", "Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G1", "Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G2", "Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G3", "Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G4", "Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G5", "Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G6", "Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G7", "Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G8", "Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G9", "Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G10","Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G11","Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G12","Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G13","Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G14","Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G15","Cat0", MP_PINK);
	MICROPROFILE_REGISTER_GROUP("G16","Cat0", MP_PINK);

	MICROPROFILE_REGISTER_GROUP("G_1_0", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_1", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_2", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_3", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_4", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_5", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_6", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_7", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_8", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_9", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_10", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_11", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_12", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_13", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_14", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_15", "Cat_1", MP_CYAN);
	MICROPROFILE_REGISTER_GROUP("G_1_16", "Cat_1", MP_CYAN);

	MICROPROFILE_REGISTER_GROUP("G_2_0", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_1", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_2", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_3", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_4", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_5", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_6", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_7", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_8", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_9", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_10", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_11", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_12", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_13", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_14", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_15", "Cat_2", MP_YELLOW);
	MICROPROFILE_REGISTER_GROUP("G_2_16", "Cat_2", MP_YELLOW);

}

void t0()
{
	MICROPROFILE_SCOPEI("G0", "T0", MP_PINK);
	MICROPROFILE_SCOPEI("G1", "T1", MP_PINK);
	MICROPROFILE_SCOPEI("G2", "T2", MP_PINK);
	MICROPROFILE_SCOPEI("G3", "T3", MP_PINK);
	MICROPROFILE_SCOPEI("G4", "T4", MP_PINK);
	MICROPROFILE_SCOPEI("G5", "T5", MP_PINK);
	MICROPROFILE_SCOPEI("G6", "T6", MP_PINK);
	MICROPROFILE_SCOPEI("G7", "T7", MP_PINK);
	MICROPROFILE_SCOPEI("G8", "T8", MP_PINK);
	MICROPROFILE_SCOPEI("G9", "T9", MP_PINK);
	MICROPROFILE_SCOPEI("G10", "T10", MP_PINK);
	MICROPROFILE_SCOPEI("G11", "T11", MP_PINK);
	MICROPROFILE_SCOPEI("G12", "T12", MP_PINK);
	MICROPROFILE_SCOPEI("G13", "T13", MP_PINK);
	MICROPROFILE_SCOPEI("G14", "T14", MP_PINK);
	MICROPROFILE_SCOPEI("G15", "T15", MP_PINK);
	MICROPROFILE_SCOPEI("G16", "T16", MP_PINK);
	usleep(4000);
}



void t1()
{
	MICROPROFILE_SCOPEI("G_1_0", "T_1_0", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_1", "T_1_1", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_2", "T_1_2", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_3", "T_1_3", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_4", "T_1_4", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_5", "T_1_5", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_6", "T_1_6", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_7", "T_1_7", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_8", "T_1_8", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_9", "T_1_9", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_10", "T_1_10", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_11", "T_1_11", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_12", "T_1_12", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_13", "T_1_13", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_14", "T_1_14", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_15", "T_1_15", MP_CYAN);
	MICROPROFILE_SCOPEI("G_1_16", "T_1_16", MP_CYAN);
	usleep(4000);
}
void t2()
{
	MICROPROFILE_SCOPEI("G_2_0", "T_2_0", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_1", "T_2_1", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_2", "T_2_2", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_3", "T_2_3", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_4", "T_2_4", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_5", "T_2_5", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_6", "T_2_6", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_7", "T_2_7", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_8", "T_2_8", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_9", "T_2_9", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_10", "T_2_10", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_11", "T_2_11", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_12", "T_2_12", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_13", "T_2_13", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_14", "T_2_14", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_15", "T_2_15", MP_YELLOW);
	MICROPROFILE_SCOPEI("G_2_16", "T_2_16", MP_YELLOW);
	usleep(4000);
}
void t3()
{
	MICROPROFILE_SCOPEI("G_3_0", "T_3_0", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_1", "T_3_1", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_2", "T_3_2", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_3", "T_3_3", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_4", "T_3_4", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_5", "T_3_5", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_6", "T_3_6", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_7", "T_3_7", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_8", "T_3_8", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_9", "T_3_9", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_10", "T_3_10", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_11", "T_3_11", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_12", "T_3_12", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_13", "T_3_13", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_14", "T_3_14", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_15", "T_3_15", MP_BLUE);
	MICROPROFILE_SCOPEI("G_3_16", "T_3_16", MP_BLUE);
	usleep(4000);
}

#endif	


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

	MICROPROFILE_TIMELINE_TOKEN(htok_three);
	MICROPROFILE_TIMELINE_TOKEN(htok_four);
	MICROPROFILE_TIMELINE_TOKEN(htok_five);
	MICROPROFILE_TIMELINE_TOKEN(htok);
	MICROPROFILE_TIMELINE_TOKEN(htok2);

#if TEST_MANY_GROUPS
	RegisterGroups();
#endif

	StartFakeWork();
	while(!g_nQuit)
	{
		MICROPROFILE_SCOPE(MAIN);

#if TEST_MANY_GROUPS
		{
			t0();
			t1();
			t2();
			t3();
		}
#else
		usleep(16000);
#endif
		static int xx = 1;
		if(0 == (++xx % 1000))
		{
			MICROPROFILE_SCOPEI("geddehest", "fiszzk", 0xff00ff);
			usleep(201 * 1000);
		} 
		C_Test();

		static int hest = 0;
		hest++;
		MICROPROFILE_TIMELINE_LEAVE_STATIC("one");
		MICROPROFILE_TIMELINE_ENTER_STATIC(MP_DARKGOLDENROD, "one");
		if(0 == hest%4)
		{
			MICROPROFILE_TIMELINE_ENTER_STATIC(MP_DARKGOLDENROD, "two");
		}
		else if(2 == hest%4)
		{
			MICROPROFILE_TIMELINE_LEAVE_STATIC("Two");
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
