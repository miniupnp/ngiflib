CC?=gcc
CFLAGS=-O -Wall -I/usr/include/SDL
CFLAGS+=-g
LDFLAGS=
LDLIBS=-lSDL

EXECUTABLES=gif2tga SDLaffgif

all:	$(EXECUTABLES)

clean:
	$(RM) *.o $(EXECUTABLES)
	$(RM) -r tmp/

check:	gif2tga
	mkdir -p tmp
	@cd samples; for gif in *.gif; do \
		base=$$(basename $$gif .gif) ;\
		../gif2tga --indexed --outdir ../tmp $$gif &&\
		mv -v ../tmp/$${base}_out01.tga ../tmp/$${base}_indexed.tga &&\
		../gif2tga --outdir ../tmp $$gif &&\
		mv -v ../tmp/$${base}_out01.tga ../tmp/$${base}_truecolor.tga ;\
	done
	@err=0 ;\
	for tga in tmp/*.tga; do \
		ref="ref/$$(basename $$tga).gz" ;\
		zcmp -b $$tga $$ref || { echo "ERROR on $$tga" ; err=1; } ;\
	done ;\
	exit $$err
		

ngiflib.o:	ngiflib.c ngiflib.h

ngiflibSDL.o:	ngiflibSDL.c ngiflibSDL.h

SDLaffgif.o:	SDLaffgif.c ngiflibSDL.h

SDLaffgif:	SDLaffgif.o ngiflibSDL.o ngiflib.o

gif2tga:	gif2tga.o ngiflib.o
