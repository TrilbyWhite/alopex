/**********************************************************************\
* ALOPEX - a tiny tiling tabbing window manager with fur
*
* Author: Jesse McClure, copyright 2012-2013
* License: GPLv3
*
*    This program is free software: you can redistribute it and/or
*    modify it under the terms of the GNU General Public License as
*    published by the Free Software Foundation, either version 3 of the
*    License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see
*    <http://www.gnu.org/licenses/>.
*
\**********************************************************************/


/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

#include "alopex.h"

static void buttonpress(XEvent *);
static void configurerequest(XEvent *);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void get_hints(Client *);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static void maprequest(XEvent *);
static void propertynotify(XEvent *);
static void unmapnotify(XEvent *);
static int  xerror(Display *, XErrorEvent *);
static void X_init();
static void X_free();
static Client *wintoclient();
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress]	= buttonpress,
	[ConfigureRequest]	= configurerequest,
	[EnterNotify]	= enternotify,
	[Expose]		= expose,
	[KeyPress]		= keypress,
	[KeyRelease]	= keyrelease,
	[PropertyNotify]	= propertynotify,
	[MapRequest]	= maprequest,
	[UnmapNotify]	= unmapnotify,
};

static int default_cursor = 64, purgX = 0, purgY = 0;
static Bool mod_down = False;
static const char *noname_window = "(WINDOW)";
static const char *def_theme_name = "icecap";

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int main(int argc, const char **argv) {
	if (argc > 1) theme_name = argv[1];
	else theme_name = def_theme_name;
	statfd = 0;
	X_init();
	draw(2);
	fd_set fds; struct timeval timeout;
	int xfd = ConnectionNumber(dpy);
	XEvent ev; int trigger;
	while (running) {
		FD_ZERO(&fds);
		memset(&timeout,0,sizeof(struct timeval));
		trigger = False;
		timeout.tv_sec = 60;
		FD_SET(xfd,&fds);
		if (statfd) {
			FD_SET(statfd,&fds);
			select((statfd > xfd ? statfd : xfd) + 1,&fds,0,0,NULL);
		}
		else {
			memset(&timeout,0,sizeof(struct timeval));
			timeout.tv_sec = 60;
			select(xfd + 1,&fds,0,0,&timeout);
		}
		if (statfd && FD_ISSET(statfd,&fds)) {
			read(statfd,instring,sizeof(instring));
			trigger = 1;
		}
		if (FD_ISSET(xfd,&fds)) while(XPending(dpy)) {
			XNextEvent(dpy,&ev);
			if (ev.type < 33 && handler[ev.type])
				handler[ev.type](&ev);
		}
		else if (!statfd) trigger = 1;
		if (trigger) draw(trigger);
	}
	X_free();
}

int die(const char *fmt) {
	fprintf(stderr,"[ALOPEX] %s\n",fmt);
	exit(1);
}

int purgatory(Window win) {
	XMoveWindow(dpy,win,purgX,purgY);
}

int set_focus(Client *c) {
	Window win; int rev;
	XGetInputFocus(dpy,&win,&rev);
	Client *cc;
	if ( (cc=wintoclient(win)) && (cc->flags & WIN_FLOAT) &&
			(m->focus && m->focus->top == c) ) return;
	if (c->flags & WIN_FOCUS) {
		XEvent ev;
		ev.type = ClientMessage; ev.xclient.window = c->win;
		ev.xclient.message_type = XInternAtom(dpy,"WM_PROTOCOLS",True);
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = XInternAtom(dpy,"WM_TAKE_FOCUS",True);
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy,c->win,False,NoEventMask,&ev);
	}
	else {
		XSetInputFocus(dpy,c->win, RevertToPointerRoot, CurrentTime);
	}
	if (c != winmarks[0]) {
		winmarks[1] = winmarks[0];
		winmarks[0] = m->focus->top;
	}
	c->flags &= ~WIN_URGENT;
}


/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

void buttonpress(XEvent *ev) {
	XButtonEvent *e = &ev->xbutton;
	Client *c;
	if ( !(c=wintoclient(e->subwindow)) ) return;
	XRaiseWindow(dpy,c->win);
	set_focus(c);
	int b = e->button;
	if (b == 2) {
		c->flags &= ~WIN_FLOAT;
		draw(2);
		return;
	}
	XGrabPointer(dpy,root,True,PointerMotionMask | ButtonReleaseMask,
		GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	XEvent ee;
	int dx, dy;
	int xx = e->x_root, yy = e->y_root;
	int wx, wy, ww, wh, ig;
	Window w;
	XGetGeometry(dpy,c->win,&w,&wx,&wy,&ww,&wh,&ig,&ig);
	while (True) {
		XMaskEvent(dpy,PointerMotionMask | ButtonReleaseMask,&ee);
		if (ee.type == MotionNotify) {
			c->flags |= WIN_FLOAT;
			dx = ee.xbutton.x_root - xx;
			dy = ee.xbutton.y_root - yy;
			if (b == 1) XMoveWindow(dpy,c->win,wx+dx,wy+dy);
			else if (b == 3) XResizeWindow(dpy,c->win,ww+dx,wh+dy);
		}
		else if (ee.type == ButtonRelease) break;
		draw(1);
	}
	XUngrabPointer(dpy, CurrentTime);
	draw(2);
}

void configurerequest(XEvent *ev) {
	XConfigureRequestEvent *e = &ev->xconfigurerequest;
	Client *c;
	if ( (c=wintoclient(e->window)) ) {
		if ( (e->value_mask & CWWidth) || (e->value_mask & CWHeight) ) {
			if ( (e->width == m->w) && (e->height == m->h) ) winmarks[2] = c;
	//		if (c->flags & WIN_FLOAT)
	//			XMoveResizeWindow(dpy,c->win,e->x, e->y, e->width, e->height);
		}
		draw(2);
		return;
	}
	XWindowChanges wc;
	wc.x = e->x; wc.y = e->y; wc.width = e->width; wc.height = e->height;
	wc.sibling = e->above; wc.stack_mode = e->detail;
	XConfigureWindow(dpy,e->window,e->value_mask,&wc);
	XFlush(dpy);
}

void enternotify(XEvent *ev) {
	Client *c = wintoclient(ev->xcrossing.window);
	if (mons->next) {
		// root_x root_y, which monitor
		// m = mons[i]
	}
	if (c && focusfollowmouse) {
		Container *C;
		for (C = m->container; C; C = C->next)
			if (C->top && C->top == c)
				m->focus = C;
	}
	if ((c && focusfollowmouse) || mons->next) draw(1);
}

void expose(XEvent *ev) {
	draw(1);
}


void get_hints(Client *c) {
	XWMHints *hint;
	if ( !(hint=XGetWMHints(dpy,c->win)) ) return;
	if (hint->flags & XUrgencyHint) c->flags |= WIN_URGENT;
	if (hint->flags & InputHint && !hint->input) c->flags |= WIN_FOCUS;
	// get icon
	// if (hint->flags & IconThingy) {
	//	if (c->icon) { cairo_surface_destroy(c->icon); c->icon = NULL; };
	//	... cairo render to c->icon at size BAR_HEIGHT ...
	// }
	XFree(hint);
}

void get_name(Client *c) {
	Client *p;
	XFree(c->title); c->title = NULL;
	XTextProperty name; char **list = NULL; int n;
	XGetTextProperty(dpy,c->win,&name,XA_WM_NAME);
	XmbTextPropertyToTextList(dpy,&name,&list,&n);
	if (n && *list) {
		c->title = strdup(*list);
		XFreeStringList(list);
	}
	else {
		if ( (p=wintoclient(c->parent)) ) c->title = strdup(p->title);
		else c->title = strdup(noname_window);
	}
}

#define MOD(x)		((x->state&~Mod2Mask)&~LockMask)
void keypress(XEvent *ev) {
	int i, ret;
	XKeyEvent *e = &ev->xkey;
	KeySym sym = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
	if (sym == XK_Super_L) mod_down = True;
	for (i = 0; i < nkeys; i++)
		if ( (sym==key[i].keysym) && key[i].arg && key[i].mod==MOD(e) ) {
			ret = key_chain(key[i].arg);
			mod_down = False;
		}
	draw(ret);
}

void keyrelease(XEvent *ev) {
	XKeyEvent *e = &ev->xkey;
	KeySym sym = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
	if (sym != XK_Super_L || !mod_down) return;
	mod_down = False;
	char str[256]; int ret = 0;
	if (input(str)) ret = key_chain(str);
	draw(ret);
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
	c->tags = m->tags;
	get_hints(c);
	if (XGetTransientForHint(dpy,c->win,&c->parent)) c->flags |= WIN_TRANSIENT;
	else c->parent = e->parent;
	get_name(c);
	if (wa.width == m->w && wa.height == m->h) winmarks[2] = c;
	// apply_rules
	if (!(c->tags)) c->tags = m->tags;
	if (!(c->tags)) c->tags = 1;
	if (!(m->tags & 0xFF)) m->tags = c->tags;
	XSelectInput(dpy, c->win, PropertyChangeMask | EnterWindowMask);
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
	draw(2);
}

void propertynotify(XEvent *ev) {
	XPropertyEvent *e = &ev->xproperty;
	Client *c;
	if (!(c=wintoclient(e->window))) return;
	if (e->atom == XA_WM_NAME) get_name(c);
	else if (e->atom == XA_WM_HINTS) get_hints(c);
	//else if (e->atom == XA_WM_CLASS) apply_rules(c);
	else return;
	draw(2);
}

void unmapnotify(XEvent *ev) {
	Client *c, *t;
	XUnmapEvent *e = &ev->xunmap;
	if (!(c=wintoclient(e->window)) || e->send_event) return;
	int i;
	for (i = 0; i < 10; i++) if (winmarks[i] == c) winmarks[i] = NULL;
	if (c == clients) clients = c->next;
	else {
		for (t = clients; t && t->next && t->next != c; t = t->next);
		t->next = c->next;
	}
	XFree(c->title);
	// if (c->icon) cairo_surface_destroy(c->icon);
	free(c); c = NULL;
	draw(2);
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
	if (FT_Init_FreeType(&library)) die("unable to init freetype");
	scr = DefaultScreen(dpy);
	root = DefaultRootWindow(dpy);
	XSetErrorHandler(xerror);
	XDefineCursor(dpy,root,XCreateFontCursor(dpy,68));
	gc = DefaultGC(dpy,scr);

	config();
	icons_init(icons_path,BAR_HEIGHT(mons->container->bar.opts)-2);
	Monitor *M;
	for (M = mons; M; M = M->next) {
		if (M->x + M->w > purgX) purgX = M->x + M->w + 10;
		if (M->y + M->h > purgX) purgY = M->y + M->h + 10;
	}
	XSelectInput(dpy,root, ExposureMask | FocusChangeMask | ButtonReleaseMask |
			SubstructureNotifyMask | SubstructureRedirectMask |
			StructureNotifyMask | PropertyChangeMask);
	XClearWindow(dpy,root);
	/* key and mouse binding */
	unsigned int mod[] = {0, LockMask, Mod2Mask, LockMask|Mod2Mask};
	KeyCode code;
	XUngrabKey(dpy,AnyKey,AnyModifier,root);
	int i, j;
	for (i = 0; i < nkeys; i++)
		if ( (code=XKeysymToKeycode(dpy,key[i].keysym)) )
			for (j = 0; j < 4; j++)
				XGrabKey(dpy,code,key[i].mod|mod[j],root,True,
						GrabModeAsync,GrabModeAsync);
	code=XKeysymToKeycode(dpy,XK_Super_L);
	for (j = 0; j < 4; j++) {
		XGrabKey(dpy,code,mod[j],root,True,GrabModeAsync,GrabModeAsync);
		XGrabButton(dpy,1,Mod4Mask | mod[j], root, True,
				ButtonPressMask,GrabModeAsync,GrabModeAsync,None,None);
		XGrabButton(dpy,2,Mod4Mask | mod[j], root, True,
				ButtonPressMask,GrabModeAsync,GrabModeAsync,None,None);
		XGrabButton(dpy,3,Mod4Mask | mod[j], root, True,
				ButtonPressMask,GrabModeAsync,GrabModeAsync,None,None);
	}
	memset(winmarks,0,10*sizeof(Client *));
	running = True;
}

void X_free() {
	Client *c;
	for (c = clients; c; c = c->next) {
		winmarks[0] = c;
		key_chain("0q");
	}
	XFlush(dpy);
	icons_free();
	deconfig();
	XCloseDisplay(dpy);
}


