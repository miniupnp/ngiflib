CC?=gcc
CFLAGS=-O -Wall -I/usr/include/SDL
CFLAGS+=-g
LDFLAGS=
LDLIBS=-lSDL

EXECUTABLES=gif2tga SDLaffgif

all:	$(EXECUTABLES)

clean:
	$(RM) *.o $(EXECUTABLES)

ngiflib.o:	ngiflib.c ngiflib.h

ngiflibSDL.o:	ngiflibSDL.c ngiflibSDL.h

SDLaffgif.o:	SDLaffgif.c ngiflibSDL.h

SDLaffgif:	SDLaffgif.o ngiflibSDL.o ngiflib.o

gif2tga:	gif2tga.o ngiflib.o
