
#ifndef __ALOPEX_H__
#define __ALOPEX_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xresource.h>
#include <X11/extensions/Xinerama.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-ft.h>

#define BAR_BOTTOM		0x1000
#define BAR_HIDE			0x2000
#define BAR_HEIGHT		0x00FF
#define ATTACH_TOP		0x0000
#define ATTACH_ABOVE		0x0001
#define ATTACH_BELOW		0x0002
#define ATTACH_BOTTOM	0x0003
#define MONOCLE			0x0000
#define RSTACK				0x0001
#define BSTACK				0x0002
#define LAST_MODE			0x0003

#define WIN_FLOAT       0x0001
#define WIN_FULL_TEST   0x0002
#define WIN_FULL        0x0003
#define WIN_TRANS       0x0005
#define WIN_FOCUS       0x0010
#define WIN_URGENT      0x0020

#define NWINMARKS			10

#define MAX_STATUS 4
/* pre-tags, post-tags, 2nd right, last right */

enum {
	TabOffset,
	TabBackground, TabBorder, TabText,
	TabFocusBackground, TabFocusBorder, TabFocusText,
	TabTopBackground, TabTopBorder, TabTopText,
	StatusOffset,
	StatusBackground, StatusBorder, StatusText,
	TagOccupied, TagView, TagAlt, TagBoth,
	ThemeLast
};

enum {
	WM_PROTOCOLS, WM_DELETE_WINDOW, WM_STATE, WM_TAKE_FOCUS,
	WM_NAME,
	NET_SUPPORTED, NET_WM_NAME, NET_WM_STATE,
	NET_WM_STATE_FULLSCREEN, NET_ACTIVE_WINDOW,
	NET_WM_WINDOW_TYPE, NET_WM_TYPE_DIALOG, NET_CLIENT_LIST,
	ATOM_LAST
};

enum { TAG_ICON_TEXT, TAG_TEXT_ICON, TAG_TEXT, TAG_ICON };

typedef struct Key {
	unsigned short int mod;
	KeySym keysym;
	const char *arg;
} Key;

typedef struct Client Client;
struct Client {
	Client *next;
	Window win, parent;
	char *title;
	cairo_surface_t *icon;
	int tags, flags, x, y, w, h;
};

typedef struct Rule {
	const char *name;
	const char *class;
	int tags;
	int flags;
} Rule;

typedef struct Bar {
	cairo_surface_t *buf;
	cairo_t *ctx;
	int opts;
	int xoff, w, h;
} Bar;

typedef struct Container Container;
struct Container {
	Container *next;
	Window win;
	cairo_t *ctx;
	int x, y, w, h, n, nn;
	int left_pad, right_pad;
	Bar *bar;
	Client *top;
};

typedef struct Margin {
	int top, bottom, left, right;
} Margin;

typedef struct Monitor Monitor;
struct Monitor {
	Bar tbar;
	Monitor *next;
	Margin margin;
	int x, y, w, h, gap;
	int tags, occ, mode, split;
	Container *container, *focus;
	cairo_surface_t *bg;
};

typedef union Theme {
	struct { double x, y, w, h, r; };
	struct { double R, G, B, A, e; };
} Theme;

typedef struct Config {
	cairo_font_face_t *font, *bfont;
	Theme theme[ThemeLast];
	Margin margin;
	int nkeys;
	Key *key;
	int nrules;
	Rule *rule;
	int statfd;
	FILE *stat;
	char **tag_name;
	int *tag_icon, tag_count, tag_mode;
	int gap, split, mode, bar_pad, chain_delay, bar_opts, attach;
	int font_size;
	Bool focus_follow;
} Config;

Monitor *mons, *m;
Client *clients, *winmarks[NWINMARKS+1];
Display *dpy;
int scr;
Window root;
GC gc;
Atom atom[ATOM_LAST];
Bool running;
FT_Library ftlib;
Config conf;
Bar sbar[MAX_STATUS];

extern void die(const char *, ...);
extern int tile();

#endif /* __ALOPEX_H__ */
