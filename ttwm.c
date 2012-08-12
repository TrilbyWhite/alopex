/***************************************************************************\
* TinyTiler AKA TabbedTiler													|
* by Jesse McClure <jesse@mccluresk9.com>, 2012								|
*    based on TinyWM by Nick Welch <mack@incise.org>, 2005					|
*    with code from DWM by ???????											|
*    and conceptual inspiration from i3wm by ??????							|
\***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <string.h>

#include <sys/select.h>

#define CLEANMASK(mask)	(mask & ~(LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define MAX(a, b)		((a) > (b) ? (a) : (b))

/************************* [1] GLOBAL DECLARATIONS *************************/
/* 1.0 FUNCTION PROTOTPYES */
static void buttonpress(XEvent *);
static void buttonrelease(XEvent *);
static void expose(XEvent *);
static void keypress(XEvent *);
static void maprequest(XEvent *);
static void motionnotify(XEvent *);
static void propertynotify(XEvent *);
static void unmapnotify(XEvent *);

static void drawbar();
static void focus(const char *);
static void fullscreen(const char *);
static void killclient();
static void move(const char *);
static void putclient(const char *);
static void spawn(const char *);
static void swap(const char *);
static void stack();
static void stackmode(const char *);
static void updatestatus();
static void *wintoclient(Window);
static void workspace(const char *);
static void quit(const char *);

/* 1.1 VARIABLE DECLARATIONS */
static Bool running;
static Display *dpy;
static int screen, sw, sh;
static Window root;
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
typedef struct Client Client;
struct Client {
	char *title;
	int tlen;
	Client *next;
	Window win;
};
static int wksp;
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
static Bool zoomed;
struct {
	uint8_t cpu,vol,bat;
	uint8_t cpu_col,vol_col,bat_col;
} status;
static long j1,j2,j3,j4;
static FILE *in;
static uint8_t onwksp;
enum {Background, Clock, SpacesNorm, SpacesActive, SpacesSel, SpacesUrg,
		BarsNorm, BarsFull, BarsCharge, BarsWarn, BarsAlarm,
		TitleNorm, TitleSel, StackNorm, StackAct, StackSel };

void swapclients(Client *, Client *);

/* These next two variables seem awkward and out of place.  They are holdovers
from Tinywm.  I'd get rid of them, but they just work so darn well and, with
a couple simpler button handlers, they provide a transient floating option.
These also serve as a tribute to, and reminder of, ttwm's heritage.  So, they
are here to stay.  -J McClure 2012 */
XWindowAttributes attr;
XButtonEvent start;

#include "config.h"
static Client *clients[WORKSPACES];
static Client *top[WORKSPACES];
static Client *focused=NULL;

/************************* [2] FUNCTION DEFINITIONS ************************/
/* 2.0 EVENT HANDLERS */

void buttonpress(XEvent *ev) {
	if (ev->xbutton.subwindow == None) return;
	if (ev->xbutton.button == 2) {
		stack();
		return;
	}
	XGrabPointer(dpy, ev->xbutton.subwindow, True,
		PointerMotionMask|ButtonReleaseMask, GrabModeAsync,
		GrabModeAsync, None, None, CurrentTime);
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
	//KeySym keysym = XKeycodeToKeysym(dpy,(KeyCode)e->keycode,0);
	KeySym keysym = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
	for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
		if ( (keysym==keys[i].keysym) && keys[i].func &&
			(CLEANMASK(keys[i].mod) == CLEANMASK(e->state)) )
			keys[i].func(keys[i].arg);
}

void maprequest(XEvent *ev) {
	Client *c;
	static XWindowAttributes wa;
	XMapRequestEvent *e = &ev->xmaprequest;
	if(!XGetWindowAttributes(dpy, e->window, &wa)) return;
	if(wa.override_redirect) return;
	if(!wintoclient(e->window)) {
		if (!(c=calloc(1,sizeof(Client))))exit(1);
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
fprintf(stderr,"end maprequest\n");
fflush(stderr);
}

void motionnotify(XEvent *ev) {
	if (ev->xkey.subwindow == None) return;
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
	if (e->atom == XA_WM_NAME) {
		XFree(c->title);
		c->title = NULL;
		c->tlen = 0;
		if (XFetchName(dpy,c->win,&c->title)) c->tlen = strlen(c->title);
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

void drawbar() {
	XFillRectangle(dpy,bar,gc[Background],0,0,sw,BARHEIGHT);
	/* CLOCK */
	static char buf[8];
	static time_t now;
	time(&now);
	int len = strftime(buf,7,"%H:%M",localtime(&now));
	XDrawString(dpy,bar,gc[Clock],2,FONTHEIGHT,buf,len);
	/*workspaces*/
	int i;
	for (i = 0; i < WORKSPACES; i++)
		XFillRectangle(dpy,bar,gc[(wksp==i ? SpacesSel : (clients[i] ?
			SpacesActive : SpacesNorm))], 6*i+40, (clients[i]?3:6),3,
			BARHEIGHT-(clients[i]?5:8));
	/* STATUS MONITORS */
	i = 6*i+45;
	XFillRectangle(dpy,bar,gc[status.cpu_col],i,BARHEIGHT-10,status.cpu,2);
	XFillRectangle(dpy,bar,gc[status.vol_col],i,BARHEIGHT-7,status.vol,2);
	XFillRectangle(dpy,bar,gc[status.bat_col],i,BARHEIGHT-4,status.bat,2);
	/* MASTER WINDOW NAME */
	i+=60;
	Client *c = clients[wksp];
	if (c)
		XDrawString(dpy,bar,gc[ (c==focused ? TitleSel : TitleNorm)],
			i,FONTHEIGHT,c->title,(c->tlen > 62 ? 62 : c->tlen));
	/* STACK TABS */
	i = sw*fact;
	if (clients[wksp]) for ( c=clients[wksp]->next; c; c=c->next ) {
		XFillRectangle(dpy,bar,gc[ (c==focused ? StackSel :
			(c==top[wksp]?StackAct:StackNorm) ) ],i,3,35,8);
		i+=40;
	}
	/* STACK WINDOW NAME */
	if (top[wksp])
		XDrawString(dpy,bar,gc[ (top[wksp]==focused ? TitleSel : TitleNorm)],
			i,FONTHEIGHT,top[wksp]->title,top[wksp]->tlen);
	/* DRAW IT */
	XCopyArea(dpy,bar,root,gc[0],0,0,sw,BARHEIGHT,0,0);
	XSync(dpy,False);
}

void focus(const char *arg) {
	if (!top[wksp]) return;
	Client *c;
	if (!focused) focused = clients[wksp];
	if (arg[0] == 'r') {
		focused = focused->next;
		if (!focused) focused = clients[wksp];
	}
	else if (arg[0] == 'l') { 
		for (c=clients[wksp]; c && c->next != focused; c = c->next);
		if (!c) for (c=clients[wksp]; c->next; c = c->next);
		focused = c;
	}
	XRaiseWindow(dpy, focused->win);
	if (focused != clients[wksp]) top[wksp] = focused;
	stack();
}

void fullscreen(const char *arg) {
	if (!focused) return;
	if ( (zoomed=~zoomed) ) XMoveResizeWindow(dpy,focused->win,0,0,sw,sh+BARHEIGHT);
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
	Client *t=NULL;
	if (arg[0] == 'r')
		t = focused->next;
	else if (arg[0] == 'l')
		for (t=clients[wksp]; t->next != focused && t->next; t = t->next);
	if (!t) t = clients[wksp];
	swapclients(t,focused);
	focused = t;
	stack();
}

static void putclient(const char *arg) {
	if (!focused) return;
	//int i = arg[0] - 49;
	// move focused to workspace i
}

void spawn(const char *arg) { system(arg); }

void swap(const char *arg) {
	swapclients(clients[wksp],top[wksp]);
	stack();
}

void swapclients(Client *a, Client *b) {
	if (!a || !b) return;
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
	if (!c) return;
	if (!c->next) {
		XMoveResizeWindow(dpy,c->win,0,BARHEIGHT,sw,sh);
		XSetInputFocus(dpy,c->win,RevertToPointerRoot,CurrentTime);
		return;
	}
	else XMoveResizeWindow(dpy,c->win,0,BARHEIGHT,
			(bstack ? sw		: sw*fact),
			(bstack ? sh*fact	: sh));
	for (c=c->next; c; c=c->next)
		XMoveResizeWindow(dpy,c->win,
			(bstack ? 0 				: sw*fact),
			(bstack ? BARHEIGHT+sh*fact	: BARHEIGHT),
			(bstack ? sw 				: sw*(1-fact)),
			(bstack ? sh*(1-fact)		: sh));
//	if (top[wksp]) XRaiseWindow(dpy, top[wksp]->win);
	if (focused) XSetInputFocus(dpy,focused->win,RevertToPointerRoot,CurrentTime);
	drawbar();
}

void stackmode(const char *arg) {
	if (arg[0] == 'i') fact += FACT_ADJUST;
	else if (arg[0] == 'd') fact -= FACT_ADJUST;
	else if (arg[0] == 'b') bstack = 1;
	else if (arg[0] == 'r') bstack = 0;
	else if (arg[0] == 't') bstack = 1-bstack;
	else return;
	if (fact < FACT_MIN) fact = FACT_MIN;
	else if (fact > 100 - FACT_MIN) fact = FACT_MIN;
	stack();
}

void updatestatus() {
	static long ln1,ln2,ln3,ln4;
	static int n;
	static char c;
	if ( (in=fopen(CPU_FILE,"r")) ) {	/* CPU MONITOR */
		fscanf(in,"cpu %ld %ld %ld %ld",&ln1,&ln2,&ln3,&ln4);
		fclose(in);
		if (ln4>j4) n=(int)100*(ln1-j1+ln2-j2+ln3-j3)/(ln1-j1+ln2-j2+ln3-j3+ln4-j4);
		else n=0;
		j1=ln1; j2=ln2; j3=ln3; j4=ln4;
		if (n > 85) status.cpu_col = BarsAlarm;
		else if (n > 60) status.cpu_col = BarsWarn;
		else status.cpu_col = BarsNorm;
		status.cpu = (n > 100 ? 100 : n) / 2;
	}
	if ( (in=fopen(AUD_FILE,"r")) ) {	/* AUDIO VOLUME MONITOR */
		fscanf(in,"%d",&n);
		fclose(in);
		if (n == -1) status.vol_col = BarsAlarm;
		else if (n == 100) status.vol_col = BarsFull;
		else if (n < 15) status.vol_col = BarsWarn;
		else status.vol_col = BarsNorm;
		if (n > -1) status.vol = n / 2;
	}
	if ( (in=fopen(BATT_NOW,"r")) ) {	/* BATTERY MONITOR */
		fscanf(in,"%ld\n",&ln1); fclose(in);
		if ( (in=fopen(BATT_FULL,"r")) ) { fscanf(in,"%ld\n",&ln2); fclose(in); }
		if ( (in=fopen(BATT_STAT,"r")) ) { fscanf(in,"%c",&c); fclose(in); }
		n = (ln1 ? ln1 * 100 / ln2 : 0);
		if (c == 'C') status.bat_col = BarsCharge;
		else if (n < 10) status.bat_col = BarsAlarm;
		else if (n < 20) status.bat_col = BarsWarn;
		else if (n > 95) status.bat_col = BarsFull;
		else status.bat_col = BarsNorm;
		status.bat = n / 2;
	}
	drawbar();
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
	int i = arg[0] - 49;
	if ( (i<0) || (i>WORKSPACES) || (wksp==i) ) return;
	wksp = i;
	Client *c;
	for (i = 0; i < WORKSPACES; i++)
		for (c = clients[i]; c; c = c->next)
			XMoveWindow(dpy,c->win,sw,BARHEIGHT);
	focused = clients[wksp];
	stack();
	drawbar();
}

void quit(const char *arg) { running = False; };

/************************* [3] MAIN & MAIN LOOP ****************************/
int main() {
	if(!(dpy = XOpenDisplay(0x0))) return 1;
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy,screen);
	sw = DisplayWidth(dpy,screen);
	sh = DisplayHeight(dpy,screen) - BARHEIGHT;

	/* CONFIGURE GRAPHIC CONTEXTS */
	bar = XCreatePixmap(dpy,root,sw,BARHEIGHT,DefaultDepth(dpy,screen));
	int i;
	gc = (GC *) calloc(NUMCOLORS, sizeof(GC));
	Colormap cmap = DefaultColormap(dpy,screen);
	XColor color;
	XGCValues val;
	val.font = XLoadFont(dpy,font);
	for (i = 0; i < NUMCOLORS; i++) {
		//XAllocNamedColor(dpy,cmap,colors[i][0],&color,&color);
		//val.border = color.pixel;
		XAllocNamedColor(dpy,cmap,colors[i],&color,&color);
		val.foreground = color.pixel;
		//XAllocNamedColor(dpy,cmap,colors[i][2,&color,&color);
		//val.background = color.pixel;
		gc[i] = XCreateGC(dpy,root,GCForeground|GCFont,&val);
	}
	XSetWindowAttributes wa;
	wa.event_mask =	ExposureMask				|
					SubstructureNotifyMask		|
					ButtonPressMask				|
					PointerMotionMask			|
					PropertyChangeMask			|
					SubstructureRedirectMask	|
					StructureNotifyMask;
	XChangeWindowAttributes(dpy,root,CWEventMask,&wa);
	XSelectInput(dpy,root,wa.event_mask);
	/* GRAB KEYS & BUTTONS */
	KeyCode code;
	XUngrabKey(dpy,AnyKey,AnyModifier,root);
	for (i = 0; i < sizeof(keys)/sizeof(keys[0]); i++)
		if ( (code=XKeysymToKeycode(dpy,keys[i].keysym)) )
			XGrabKey(dpy,code,keys[i].mod,root,True,GrabModeAsync,GrabModeAsync);
	XGrabButton(dpy,AnyButton,MODKEY,root,True,ButtonPressMask,GrabModeAsync,GrabModeAsync,None,None);
	/* MAIN LOOP */
	XEvent ev;
	in = fopen(CPU_FILE,"r");
	fscanf(in,"cpu %ld %ld %ld %ld",&j1,&j2,&j3,&j4);
	fclose(in);
	updatestatus();
	int fd,r;
	struct timeval tv;
	fd_set rfds;
	fd = ConnectionNumber(dpy);
	running = True;
	while (running) {
		memset(&tv,0,sizeof(tv));
		tv.tv_usec = 250000;
		FD_ZERO(&rfds);
		FD_SET(fd,&rfds);
		r = select(fd+1,&rfds,0,0,&tv);
		if (r == 0)
			updatestatus();
		else
			while (XPending(dpy)) {
				XNextEvent(dpy,&ev);
				if (handler[ev.type]) handler[ev.type](&ev);
			}
	}
	return 0;
}

// vim: ts=4

