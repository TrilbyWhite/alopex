
#include "alopex.h"

/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

static void ibar_draw();
static void input_init();
static void input_free();
static int  loop(char *);

static Window ibar;
static XIC xic;

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int input(char *str) {
	input_init();
	int len = loop(str);
	input_free();
	return len;
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

void ibar_draw() {
}

void input_init() {
	int i;
	for (i = 0; i < 1000; i++) {
		if (XGrabKeyboard(dpy, root, True, GrabModeAsync, GrabModeAsync,
				CurrentTime) == GrabSuccess) break;
		usleep(1000);
	}
	// create window ibar
	if (i == 1000) return;
	XIM xim = XOpenIM(dpy,NULL,NULL,NULL);
	if (!xim) die("No X input method could be opened\n");
	xic = XCreateIC(xim, XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
			XNClientWindow, ibar, XNFocusWindow, ibar, NULL);
}

void input_free() {
	// destroy window ibar
	XDestroyIC(xic);
}

int loop(char *str) {
	XEvent ev; XKeyEvent *e; KeySym sym;
	char txt[32]; int del = 0;
	Status stat;
	str[0] = '\0';
	while (True) {
		if (!XCheckMaskEvent(dpy,KeyPressMask,&ev)) {
			usleep(10000);
			if ( (++del) > 50 ) break;
			continue;
		}
		del = 0; e = &ev.xkey;
		XmbLookupString(xic,e,txt,sizeof(txt),&sym,&stat);
		if (stat == XBufferOverflow) continue;
		// if e->state ...
		if (sym == XK_Return) break;
		else if (sym == XK_Escape) {str[0] = '\0'; break; }
		else if (!iscntrl(*txt)) {
			// allow insertion
			strcat(str,txt);
			ibar_draw();
		}
	}
	XUngrabKeyboard(dpy,CurrentTime);
	return(strlen(str));
}

