CFLAGS+=-Os -Wall -Wno-unused-result
PROG=ttwm
LIBS=-lX11

all: $(PROG).c config.h
	@$(CC) $(CFLAGS) $(LIBS) -o $(PROG) $(PROG).c
	@strip $(PROG)

clean:
	@rm -f $(PROG)

tarball:
	@rm -f ttwm.tar.gz
	@tar -czf ttwm.tar.gz *

install:
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@install -m755 ttwm ${DESTDIR}${PREFIX}/bin/ttwm

install_vol_script:
	@install -m755 vol ${DESTDIR}${PREFIX}/bin/vol

