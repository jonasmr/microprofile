
CC=clang
CPP=clang++
LD=clang++
TARGET=../microprofile.h

all: $(TARGET)

embed: embed.c
	clang -g embed.c -o embed

$(TARGET): embed microprofile.h microprofile.html
	./embed $(TARGET) microprofile.h microprofile.html ____embed____ g_MicroProfileHtml MICROPROFILE_EMBED_HTML


