/***************************************************************************\
*' TinyTiler AKA TabbedTiler													|
* by Jesse McClure <jesse@mccluresk9.com>, 2012								|
*    based on TinyWM by Nick Welch <mack@incise.org>, 2005					|
*    with code from DWM by ???????											|
*    and conceptual inspiration from i3wm by ??????							|
\***************************************************************************/


/************************* [0] INCLUDES & DEFINES *************************/
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
#include <X11/cursorfont.h>
#include <string.h>
#include <sys/select.h>

#define CLEANMASK(mask)	( (mask&~LockMask)&~Mod2Mask )
#define MAX(a, b)		((a) > (b) ? (a) : (b))
#define RECTABAR_LINELENGTH		256

/************************* [1] GLOBAL DECLARATIONS *************************/
/* 1.0 TYPEDEFS & STRUCTS */
typedef struct Client Client;
struct Client {
	char *title;
	int tlen;
	int x, y;
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
enum {Background, Clock, SpacesNorm, SpacesActive, SpacesSel, SpacesUrg,
		TitleNorm, TitleSel, StackNorm, StackAct, StackSel,
		StackNormBG, StackActBG, StackSelBG, LASTColor };
enum {Tiled, Floating, ExTiled, ExFloating, LASTStack };

/* 1.1 FUNCTION PROTOTPYES	*/
/*	1.1.1 Events			*/
static void buttonpress(XEvent *);
static void buttonrelease(XEvent *);
static void expose(XEvent *);
static void keypress(XEvent *);
static void maprequest(XEvent *);
static void motionnotify(XEvent *);
static void propertynotify(XEvent *);
static void unmapnotify(XEvent *);
/*	1.1.2 Other Functions	*/
static void die(const char *msg, ...);
static void drawbar();
static void exscreen(const char *);
static void focus(const char *);
static void fullscreen(const char *);
static void killclient(const char *);
static void move(const char *);
static Client *pullclient(Client *);
static Client * pushclient(Client *,Client **);
static void putclient(const char *);
static void rectabar();
static void spawn(const char *);
static void swap(const char *);
static void stack_tile(Client *,int,int,int,int);
static void stack_float(Client *);
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
static Drawable bar,sbar;
static GC *gc,sgc;
static Colormap cmap;
static XFontStruct *fontstruct;
static int fontheight;
static int wksp, onwksp, onstack;
static Bool zoomed;

static Client *focused=NULL;
#include "config.h"
static Client *clients[WORKSPACES][LASTStack];
static Client *top[WORKSPACES][LASTStack];
static Bool urg[WORKSPACES];
static FILE *inpipe;
static XWindowAttributes attr;
static XButtonEvent start;


/************************* [2] FUNCTION DEFINITIONS ************************/
/* 2.0 EVENT HANDLERS */

void buttonpress(XEvent *ev) {
	Client *c;
	if (!(c=wintoclient(ev->xbutton.subwindow))) return;
	if (ev->xbutton.button == 2) {
		focused = pushclient(pullclient(c),&clients[wksp][Tiled]);
		stack();
		return;
	}
	else {
		focused = pushclient(pullclient(c),&clients[wksp][Floating]);
		stack();
	}
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
		if (XFetchName(dpy,c->win,&c->title)) c->tlen = strlen(c->title);
		XSelectInput(dpy,c->win,PropertyChangeMask);
		pushclient(c,&clients[wksp][Tiled]);
		Atom rtype;
		int fmt;
		unsigned long nitem,bytes;
		unsigned char *prop;
		XGetWindowProperty(dpy,c->win,XInternAtom(dpy,"WM_TRANSIENT_FOR",True),
			0,0,False,AnyPropertyType,&rtype,&fmt,&nitem,&bytes,&prop);
		if (rtype != None) /* found WM_TRANSIENT_FOR atom */
			pushclient(pullclient(c),&clients[wksp][Floating]);
		XMapWindow(dpy,c->win);
		focused = c;
		stack();
	}
}

void motionnotify(XEvent *ev) {
	int xdiff, ydiff;
	Client *c;
	while(XCheckTypedEvent(dpy, MotionNotify, ev));
	xdiff = ev->xbutton.x_root - start.x_root;
	ydiff = ev->xbutton.y_root - start.y_root;
	if (!(c=wintoclient(ev->xbutton.window))) return;
	if (start.button == 1)
		XMoveWindow(dpy,c->win,(c->x=attr.x+xdiff),(c->y=attr.y+ydiff));
	else if (start.button == 3)
		XResizeWindow(dpy,c->win,attr.width+xdiff,attr.height+ydiff);
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
	Client *c;
	XUnmapEvent *e = &ev->xunmap;
	if(!(c = wintoclient(e->window))) return;
	if(!e->send_event) {
		pullclient(c);
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
	Bool occupied = False;
	i = strftime(buf,7,"%H:%M",localtime(&now));
	XDrawString(dpy,bar,gc[Clock],2,fontheight,buf,i);
	i = XTextWidth(fontstruct,buf,i) + 7 + fontstruct->max_bounds.lbearing;
	/* WORKSPACES */
	for (j = 0; j < WORKSPACES; j++) {
		occupied = clients[j][Tiled] || clients[j][Floating];
		XFillRectangle(dpy,bar,gc[
			(wksp==j	?	SpacesSel 		:
			(urg[j]		?	SpacesUrg 		: 
			(occupied	?	SpacesActive 	: SpacesNorm ))) ],
			6*j+i, (occupied?3:6),3,fontheight-(occupied?3:6));
	}
	i += 6*j+2; /* 6px for each workspace */
	/* STATUSBAR */
	XCopyArea(dpy,sbar,bar,sgc,0,0,STATUSBARSPACE,barheight,i,0);
	i += STATUSBARSPACE+4;
	/* MASTER WINDOW TAB & NAME */
	int tab_width = sw-i;
	int tab_count = 0;
	Client *c = clients[wksp][Tiled];
	if (c) {
		for ( c=clients[wksp][Tiled]; c; c=c->next ) tab_count++;
		c = clients[wksp][Tiled];
		if (c->next) tab_width = sw*fact-i;
		if (!columns) tab_width = (sw-i)/tab_count;
		XFillRectangle(dpy,bar,gc[ (c==focused ? StackSel : StackAct) ],
			i+1,0,tab_width-2,barheight);
		XFillRectangle(dpy,bar,gc[ (c==focused ? StackSel : StackAct) ],
			i,(topbar ? 1 : 0),tab_width,barheight-1);
		XFillRectangle(dpy,bar,gc[ (c==focused ? StackSelBG : StackActBG) ],
			i+1,(topbar ? 1 : 0),tab_width-2,barheight-1);
		XDrawString(dpy,bar,gc[ (c==focused ? TitleSel : TitleNorm)],
			i+4,fontheight,c->title,(c->tlen > 62 ? 62 : c->tlen));
	}
	/* STACK TABS */
	i+=tab_width;
	int max_tlen;
	if (c && clients[wksp][Tiled]->next) {
		if (columns) tab_width = (sw-i)/(tab_count-1);
		for ( c=clients[wksp][Tiled]->next; c; c=c->next ) {
			XFillRectangle(dpy,bar,gc[ (c==focused ? StackSel :
				(c==top[wksp][Tiled] ? StackAct:StackNorm) ) ],
				i+1,0,tab_width-2,barheight);
			XFillRectangle(dpy,bar,gc[ (c==focused ? StackSel :
				(c==top[wksp][Tiled]?StackAct:StackNorm) ) ],
				i,(topbar ? 1 : 0),tab_width,barheight-1);
			XFillRectangle(dpy,bar,gc[ (c==focused ? StackSelBG :
				(c==top[wksp][Tiled]?StackActBG:StackNormBG) ) ],
				i+1,(topbar ? 1 : 0),tab_width-2,barheight-1);
			max_tlen = (tab_width > 8 ? (tab_width-8)/fontstruct->max_bounds.width : 1);
			XDrawString(dpy,bar,gc[(c==focused ? TitleSel : TitleNorm)],i+4,
				fontheight,c->title,(c->tlen > max_tlen ? max_tlen : c->tlen));
			i+=tab_width;
		}
	}
	/* DRAW IT */
	XCopyArea(dpy,bar,root,gc[0],0,0,sw,barheight,0,(topbar ? 0 : sh));
	XSync(dpy,False);
}

void exscreen(const char *arg) {
	int i;
	if (arg[0] == 'd') {
		for (i = 0; i < WORKSPACES; i++) {
			while (clients[i][ExTiled])
				pushclient(pullclient(clients[i][ExTiled]),&clients[i][Tiled]);
			while (clients[i][ExFloating])
				pushclient(pullclient(clients[i][ExFloating]),&clients[i][Floating]);
		}
		system("xrandr --output " VIDEO1 " --auto --output " VIDEO2 " --off");
		screen = DefaultScreen(dpy);
		sw = DisplayWidth(dpy,screen);
		sh = DisplayHeight(dpy,screen) - barheight;
	}
	else if (arg[0] == 's') {
		if (focused) pushclient(pullclient(focused),&clients[wksp][ExTiled]);
	}
	else if (arg[0] == 'r')  {
		if (focused) pushclient(pullclient(focused),&clients[wksp][Tiled]);
	}
	else if (arg[0] == 'a') {
		system("xrandr --output " VIDEO1 " --auto --output " VIDEO2 " --auto --below " VIDEO1);
		screen = DefaultScreen(dpy);
		sw = DisplayWidth(dpy,screen);
		sh = DisplayHeight(dpy,screen) - barheight;
		int nsizes,i;
		XRRScreenSize *xrrs = XRRSizes(dpy,screen,&nsizes);
		XRRScreenConfiguration *rrConf = XRRGetScreenInfo(dpy,root);
		Rotation rrRot;
		i = XRRConfigCurrentConfiguration(rrConf,&rrRot);
		XRRFreeScreenConfigInfo(rrConf);
		exsw = xrrs[i].width;
		exsh = xrrs[i].height;
	}
	else return;
	stack();
}

void focus(const char *arg) {
	if (!top[wksp][Tiled]) return; /* only one client, do nothing */
	Client *c;
	if (!focused) focused = clients[wksp][Tiled]; /* nothing focused? focus master */
	if (arg[0] == 'r') { /* focus right */
		focused = focused->next;
		if (!focused) focused = clients[wksp][Tiled];
	}
	else if (arg[0] == 'l') {  /* focus left */
		for (c=clients[wksp][Tiled]; c && c->next != focused; c = c->next);
		if (!c) for (c=clients[wksp][Tiled]; c->next; c = c->next);
		focused = c;
	}
	else if (arg[0] == 'o') { /* swap focus between master/stack */
		if (focused == top[wksp][Tiled]) focused = clients[wksp][Tiled];
		else focused = top[wksp][Tiled];
	}
	if (focused != clients[wksp][Tiled]) top[wksp][Tiled] = focused;
	stack();
}

void fullscreen(const char *arg) {
	if (!focused) return;
	if ( (zoomed=~zoomed) ) {
		XMoveResizeWindow(dpy,focused->win,0,0,sw,sh+barheight);
		XRaiseWindow(dpy,focused->win);
	}
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
	if (!top[wksp][Tiled]) return;
	Client *t=NULL; /* t is target client to swap with focused client */
	if (arg[0] == 'r')
		t = focused->next;
	else if (arg[0] == 'l')
		for (t=clients[wksp][Tiled]; t->next != focused && t->next; t = t->next);
	if (!t) t = clients[wksp][Tiled];
	swapclients(t,focused); /* switch focused client with target */
	focused = t;
	stack();
}

Client *pullclient(Client *c) {
	Client *t;
	wintoclient(c->win);
	if (focused == c) focused = focused->next;
	if (top[onwksp][Tiled] == c) top[onwksp][Tiled]=(c->next ? c->next : 
			(clients[onwksp][Tiled]->next == c ? NULL : clients[onwksp][Tiled]->next));
	if (clients[onwksp][onstack] == c) clients[onwksp][onstack] = c->next;
	else {
		for (t = clients[onwksp][onstack]; t && t->next != c; t = t->next);
		t->next = c->next;
	}
	if (top[onwksp][Tiled] && top[onwksp][Tiled] == clients[onwksp][Tiled])
		top[onwksp][Tiled]=top[onwksp][Tiled]->next;
	int i = 0;
	while (!focused) {
		focused = clients[onwksp][i];
		if (++i == LASTStack) break;
	}
	return c;
}

Client *pushclient(Client *c,Client **stack) {
	c->next = *stack;
	int i;
	for (i = 0; i < WORKSPACES; i++)
		if (*stack == clients[i][Tiled]) top[i][Tiled] = clients[i][Tiled];
	*stack = c;
	return c;
}

void putclient(const char *arg) {
	if (!focused) return;
	int i = arg[0] - 49; /* cheap way to convert char number to int number */
	if (i == wksp || i < 0 || i > WORKSPACES - 1) return; /* valid workspace #? */
	Client *c = focused; /* c=client to move; t=client pointing to c */
	focused=(focused->next ? focused->next : clients[wksp][Tiled]);
	pullclient(c);
	top[i][Tiled] = clients[i][Tiled];
	pushclient(c,&clients[i][Tiled]);
	XMoveWindow(dpy,c->win,-sw-exsw,barheight);
	stack();
}

void rectabar() {
	static XColor color;
	static char colstring[8];
	static char line[RECTABAR_LINELENGTH+1];
	static int rx,ry,rw,rh,l;
	XFillRectangle(dpy,sbar,gc[Background],0,0,STATUSBARSPACE,barheight);
	if (fgets(line,RECTABAR_LINELENGTH,inpipe) == NULL) return;
	int x=0; char *t,*c = line;
	while (*c != '\n') {
		if (*c == '{') {
			if (*(++c) == '#') {
				strncpy(colstring,c,7);
				XAllocNamedColor(dpy,cmap,colstring,&color,&color);
				XSetForeground(dpy,sgc,color.pixel);
			}
			else if (*c == 'r') {
				if ( sscanf(c,"r %dx%d+%d+%d}",&rw,&rh,&rx,&ry) == 4)
					XDrawRectangle(dpy,sbar,sgc,x+rx,ry,rw,rh);
			}
			else if (*c == 'R') {
				if ( sscanf(c,"R %dx%d+%d+%d}",&rw,&rh,&rx,&ry) == 4)
					XFillRectangle(dpy,sbar,sgc,x+rx,ry,rw,rh);
			}
			else if (*c == '+') {
				if ( sscanf(c,"+%d}",&rw) == 1) x+=rw;
			}
			c = strchr(c,'}')+1;
		}
		else {
			if ((t=strchr(c,'{')) == NULL) t=strchr(c,'\n');
			l = (t == NULL ? 0 : t-c);
			if (l > 0) {
				XDrawString(dpy,sbar,sgc,x,fontheight,c,l);
				x+=XTextWidth(fontstruct,c,l);
			}
			c+=l;
		}
	}
	drawbar();
}

void spawn(const char *arg) {
	system(arg);
}

void swap(const char *arg) {
	swapclients(clients[wksp][Tiled],top[wksp][Tiled]);
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

void stack_tile(Client *stack,int x, int y, int w, int h) {
	if (!stack) return;
	else if (!stack->next || !columns) {
		XMoveResizeWindow(dpy,stack->win,x,y,w,h);
		if (!columns) XRaiseWindow(dpy,stack->win);
	}
	else {
		XMoveResizeWindow(dpy,stack->win,x,y,
			(bstack ? w : w*fact),(bstack ? h*fact : h));
		while ( (stack=stack->next) )
			XMoveResizeWindow(dpy,stack->win,
				(bstack ? x : w*fact), (bstack ? y+h*fact : y),
				(bstack ? w : w-(int)(w*fact)), (bstack ? h-(int)(h*fact) : h));
	}
}

void stack_float(Client *stack) {
	while (stack) {
		XMoveWindow(dpy,stack->win,stack->x,stack->y);
		XRaiseWindow(dpy,stack->win);
		stack = stack->next;
	}
}

void stack() {
	zoomed = False;
	stack_tile(clients[wksp][Tiled],0,(topbar ? barheight : 0),sw,sh);
	if (top[wksp][Tiled]) XRaiseWindow(dpy, top[wksp][Tiled]->win);
	stack_float(clients[wksp][Floating]);
	/* external monitor: */
	stack_tile(clients[wksp][ExTiled],0,(topbar ? barheight : 0),sw,sh);
	if (top[wksp][ExTiled]) XRaiseWindow(dpy, top[wksp][ExTiled]->win);
	stack_float(clients[wksp][ExFloating]);
	/* input focus: */
	if (focused) XSetInputFocus(dpy,focused->win,RevertToPointerRoot,CurrentTime);
	drawbar();
}

void stackmode(const char *arg) {
	static int oldbarheight;
	if (arg[0] == 'i') fact += FACT_ADJUST;
	else if (arg[0] == 'd') fact -= FACT_ADJUST;
	else if (arg[0] == 'b') bstack = 1;
	else if (arg[0] == 'r') bstack = 0;
	else if (arg[0] == 't') bstack = 1-bstack;
	else if (arg[0] == 's') columns = False;
	else if (arg[0] == 'm') columns = True;
	else if (arg[0] == 'f') {
		topbar = (topbar ? False : True);
		XClearArea(dpy,root,0,0,sw,sh+barheight,True);
	}
	else if (arg[0] == 'h') {
		if (barheight > 0) {
			oldbarheight = barheight;
			sh+=barheight; barheight = 0;
		}
		else {
			barheight = oldbarheight;
			sh-=barheight;
		}
		XClearArea(dpy,root,0,0,sw,sh+barheight,True);
	}
	else return;
	if (fact < FACT_MIN) fact = FACT_MIN;
	else if (fact > 100 - FACT_MIN) fact = FACT_MIN;
	stack();
}

void *wintoclient(Window w) {
	Client *c;
	int i,j;
	for (i = 0; i < WORKSPACES; i++) {
		onwksp = i;
		for (j = 0; j < LASTStack; j++) {
			onstack = j;
			for (c = clients[i][j]; c && c->win != w; c = c->next);
			if (c) return c;
		}
	}
	return NULL;
}

void workspace(const char *arg) {
	int j,i = arg[0] - 49; /* cheap way to convert char number to int */
	if ( (i<0) || (i>WORKSPACES) || (wksp==i) ) return;
	wksp = i;
	Client *c;
	for (i = 0; i < WORKSPACES; i++) for (j = 0; j < LASTStack; j++) /* "hide" all, by moving off screen */
		for (c = clients[i][j]; c; c = c->next)
			XMoveWindow(dpy,c->win,-sw-exsw,barheight);
	focused = clients[wksp][Tiled];
	if (!focused) focused = clients[wksp][Floating];
	stack();
	drawbar();
}

void quit(const char *arg) {
	running = False;
}

int xerror(Display *d, XErrorEvent *ev) {
	char msg[1024];
	XGetErrorText(dpy,ev->error_code,msg,sizeof(msg));
	fprintf(stderr,"======== TTWM ERROR ========\nrequest=%d; error=%d\n%s\n============================\n",
		ev->request_code,ev->error_code,msg);
	return 0;
}

/************************* [3] MAIN & MAIN LOOP ****************************/
int main(int argc, const char **argv) {
	if (argc > 1) inpipe = popen(argv[1],"r");
	else inpipe = stdin;
	if(!(dpy = XOpenDisplay(0x0))) return 1;
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy,screen);
	XSetErrorHandler(xerror);
	XDefineCursor(dpy,root,XCreateFontCursor(dpy,TTWM_CURSOR));

	/* CONFIGURE GRAPHIC CONTEXTS */
	unsigned int i,j;
	gc = (GC *) calloc(LASTColor, sizeof(GC));
	cmap = DefaultColormap(dpy,screen);
	XColor color;
	XGCValues val;
	val.font = XLoadFont(dpy,font);
	fontstruct = XQueryFont(dpy,val.font);
	fontheight = fontstruct->ascent+1;
	if (barheight == 0) barheight = fontheight+fontstruct->descent+1;
	sw = DisplayWidth(dpy,screen);
	sh = DisplayHeight(dpy,screen) - barheight;
	bar = XCreatePixmap(dpy,root,sw,barheight,DefaultDepth(dpy,screen));
	sbar = XCreatePixmap(dpy,root,STATUSBARSPACE,barheight,DefaultDepth(dpy,screen));
	for (i = 0; i < LASTColor; i++) {
		XAllocNamedColor(dpy,cmap,colors[i],&color,&color);
		val.foreground = color.pixel;
		gc[i] = XCreateGC(dpy,root,GCForeground|GCFont,&val);
	}
	sgc = XCreateGC(dpy,root,GCFont,&val);
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
	int xfd,sfd,r;
	struct timeval tv;
	fd_set rfds;
	sfd = fileno(inpipe);
	xfd = ConnectionNumber(dpy);
	XFillRectangle(dpy,sbar,gc[Background],0,0,STATUSBARSPACE,barheight);
	drawbar();
	while (running) {
		memset(&tv,0,sizeof(tv));
		tv.tv_sec = 10;
		FD_ZERO(&rfds);
		FD_SET(sfd,&rfds);
		FD_SET(xfd,&rfds);
		r = select(xfd+1,&rfds,0,0,&tv);
		if (r == 0) drawbar();
		if (FD_ISSET(sfd,&rfds)) rectabar();
		if (FD_ISSET(xfd,&rfds))
			while (XPending(dpy)) {
				XNextEvent(dpy,&ev);
				if (handler[ev.type]) handler[ev.type](&ev);
			}
	}
	/* clean up needed here */
	XFreeFontInfo(NULL,fontstruct,1);
	XUnloadFont(dpy,val.font);
	return 0;
}

// vim: ts=4

