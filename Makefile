CC?=gcc
CFLAGS?=-O -Wall
CFLAGS+=-Wextra
CFLAGS+=-g
#CFLAGS+=-DDEBUG
CFLAGS+=-DNGIFLIBSDL_LOG
CFLAGS+=$(shell pkg-config sdl --cflags)
LDFLAGS=$(shell pkg-config sdl --libs-only-L)
LDLIBS=$(shell pkg-config sdl --libs-only-l)
DEPFLAGS = -MM -MT $(patsubst %.d,%.o,$@) -MT $@

OS=$(shell uname -s)
ifeq ($(OS), Darwin)
LDLIBS += -framework Cocoa
endif

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
	@for gif in $(REFIMGS); do \
		base=$$(basename $$gif .gif) ;\
		./gif2tga --indexed --outbase tmp/$${base}_indexed samples/$${base}.gif && \
		./gif2tga --outbase tmp/$${base}_truecolor samples/$${base}.gif ;\
	done
	@err=0 ;\
	for tga in tmp/*.tga; do \
		ref="ref/$$(basename $$tga|sed 's/_out01//').gz" ;\
		gunzip -c $$ref | cmp -l /dev/stdin $$tga || { echo "ERROR on $$tga" ; err=1; } ;\
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
