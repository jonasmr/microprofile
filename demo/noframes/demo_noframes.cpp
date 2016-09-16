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
#include "unistd.h"


#include "microprofile.h"

uint32_t g_nQuit;
void StartFakeWork();
void StopFakeWork();

void Run2Sec()
{
	uint64_t nStart = MicroProfileTick();
	uint64_t nTicksPerSecond = MicroProfileTicksPerSecondCpu();
	float fTime = 0;
	do
	{
		MICROPROFILE_SCOPEI("spin", "HEST", -1);		
		for(int j = 0; j < 10; ++j)
		{
			MICROPROFILE_SCOPEI("spin", "FISK  inner0", MP_CYAN);
			usleep(250);
		}
		for(int k = 0; k < 10; ++k)
		{
			MICROPROFILE_SCOPEI("spin", "GED inner0", MP_DARKGOLDENROD);
			usleep(250);
		}
		usleep(3000);
		uint64_t nEnd = MicroProfileTick();
		fTime = (nEnd - nStart) / (float)nTicksPerSecond;
		printf("\r%4.2f", fTime);		
	}while(fTime < 2);
	printf("\n");
	MicroProfileDumpFileImmediately("2sec.html", "2sec.csv", nullptr);


}

int main(int argc, char* argv[])
{
	MicroProfileOnThreadCreate("Main");
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);

	printf("running 2 sec test\n");
	//StartFakeWork();
	Run2Sec();
	g_nQuit = 1;
	//StopFakeWork();
	
	MicroProfileShutdown();

	return 0;
}

MICROPROFILE_DECLARE_LOCAL_ATOMIC_COUNTER(ThreadsStarted);
MICROPROFILE_DEFINE_LOCAL_ATOMIC_COUNTER(ThreadSpinSleep, "/runtime/spin_sleep");
MICROPROFILE_DECLARE_LOCAL_COUNTER(LocalCounter);
MICROPROFILE_DEFINE_LOCAL_COUNTER(LocalCounter, "/runtime/localcounter");
