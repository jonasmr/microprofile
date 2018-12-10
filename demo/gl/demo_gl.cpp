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

#include "microprofile.h"

MICROPROFILE_DEFINE_LOCAL_ATOMIC_COUNTER(ThreadSpinSleep, "/runtime/spin_sleep");


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
const char* g_PixelShaderCode = "\
#version 150 \n \
in vec2 TC0; \
in vec4 Color; \
out vec4 Out0; \
 \
void main(void)   \
{   \
	Out0 = Color;\
} \
";
	const char* g_VertexShaderCode = " \
#version 150 \n \
in vec3 VertexIn; \
in vec4 ColorIn; \
in vec2 TC0In; \
out vec2 TC0; \
out vec4 Color; \
 \
void main(void)   \
{ \
	Color = ColorIn; \
	TC0 = TC0In; \
	gl_Position = vec4(VertexIn, 1.0); \
} \
";
uint32_t g_VAO;
uint32_t g_VertexBuffer;
int 	g_LocVertexIn;
int 	g_LocColorIn;
int 	g_LocTC0In;
int 	g_LocTex;
int 	g_LocProjectionMatrix;
int 	g_LocfRcpFontHeight;
GLuint g_VertexShader;
GLuint g_PixelShader;
GLuint g_Program;
GLuint g_FontTexture;


#ifdef _WIN32
#define __BREAK() __debugbreak()
#else
#define __BREAK() __builtin_trap()
#endif

#ifndef MP_ASSERT
#define MP_ASSERT(x) do{if(!(x)){__BREAK();}}while(0)
#endif

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
		if(pEvt->key.keysym.sym == 'd')
		{
			MicroProfileDumpFile("../dump.html", "../dump.csv", -1, -1);
		}
	}



}

void CheckGlError()
{
	int r = glGetError();
	if(r != 0)
	{
		printf("error %d\n", r);
		__BREAK();
	}
}
void DumpGlLog(GLuint handle)
{
	return;
	int nLogLen = 0;
	MP_ASSERT(0 == glGetError());
	glGetObjectParameterivARB(handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &nLogLen);
	if(nLogLen > 1) 
	{
		char* pChars = (char*)malloc(nLogLen);
		int nLen = 0;
		glGetInfoLogARB(handle, nLogLen, &nLen, pChars);

		printf("COMPILE MESSAGE\n%s\n\n", pChars);

		free(pChars);
		__BREAK();
	}
	CheckGlError();
}

GLuint CreateProgram(int nType, const char* pShader)
{
	MP_ASSERT(0 == glGetError());
	GLuint handle = glCreateShaderObjectARB(nType);
	glShaderSource(handle, 1, (const char**)&pShader, 0);
	glCompileShader(handle);
	DumpGlLog(handle);
	MP_ASSERT(handle);	
	MP_ASSERT(0 == glGetError());
	return handle;
}
struct SDrawElement
{
	float x, y;
	uint32_t nColor;
};
#define QUADS 32

void InitGLBuffers()
{
	glGenBuffers(1, &g_VertexBuffer);
	glGenVertexArrays(1, &g_VAO);
	g_PixelShader = CreateProgram(GL_FRAGMENT_SHADER_ARB, g_PixelShaderCode);
	g_VertexShader = CreateProgram(GL_VERTEX_SHADER_ARB, g_VertexShaderCode);
	g_Program = glCreateProgramObjectARB();
	glAttachObjectARB(g_Program, g_PixelShader);
	glAttachObjectARB(g_Program, g_VertexShader);

	g_LocVertexIn = 1;
	g_LocColorIn = 2;

	glBindAttribLocation(g_Program, g_LocColorIn, "ColorIn");
	glBindAttribLocation(g_Program, g_LocVertexIn, "VertexIn");
	glLinkProgramARB(g_Program);
	DumpGlLog(g_Program);
	glBindVertexArray(g_VAO);
	float xx[6] = {-.5, -.5, 0.5, -.5, 0.5, 0.5};
	float yy[6] = {-0.5, 0.5,  -.5, 0.5, -.5, 0.5};
	SDrawElement D[QUADS*6];
	for(uint32_t i = 0; i < QUADS*6; ++i)
	{
		D[i].x = xx[i%6];
		D[i].y = yy[i%6];
		D[i].nColor = (uint32_t)-1;

	}

	glBindBuffer(GL_ARRAY_BUFFER, g_VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(D), &D[0], GL_STREAM_DRAW);
	int nStride = sizeof(SDrawElement);
	glVertexAttribPointer(g_LocVertexIn, 2, GL_FLOAT, 0 , nStride, 0);
	glVertexAttribPointer(g_LocColorIn, 4, GL_UNSIGNED_BYTE, GL_TRUE, nStride, (void*)(offsetof(SDrawElement, nColor)));
	glEnableVertexAttribArray(g_LocVertexIn);
	glEnableVertexAttribArray(g_LocColorIn);
	glBindVertexArray(0);

}


void DrawGLStuff()
{
	MICROPROFILE_SCOPEI("MicroProfile", "DRAWSTUFF", 0xffff3456);
	MICROPROFILE_SCOPEGPUI("DRAWSTUFF", 0xffff4444);
	glUseProgramObjectARB(g_Program);
	glBindVertexArray(g_VAO);
	static float f = 0;
	f += 0.01f;
	int amount =(int)( (((cos(f) + 1) * 0.5f) *10)+ 2);
	MP_ASSERT(amount > 0);
	for(int i = 0; i < amount; ++i)
	{
		MICROPROFILE_SCOPEGPUI("DRAWSTUFF_INNER", 0xffff4444);

		glDrawArrays(GL_TRIANGLES, 0, QUADS*6);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgramObjectARB(0);
	glBindVertexArray(0);

}

MICROPROFILE_DEFINE_LOCAL_ATOMIC_COUNTER(SDLFrameEvents, "/runtime/sdl_frame_events");
int main(int argc, char* argv[])
{


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
	InitGLBuffers();
#if MICROPROFILE_ENABLED
	MicroProfileGpuInitGL();
#endif
	SDL_GL_SetSwapInterval(1);
	while(!g_nQuit)
	{
		MICROPROFILE_SCOPE(MAIN);
		MICROPROFILE_COUNTER_ADD("engine/frames", 1);

		SDL_Event Evt;
		while(SDL_PollEvent(&Evt))
		{
			MICROPROFILE_COUNTER_LOCAL_ADD_ATOMIC(SDLFrameEvents, 1);
			HandleEvent(&Evt);
		}

		MICROPROFILE_COUNTER_LOCAL_UPDATE_SET_ATOMIC(SDLFrameEvents);
		glClearColor(0.3f,0.4f,0.6f,0.f);
		glViewport(0, 0, WIDTH, HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		DrawGLStuff();

		MicroProfileFlip(0);
		MICROPROFILE_SCOPEI("MAIN", "Flip", 0xffee00);
		SDL_GL_SwapWindow(pWindow);

		static bool bOnce = false;
		if(!bOnce)
		{
			bOnce = true;
			printf("open localhost:%d in chrome to capture profile data\n", MicroProfileWebServerPort());
		}
	
	}

	MicroProfileShutdown();
  	SDL_GL_DeleteContext(glcontext);  
 	SDL_DestroyWindow(pWindow);
 	SDL_Quit();


	return 0;
}
