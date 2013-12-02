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
#include "config.h"

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
static cairo_font_face_t *cfont;

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int main(int argc, const char **argv) {
	X_init();
	XEvent ev;
	draw();
	while (running && !XNextEvent(dpy,&ev))
		if (ev.type < 33 && handler[ev.type]) handler[ev.type](&ev);
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

void configurerequest(XEvent *ev) {
// Not working ... at all.
	XConfigureRequestEvent *e = &ev->xconfigurerequest;
	Client *c;
	if ( (c=wintoclient(e->window)) ) {
		if ( (e->value_mask & CWWidth) || (e->value_mask & CWHeight) ) {
			if ( (e->width == m->w) && (e->height == m->h) ) winmarks[2] = c;
//			if (c->flags & WIN_FLOAT)
//				XMoveResizeWindow(dpy,c->win,e->x, e->y, e->width, e->height);
		}
		draw();
		return;
	}
return;
	XWindowChanges wc;
	wc.x = e->x; wc.y = e->y; wc.width = e->width; wc.height = e->height;
	wc.sibling = e->above; wc.stack_mode = e->detail;
	XConfigureWindow(dpy,e->window,e->value_mask,&wc);
	XFlush(dpy);
}

void enternotify(XEvent *ev) {
	Client *c = wintoclient(ev->xcrossing.window);
	if (c && focusfollowmouse) {
		// TODO
	}
	if (mons->next) {
		// root_x root_y, which monitor
		// m = mons[i]
	}
	if ((c && focusfollowmouse) || mons->next) draw();
}

void expose(XEvent *ev) {
	draw();
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
	for (i = 0; i < sizeof(key)/sizeof(key[0]); i++)
		if ( (sym==key[i].keysym) && key[i].arg && key[i].mod==MOD(e) ) {
			ret = key_chain(key[i].arg);
			mod_down = False;
		}
	if (ret) draw();
}

void keyrelease(XEvent *ev) {
	XKeyEvent *e = &ev->xkey;
	KeySym sym = XkbKeycodeToKeysym(dpy,(KeyCode)e->keycode,0,0);
	if (sym != XK_Super_L || !mod_down) return;
	mod_down = False;
	char str[256]; int ret = 0;
	if (input(str)) ret = key_chain(str);
	if (ret) draw();
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
	draw();
}

void propertynotify(XEvent *ev) {
	XPropertyEvent *e = &ev->xproperty;
	Client *c;
	if (!(c=wintoclient(e->window))) return;
	if (e->atom == XA_WM_NAME) get_name(c);
	else if (e->atom == XA_WM_HINTS) get_hints(c);
	//else if (e->atom == XA_WM_CLASS) apply_rules(c);
	else return;
	draw();
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

static cairo_t *X_init_cairo_img_create(cairo_surface_t **buf, int w, int h) {
	*buf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,w,h);
	cairo_t *ctx = cairo_create(*buf);
	cairo_set_line_join(ctx,CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_width(ctx,theme[tabRGBAFocus].e);
	cairo_set_font_face(ctx,cfont);
	cairo_set_font_size(ctx,font_size);
	return ctx;
}

static cairo_t *X_init_cairo_create(Pixmap *buf, int w, int h) {
	*buf = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy,scr));
	cairo_surface_t *t;
	t = cairo_xlib_surface_create(dpy, *buf, DefaultVisual(dpy,scr),w,h);
	cairo_t *ctx = cairo_create(t);
	cairo_surface_destroy(t);
	cairo_set_line_join(ctx,CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_width(ctx,theme[statRGBA].e);
	cairo_set_font_face(ctx,cfont);
	cairo_set_font_size(ctx,font_size);
	return ctx;
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
	if (FT_Init_FreeType(&library) |
			FT_New_Face(library,font_path,0,&face) |
			FT_Set_Pixel_Sizes(face,0,font_size) )
		die("unable to init freetype lib and load font");
	cfont = cairo_ft_font_face_create_for_ft_face(face,0);
	/* monitors */
	if (!(mons=calloc(1,sizeof(Monitor))))
		die("unable to allocatememory");
	m = mons;
	Monitor *M;
	Container *C, *CC = NULL;
	int i,j, bh = (BAR_HEIGHT(bar_opts) ? BAR_HEIGHT(bar_opts) :
			font_size+4);
	icons_init(icons_path,bh);
	wa.override_redirect = True;
	wa.event_mask = ExposureMask;
	wa.background_pixmap = ParentRelative;
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
			C->bar.opts = bar_opts | bh;
			// TODO replace "12" with font height
			C->bar.win = XCreateSimpleWindow(dpy, root, M->x,
					(bar_opts & BAR_TOP ? M->y : M->y + M->h - bh),
					M->w, bh, 0, 0, 0);
			XChangeWindowAttributes(dpy,C->bar.win,CWOverrideRedirect |
					CWEventMask | CWBackPixmap, &wa);
			C->bar.ctx = X_init_cairo_create(&C->bar.buf,M->w,bh);
			XMapWindow(dpy,C->bar.win);
			if (!M->container) M->container = C;
			if (CC) CC->next = C;
			CC = C;
		}
		M->sbar[0].width = M->w/2;
		M->sbar[1].width = M->w/2;
		M->sbar[0].height = BAR_HEIGHT(M->container->bar.opts);
		M->sbar[1].height = M->sbar[0].height;
		M->sbar[0].ctx = X_init_cairo_img_create(&M->sbar[0].buf,
				M->sbar[0].width,M->sbar[0].height);
		M->sbar[1].ctx = X_init_cairo_img_create(&M->sbar[1].buf,
				M->sbar[1].width,M->sbar[1].height);
		//M->tags = 1;
		M->mode = RSTACK;
		M->focus = M->container;
		if (M->x + M->w > purgX) purgX = M->x + M->w + 10;
		if (M->y + M->h > purgX) purgY = M->y + M->h + 10;
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
	memset(winmarks,0,10*sizeof(Client *));
	running = True;
}

void X_free() {
	Client *c;
	Monitor *M;
	Container *C, *CC = NULL;
	for (c = clients; c; c = c->next) {
		winmarks[0] = c;
		key_chain("0q");
	}
	XFlush(dpy);
	for (M = mons; M; M = M->next) {
		cairo_surface_destroy(M->sbar[0].buf);
		cairo_destroy(M->sbar[0].ctx);
		cairo_surface_destroy(M->sbar[1].buf);
		cairo_destroy(M->sbar[1].ctx);
		for (C = M->container; C; CC = C, C = C->next) {
			if (CC) free(CC);
			cairo_destroy(C->bar.ctx);
			XFreePixmap(dpy,C->bar.buf);
			XDestroyWindow(dpy,C->bar.win);
		}
	}
	icons_free();
	cairo_font_face_destroy(cfont);
	FT_Done_Face(face);
	free(C);
	free(mons);
	XCloseDisplay(dpy);
}


