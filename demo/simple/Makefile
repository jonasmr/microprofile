LDFLAGS+=-lpthread
CFLAGS=-I../..
#CFLAGS+=-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
CFLAGS+=-Wall -DMICROPROFILE_UI=0 -DMICROPROFILE_WEBSERVER=1 -DMICROPROFILE_GPU_TIMERS=0
CFLAGS+=-g -O0 -Wno-invalid-offsetof -Wall -Werror
CFLAGS+=-Wno-unused-variable

#CFLAGS+=-DMICROPROFILE_ENABLED=0
CPPFLAGS=$(CFLAGS)
CPPFLAGS+=-std=c++14


CPP_SOURCES = demo_simple.cpp ../../microprofile.cpp
C_SOURCES =
ASM_SOURCES =

TARGET=demo_simple
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



