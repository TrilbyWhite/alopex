
PROG     =  alopex
VER      =  3.0a
CC       ?= gcc
CFLAGS   += `pkg-config --cflags x11 cairo`
LDFLAGS  += `pkg-config --libs x11 cairo`
PREFIX   ?= /usr
MODULES  =  alopex draw input key_chain tile
HEADERS  =  alopex.h config.h
VPATH		=  src

${PROG}: ${MODULES:%=%.o}
	@cd src && ${CC} -o ../${PROG} ${MODULES:%=%.o} ${LDFLAGS}

%.o: %.c ${HEADERS}
	@${CC} -c -o src/$@ $< ${CFLAGS} ${OPTS}

install: ${PROG}
	@echo Not yet

clean:
	@rm -f ${PROG} ${PROG}-${VER}.tar.gz
	@cd src && rm -f ${MODULES:%=%.o}

dist: clean
	@tar -czf ${PROG}-${VER}.tar.gz *

.PHONY: clean dist
