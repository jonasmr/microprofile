#include "microprofile.h"
#include <unistd.h>

void C_Test()
{
	MICROPROFILE_ENTERI("C", "C_TEST", 0xff00ff);
	for(uint32_t i = 0; i < 20; ++i)
	{
		MICROPROFILE_ENTERI("C", "C_TEST_INNER", 0xff00ff);
		usleep(10);
		MICROPROFILE_LEAVE();
	}
	MICROPROFILE_LEAVE();
}
