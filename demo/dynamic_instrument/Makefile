UNAME_S := $(shell uname -s)

LDFLAGS+=-lpthread

ifeq ($(UNAME_S),Linux)
	LDFLAGS+=-ldl -rdynamic
endif


CFLAGS=-Wno-c++11-extensions -I../..

CFLAGS+=-Wall -DMICROPROFILE_UI=0 -DMICROPROFILE_WEBSERVER=1 -DMICROPROFILE_GPU_TIMERS=0
CFLAGS+=-g -O0 -Wno-invalid-offsetof -Wall -Werror
CFLAGS+=-DMICROPROFILE_DYNAMIC_INSTRUMENT_ENABLE
CFLAGS+= -I../../distorm/include


CPPFLAGS=$(CFLAGS)
CPPFLAGS+=-std=c++14


CPP_SOURCES = demo_dynamic_instrument.cpp ../../microprofile.cpp
C_SOURCES =
ASM_SOURCES =


# Remember to update submodule and compile distorm
##CFLAGS+=-DMICROPROFILE_BREAK_ON_PATCH_FAIL=1
LDFLAGS+=../../distorm/distorm3.a
ASM_SOURCES+=../../patch_osx.s

TARGET=demo_dynamic_instrument
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

../../microprofile.o: ../../microprofile.cpp ../../microprofile.h \
  ../../microprofile_html.h

.s.o:
	$(CC) -c $< -o $@

.cpp.o:
	$(CPP) -c $< $(CPPFLAGS) -o $@

.c.o:
	$(CC) -c $< $(CFLAGS) -o $@


clean: depend
	rm -f *.o ../../*.o $(TARGET)

depend: $(CPP_SOURCES) $(C_SOURCES)
	$(CPP) -MM $(CPPFLAGS) $(CPP_SOURCES) >> .depend

embed_target:
	cd ../../src && make

embed: embed_target all



