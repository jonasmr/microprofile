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

#if defined(__APPLE__) || defined(__linux__)
#include <unistd.h>
#endif

#include "glinc.h"

#define MICROPROFILE_DEBUG 1
#define MICROPROFILE_WEBSERVER_MAXFRAMES 20
#define MICROPROFILE_IMPL
#define MICROPROFILE_GPU_TIMERS_GL 1
#include "microprofile.h"
#include "microprofileui.h"




#ifdef main
#undef main
#endif

#ifdef _WIN32
#undef near
#undef far
#define snprintf _snprintf
#include <windows.h>
void usleep(__int64 usec) ;
#endif


#define WIDTH 1024
#define HEIGHT 600

uint32_t g_nQuit = 0;
uint32_t g_MouseX = 0;
uint32_t g_MouseY = 0;
uint32_t g_MouseDown0 = 0;
uint32_t g_MouseDown1 = 0;
int g_MouseDelta = 0;

MICROPROFILE_DEFINE(MAIN, "MAIN", "Main", 0xff0000);



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
		if(pEvt->key.keysym.sym == 'x')
		{
			bool bForceEnable = MicroProfileGetForceEnable();
			MicroProfileSetForceEnable(!bForceEnable);
			printf("force enable is %d\n", !bForceEnable);
		}
		if(pEvt->key.keysym.sym == 'c')
		{
			bool bEnable = MicroProfileGetEnableAllGroups();
			MicroProfileSetEnableAllGroups(!bEnable);
			printf("enable all groups is %d\n", !bEnable);
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
#if MICROPROFILE_WEBSERVER
		if(pEvt->key.keysym.sym == 'd')
		{
			MicroProfileDumpFile("../dump.html", "../dump.csv");
		}
#endif
		if(pEvt->key.keysym.sym == 'l')
		{
			MicroProfileCustomGroupToggle("Custom1");
		}

		if(pEvt->key.keysym.sym == 't')
		{
			static bool toggle = false;
			if(toggle)
			{
				MicroProfileEnableCategory("ThreaDS");
			}
			else
			{
				MicroProfileDisableCategory("ThreaDS");
			}
			toggle = !toggle;
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

void MicroProfileDrawInit();
void MicroProfileBeginDraw(uint32_t nWidth, uint32_t nHeight, float* prj);
void MicroProfileEndDraw();

void StartFakeWork();
void StopFakeWork();


#ifdef _WIN32
#define __BREAK() __debugbreak()
#else
#define __BREAK() __builtin_trap()
#endif
int main(int argc, char* argv[])
{

	MICROPROFILE_REGISTER_GROUP("Thread0", "Threads", 0x88008800);
	MICROPROFILE_REGISTER_GROUP("Thread1", "Threads", 0x88008800);
	MICROPROFILE_REGISTER_GROUP("Thread2", "Threads", 0x88008800);
	MICROPROFILE_REGISTER_GROUP("Thread2xx", "Threads", 0x88008800);
	MICROPROFILE_REGISTER_GROUP("GPU", "main", 0x88fff00f);
	MICROPROFILE_REGISTER_GROUP("MAIN", "main", 0x88fff00f);

	printf("press 'z' to toggle microprofile drawing\n");
	printf("press 'right shift' to pause microprofile update\n");
	printf("press 'x' to toggle profiling\n");
	printf("press 'c' to toggle enable of all profiler groups\n");
	MicroProfileOnThreadCreate("AA_Main");
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
	MicroProfileGpuInitGL();
	MicroProfileDrawInit();
	MP_ASSERT(glGetError() == 0);
	MicroProfileToggleDisplayMode();
	
	MicroProfileInitUI();

	MicroProfileCustomGroup("Custom1", 2, 47, 2.f, MICROPROFILE_CUSTOM_BARS);
	MicroProfileCustomGroupAddTimer("Custom1", "MicroProfile", "Draw");
	MicroProfileCustomGroupAddTimer("Custom1", "MicroProfile", "Detailed View");

	MicroProfileCustomGroup("Custom2", 2, 100, 20.f, MICROPROFILE_CUSTOM_BARS|MICROPROFILE_CUSTOM_BAR_SOURCE_MAX);
	MicroProfileCustomGroupAddTimer("Custom2", "MicroProfile", "Draw");
	MicroProfileCustomGroupAddTimer("Custom2", "MicroProfile", "Detailed View");


	MicroProfileCustomGroup("Custom3", 2, 0, 5.f, MICROPROFILE_CUSTOM_STACK|MICROPROFILE_CUSTOM_STACK_SOURCE_MAX);
	MicroProfileCustomGroupAddTimer("Custom3", "MicroProfile", "Draw");
	MicroProfileCustomGroupAddTimer("Custom3", "MicroProfile", "Detailed View");



	MicroProfileCustomGroup("ThreadSafe", 6, 10, 600.f, MICROPROFILE_CUSTOM_BARS | MICROPROFILE_CUSTOM_STACK);
	MicroProfileCustomGroupAddTimer("ThreadSafe", "ThreadSafe", "main");
	MicroProfileCustomGroupAddTimer("ThreadSafe", "ThreadSafe", "inner0");
	MicroProfileCustomGroupAddTimer("ThreadSafe", "ThreadSafe", "inner1");
	MicroProfileCustomGroupAddTimer("ThreadSafe", "ThreadSafe", "inner2");
	MicroProfileCustomGroupAddTimer("ThreadSafe", "ThreadSafe", "inner3");
	MicroProfileCustomGroupAddTimer("ThreadSafe", "ThreadSafe", "inner4");
#endif
	MICROPROFILE_COUNTER_CONFIG("memory/main", MICROPROFILE_COUNTER_FORMAT_BYTES, 10ll<<30ll);
	MICROPROFILE_COUNTER_CONFIG("memory/gpu/indexbuffers", MICROPROFILE_COUNTER_FORMAT_BYTES, 0);
	MICROPROFILE_COUNTER_CONFIG("memory/gpu/vertexbuffers", MICROPROFILE_COUNTER_FORMAT_BYTES, 0);
	MICROPROFILE_COUNTER_CONFIG("memory/mainx", MICROPROFILE_COUNTER_FORMAT_BYTES, 10000);

	MICROPROFILE_COUNTER_ADD("memory/main", 1000);
	MICROPROFILE_COUNTER_ADD("memory/gpu/vertexbuffers", 1000);
	MICROPROFILE_COUNTER_ADD("memory/gpu/indexbuffers", 200);
	MICROPROFILE_COUNTER_ADD("memory//", 10<<10);
	MICROPROFILE_COUNTER_ADD("memory//main", (32ll<<30ll) + (1ll <<29ll));
	MICROPROFILE_COUNTER_ADD("//memory//mainx/\\//", 1000);
	MICROPROFILE_COUNTER_ADD("//memoryx//mainx/", 1000);
	MICROPROFILE_COUNTER_ADD("//memoryy//main/", 1000);
	MICROPROFILE_COUNTER_ADD("//\\\\///lala////lelel", 1000);
	MICROPROFILE_COUNTER_CONFIG("engine/frames", MICROPROFILE_COUNTER_FORMAT_DEFAULT, 1000ll);
	//MICROPROFILE_COUNTER_ADD("//\\\\///", 1000); // this should assert as theres only delimiters
	StartFakeWork();
	while(!g_nQuit)
	{
		MICROPROFILE_SCOPE(MAIN);
		MICROPROFILE_COUNTER_ADD("engine/frames", 1);

		SDL_Event Evt;
		while(SDL_PollEvent(&Evt))
		{
			MICROPROFILE_COUNTER_ADD("engine/sdl_events", 1);
			HandleEvent(&Evt);
		}

		glClearColor(0.3f,0.4f,0.6f,0.f);
		glViewport(0, 0, WIDTH, HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


#if 1||FAKE_WORK
		{
			MICROPROFILE_SCOPEI("BMain", "Dummy", 0xff3399ff);
			for(uint32_t i = 0; i < 14; ++i)
			{
				MICROPROFILE_SCOPEI("BMain", "1ms", 0xff3399ff);
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
			MICROPROFILE_SCOPEGPUI("MicroProfileDraw", 0x88dd44);
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
	StopFakeWork();

	MicroProfileShutdown();

  	SDL_GL_DeleteContext(glcontext);  
 	SDL_DestroyWindow(pWindow);
 	SDL_Quit();


	return 0;
}
