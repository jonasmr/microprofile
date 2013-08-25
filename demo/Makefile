
LDFLAGS=-framework OpenGL `sdl2-config --static-libs` -lpthread

CFLAGS=`sdl2-config --cflags` -I.. -Iglew -DGLEW_STATIC -Wno-c++11-extensions
CFLAGS+=-g -O0 -Wno-invalid-offsetof

CPPFLAGS=$(CFLAGS)
CPPFLAGS+=-Wno-lambda-extensions -stdlib=libc++ -std=c++11

CPP_SOURCES = demo.cpp microprofile.cpp
C_SOURCES = glew/glew.c

TARGET=demo
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