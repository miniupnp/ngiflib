CC=gcc
CFLAGS=-O -Wall -I/usr/include/SDL
CFLAGS+=-g
LDFLAGS=-lSDL

all:	gif2tga SDLaffgif ngiflib.o ngiflibSDL.o

ngiflib.o:	ngiflib.c

ngiflibSDL.o:	ngiflibSDL.c

SDLaffgif.o:	SDLaffgif.c

SDLaffgif:	SDLaffgif.o ngiflibSDL.o ngiflib.o

gif2tga:	gif2tga.o ngiflib.o
