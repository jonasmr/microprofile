LDFLAGS+=-lpthread
FLAGS=-I../..
#FLAGS+=-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
FLAGS+=-Wall -DMICROPROFILE_UI=0 -DMICROPROFILE_WEBSERVER=1 -DMICROPROFILE_GPU_TIMERS=0
FLAGS+=-g -O0 -Wall -Werror
FLAGS+=-Wno-unused-variable

CPPFLAGS= $(FLAGS) -std=c++14 -Wno-invalid-offsetof

CFLAGS = $(FLAGS) -std=c99 


CPP_SOURCES = microprofile.cpp ../../microprofile.cpp
C_SOURCES = demo.c

TARGET=demo_c
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
	

