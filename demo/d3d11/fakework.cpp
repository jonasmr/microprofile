#include <stdio.h>
#include <thread>
#include <atomic>
#include "microprofile.h"


#ifndef _WIN32
#include <unistd.h>
#endif

extern uint32_t g_nQuit;

#ifdef _WIN32
#undef near
#undef far
#define snprintf _snprintf
#include <windows.h>
void usleep(__int64 usec) 
{ 
	if(usec > 20000)
	{
		Sleep((DWORD)(usec/1000));
	}
	else if(usec >= 1000)
	{
		timeBeginPeriod(1);
		Sleep((DWORD)(usec/1000));
		timeEndPeriod(1);
	}
	else
	{
		__int64 time1 = 0, time2 = 0, freq = 0;
		QueryPerformanceCounter((LARGE_INTEGER *) &time1);
		QueryPerformanceFrequency((LARGE_INTEGER *)&freq);

		do {
			QueryPerformanceCounter((LARGE_INTEGER *) &time2);
		} while((time2-time1)*1000000ll/freq < usec);
	}
}
#endif


void spinsleep(int64_t nUs)
{
	MICROPROFILE_SCOPEI("spin","sleep", 0xffff);
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

MICROPROFILE_DECLARE(ThreadSafeMain);
MICROPROFILE_DECLARE(ThreadSafeInner0);
MICROPROFILE_DECLARE(ThreadSafeInner1);
MICROPROFILE_DECLARE(ThreadSafeInner2);
MICROPROFILE_DECLARE(ThreadSafeInner3);
MICROPROFILE_DECLARE(ThreadSafeInner4);
MICROPROFILE_DEFINE(ThreadSafeInner4,"ThreadSafe", "Inner4", 0xff00ff00);
MICROPROFILE_DEFINE(ThreadSafeInner3,"ThreadSafe", "Inner3", 0xff773744);
MICROPROFILE_DEFINE(ThreadSafeInner2,"ThreadSafe", "Inner2", 0xff990055);
MICROPROFILE_DEFINE(ThreadSafeInner1,"ThreadSafe", "Inner1", 0xffaa00aa);
MICROPROFILE_DEFINE(ThreadSafeInner0,"ThreadSafe", "Inner0", 0xff00bbee);
MICROPROFILE_DEFINE(ThreadSafeMain,"ThreadSafe", "Main", 0xffdd3355);



void WorkerThreadLong(int threadId)
{
	uint32_t c0 = 0xff3399ff;
	uint32_t c1 = 0xffff99ff;
	char name[100];
	snprintf(name, 99, "Worker_long%d", threadId);
	MicroProfileOnThreadCreate(&name[0]);
	while(!g_nQuit)
	{
		MICROPROFILE_ENTERI("long", "outer 150ms", c0);
		MICROPROFILE_META_CPU("Sleep",100);
		usleep(100*1000);
		for(int i = 0; i < 10; ++i)
		{
			MICROPROFILE_SCOPEI("long", "inner 5ms", c1); 
			MICROPROFILE_META_CPU("Sleep",5);
			usleep(5000);
		}

		MICROPROFILE_LEAVE();
	}
}

void WorkerThread(int threadId)
{
	char name[100];
	snprintf(name, 99, "Worker%d", threadId);
	MicroProfileOnThreadCreate(&name[0]);
	uint32_t c0 = 0xff3399ff;
	uint32_t c1 = 0xffff99ff;
	uint32_t c2 = 0xff33ff00;
	uint32_t c3 = 0xff3399ff;
	uint32_t c4 = 0xff33ff33;
	while(!g_nQuit)
	{
		switch(threadId)
		{
		case 0:
		{
			usleep(100);
			{
				MICROPROFILE_SCOPEI("Thread0", "Work Thread0", c4); 
				MICROPROFILE_META_CPU("Sleep",10);
				usleep(200);
				{
					MICROPROFILE_SCOPEI("Thread0", "Work Thread1", c3); 
					MICROPROFILE_META_CPU("DrawCalls", 1);
					MICROPROFILE_META_CPU("DrawCalls", 1);
					MICROPROFILE_META_CPU("DrawCalls", 1);
					usleep(200);
					{
						MICROPROFILE_SCOPEI("Thread0", "Work Thread2", c2); 
						MICROPROFILE_META_CPU("DrawCalls", 1);
						usleep(200);
						{
							MICROPROFILE_SCOPEI("Thread0", "Work Thread3", c1); 
							MICROPROFILE_META_CPU("DrawCalls", 4);
							MICROPROFILE_META_CPU("Triangles",1000);
							usleep(200);
						}
					}
				}
			}
		}
		break;
		
		case 1:
			{
				usleep(100);
				MICROPROFILE_SCOPEI("Thread1", "Work Thread 1", c1);
				usleep(2000);
			}
			break;

		case 2:
			{
				usleep(1000);
				{
					MICROPROFILE_SCOPEI("Thread2", "Worker2", c0); 
					spinsleep(100000);
					{
						MICROPROFILE_SCOPEI("Thread2", "InnerWork0", c1); 
						spinsleep(100);
						{
							MICROPROFILE_SCOPEI("Thread2", "InnerWork1", c2); 
							usleep(100);
							{
								MICROPROFILE_SCOPEI("Thread2", "InnerWork2", c3); 
								usleep(100);
								{
									MICROPROFILE_SCOPEI("Thread2", "InnerWork3", c4); 
									spinsleep(50000);
								}
							}
						}
					}
				}
			}
			break;
		case 3:
			{
				MICROPROFILE_SCOPEI("ThreadWork", "MAIN", c0);
				usleep(1000);;
				for(uint32_t i = 0; i < 10; ++i)
				{
					MICROPROFILE_SCOPEI("ThreadWork", "Inner0", c1);
					usleep(100);
					for(uint32_t j = 0; j < 4; ++j)
					{
						MICROPROFILE_SCOPEI("ThreadWork", "Inner1", c4);
						usleep(50);
						MICROPROFILE_SCOPEI("ThreadWork", "Inner2", c2);
						usleep(50);
						MICROPROFILE_SCOPEI("ThreadWork", "Inner3", c3);
						usleep(50);
						MICROPROFILE_SCOPEI("ThreadWork", "Inner4", c3);
						usleep(50);
					}
				}


			}
			break;
		default:
			
			MICROPROFILE_ENTER(ThreadSafeMain);
			usleep(1000);;
			for(uint32_t i = 0; i < 5; ++i)
			{
				MICROPROFILE_SCOPE(ThreadSafeInner0);
				usleep(1000);
				for(uint32_t j = 0; j < 4; ++j)
				{
					MICROPROFILE_META_CPU("custom_very_long_meta", 1);
					MICROPROFILE_SCOPE(ThreadSafeInner1);
					usleep(500);
					MICROPROFILE_SCOPE(ThreadSafeInner2);
					usleep(150);
					MICROPROFILE_SCOPE(ThreadSafeInner3);
					usleep(150);
					MICROPROFILE_SCOPE(ThreadSafeInner4);
					usleep(150);
				}
			}
			MICROPROFILE_LEAVE();
			break;
		}
	}
}

std::thread t0;
std::thread t1;
std::thread t2;
std::thread t3;
std::thread t42;
std::thread t43;
std::thread t44;
std::thread t45;
std::thread tlong;

void StartFakeWork()
{
	t0 = std::thread(WorkerThread, 0);
	t1 = std::thread(WorkerThread, 1);
	t2 = std::thread(WorkerThread, 2);
	t3 = std::thread(WorkerThread, 3);
	t42 = std::thread(WorkerThread, 42);
	t43 = std::thread(WorkerThread, 43);
	t44 = std::thread(WorkerThread, 44);
	t45 = std::thread(WorkerThread, 45);
	tlong = std::thread(WorkerThreadLong, 0);
}
void StopFakeWork()
{
	t0.join();
	t1.join();
	t2.join();
	t3.join();
	t42.join();
	t43.join();
	t44.join();
	t45.join();
	tlong.join();
}