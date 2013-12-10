
PROG     =  alopex
VER      =  3.0a
CC       ?= gcc
CFLAGS   += `pkg-config --cflags x11 cairo freetype2`
LDFLAGS  += `pkg-config --libs x11 cairo freetype2`
PREFIX   ?= /usr
MODULES  =  alopex config draw input key_chain tile
ICONS		=  tag cpu mem aud bat wfi clk
HEADERS  =  alopex.h
MANPAGES =  alopex.1
VPATH    =  src:doc:icons

${PROG}: ${MODULES:%=%.o} icons.png
	@cd src && ${CC} -o ../${PROG} ${MODULES:%=%.o} ${LDFLAGS}

icons.png: ${ICONS:%=%.svg}
	@echo -e "\033[1;34m  -->\033[0m Building icons.png"
	@cd icons && ./makeicons

%.o: %.c ${HEADERS}
	@echo -e "\033[1;34m  -->\033[0m Building $<"
	@${CC} -c -o src/$@ $< ${CFLAGS} ${OPTS}

install: ${PROG}
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}
	@install -Dm644 share/config.icecap ${DESTDIR}${PREFIX}/share/${PROG}/config.icecap
	@install -Dm644 share/config.tundra ${DESTDIR}${PREFIX}/share/${PROG}/config.tundra
	@install -Dm644 icons/icons.png ${DESTDIR}${PREFIX}/share/${PROG}/icons.png

${MANPAGES}: alopex.%: alopex-%.tex
	@latex2man $< $@

man: ${MANPAGES}

clean:
	@rm -f ${PROG} ${PROG}-${VER}.tar.gz
	@cd src && rm -f ${MODULES:%=%.o}
	@rm -f icons/icons.png

dist: clean
	@tar -czf ${PROG}-${VER}.tar.gz *

.PHONY: clean dist man
