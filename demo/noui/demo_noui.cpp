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

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

#define MICRO_PROFILE_IMPL
#define MICROPROFILE_UI 0
#include "microprofile.h"

MICROPROFILE_DEFINE(MAIN, "MAIN", "Main", 0xff0000);

uint32_t g_nQuit = 0;

void StartFakeWork();
void StopFakeWork();


int main(int argc, char* argv[])
{
	printf("open localhost:1338 in chrome to capture profile data\n");
	printf("press ctrl-c to quit\n");
	MicroProfileOnThreadCreate("Main");

	//turn on profiling
	MicroProfileSetForceEnable(true);
	MicroProfileSetEnableAllGroups(true);

	StartFakeWork();
	while(!g_nQuit)
	{
		MICROPROFILE_SCOPE(MAIN);
		{
			MICROPROFILE_SCOPEI("Main", "Dummy", 0xff3399ff);
			usleep(16000);
		}
		MicroProfileFlip();

	}

	StopFakeWork();
	
	MicroProfileShutdown();

	return 0;
}
