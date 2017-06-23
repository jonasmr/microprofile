UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LDFLAGS=-lGL `sdl2-config --static-libs` -lpthread
endif
ifeq ($(UNAME_S),Darwin)
	LDFLAGS=-framework OpenGL `sdl2-config --static-libs` -lpthread
endif

CFLAGS=`sdl2-config --cflags` -I../.. -I../glew -DGLEW_STATIC -Wno-c++11-extensions -Wall -Werror -I. -Wno-deprecated-declarations
CFLAGS+=-Wall -DMICROPROFILE_USE_CONFIG 
CFLAGS+=-DMICROPROFILE_GPU_TIMERS_MULTITHREADED=1

CFLAGS+=-g -O0 -Wno-invalid-offsetof

CPPFLAGS=$(CFLAGS)
CPPFLAGS+=-stdlib=libc++ -std=c++11


CPP_SOURCES = demo_gl.cpp ../../microprofile.cpp
C_SOURCES = ../glew/glew.c

TARGET=demo_gl
CC=clang
CPP=clang++
LD=clang++

CPP_OBJS = $(patsubst %.cpp,%.o,$(CPP_SOURCES))
C_OBJS = $(patsubst %.c,%.o,$(C_SOURCES))

all: $(TARGET)

$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(LD) -o $(TARGET) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) $(CPPFLAGS)

-include .depend

.cpp.o: 
	$(CPP) -c $< $(CPPFLAGS) -o $@

.c.o:
	$(CC) -c $< $(CFLAGS) -o $@


clean: depend
	rm *.o $(TARGET)

depend: $(CPP_SOURCES) $(C_SOURCES)
	$(CC) -MM $(CFLAGS) $(C_SOURCES) > .depend
	$(CPP) -MM $(CPPFLAGS) $(CPP_SOURCES) >> .depend

embed_target:
	cd ../../src && make

embed: embed_target all
	

