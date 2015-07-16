
CC=clang
CPP=clang++
LD=clang++
TARGET=../microprofile_html.h

all: $(TARGET)

embed: embed.c
	clang -g embed.c -o embed

$(TARGET): embed microprofile.html
	./embed $(TARGET) microprofile.html ____embed____ g_MicroProfileHtml MICROPROFILE_EMBED_HTML


