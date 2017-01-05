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

#ifndef _WIN32
#include <unistd.h>
#endif

#include "microprofile.h"

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
			usleep(16000);
			MICROPROFILE_LEAVE();
		}
		static int xx = 1;
		if(0 == (++xx % 1000))
		{
			MICROPROFILE_ENTERI("geddehest", "fiszzk", 0xff00ff);
			usleep(201 * 1000);
			MICROPROFILE_LEAVE();
		} 

		// MICROPROFILE_COUNTER_LOCAL_ADD(LocalCounter, 3);
		// MICROPROFILE_COUNTER_LOCAL_SUB(LocalCounter, 1);
		MicroProfileFlip(0);
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
				usleep(1000*1000);	
				printf("sleep done, spike.html should be saved in 5 frames\n");
			}
		}
		#endif
		if(0 == (xx % 3000) && 0 != (xx % 1000))
		{
			MICROPROFILE_ENTERI("hest", "fisk", 0xff00ff);
			printf("sleep 500\n");
			usleep(75 * 1000);
			MICROPROFILE_LEAVE();
		}
		MICROPROFILE_LEAVE();
	}
	
	MicroProfileShutdown();

	return 0;
}
