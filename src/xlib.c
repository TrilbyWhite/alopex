
#include "alopex.h"
#include "actions.h"

#define SELECT_EVENTS	\
	ExposureMask | FocusChangeMask | ButtonReleaseMask | \
	SubstructureNotifyMask | SubstructureRedirectMask | \
	StructureNotifyMask | PropertyChangeMask

#define GRABMODE	GrabModeAsync, GrabModeAsync
#define GRABMODE2	ButtonPressMask, GrabModeAsync, GrabModeAsync, \
	None, None

extern int config_init(const char *);
extern int config_free();
extern int draw_bars(Bool);
extern int sbar_parse(Bar *, const char *);
extern char *get_text(Client *, int);

static int apply_rules(Client *);
static int get_hints(Client *);
static int get_icon(Client *);
static int get_name(Client *);

static void buttonpress(XEvent *);
static void configurerequest(XEvent *);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void keypress(XEvent *);
//static void keyrelease(XEvent *);
static void propertynotify(XEvent *);
static void maprequest(XEvent *);
static void unmapnotify(XEvent *);

//static Bool mod_down = False;
static int purgX, purgY;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress]       = buttonpress,
	[ConfigureRequest]  = configurerequest,
	[EnterNotify]       = enternotify,
	[Expose]            = expose,
	[KeyPress]          = keypress,
//	[KeyRelease]        = keyrelease,
	[MapRequest]        = maprequest,
	[PropertyNotify]    = propertynotify,
	[UnmapNotify]       = unmapnotify,
};

int free_mons(const char *bg, const char *cont) {
	int i;
	Monitor *M;
	Container *C;
	for (M = mons; M; M = M->next) {
		for (C = M->container; C; C = C->next) {
			cairo_destroy(C->bar->ctx);
			cairo_surface_destroy(C->bar->buf);
			free(C->bar);
			cairo_destroy(C->ctx);
			XDestroyWindow(dpy, C->win);
		}
		cairo_destroy(M->tbar.ctx);
		cairo_surface_destroy(M->tbar.buf);
		free(M->container);
		cairo_surface_destroy(M->bg);
	}
	for (i = 0; i < MAX_STATUS; i++) {
		cairo_destroy(sbar[i].ctx);
		cairo_surface_destroy(sbar[i].buf);
	}
	free(mons);
	return 0;
}

int get_mons(const char *bg, const char *cont) {
	/* get container counts */
	int i, j, n, *counts = NULL, ncounts;
	char *tok = NULL, *tmp = strdup(cont);
	tok = strtok(tmp," ");
	for (i = 0; tok; i++) {
		counts = realloc(counts, (i+1) * sizeof(int *));
		counts[i] = atoi(tok);
		tok = strtok(NULL," ");
	}
	free(tmp);
	counts = realloc(counts, (i+1) * sizeof(int *));
	counts[i] = -1;
	ncounts = i+1;
	/* get monitor geometries */
	mons = NULL;
	XineramaScreenInfo *geom = XineramaQueryScreens(dpy, &n);
	int minX = 0, minY = 0, maxX = 0, maxY = 0;
	for(i = 0; i < n; i++) {
		mons = realloc(mons, (i+1) * sizeof(Monitor));
		memset(&mons[i], 0, sizeof(Monitor));
		mons[i].x = geom[i].x_org;
		mons[i].y = geom[i].y_org;
		mons[i].w = geom[i].width;
		mons[i].h = geom[i].height;
		if (minX > mons[i].x) minX = mons[i].x;
		if (minY > mons[i].y) minY = mons[i].y;
		if (maxX < mons[i].x + mons[i].w) maxX = mons[i].x + mons[i].w;
		if (maxY < mons[i].y + mons[i].h) maxY = mons[i].y + mons[i].h;
	}
	/* create monitors and set background */
	cairo_surface_t *src, *dest;
	src = cairo_image_surface_create_from_png(bg);
if (cairo_surface_status(src) != CAIRO_STATUS_SUCCESS) {
	src = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, maxX, maxY);
	cairo_t *blank = cairo_create(src);
	cairo_set_source_rgba(blank, 0, 0, 0, 1);
	cairo_paint(blank);
	cairo_destroy(blank);
}
	int imgw = cairo_image_surface_get_width(src);
	int imgh = cairo_image_surface_get_height(src);
	Pixmap pix = XCreatePixmap(dpy, root, maxX-minX, maxY-minY,
			DefaultDepth(dpy,scr));
	dest = cairo_xlib_surface_create(dpy, pix, DefaultVisual(dpy,scr),
			maxX-minX, maxY-minY);
	cairo_t *ctx, *rctx;
	rctx = cairo_create(dest);
	Monitor *M;
	for(i = 0; i < n; i++) {
		M = &mons[i];
		if (i < n - 1) M->next = &mons[i+1];
		M->gap = conf.gap;
		M->split = conf.split;
		M->container = calloc(ncounts, sizeof(Container));
		M->focus = mons[i].container;
		M->mode = conf.mode;
		M->bg = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,M->w,M->h);
		M->margin = conf.margin;
		/* paint monitor bg */
		ctx = cairo_create(M->bg);
		cairo_scale(ctx, M->w / (float) imgw, M->h / (float) imgh);
		cairo_set_source_surface(ctx, src, 0, 0);
		cairo_paint(ctx);
		cairo_destroy(ctx);
		/* paint root pixmap */
		cairo_save(rctx);
		cairo_scale(rctx, M->w / (float) imgw, M->h / (float) imgh);
		cairo_translate(rctx, M->x, M->y);
		cairo_set_source_surface(rctx, src, 0, 0);
		cairo_paint(rctx);
		cairo_restore(rctx);
		Container *C;
		cairo_surface_t *t;
		for (j = 0; j < ncounts; j++) {
			C = &M->container[j];
			if (j < ncounts - 1) C->next = &M->container[j+1];
			C->n = counts[j];
			C->nn = C->n;
			C->bar = calloc(1, sizeof(Bar));
			C->bar->opts = conf.bar_opts;
			C->bar->w = mons[i].w;
			C->bar->h = conf.bar_opts & BAR_HEIGHT;
			C->bar->buf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
					C->bar->w, C->bar->h);
			C->bar->ctx = cairo_create(C->bar->buf);
			cairo_set_font_face(C->bar->ctx, conf.font);
			cairo_set_font_size(C->bar->ctx, conf.font_size);
			C->win = XCreateSimpleWindow(dpy,root,0,0,M->w,C->bar->h,0,0,0);
			XSelectInput(dpy, C->win, SELECT_EVENTS);
			t = cairo_xlib_surface_create(dpy, C->win,
					DefaultVisual(dpy,scr), M->w, C->bar->h);
			C->ctx = cairo_create(t);
			cairo_surface_destroy(t);
			XMapWindow(dpy, C->win);
		}
		M->tbar.buf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				mons->w / 2, conf.bar_opts & BAR_HEIGHT);
		M->tbar.ctx = cairo_create(M->tbar.buf);
		M->tbar.h = conf.bar_opts & BAR_HEIGHT;
		cairo_set_font_face(M->tbar.ctx, conf.font);
		cairo_set_font_size(M->tbar.ctx, conf.font_size);
	}
	for (i = 0; i < MAX_STATUS; i++) {
		sbar[i].buf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
				mons->w / 2, conf.bar_opts & BAR_HEIGHT);
		sbar[i].ctx = cairo_create(sbar[i].buf);
		sbar[i].h = conf.bar_opts & BAR_HEIGHT;
		cairo_set_font_face(sbar[i].ctx, conf.font);
		cairo_set_font_size(sbar[i].ctx, conf.font_size);
	}
	cairo_surface_destroy(src);
	cairo_surface_destroy(dest);
	cairo_destroy(rctx);
	XSetWindowBackgroundPixmap(dpy, root, pix);
	XFreePixmap(dpy, pix);
	m = mons;
	free(counts);
	return 0;
}

#define MAX_LINE	256
int main_loop() {
	char line[MAX_LINE], *c;
	fd_set fds;
	int n, redraw, ret;
	int xfd = ConnectionNumber(dpy);
	int nfd = (conf.statfd > xfd ? conf.statfd : xfd) + 1;
	XEvent ev;
	while (running) {
		FD_ZERO(&fds);
		FD_SET(xfd, &fds);
		FD_SET(conf.statfd, &fds);
		select(nfd, &fds, 0, 0, NULL);
		if (conf.statfd && FD_ISSET(conf.statfd, &fds)) {
			fgets(line, MAX_LINE, conf.stat);
			redraw = sbar_parse(&sbar[(n=0)], (c=line));
			while ( (c=strchr(c, '&')) && (++n) < MAX_STATUS)
				redraw += sbar_parse(&sbar[n], (++c));
			if (redraw) tile();
			else draw_bars(True);
			XFlush(dpy);
		}
		if (FD_ISSET(xfd, &fds)) while (XPending(dpy)) {
			XNextEvent(dpy, &ev);
			if (ev.type < LASTEvent && handler[ev.type])
				handler[ev.type](&ev);
		}
	}
	return 0;
}

int purgatory(Window w) {
	XMoveWindow(dpy, w, purgX, purgY);
}

int set_focus() {
	Client *c = winmarks[1];
	if (!c) return 1;
	Window win; int rev;
	/* raise window */
	if (c->flags & WIN_FLOAT) {
//if (!(conditional for 0ad type windows))
		XRaiseWindow(dpy, c->win);
	}
	else {
		XWindowChanges wc;
		wc.sibling = m->container->win;
		wc.stack_mode = Below;
		XConfigureWindow(dpy, c->win, CWSibling | CWStackMode, &wc);
	}
	/* set input focus */
	if (c->flags & WIN_FOCUS) send_message(c, WM_PROTOCOLS, WM_TAKE_FOCUS);
	else XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
	c->flags &= ~WIN_URGENT;
	return 0;
}

Client *wintoclient(Window w) {
	Client *c;
	for (c = clients; c; c = c->next)
		if (c->win == w) return c;
	return NULL;
}

int xerror(Display *d, XErrorEvent *e) {
	char msg[1024];
	XGetErrorText(d, e->error_code, msg, sizeof(msg));
	fprintf(stderr,"[X11 %d:%d] %s\n",
			e->request_code, e->error_code, msg);
}

int xlib_init(const char *theme_name) {
	/* INITS */
	if (!setlocale(LC_CTYPE,"")) die("unable to set locale");
	if (!XSupportsLocale()) die("unsupported locale");
	if (!XSetLocaleModifiers("")) die("unable to set locale modifiers");
	if (!(dpy=XOpenDisplay(0x0))) die("unable to open X display");
	if (FT_Init_FreeType(&ftlib)) die("unable to init freetype");
	/* GLOBALS */
	scr = DefaultScreen(dpy);
	root = DefaultRootWindow(dpy);
	XSetErrorHandler(xerror);
	XDefineCursor(dpy, root, XCreateFontCursor(dpy,68));
	gc = DefaultGC(dpy, scr);
	/* CONFIG */
	config_init(theme_name);
	Monitor *M;
	for (M = mons; M; M = M->next) {
		if (M->x + M->w > purgX) purgX = M->x + M->w + 10;
		if (M->y + M->h > purgY) purgY = M->y + M->h + 10;
	}
	XClearWindow(dpy,root);
	/* BINDING + GRABS */
//XSetWindowAttributes wa;
//wa.event_mask = SELECT_EVENTS;
//XChangeWindowAttributes(dpy,root,CWEventMask,&wa);
	XSelectInput(dpy, root, SELECT_EVENTS);
	unsigned int mod[] = {0, LockMask, Mod2Mask, LockMask|Mod2Mask};
	KeyCode code;
	Key *key;
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	XSync(dpy,True);
	int i, j;
	for (i = 0; i < conf.nkeys; i++) {
		key = &conf.key[i];
		if ( (code=XKeysymToKeycode(dpy, key->keysym)) ) {
			for (j = 0; j < 4; j++)
				XGrabKey(dpy, code, key->mod|mod[j], root, True, GRABMODE);
		}
	}
//	code = XKeysymToKeycode(dpy, XK_Super_L);
	for (j = 0; j < 4; j++) {
//		XGrabKey(dpy, code, mod[j], root, True, GRABMODE);
		XGrabButton(dpy,1,Mod4Mask|mod[j],root,True,GRABMODE2);
		XGrabButton(dpy,2,Mod4Mask|mod[j],root,True,GRABMODE2);
		XGrabButton(dpy,3,Mod4Mask|mod[j],root,True,GRABMODE2);
	}
	memset(winmarks, 0, 10*sizeof(Client *));
	tile();
	XFlush(dpy);
	running = True;
	atoms_init();
	return 0;
}

int xlib_free() {
	Client *c;
	for (c = clients; c; c = c->next) killclient(c);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	XUngrabButton(dpy, AnyButton, AnyModifier, root);
	XFlush(dpy);
	config_free();
	FT_Done_FreeType(ftlib);
	XCloseDisplay(dpy);
	return 0;
}

/**** LOCAL FUNCTIONS ****/

int apply_rules(Client *c) {
	XClassHint *hint = XAllocClassHint();
	XGetClassHint(dpy, c->win, hint);
	int i, m;
	const char *rc, *rn, *hc = hint->res_class, *hn = hint->res_name;
	for (i = 0; i < conf.nrules; i++) {
		rc = conf.rule[i].class;
		rn = conf.rule[i].name;
		m = 0;
		if (rc && hc && !strncmp(rc, hc, strlen(rc))) m++;
		if (rn && hn && !strncmp(rn, hn, strlen(rn))) m++;
		if ( (m && !(rc && rn)) || (m == 2) ) {
			if (conf.rule[i].tags > 0) c->tags = conf.rule[i].tags;
			c->flags |= conf.rule[i].flags;
		}
	}
	XFree(hint);
	return 0;
}

int get_hints(Client *c) {
	XWMHints *hint;
	if ( !(hint = XGetWMHints(dpy,c->win)) ) return;
	if (hint->flags & XUrgencyHint) c->flags |= WIN_URGENT;
	if (hint->flags & InputHint && !hint->input) c->flags |= WIN_FOCUS;
	if (hint->flags & (IconPixmapHint | IconMaskHint)) get_icon(c);
	XFree(hint);
	// TODO EWMH checks
	return 0;
}

int get_icon(Client *c) {
	// TODO: needs testing
	XWMHints *hint;
	if (!(hint=XGetWMHints(dpy,c->win))) return;
	if (!(hint->flags & (IconPixmapHint | IconMaskHint))) return;
	if (c->icon) { cairo_surface_destroy(c->icon); c->icon = NULL; }
	int w, h, ig, sz = conf.font_size;
	Window wig;
	XGetGeometry(dpy, hint->icon_pixmap, &wig, &ig, &ig, &w, &h, &ig, &ig);
	if (c->icon) cairo_surface_destroy(c->icon);
	cairo_surface_t *icon, *mask = NULL;
	icon = cairo_xlib_surface_create(dpy, hint->icon_pixmap,
			DefaultVisual(dpy,scr), w, h);
	if (hint->flags & IconMaskHint)
	//mask = cairo_xlib_surface_create(dpy, hint->icon_mask,
	//		DefaultVisual(dpy,scr), w, h);
	mask = cairo_xlib_surface_create_for_bitmap(dpy,
			hint->icon_mask, DefaultScreenOfDisplay(dpy), w, h);
	c->icon = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, sz, sz);
	cairo_t *ctx = cairo_create(c->icon);
	cairo_scale(ctx, sz / (double) w, sz / (double) h);
	cairo_set_source_surface(ctx, icon, 0, 0);
	if (mask) cairo_mask_surface(ctx, mask, 0, 0);
	else cairo_paint(ctx);
	cairo_destroy(ctx);
	cairo_surface_destroy(icon);
	if (mask) cairo_surface_destroy(mask);
	return 0;
}

int get_name(Client *c) {
	static const char *noname = "(no name)";
	Client *p;
	if (c->title && c->title != noname) XFree(c->title);
	if ( !(c->title=get_text(c,NET_WM_NAME)) )
		if ( !(c->title=get_text(c,XA_WM_NAME)) )
			c->title = strdup(noname);
	return 0;
}


/**** EVENT HANDLERS ****/

void buttonpress(XEvent *ev) {
	XButtonEvent *e = &ev->xbutton;
	Client *c;
	if ( !(c=wintoclient(e->subwindow)) ) return;
	if (c->flags & WIN_FLOAT) XRaiseWindow(dpy, c->win);
	if (c != winmarks[1]) {
		winmarks[0] = winmarks[1];
		winmarks[1] = c;
		tile();
	}
	int b = e->button;
	if (b > 3) return;
	c->flags &= ~WIN_FULL;
	if (b == 2) {
		tile();
		return;
	}
	XGrabPointer(dpy, root, True, PointerMotionMask | ButtonReleaseMask,
			GRABMODE, None, None, CurrentTime);
	XEvent ee;
	int dx, dy;
	int xx = e->x_root, yy = e->y_root;
	while (True) {
		//XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		expose(ev);
		XMaskEvent(dpy, PointerMotionMask | ButtonReleaseMask, &ee);
		if (ee.type == ButtonRelease) break;
if (!(c->flags & WIN_FLOAT)) {
	c->flags |= WIN_FLOAT;
	tile();
if (xx < c->x) c->x = xx;
if (xx > c->x + c->w) c->x = xx - c->w;
if (yy < c->y) c->y = yy;
if (yy > c->y + c->h) c->y = yy - c->h;
}
		XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		dx = ee.xbutton.x_root - xx; xx = ee.xbutton.x_root;
		dy = ee.xbutton.y_root - yy; yy = ee.xbutton.y_root;
		if (b == 1) { c->x += dx; c->y += dy; }
		else if (b == 3) { c->w += dx; c->h += dy; }
	}
	XUngrabPointer(dpy, CurrentTime);
}

void configurerequest(XEvent *ev) {
fprintf(stderr,"CONFIGURE\n");
	XConfigureRequestEvent *e = &ev->xconfigurerequest;
	Client *c;
	if ( (c=wintoclient(e->window)) ) {
		if ( (e->value_mask & CWWidth) && (e->value_mask & CWHeight) ) {
			if ( (e->width == m->w) && (e->height == m->h) ) {
				c->flags |= WIN_FULL;
				tile();
			}
			else {
				if (e->value_mask & CWWidth) c->w = e->width;
				if (e->value_mask & CWHeight) c->w = e->height;
				// TODO x and y
			}
		}
	}
	if (!c || (c->flags & WIN_FLOAT) ) {
fprintf(stderr,"-- CONFIGURE\n");
if (e->window == root) return;
fprintf(stderr,">> CONFIGURE\n");
		XWindowChanges wc;
		wc.x=e->x; wc.y=e->y; wc.width=e->width; wc.height=e->height;
		wc.sibling = e->above; wc.stack_mode = e->detail;
		XConfigureWindow(dpy, e->window, e->value_mask, &wc);
		XFlush(dpy);
	}
}

void enternotify(XEvent *ev) {
	// TODO check monitors
	if (!conf.focus_follow) return;
	Client *c = wintoclient(ev->xcrossing.window);
	if (!c) return;
	Container *C;
	for (C = m->container; C; C = C->next)
		if (C->top && C->top == c) m->focus = C;
	tile();
}

void expose(XEvent *ev) {
	Monitor *M; Container *C;
//	draw_bars(True);
	for (M = mons; M; M = M->next) {
		for (C = M->container; C; C = C->next){
			cairo_set_source_surface(C->ctx, C->bar->buf, 0, 0);
			cairo_paint(C->ctx);
		}
	}
}

void keypress(XEvent *ev) {
	int i;
	XKeyEvent *e = &ev->xkey;
	KeySym sym = XkbKeycodeToKeysym(dpy, e->keycode, 0, 0);
//	if (sym == XK_Super_L) mod_down = True;
	for (i = 0; i < conf.nkeys; i++) {
		if ( (sym==conf.key[i].keysym) && (conf.key[i].arg) &&
				conf.key[i].mod == ((e->state&~Mod2Mask)&~LockMask) ) {
			command(conf.key[i].arg);
			//mod_down = False;
		}
	}
}

//void keyrelease(XEvent *ev) {
//	XKeyEvent *e = &ev->xkey;
//	KeySym sym = XkbKeycodeToKeysym(dpy, e->keycode, 0, 0);
//	if (sym != XK_Super_L || !mod_down) return;
//	mod_down = False;
//	char str[MAX_LINE];
//	if (input(str)) command(str);
//}

void maprequest(XEvent *ev) {
	XMapRequestEvent *e = &ev->xmaprequest;
	XWindowAttributes wa;
	if (	!XGetWindowAttributes(dpy, e->window, &wa) ||
			wa.override_redirect ) return;
	Client *c;
	if ( (c=wintoclient(e->window)) ) return;
	c = calloc(1, sizeof(Client));
	c->win = e->window;
// check for fullscreen
	get_hints(c);
	get_icon(c);
	get_name(c);
	apply_rules(c);
	if (!c->tags && !(c->tags=m->tags)) c->tags = m->tags = 1;
	if (XGetTransientForHint(dpy, c->win, &c->parent))
		c->flags |= WIN_TRANS;
	else
		c->parent = e->parent;
	c->x = wa.x; c->y = wa.y; c->w = wa.width; c->h = wa.height;
	if (c->w == m->w && c->h == m->h) c->flags |= WIN_FULL;
	XSelectInput(dpy, c->win, PropertyChangeMask | EnterWindowMask);
	Client *p;
	switch (conf.attach) {
		case ATTACH_TOP: c->next = clients; clients = c; break;
		case ATTACH_BOTTOM: push_client(c, NULL); break;
		case ATTACH_ABOVE: push_client(c, m->focus->top); break;
		case ATTACH_BELOW: push_client(c, m->focus->top->next); break;
	}
	winmarks[0] = winmarks[1];
	winmarks[1] = c;
	purgatory(c->win);
	XMapWindow(dpy, c->win);
	tile();
}

void propertynotify(XEvent *ev) {
	XPropertyEvent *e = &ev->xproperty;
	Client *c;
	if (e->window == root) {
		char *cmd;
		if (!XFetchName(dpy, root, &cmd)) return;
		if (strncmp("ALOPEX: ", cmd, 8) == 0) command(cmd + 8);
		if (cmd) XFree(cmd);
		return;
	}
	if ( !(c=wintoclient(e->window)) ) return;
	if (e->atom == XA_WM_NAME) get_name(c);
	else if (e->atom == XA_WM_HINTS) get_hints(c);
	else if (e->atom == XA_WM_CLASS) apply_rules(c);
	// icon ?
	else return;
	tile();
}

void unmapnotify(XEvent *ev) {
	Client *c, *t;
	XUnmapEvent *e = &ev->xunmap;
	if (!(c=wintoclient(e->window))) return;
	winmarks[1] = winmarks[0];
	winmarks[0] = NULL;
	if (c == clients) clients = c->next;
	else {
		for (t = clients; t && t->next && t->next != c; t = t->next);
		t->next = c->next;
	}
	XFree(c->title);
	free(c); c = NULL;
	tile();
}

