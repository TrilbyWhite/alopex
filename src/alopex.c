
#include "alopex.h"

extern int xlib_init(const char *);
extern int xlib_free();
extern int main_loop();

void handler(int sig) {
	if (sig == SIGCHLD)
		while (waitpid(-1, NULL, WNOHANG) > 0);
}

void die(const char *fmt, ...) {
	fprintf(stderr,"ALOPEX: ");
	va_list arg;
	va_start(arg, fmt);
	vfprintf(stderr, fmt, arg);
	va_end(arg);
	fprintf(stderr,"\n");
	exit(1);
}

int main(int argc, const char **argv) {
	signal(SIGCHLD, &handler);
	if (argc > 1) xlib_init(argv[1]);
	else xlib_init(NULL);
	main_loop();
	xlib_free();
	return 0;
}
