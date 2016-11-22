CC=gcc
CFLAGS=-Wall -I/usr/include/SDL
LDFLAGS=-lSDL

all:	SDLaffgif ngiflib.o ngiflibSDL.o

ngiflib.o:	ngiflib.c

ngiflibSDL.o:	ngiflibSDL.c

SDLaffgif.o:	SDLaffgif.c

SDLaffgif:	SDLaffgif.o ngiflibSDL.o ngiflib.o

