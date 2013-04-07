CFLAGS	+=	-Os -Wall -Wno-unused-parameter -Wno-unused-result
PROG	=	ttwm
LIBS	=	-lX11 -lXrandr
PREFIX	?=	/usr
MANDIR	?=	/usr/share/man
VER		=	2.0b
HEADERS	=	config.h icons.h

${PROG}: ${PROG}.c ${HEADERS}
	@${CC} ${CFLAGS} ${LIBS} -o ${PROG} ${PROG}.c
	@strip ${PROG}

clean:
	@rm -f ${PROG}

tarball: clean
	@rm -f ttwm-${VER}.tar.gz
	@tar -czf ttwm-${VER}.tar.gz *

install: ${PROG}
	@install -Dm755 ${PROG} ${DESTDIR}${PREFIX}/bin/${PROG}


