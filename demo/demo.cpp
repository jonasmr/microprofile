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
#include <SDL.h>
#include <string>
#include <thread>
#include <atomic>

#include "microprofile.h"
#include "glinc.h"

#ifdef main
#undef main
#endif

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


#define WIDTH 1024
#define HEIGHT 600

uint32_t g_nQuit = 0;
uint32_t g_MouseX = 0;
uint32_t g_MouseY = 0;
uint32_t g_MouseDown0 = 0;
uint32_t g_MouseDown1 = 0;
int g_MouseDelta = 0;


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
MICROPROFILE_DEFINE(MAIN, "MAIN", "Main", 0xff0000);

void WorkerThreadLong(int threadId)
{
	uint32_t c0 = 0xff3399ff;
	uint32_t c1 = 0xffff99ff;
	char name[100];
	snprintf(name, 99, "Worker_long%d", threadId);
	MicroProfileOnThreadCreate(&name[0]);
	while(!g_nQuit)
	{
		MICROPROFILE_SCOPEI("long", "outer 150ms", c0); 
		usleep(100*1000);
		for(int i = 0; i < 10; ++i)
		{
			MICROPROFILE_SCOPEI("long", "inner 5ms", c1); 
			usleep(5000);
		}
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
					usleep(200);
					{
						MICROPROFILE_SCOPEI("Thread2", "InnerWork0", c1); 
						usleep(100);
						{
							MICROPROFILE_SCOPEI("Thread2", "InnerWork1", c2); 
							usleep(100);
							{
								MICROPROFILE_SCOPEI("Thread2", "InnerWork2", c3); 
								usleep(100);
								{
									MICROPROFILE_SCOPEI("Thread2", "InnerWork3", c4); 
									usleep(100);
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
			
			MICROPROFILE_SCOPE(ThreadSafeMain);
			usleep(1000);;
			for(uint32_t i = 0; i < 5; ++i)
			{
				MICROPROFILE_SCOPE(ThreadSafeInner0);
				usleep(1000);
				for(uint32_t j = 0; j < 4; ++j)
				{
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
			break;
		}
	}
}


void HandleEvent(SDL_Event* pEvt)
{
	switch(pEvt->type)
	{
	case SDL_QUIT:
		g_nQuit = true;
		break;
	case SDL_KEYUP:
		if(pEvt->key.keysym.scancode == SDL_SCANCODE_ESCAPE)
		{
			g_nQuit = 1;
		}
		if(pEvt->key.keysym.sym == 'z')
		{
			MicroProfileToggleDisplayMode();
		}
		if(pEvt->key.keysym.scancode == SDL_SCANCODE_RSHIFT)
		{
			MicroProfileTogglePause();
		}
		if(pEvt->key.keysym.scancode == SDL_SCANCODE_LCTRL)
		{
			MicroProfileModKey(0);
		}
		if(pEvt->key.keysym.sym == 'a')
		{
			MicroProfileDumpTimers();
		}
		if(pEvt->key.keysym.sym == 'd')
		{
			MicroProfileDumpState();
		}
		break;


		break;
	case SDL_KEYDOWN:
		if(pEvt->key.keysym.scancode == SDL_SCANCODE_LCTRL)
		{
			MicroProfileModKey(1);
		}
		break;
	case SDL_MOUSEMOTION:
		g_MouseX = pEvt->motion.x;
		g_MouseY = pEvt->motion.y;
		break;
	case SDL_MOUSEBUTTONDOWN:
		if(pEvt->button.button == 1)
			g_MouseDown0 = 1;
		else if(pEvt->button.button == 3)
			g_MouseDown1 = 1;
		break;
	case SDL_MOUSEBUTTONUP:
		if(pEvt->button.button == 1)
		{
			g_MouseDown0 = 0;
		}
		else if(pEvt->button.button == 3)
		{
			g_MouseDown1 = 0;
		}
		break;
	case SDL_MOUSEWHEEL:
			g_MouseDelta -= pEvt->wheel.y;
		break;
	}



}

void MicroProfileQueryInitGL();
void MicroProfileDrawInit();
void MicroProfileBeginDraw(uint32_t nWidth, uint32_t nHeight, float* prj);
void MicroProfileEndDraw();

#ifdef _WIN32
#define __BREAK() __debugbreak()
#else
#define __BREAK() __builtin_trap()
#endif
int main(int argc, char* argv[])
{
	printf("press 'z' to toggle microprofile drawing\n");
	printf("press 'right shift' to pause microprofile update\n");
	MicroProfileOnThreadCreate("Main");

	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		return 1;
	}


	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,    	    8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,  	    8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,   	    8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,  	    8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  	    24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,  	    8);	
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,		    32);	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,	    1);	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetSwapInterval(1);

	SDL_Window * pWindow = SDL_CreateWindow("microprofiledemo", 10, 10, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
	if(!pWindow)
		return 1;

	SDL_GLContext glcontext = SDL_GL_CreateContext(pWindow);

	glewExperimental=1;
	GLenum err=glewInit();
	if(err!=GLEW_OK)
	{
		__BREAK();
	}
	glGetError(); //glew generates an error
		


#if MICROPROFILE_ENABLED
	MicroProfileQueryInitGL();
	MicroProfileDrawInit();
	MP_ASSERT(glGetError() == 0);
#endif
#define FAKE_WORK 1
#if FAKE_WORK
	std::thread t0(WorkerThread, 0);
	std::thread t1(WorkerThread, 1);
	std::thread t2(WorkerThread, 2);
	std::thread t3(WorkerThread, 3);
	std::thread t42(WorkerThread, 42);
	std::thread t43(WorkerThread, 43);
	std::thread t44(WorkerThread, 44);
	std::thread t45(WorkerThread, 45);
	std::thread tlong(WorkerThreadLong, 0);
#endif

	while(!g_nQuit)
	{
		MICROPROFILE_SCOPE(MAIN);

		SDL_Event Evt;
		while(SDL_PollEvent(&Evt))
		{
			HandleEvent(&Evt);
		}

		glClearColor(0.3f,0.4f,0.6f,0.f);
		glViewport(0, 0, WIDTH, HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


#if 1||FAKE_WORK
		{
			MICROPROFILE_SCOPEI("Main", "Dummy", 0xff3399ff);
			for(uint32_t i = 0; i < 14; ++i)
			{
				MICROPROFILE_SCOPEI("Main", "1ms", 0xff3399ff);
				MICROPROFILE_META_CPU("Sleep",1);

				usleep(1000);
			}
		}
#endif




		MicroProfileMouseButton(g_MouseDown0, g_MouseDown1);
		MicroProfileMousePosition(g_MouseX, g_MouseY, g_MouseDelta);
		g_MouseDelta = 0;


		MicroProfileFlip();
		{
			MICROPROFILE_SCOPEGPUI("GPU", "MicroProfileDraw", 0x88dd44);
			float projection[16];
			float left = 0.f;
			float right = WIDTH;
			float bottom = HEIGHT;
			float top = 0.f;
			float near = -1.f;
			float far = 1.f;
			memset(&projection[0], 0, sizeof(projection));

			projection[0] = 2.0f / (right - left);
			projection[5] = 2.0f / (top - bottom);
			projection[10] = -2.0f / (far - near);
			projection[12] = - (right + left) / (right - left);
			projection[13] = - (top + bottom) / (top - bottom);
			projection[14] = - (far + near) / (far - near);
			projection[15] = 1.f; 
 
 			glEnable(GL_BLEND);
 			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 			glDisable(GL_DEPTH_TEST);
#if MICROPROFILE_ENABLED
			MicroProfileBeginDraw(WIDTH, HEIGHT, &projection[0]);
			MicroProfileDraw(WIDTH, HEIGHT);
			MicroProfileEndDraw();
#endif
			glDisable(GL_BLEND);
		}

		MICROPROFILE_SCOPEI("MAIN", "Flip", 0xffee00);
		SDL_GL_SwapWindow(pWindow);
	}

	#if FAKE_WORK
	t0.join();
	t1.join();
	t2.join();
	t3.join();
	t42.join();
	t43.join();
	t44.join();
	t45.join();
	tlong.join();
	#endif
	MicroProfileShutdown();

  	SDL_GL_DeleteContext(glcontext);  
 	SDL_DestroyWindow(pWindow);
 	SDL_Quit();


	return 0;
}
