LDFLAGS+=-lpthread
FLAGS=-I../..
#FLAGS+=-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
FLAGS+=-Wall -DMICROPROFILE_UI=0 -DMICROPROFILE_WEBSERVER=1 -DMICROPROFILE_GPU_TIMERS=0
FLAGS+=-g -O0 -Wno-invalid-offsetof -Wall -Werror

CPPFLAGS= $(FLAGS) -std=c++11
CFLAGS = $(FLAGS) -std=c99 



CPP_SOURCES = microprofile.cpp ../../microprofile.cpp
C_SOURCES = demo_noui.c

TARGET=demo_noui_c
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
	rm -f *.o ../../microprofile.o $(TARGET)

depend: $(CPP_SOURCES) $(C_SOURCES)
	$(CXX) -MM $(CPPFLAGS) $(CPP_SOURCES) >> .depend

embed_target:
	cd ../../src && make

embed: embed_target all
	

