
PROG     =  alopex
VER      =  3.0a
CC       ?= gcc
CFLAGS   += `pkg-config --cflags x11 cairo freetype2 xinerama`
LDFLAGS  += `pkg-config --libs x11 cairo freetype2 xinerama`
PREFIX   ?= /usr
MODULES  =  actions alopex atoms config draw tile xlib
#ICONS		=  tag cpu mem aud bat wfi clk
HEADERS  =  alopex.h actions.h
MANPAGES =  alopex.1
VPATH    =  src:doc

${PROG}: ${MODULES:%=%.o}
	@echo -e "\033[1;34m  ->\033[0m Linking alopex"
	@cd src && ${CC} -o ../${PROG} ${MODULES:%=%.o} ${LDFLAGS}

icons.png: ${ICONS:%=%.svg}
	@echo -e "\033[1;34m  ->\033[0m Building icons.png"
	@cd icons && ./makeicons

%.o: %.c ${HEADERS}
	@echo -e "\033[1;34m  ->\033[0m Building $<"
	@${CC} -c -o src/$@ $< ${CFLAGS} ${OPTS}

install: ${PROG}
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
	@mkdir -p ${DESTDIR}${PREFIX}/share/${PROG}
	@install -m644 -t ${DESTDIR}${PREFIX}/share/${PROG}/ share/*
	@install -Dm644 doc/alopex.1 ${DESTDIR}${PREFIX}/share/man/man1/alopex.1

${MANPAGES}: alopex.%: alopex-%.tex
	@latex2man $< doc/$@

man: ${MANPAGES}

clean:
	@rm -f ${PROG} ${PROG}-${VER}.tar.gz
	@cd src && rm -f ${MODULES:%=%.o}

dist: clean
	@tar -czf ${PROG}-${VER}.tar.gz *

.PHONY: clean dist man
