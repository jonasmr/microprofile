LDFLAGS+=-lpthread
CFLAGS=-Wno-c++11-extensions -I../..
#CFLAGS+=-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
CFLAGS+=-Wall -DMICROPROFILE_UI=0 -DMICROPROFILE_WEBSERVER=1 -DMICROPROFILE_GPU_TIMERS=0
CFLAGS+=-g -O0 -Wno-invalid-offsetof -Wall -Werror

#CFLAGS+=-DMICROPROFILE_ENABLED=0
CPPFLAGS=$(CFLAGS)
CPPFLAGS+=-stdlib=libc++ -std=c++11


CFLAGS+=-DMICROPROFILE_DYNAMIC_INSTRUMENT_ENABLE

LDFLAGS+=../../distorm/distorm3.a
ASM_SOURCES+=../../patch_osx.s
CFLAGS+= -I../../distorm/include



CPP_SOURCES = microprofile.cpp ../../microprofile.cpp
C_SOURCES = demo_noui.c

TARGET=demo_noui_c
CC=clang
CPP=clang++
LD=clang++

ASM_OBJS = $(patsubst %.s,%.o,$(ASM_SOURCES))
CPP_OBJS = $(patsubst %.cpp,%.o,$(CPP_SOURCES))
C_OBJS = $(patsubst %.c,%.o,$(C_SOURCES))

all: $(TARGET)

$(TARGET): $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS)
	$(LD) -o $(TARGET) $(C_OBJS) $(CPP_OBJS) $(ASM_OBJS) $(LDFLAGS) $(CPPFLAGS)

-include .depend


.s.o:
	as -masm=intel $< -o $@

.cpp.o: 
	$(CPP) -c $< $(CPPFLAGS) -o $@

.c.o:
	$(CC) -c $< $(CFLAGS) -o $@


clean: depend
	rm -f *.o ../../microprofile.o $(TARGET)

depend: $(CPP_SOURCES) $(C_SOURCES)
	$(CPP) -MM $(CPPFLAGS) $(CPP_SOURCES) >> .depend

embed_target:
	cd ../../src && make

embed: embed_target all
	

