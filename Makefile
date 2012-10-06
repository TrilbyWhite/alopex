CFLAGS	+=	-Os -Wall -Wno-unused-parameter -Wno-unused-result
PROG	=	ttwm
LIBS	=	-lX11 -lXrandr
PREFIX	?=	/usr
MANDIR	?=	/usr/share/man

all: $(PROG) tbars

$(PROG): $(PROG).c config.h
	@$(CC) $(CFLAGS) $(LIBS) -o $(PROG) $(PROG).c
	@strip $(PROG)
	@gzip -c $(PROG).1 > $(PROG).1.gz

tbars: tbars.c
	@$(CC) $(CFLAGS) -lX11 -o tbars tbars.c
	@strip tbars

clean:
	@rm -f $(PROG)
	@rm -f $(PROG).1.gz
	@rm -f tbars

tarball: clean
	@rm -f ttwm.tar.gz
	@tar -czf ttwm.tar.gz *

install:
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@install -m755 ttwm ${DESTDIR}${PREFIX}/bin/ttwm
	@install -m755 tbars ${DESTDIR}${PREFIX}/bin/tbars
	@install -Dm666 $(PROG).1.gz ${DESTDIR}${MANDIR}/man1/$(PROG).1.gz
	@install -m755 vol ${DESTDIR}${PREFIX}/bin/vol

