UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	LDFLAGS=-lGL `sdl2-config --static-libs` -lpthread
endif
ifeq ($(UNAME_S),Darwin)
	LDFLAGS=-framework OpenGL `sdl2-config --static-libs` -lpthread
endif

CFLAGS=`sdl2-config --cflags` -I../.. -I../glew -DGLEW_STATIC -Wall -Werror -I. -Wno-deprecated-declarations
CFLAGS+=-Wall -DMICROPROFILE_USE_CONFIG -Wno-format-truncation
CFLAGS+=-DMICROPROFILE_GPU_TIMERS_MULTITHREADED=1

CFLAGS+=-g -O0 

CPPFLAGS=$(CFLAGS)
#CPPFLAGS+=-Wno-c++11-extensions
#CPPFLAGS+=-stdlib=libc++ -std=c++11 -Wno-invalid-offsetof


CPP_SOURCES = demo_gl.cpp ../../microprofile.cpp
C_SOURCES = ../glew/glew.c

TARGET=demo_gl
CC ?= clang
CXX ?= clang++
LD = $(CXX)

CPP_OBJS = $(patsubst %.cpp,%.o,$(CPP_SOURCES))
C_OBJS = $(patsubst %.c,%.o,$(C_SOURCES))

all: $(TARGET)

$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(LD) -o $(TARGET) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) $(CPPFLAGS)

-include .depend

.cpp.o: 
	$(CXX) -c $< $(CPPFLAGS) -o $@

.c.o:
	$(CC) -c $< $(CFLAGS) -o $@


clean: depend
	rm -f *.o ../../*.o ../glew/*.o $(TARGET)

depend: $(CPP_SOURCES) $(C_SOURCES)
	$(CC) -MM $(CFLAGS) $(C_SOURCES) > .depend
	$(CXX) -MM $(CPPFLAGS) $(CPP_SOURCES) >> .depend

embed_target:
	cd ../../src && make

embed: embed_target all
	

