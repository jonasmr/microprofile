#include "microprofile.h"
#ifndef _WIN32
#include <unistd.h>
#endif

void C_Test()
{
	MICROPROFILE_ENTERI("C", "C_TEST", 0xff00ff);
	for(uint32_t i = 0; i < 20; ++i)
	{
		MICROPROFILE_ENTERI("C", "C_TEST_INNER", 0xff00ff);
#ifndef _WIN32
		usleep(10);
#endif
		MICROPROFILE_LEAVE();
	}
	MICROPROFILE_LEAVE();
}
