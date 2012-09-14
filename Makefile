CFLAGS	+=	-Os -Wall -Werror -Wextra -Wno-unused-parameter
PROG	=	ttwm
LIBS	=	-lX11 -lXrandr
PREFIX	?=	/usr
MANDIR	?=	/usr/share/man

all: $(PROG).c config.h
	@$(CC) $(CFLAGS) $(LIBS) -o $(PROG) $(PROG).c
	@strip $(PROG)
	@gzip -c $(PROG).1 > $(PROG).1.gz

clean:
	@rm -f $(PROG)
	@rm -f $(PROG).1.gz

tarball: clean
	@rm -f ttwm.tar.gz
	@tar -czf ttwm.tar.gz *

install:
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@install -m755 ttwm ${DESTDIR}${PREFIX}/bin/ttwm
	@install -Dm666 $(PROG).1.gz ${DESTDIR}${MANDIR}/man1/$(PROG).1.gz

install_vol_script:
	@install -m755 vol ${DESTDIR}${PREFIX}/bin/vol

