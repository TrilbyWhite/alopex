/***************************************************************************\
* TinyTiler AKA TabbedTiler													|
* by Jesse McClure <jesse@mccluresk9.com>, 2012								|
*    based on TinyWM by Nick Welch <mack@incise.org>, 2005					|
*    with code from DWM by ???????											|
*    and conceptual inspiration from i3wm by ??????							|
\***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xrandr.h>
#include <string.h>
#include <sys/select.h>

#define CLEANMASK(mask)	( (mask&~LockMask)&~Mod2Mask )
#define MAX(a, b)		((a) > (b) ? (a) : (b))

/************************* [1] GLOBAL DECLARATIONS *************************/
/* 1.0 TYPEDEFS & STRUCTS */
typedef struct Client Client;
struct Client {
	char *title;
	int tlen;
	Client *next;
	Window win;
};
typedef struct {
	unsigned int mask, button;
	void (*func)();
} Button;
typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const char *);
	const char *arg;
} Key;
struct {
	uint8_t cpu,vol,bat;
	uint8_t cpu_col,vol_col,bat_col;
} status;
enum {Background, Clock, SpacesNorm, SpacesActive, SpacesSel, SpacesUrg,
		TitleNorm, TitleSel, StackNorm, StackAct, StackSel,
		StackNormBG, StackActBG, StackSelBG, LASTColor };

/* 1.1 FUNCTION PROTOTPYES */
static void buttonpress(XEvent *);
static void buttonrelease(XEvent *);
static void expose(XEvent *);
static void keypress(XEvent *);
static void maprequest(XEvent *);
static void motionnotify(XEvent *);
static void propertynotify(XEvent *);
static void unmapnotify(XEvent *);

static void die(const char *msg, ...);
static void drawbar();
static void exscreen(const char *);
static void focus(const char *);
static void fullscreen(const char *);
static void killclient(const char *);
static void move(const char *);
static void putclient(const char *);
static void spawn(const char *);
static void swap(const char *);
static void stack();
static void stackmode(const char *);
static void swapclients(Client *, Client *);
static void *wintoclient(Window);
static void workspace(const char *);
static void quit(const char *);

/* 1.2 VARIABLE DECLARATIONS */
static Display *dpy;
static int screen, sw, sh, exsw, exsh;
static Window root;
static Bool running=True;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress]		= buttonpress,
	[ButtonRelease]		= buttonrelease,
	[Expose]			= expose,
	[KeyPress]			= keypress,
	[MapRequest]		= maprequest,
	[PropertyNotify]	= propertynotify,
	[MotionNotify]		= motionnotify,
	[UnmapNotify]		= unmapnotify
};
static Drawable bar;
static GC *gc;
static XFontStruct *fontstruct;
static int fontheight;
static int wksp, onwksp;
static Bool zoomed;

static Client *focused=NULL;
static Window *exwin;
#include "config.h"
static Client *clients[WORKSPACES];
static Client *top[WORKSPACES];
static Bool urg[WORKSPACES];

/* These next two variables seem awkward and out of place.  They are holdovers
from Tinywm.  I'd get rid of them, but they just work so darn well and, with
a couple simpler button handlers, they provide a transient floating option.
These also serve as a tribute to, and reminder of, ttwm's heritage.  So, they
are here to stay.  -J McClure 2012 */
XWindowAttributes attr;
XButtonEvent start;

/************************* [2] FUNCTION DEFINITIONS ************************/
/* 2.0 EVENT HANDLERS */

void buttonpress(XEvent *ev) {
	if (ev->xbutton.subwindow == None) return;
	if (ev->xbutton.button == 2) { stack(); return; }
	XGrabPointer(dpy, ev->xbutton.subwindow, True, PointerMotionMask |
		ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	XGetWindowAttributes(dpy, ev->xbutton.subwindow, &attr);
	start = ev->xbutton;
}

void buttonrelease(XEvent *ev) {
	XUngrabPointer(dpy, CurrentTime);
}

void expose(XEvent *ev) {
	if (ev->xexpose.count == 0) drawbar();
}

void keypress(XEvent *ev) {
	unsigned int i;
	XKeyEvent *e = &ev->xkey;
	KeySym keysym = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
	for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
		if ( (keysym==keys[i].keysym) && keys[i].func &&
				keys[i].mod == ((e->state&~Mod2Mask)&~LockMask) )
			keys[i].func(keys[i].arg);
}

void maprequest(XEvent *ev) {
	Client *c;
	static XWindowAttributes wa;
	XMapRequestEvent *e = &ev->xmaprequest;
	if (!XGetWindowAttributes(dpy, e->window, &wa)) return;
	if (wa.override_redirect) return;
	if (!wintoclient(e->window)) {
		if (!(c=calloc(1,sizeof(Client))))
			die("Failed to allocate %d bytes for new client\n",sizeof(Client));
		c->win = e->window;
		c->next = clients[wksp];
		top[wksp] = clients[wksp];
		if (XFetchName(dpy,c->win,&c->title)) c->tlen = strlen(c->title);
		XSelectInput(dpy,c->win,PropertyChangeMask);
		clients[wksp] = c;
		XMapWindow(dpy,c->win);
		focused = c;
		stack();
	}
}

void motionnotify(XEvent *ev) {
	int xdiff, ydiff;
	while(XCheckTypedEvent(dpy, MotionNotify, ev));
	xdiff = ev->xbutton.x_root - start.x_root;
	ydiff = ev->xbutton.y_root - start.y_root;
	XMoveResizeWindow(dpy, ev->xmotion.window,
		attr.x + (start.button==1 ? xdiff : 0),
		attr.y + (start.button==1 ? ydiff : 0),
		MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
		MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
}

void propertynotify(XEvent *ev) {
	XPropertyEvent *e = &ev->xproperty;
	Client *c;
	if ( !(c=wintoclient(e->window)) ) return;
if (c==focused) stack();
	if (e->atom == XA_WM_NAME) {
		XFree(c->title);
		c->title = NULL;
		c->tlen = 0;
		if (XFetchName(dpy,c->win,&c->title)) c->tlen = strlen(c->title);
		drawbar();
	}
	else if (e->atom == XA_WM_HINTS) {
		XWMHints *hint;
		if ( (hint=XGetWMHints(dpy,c->win)) && (hint->flags & XUrgencyHint) ) {
			wintoclient(c->win);
			urg[onwksp] = True;
		}
		drawbar();
	}
}

void unmapnotify(XEvent *ev) {
	Client *c, *t;
	XUnmapEvent *e = &ev->xunmap;
	if(!(c = wintoclient(e->window))) return;
	if(e->send_event); // ignore send_events for now.
	else {
		if (focused == c) focused=(focused->next ? focused->next : clients[wksp]);
		if (top[onwksp] == c) top[onwksp]=(c->next ? c->next : 
				(clients[onwksp]->next == c ? NULL : clients[onwksp]->next));
		if (clients[onwksp] == c) clients[onwksp] = c->next;
		else {
			for (t = clients[onwksp]; t->next != c; t = t->next);
			t->next = c->next;
		}
		if (top[onwksp] && top[onwksp] == clients[onwksp])
			top[onwksp]=top[onwksp]->next;
		XFree(c->title);
		free(c);
	}
	stack();
}

/* 2.1 OTHER FUNCTIONS */

static void die(const char *msg, ...) {
	va_list args;
	va_start(args,msg);
	vfprintf(stderr,msg,args);
	va_end(args);
	exit(1);
}

void drawbar() {
	urg[wksp] = False;
	XFillRectangle(dpy,bar,gc[Background],0,0,sw,barheight);
	/* CLOCK */
	static char buf[8];
	static time_t now;
	time(&now);
	int i,j;
	i = strftime(buf,7,"%H:%M",localtime(&now));
	XDrawString(dpy,bar,gc[Clock],2,fontheight,buf,i);
	i = XTextWidth(fontstruct,buf,i) + 7 + fontstruct->max_bounds.lbearing;
	/* WORKSPACES */
	for (j = 0; j < WORKSPACES; j++)
		XFillRectangle(dpy,bar,gc[
			(wksp==j	?	SpacesSel 		:
			(urg[j]		?	SpacesUrg 		: 
			(clients[j] ?	SpacesActive 	: SpacesNorm ))) ],
			6*j+i, (clients[j]?3:6),3,fontheight-(clients[j]?3:6));
	i += 6*j+STATUSBARSPACE; /* 6px for each workspace */
	/* MASTER WINDOW TAB & NAME */
	int tab_width = sw-i;
	int tab_count = 0;
	Client *c = clients[wksp];
	if (c) {
		for ( c=clients[wksp]; c; c=c->next ) tab_count++;
		c = clients[wksp];
		if (c->next) tab_width = sw*fact-i;
		if (!columns) tab_width = (sw-i)/tab_count;
		XFillRectangle(dpy,bar,gc[ (c==focused ? StackSel : StackAct) ],
			i+1,0,tab_width-2,barheight);
		XFillRectangle(dpy,bar,gc[ (c==focused ? StackSel : StackAct) ],
			i,1,tab_width,barheight-1);
		XFillRectangle(dpy,bar,gc[ (c==focused ? StackSelBG : StackActBG) ],
			i+1,1,tab_width-2,barheight);
		XDrawString(dpy,bar,gc[ (c==focused ? TitleSel : TitleNorm)],
			i+4,fontheight,c->title,(c->tlen > 62 ? 62 : c->tlen));
	}
	/* STACK TABS */
	i+=tab_width;
	int max_tlen;
	if (c && clients[wksp]->next) {
		if (columns) tab_width = (sw-i)/(tab_count-1);
		for ( c=clients[wksp]->next; c; c=c->next ) {
			XFillRectangle(dpy,bar,gc[ (c==focused ? StackSel :
				(c==top[wksp]?StackAct:StackNorm) ) ],i+1,0,tab_width-2,barheight);
			XFillRectangle(dpy,bar,gc[ (c==focused ? StackSel :
				(c==top[wksp]?StackAct:StackNorm) ) ],i,1,tab_width,barheight-1);
			XFillRectangle(dpy,bar,gc[ (c==focused ? StackSelBG :
				(c==top[wksp]?StackActBG:StackNormBG) ) ],i+1,1,tab_width-2,barheight);
			max_tlen = (tab_width > 8 ? (tab_width-8)/fontstruct->max_bounds.width : 1);
			XDrawString(dpy,bar,gc[(c==focused ? TitleSel : TitleNorm)],i+4,
				fontheight,c->title,(c->tlen > max_tlen ? max_tlen : c->tlen));
			i+=tab_width;
		}
	}
	/* DRAW IT */
	XCopyArea(dpy,bar,root,gc[0],0,0,sw,barheight,0,0);
	XSync(dpy,False);
}

void exscreen(const char *arg) {
	if (arg[0] == 'd') {
		exwin[wksp] = 0;
		system("xrandr --output " VIDEO1 " --auto --output " VIDEO2 " --off");
		screen = DefaultScreen(dpy);
		sw = DisplayWidth(dpy,screen);
		sh = DisplayHeight(dpy,screen) - barheight;
	}
	else if (arg[0] == 's') exwin[wksp] = (focused ? focused->win : clients[wksp]->win);
	else if (arg[0] == 'r') exwin[wksp] = 0;
	else if (arg[0] == 'a') {
		system("xrandr --output " VIDEO1 " --auto --output " VIDEO2 " --auto --below " VIDEO1);
		screen = DefaultScreen(dpy);
		sw = DisplayWidth(dpy,screen);
		sh = DisplayHeight(dpy,screen) - barheight;
		int nsizes;
		XRRScreenSize *xrrs = XRRSizes(dpy,screen,&nsizes);
		if (nsizes != 2) {
			/* wrong number of monitors */
		}
		exsw = xrrs[0].width;
		exsh = xrrs[0].height;
	}
	else return;
	stack();
}

void focus(const char *arg) {
	if (!top[wksp]) return; /* only one client, do nothing */
	Client *c;
	if (!focused) focused = clients[wksp]; /* nothing focused? focus master */
	if (arg[0] == 'r') { /* focus right */
		focused = focused->next;
		if (!focused) focused = clients[wksp];
	}
	else if (arg[0] == 'l') {  /* focus left */
		for (c=clients[wksp]; c && c->next != focused; c = c->next);
		if (!c) for (c=clients[wksp]; c->next; c = c->next);
		focused = c;
	}
	else if (arg[0] == 'o') { /* swap focus between master/stack */
		if (focused == top[wksp]) focused = clients[wksp];
		else focused = top[wksp];
	}
	if (focused != clients[wksp]) top[wksp] = focused;
	stack();
}

void fullscreen(const char *arg) {
	if (!focused) return;
	if ( (zoomed=~zoomed) ) XMoveResizeWindow(dpy,focused->win,0,0,sw,sh+barheight);
	else stack();
}

void killclient(const char *arg) {
	if (!focused) return;
	XEvent ev;
	ev.type = ClientMessage;
	ev.xclient.window = focused->win;
	ev.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", True);
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(dpy,focused->win,False,NoEventMask,&ev);
}

void move(const char *arg) {
	if (!top[wksp]) return;
	Client *t=NULL; /* t is target client to swap with focused client */
	if (arg[0] == 'r')
		t = focused->next;
	else if (arg[0] == 'l')
		for (t=clients[wksp]; t->next != focused && t->next; t = t->next);
	if (!t) t = clients[wksp];
	swapclients(t,focused); /* switch focused client with target */
	focused = t;
	stack();
}

static void putclient(const char *arg) {
	if (!focused) return;
	int i = arg[0] - 49; /* cheap way to convert char number to int number */
	if (i == wksp || i < 0 || i > WORKSPACES - 1) return; /* valid workspace #? */
	Client *t, *c = focused; /* c=client to move; t=client pointing to c */
	/* remove c from clients[wksp] */
	focused=(focused->next ? focused->next : clients[wksp]);
	if (top[wksp] == c) top[wksp]=(c->next ? c->next : 
			(clients[wksp]->next == c ? NULL : clients[wksp]->next));
	if (clients[wksp] == c) clients[wksp] = c->next;
	else {
		for (t = clients[wksp]; t->next != c; t = t->next);
		t->next = c->next;
	}
	if (top[wksp] && top[wksp] == clients[wksp])
		top[wksp]=top[wksp]->next;
	/* prepend c to clients[i] a.k.a. the target workspace */
	c->next = clients[i];
	top[i] = clients[i];
	clients[i] = c;
	/* "hide" c by moving off screen */
	XMoveWindow(dpy,c->win,sw,barheight);
	stack();
}

void spawn(const char *arg) {
	system(arg);
}

void swap(const char *arg) {
	swapclients(clients[wksp],top[wksp]);
	stack();
}

void swapclients(Client *a, Client *b) {
	if (!a || !b) return; /* no clients? do nothing */
	char *title;
	int tlen;
	Window win;
	title = a->title;
	tlen = a->tlen;
	win = a->win;
	a->title = b->title;
	a->tlen = b->tlen;
	a->win = b->win;
	b->title = title;
	b->tlen = tlen;
	b->win = win;
}

void stack() {
	zoomed = False;
	Client *c = clients[wksp];
	if (!c) return; /* no clients = nothing to do */
	if (exwin[wksp]) /* client on external monitor? */
		XMoveResizeWindow(dpy,exwin[wksp],0,sh+barheight,exsw,exsh);
	if (c->win == exwin[wksp]) c=c->next;
	if (!c) return; /* no remaining clients = nothing to do */
	else if ( (!c->next) || (c->next->win == exwin[wksp] && !c->next->next) ) {
		/* only one client = full screen */
		XMoveResizeWindow(dpy,c->win,0,barheight,sw,sh);
		//XSetInputFocus(dpy,c->win,RevertToPointerRoot,CurrentTime);
	}
	else {
		if (columns) {
			XMoveResizeWindow(dpy,c->win,0,barheight, 
				(bstack ? sw		: sw*fact),
				(bstack ? sh*fact	: sh));
			for (c=c->next; c; c=c->next)
				if (c->win != exwin[wksp])
					XMoveResizeWindow(dpy,c->win,
						(bstack ? 0 					: sw*fact),
						(bstack ? barheight+sh*fact		: barheight),
						(bstack ? sw 					: sw - (int)(sw*fact)),
						(bstack ? sh - (int)(sh*fact)	: sh));
		}
		else {
			for (c=c; c; c=c->next)
				if (c->win != exwin[wksp]) XMoveResizeWindow(dpy,c->win,0,barheight,sw,sh);
		}
	}
	if (top[wksp]) XRaiseWindow(dpy, top[wksp]->win);
	if (focused) {
		XRaiseWindow(dpy, focused->win);
		XSetInputFocus(dpy,focused->win,RevertToPointerRoot,CurrentTime);
	}
	drawbar();
}

void stackmode(const char *arg) {
	if (arg[0] == 'i') fact += FACT_ADJUST;
	else if (arg[0] == 'd') fact -= FACT_ADJUST;
	else if (arg[0] == 'b') bstack = 1;
	else if (arg[0] == 'r') bstack = 0;
	else if (arg[0] == 't') bstack = 1-bstack;
	else if (arg[0] == 's') columns = False;
	else if (arg[0] == 'm') columns = True;
	else return;
	if (fact < FACT_MIN) fact = FACT_MIN;
	else if (fact > 100 - FACT_MIN) fact = FACT_MIN;
	stack();
}

void *wintoclient(Window w) {
	Client *c;
	int i;
	for (i = 0; i < WORKSPACES; i++) {
		onwksp = i;
		for (c = clients[i]; c && c->win != w; c = c->next);
		if (c) return c;
	}
	return NULL;
}

void workspace(const char *arg) {
	int i = arg[0] - 49; /* cheap way to convert char number to int */
	if ( (i<0) || (i>WORKSPACES) || (wksp==i) ) return;
	wksp = i;
	Client *c;
	for (i = 0; i < WORKSPACES; i++) /* "hide" all, by moving off screen */
		for (c = clients[i]; c; c = c->next)
			XMoveWindow(dpy,c->win,-sw-exsw,barheight);
	focused = clients[wksp];
	stack();
	drawbar();
}

void quit(const char *arg) {
	running = False;
};

int xerror(Display *d, XErrorEvent *ev) {
	fprintf(stderr,"ttwm error: request=%d; error=%d\n",ev->request_code,ev->error_code);
	return 0;
}

/************************* [3] MAIN & MAIN LOOP ****************************/
int main() {
	if(!(dpy = XOpenDisplay(0x0))) return 1;
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy,screen);
	XSetErrorHandler(xerror);
	exwin = (Window *) calloc(WORKSPACES,sizeof(Window));

	/* CONFIGURE GRAPHIC CONTEXTS */
	unsigned int i,j;
	gc = (GC *) calloc(LASTColor, sizeof(GC));
	Colormap cmap = DefaultColormap(dpy,screen);
	XColor color;
	XGCValues val;
	val.font = XLoadFont(dpy,font);
	fontstruct = XQueryFont(dpy,val.font);
	fontheight = fontstruct->ascent+1;
	if (barheight == 0) barheight = fontheight+fontstruct->descent+1;
	sw = DisplayWidth(dpy,screen);
	sh = DisplayHeight(dpy,screen) - barheight;
	bar = XCreatePixmap(dpy,root,sw,barheight,DefaultDepth(dpy,screen));
	for (i = 0; i < LASTColor; i++) {
		XAllocNamedColor(dpy,cmap,colors[i],&color,&color);
		val.foreground = color.pixel;
		gc[i] = XCreateGC(dpy,root,GCForeground|GCFont,&val);
	}
	XSetWindowAttributes wa;
	wa.event_mask =	ExposureMask				|
					FocusChangeMask				|
					SubstructureNotifyMask		|
					ButtonReleaseMask			|
					PointerMotionMask			|
					PropertyChangeMask			|
					SubstructureRedirectMask	|
					StructureNotifyMask;
	XChangeWindowAttributes(dpy,root,CWEventMask,&wa);
	XSelectInput(dpy,root,wa.event_mask);
	/* GRAB KEYS & BUTTONS */
	unsigned int mods[] = {0, LockMask,Mod2Mask,LockMask|Mod2Mask};
	KeyCode code;
	XUngrabKey(dpy,AnyKey,AnyModifier,root);
	for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++) 
		if ( (code=XKeysymToKeycode(dpy,keys[i].keysym)) ) for (j = 0; j < 4; j++)
			XGrabKey(dpy,code,keys[i].mod|mods[j],root,True,GrabModeAsync,GrabModeAsync);
	XGrabButton(dpy,AnyButton,MODKEY,root,True,ButtonPressMask,GrabModeAsync,
		GrabModeAsync,None,None);
	/* MAIN LOOP */
	XEvent ev;
	drawbar();
	while (running && ! XNextEvent(dpy,&ev))
		if (handler[ev.type]) handler[ev.type](&ev);
	/* clean up needed here */
	free(exwin);
	XFreeFontInfo(NULL,fontstruct,1);
	XUnloadFont(dpy,val.font);
	return 0;
}

// vim: ts=4

