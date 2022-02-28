
PROG     =  alopex
VER      =  4.0a
CC       ?= gcc
CFLAGS   += -fcommon $(shell pkg-config --cflags x11 cairo freetype2 xinerama)
LDLIBS   += $(shell pkg-config --libs x11 cairo freetype2 xinerama)
PREFIX   ?= /usr
MODULES  =  actions alopex atoms config draw tile xlib
HEADERS  =  alopex.h actions.h
MANPAGES =  alopex.1
VPATH    =  src:doc

${PROG}: ${MODULES:%=%.o}

%.o: %.c ${HEADERS}

install: ${PROG}
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
	@mkdir -p ${DESTDIR}${PREFIX}/share/${PROG}
	@install -m644 -t ${DESTDIR}${PREFIX}/share/${PROG}/ share/*
	@install -Dm644 doc/alopex.1 ${DESTDIR}${PREFIX}/share/man/man1/alopex.1

${MANPAGES}: alopex.%: alopex-%.tex
	@latex2man $< doc/$@

man: ${MANPAGES}

clean:
	@rm -f ${PROG}-${VER}.tar.gz
	@rm -f ${MODULES:%=%.o}

distclean: clean
	@rm -f ${PROG}

dist: distclean
	@tar -czf ${PROG}-${VER}.tar.gz *

.PHONY: clean dist distclean man
