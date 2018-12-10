LDFLAGS+=-lpthread
FLAGS=-I../..
#FLAGS+=-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
FLAGS+=-Wall -DMICROPROFILE_UI=0 -DMICROPROFILE_WEBSERVER=1 -DMICROPROFILE_GPU_TIMERS=0
FLAGS+=-g -O0 -Wno-invalid-offsetof -Wall -Werror
#FLAGS+=-DMICROPROFILE_ENABLED=0

CPPFLAGS= $(FLAGS) -std=c++11
CFLAGS = $(FLAGS) -std=c99 



CPP_SOURCES = fakework.cpp demo_noui.cpp ../../microprofile.cpp
C_SOURCES = test.c
ASM_SOURCES = 


#comment in the following lines to enable function instrumentation.
#requires distorm to be present and compiled (IE distorm3.a must be present)
CFLAGS+=-DMICROPROFILE_DYNAMIC_INSTRUMENT_ENABLE
#CFLAGS+=-DMICROPROFILE_BREAK_ON_PATCH_FAIL=1
LDFLAGS+=../../distorm/distorm3.a
ASM_SOURCES+=../../patch_osx.s
CFLAGS+= -I../../distorm/include

TARGET=demo_noui
CC ?= clang
CXX ?= clang++
LD = $(CXX)

ASM_OBJS = $(patsubst %.s,%.o,$(ASM_SOURCES))
CPP_OBJS = $(patsubst %.cpp,%.o,$(CPP_SOURCES))
C_OBJS = $(patsubst %.c,%.o,$(C_SOURCES))

all: $(TARGET)

$(TARGET): $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS)
	$(LD) -o $(TARGET) $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS) $(LDFLAGS) $(CPPFLAGS)

-include .depend

../../microprofile.o: ../../microprofile.cpp ../../microprofile.h \
  ../../microprofile_html.h

.s.o:
	as -masm=intel $< -o $@

.cpp.o: 
	$(CXX) -c $< $(CPPFLAGS) -o $@

.c.o:
	$(CC) -c $< $(CFLAGS) -o $@


clean: depend
	rm -f *.o ../../microprofile.o $(TARGET)

depend: $(CPP_SOURCES) $(C_SOURCES)
	$(CXX) -MM $(CPPFLAGS) $(CPP_SOURCES) >> .depend

embed_target:
	cd ../../src && make

embed: embed_target all

	

