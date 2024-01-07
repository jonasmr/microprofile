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

#ifdef _WIN32
void usleep(__int64 s);
#else
#include "unistd.h"
#endif

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
	MicroProfileDumpFileImmediately("2sec", "2sec", nullptr);


}

int main(int argc, char* argv[])
{
	MicroProfileOnThreadCreate("Main");
	MicroProfileSetEnableAllGroups(true);
	MicroProfileSetForceMetaCounters(true);
	MicroProfileEnableFrameExtraCounterData();

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
