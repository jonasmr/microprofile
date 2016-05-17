UNAME_S := $(shell uname -s)
# ifeq ($(UNAME_S),Linux)
# 	LDFLAGS=-lGL `sdl2-config --static-libs` -lpthread
# endif
# ifeq ($(UNAME_S),Darwin)
# 	LDFLAGS=-framework OpenGL `sdl2-config --static-libs` -lpthread
# endif

CFLAGS=-Wno-c++11-extensions -I../..
#CFLAGS+=-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
CFLAGS+=-Wall -DMICROPROFILE_UI=0 -DMICROPROFILE_WEBSERVER=1 -DMICROPROFILE_GPU_TIMERS=0
CFLAGS+=-g -O0 -Wno-invalid-offsetof -Wall -Werror

#CFLAGS+=-DMICROPROFILE_ENABLED=0
CPPFLAGS=$(CFLAGS)
CPPFLAGS+=-stdlib=libc++ -std=c++11


CPP_SOURCES = fakework.cpp demo_noui.cpp
C_SOURCES =

TARGET=demo_noui
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
	$(CPP) -MM $(CPPFLAGS) $(CPP_SOURCES) >> .depend

embed_target:
	cd ../../src && make

embed: embed_target all
	

