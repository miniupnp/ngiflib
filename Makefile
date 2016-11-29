CC?=gcc
CFLAGS?=-O -Wall
CFLAGS+=-I/usr/include/SDL
CFLAGS+=-g
#CFLAGS+=-DDEBUG
LDFLAGS=
LDLIBS=-lSDL
DEPFLAGS = -MM -MT $(patsubst %.d,%.o,$@) -MT $@

EXECUTABLES=gif2tga SDLaffgif

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
DEPS = $(patsubst %.o,%.d,$(OBJS))

REFIMGS = $(patsubst %_indexed.tga.gz,%.gif,$(wildcard ref/*_indexed.tga.gz))

all:	$(EXECUTABLES)

clean:	depclean
	$(RM) *.o $(EXECUTABLES)
	$(RM) -r tmp/

depclean:
	$(RM) $(DEPS)

depend:	$(DEPS)

check:	gif2tga
	mkdir -p tmp
	@cd samples; for gif in $(REFIMGS); do \
		base=$$(basename $$gif .gif) ;\
		../gif2tga --indexed --outdir ../tmp $${base}.gif &&\
		mv -v ../tmp/$${base}_out01.tga ../tmp/$${base}_indexed.tga &&\
		../gif2tga --outdir ../tmp $${base}.gif &&\
		mv -v ../tmp/$${base}_out01.tga ../tmp/$${base}_truecolor.tga ;\
	done
	@err=0 ;\
	for tga in tmp/*.tga; do \
		ref="ref/$$(basename $$tga).gz" ;\
		zcmp -b $$tga $$ref || { echo "ERROR on $$tga" ; err=1; } ;\
	done ;\
	exit $$err
		
SDLaffgif:	SDLaffgif.o ngiflibSDL.o ngiflib.o

gif2tga:	gif2tga.o ngiflib.o

%.o:	%.c %.d

%.d:	%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ $<

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),depclean)
-include $(DEPS)
endif
endif
