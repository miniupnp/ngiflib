CC?=gcc
CFLAGS?=-O -Wall
CFLAGS+=-Wextra
CFLAGS+=-g
#CPPFLAGS+=-DDEBUG
CPPFLAGS+=-DNGIFLIBSDL_LOG
CPPFLAGS+=$(shell pkg-config sdl --cflags)
LDFLAGS=$(shell pkg-config sdl --libs-only-L)
LDLIBS=$(shell pkg-config sdl --libs-only-l)
DEPFLAGS = -MM -MT $(patsubst %.d,%.o,$@) -MT $@

OS=$(shell uname -s)
ifeq ($(OS), Darwin)
LDLIBS += -framework Cocoa
endif

EXECUTABLES=gif2tga SDLaffgif gif2txt

SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
DEPS = $(patsubst %.o,%.d,$(OBJS))

REFIMGS = $(patsubst %_indexed.tga.gz,%.gif,$(wildcard ref/*_indexed.tga.gz))
REFIMGS += $(patsubst %_indexed_out01.tga.gz,%.gif,$(wildcard ref/*_indexed_out01.tga.gz))
INVALIDIMGS = $(wildcard invalid_gif/*.gif)

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
		if [ ! -f $$ref ] ; then ref="ref/$$(basename $$tga).gz" ; fi ;\
		gunzip -c $$ref | cmp -l /dev/stdin $$tga || { echo "ERROR on $$tga" ; err=1; } ;\
	done ;\
	exit $$err
	mkdir -p tmp/invalid
	@for gif in $(INVALIDIMGS); do \
		base=$$(basename $$gif .gif) ;\
		./gif2tga --indexed --outbase tmp/invalid/$${base} $${gif} || { echo "ERROR on $$gif" ; exit 1; } ; \
	done

SDLaffgif:	SDLaffgif.o ngiflibSDL.o ngiflib.o

gif2tga:	gif2tga.o ngiflib.o

gif2txt:	gif2txt.o ngiflib.o

%.o:	%.c %.d

%.d:	%.c
	$(CC) $(CPPFLAGS) $(DEPFLAGS) -o $@ $<

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),depclean)
-include $(DEPS)
endif
endif
