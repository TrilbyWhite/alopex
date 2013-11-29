
#include "alopex.h"
#include "config.h"

/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

static void enternotify(XEvent *);
static void expose(XEvent *);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static void maprequest(XEvent *);
static void unmapnotify(XEvent *);
static int  xerror(Display *, XErrorEvent *);
static void X_init();
static void X_free();
static Client *wintoclient();
static void (*handler[LASTEvent]) (XEvent *) = {
	[EnterNotify]	= enternotify,
	[Expose]			= expose,
	[KeyPress]		= keypress,
	[KeyRelease]	= keyrelease,
	[MapRequest]	= maprequest,
	[UnmapNotify]	= unmapnotify,
};

static int default_cursor = 64;
static Bool mod_down = False;

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int main(int argc, const char **argv) {
	X_init();
	XEvent ev;
	while (running && !XNextEvent(dpy,&ev))
		if (ev.type < 33 && handler[ev.type]) handler[ev.type](&ev);
	X_free();
}

void die(const char *fmt) {
	fprintf(stderr,"[ALOPEX] %s\n",fmt);
	exit(1);
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

void enternotify(XEvent *ev) {
	Client *c = wintoclient(ev->xcrossing.window);
	if (c && focusfollowmouse) focused = c;
	if (mons->next) {
		// root_x root_y, which monitor
		// m = mons[i]
	}
	if ((c && focusfollowmouse) || mons->next) draw();
}

void expose(XEvent *ev) {
	draw();
}

#define MOD(x)		((x->state&~Mod2Mask)&~LockMask)
void keypress(XEvent *ev) {
	int i;
	XKeyEvent *e = &ev->xkey;
	KeySym sym = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
	if (sym == XK_Super_L) mod_down = True;
	for (i = 0; i < sizeof(key)/sizeof(key[0]); i++)
		if ( (sym==key[i].keysym) && key[i].arg && key[i].mod==MOD(e) ) {
			key_chain(key[i].arg);
			mod_down = False;
		}
}

void keyrelease(XEvent *ev) {
	XKeyEvent *e = &ev->xkey;
	KeySym sym = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
	if (sym != XK_Super_L || !mod_down) return;
	mod_down = False;
	char str[256];
	if (input(str)) key_chain(str);
	draw();
}

void maprequest(XEvent *ev) {
	Client *c, *p;
	XWindowAttributes wa;
	XMapRequestEvent *e = &ev->xmaprequest;
	if (!XGetWindowAttributes(dpy,e->window,&wa) || wa.override_redirect)
		return;
	if (wintoclient(e->window)) return;
	if (!(c=calloc(1,sizeof(Client)))) die("unable to allocate memory");
	c->win = e->window;
	// apply_rules
	// get title
	c->tags = 1;
	// get hints
	if (clients) {
		/*if (client_opts & ATTACH_ABOVE) {
		}
		else if (client_opts & ATTACH_BELOW) {
		}
		else */if (client_opts & ATTACH_BOTTOM) {
			for (p = clients; p->next; p = p->next);
			p->next = c;
		}
		else /* if (client_opts & ATTACH_TOP) */ {
			p = clients;
			clients = c;
			clients->next = p;
		}
	}
	else {
		clients = c;
	}
	XMapWindow(dpy,c->win);
	draw();
}

void unmapnotify(XEvent *ev) {
	Client *c, *t;
	XUnmapEvent *e = &ev->xunmap;
	if (!(c=wintoclient(e->window)) || e->send_event)
		return;
	if (c == focused) focused = c->next;
	if (c == clients) clients = c->next;
	else {
		for (t = clients; t && t->next && t->next != c; t = t->next);
		t->next = c->next;
	}
	//XFree(c->title);
	free(c); c = NULL;
	draw();
}

Client *wintoclient(Window w) {
	Client *c;
	for (c = clients; c; c=c->next)
		if (c->win == w) break;
	return c;
}

int xerror(Display *d, XErrorEvent *e) {
	char msg[1024];
	XGetErrorText(d,e->error_code,msg,sizeof(msg));
	fprintf(stderr,"[X11 %d:%d] %s\n",e->request_code,e->error_code,msg);
}

void X_init() {
	if (!setlocale(LC_CTYPE,"")) die("unable to set locale");
	if (!XSupportsLocale()) die("unsupported locale");
	if (!XSetLocaleModifiers("")) die("unable to set locale modifiers");
	if (!(dpy=XOpenDisplay(0x0))) die("unable to open X display");
	// query xrandr
	scr = DefaultScreen(dpy);
	root = DefaultRootWindow(dpy);
	XSetErrorHandler(xerror);
	XDefineCursor(dpy,root,XCreateFontCursor(dpy,68));
	XSetWindowAttributes wa;
	gc = DefaultGC(dpy,scr);

	/* monitors */
	if (!(mons=calloc(1,sizeof(Monitor))))
		die("unable to allocatememory");
	m = mons;
	Monitor *M;
	Container *C, *CC = NULL;
	int i,j, bh;
	wa.override_redirect = True;
	wa.event_mask = ExposureMask;
	for (M = mons; M; M = M->next) {
		// TODO monitor checks ?
		M->x = 0; M->y = 0;
		M->w = DisplayWidth(dpy,scr);
		M->h = DisplayHeight(dpy,scr);
		//
		M->gap = container_pad;
		for (i = 0; i < sizeof(containers)/sizeof(containers[0]); i++) {
			C = calloc(1,sizeof(Container));
			C->n = containers[i];
			C->bar.opts = bar_opts;
			// TODO replace "12" with font height
			bh = (BAR_HEIGHT(bar_opts) ? BAR_HEIGHT(bar_opts) : 12 );
			C->bar.win = XCreateSimpleWindow(dpy, root, M->x,
					(bar_opts & BAR_TOP ? M->y : M->y + M->h - bh),
					M->w, bh, 0, 0, 0);
			C->bar.buf = XCreatePixmap(dpy, root, M->w, bh,
					DefaultDepth(dpy,scr));
			XChangeWindowAttributes(dpy,C->bar.win,CWOverrideRedirect |
					CWEventMask, &wa);
			XMapWindow(dpy,C->bar.win);
			cairo_surface_t *t = cairo_xlib_surface_create(dpy,C->bar.buf,
					DefaultVisual(dpy,scr),M->w,bh);
			C->bar.ctx = cairo_create(t);
			cairo_surface_destroy(t);
			if (!M->container) M->container = C;
			if (CC) CC->next = C;
			CC = C;
		}
		M->tags = 1;
		M->mode = RSTACK;
		M->focus = M->container;
	}
	/* configure root window */
	wa.event_mask = ExposureMask | FocusChangeMask | ButtonReleaseMask |
			SubstructureNotifyMask | SubstructureRedirectMask |
			StructureNotifyMask | PropertyChangeMask;
	XChangeWindowAttributes(dpy,root,CWEventMask,&wa);
	XSelectInput(dpy,root,wa.event_mask);
	/* key and mouse binding */
	unsigned int mod[] = {0, LockMask, Mod2Mask, LockMask|Mod2Mask};
	KeyCode code;
	XUngrabKey(dpy,AnyKey,AnyModifier,root);
	for (i = 0; i < sizeof(key)/sizeof(key[0]); i++)
		if ( (code=XKeysymToKeycode(dpy,key[i].keysym)) )
			for (j = 0; j < 4; j++)
				XGrabKey(dpy,code,key[i].mod|mod[j],root,True,
						GrabModeAsync,GrabModeAsync);
	code=XKeysymToKeycode(dpy,XK_Super_L);
	for (j = 0; j < 4; j++)
		XGrabKey(dpy,code,mod[j],root,True,GrabModeAsync,GrabModeAsync);
	running = True;
}

void X_free() {
	Monitor *M;
	Container *C, *CC = NULL;
	for (M = mons; M; M = M->next) {
		for (C = M->container; C; CC = C, C = C->next) {
			if (CC) free(CC);
			cairo_destroy(C->bar.ctx);
			XFreePixmap(dpy,C->bar.buf);
			XDestroyWindow(dpy,C->bar.win);
		}
	}
	free(C);
	free(mons);
	XCloseDisplay(dpy);
}


