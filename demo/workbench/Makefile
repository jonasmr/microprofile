UNAME_S := $(shell uname -s)
LDFLAGS+=-lpthread

ifeq ($(UNAME_S),Linux)
	LDFLAGS+=-ldl -rdynamic
endif

FLAGS=-I../..
#FLAGS+=-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
FLAGS+=-Wall -DMICROPROFILE_UI=0 -DMICROPROFILE_WEBSERVER=1 -DMICROPROFILE_GPU_TIMERS=0
FLAGS+=-g -O0 -Wall -Werror
#FLAGS+=-DMICROPROFILE_ENABLED=0
FLAGS+=-DMICROPROFILE_DYNAMIC_INSTRUMENT_ENABLE
FLAGS+= -I../../distorm/include
FLAGS+=-Wno-unused-function
#FLAGS+=-Wno-misleading-indentation 



FLAGS+=-Wno-unused-variable




CPPFLAGS= $(FLAGS) -std=c++11 -Wno-invalid-offsetof

CFLAGS = $(FLAGS) -std=c99



CPP_SOURCES = fakework.cpp demo_workbench.cpp ../../microprofile.cpp
C_SOURCES = test.c
ASM_SOURCES =


#comment in the following lines to enable function instrumentation.
#requires distorm to be present and compiled (IE distorm3.a must be present)
#CFLAGS+=-DMICROPROFILE_BREAK_ON_PATCH_FAIL=1
LDFLAGS+=../../distorm/distorm3.a
ASM_SOURCES+=../../patch_osx.s

TARGET=demo_workbench
CC = clang
CXX = clang++
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
	clang -c $< -o $@

.cpp.o:
	$(CXX) -c $< $(CPPFLAGS) -o $@

.c.o:
	$(CC) -c $< $(CFLAGS) -o $@


clean: depend
	rm -f *.o ../../*.o $(TARGET)

depend: $(CPP_SOURCES) $(C_SOURCES)
	$(CXX) -MM $(CPPFLAGS) $(CPP_SOURCES) >> .depend

embed_target:
	cd ../../src && make

embed: embed_target all



