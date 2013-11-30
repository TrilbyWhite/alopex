
#ifndef __ALOPEX_H__
#define __ALOPEX_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <locale.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-ft.h>

#define BAR_TOP				0x0100
#define BAR_BOTTOM			0x0200
#define BAR_VISIBLE			0x0400

#define BAR_HEIGHT(x)		(x & 0x00FF)
#define SET_BAR_HEIGHT(x)	x

#define RSTACK					0x1000
#define BSTACK					0x2000

#define ATTACH_TOP			0x0001
#define ATTACH_BOTTOM		0x0002
#define ATTACH_ABOVE			0x0003
#define ATTACH_BELOW			0x0004

/* Theme elements */
#define tabOffset				0x01
#define tabRGBAFocus			0x02
#define tabRGBATop			0x03
#define tabRGBAOther			0x04
#define tabRGBAFocusBrd		0x05
#define tabRGBATopBrd		0x06
#define tabRGBAOtherBrd		0x07
#define tabRGBAFocusText	0x08
#define tabRGBATopText		0x09
#define tabRGBAOtherText	0x0a
#define statOffset			0x0b
#define statRGBA				0x0c
#define statRGBABrd			0x0d
#define statRGBAText			0x0e
#define tagRGBAOcc			0x10
#define tagRGBASel			0x11
#define tagRGBAFoc			0x12
#define tagRGBAAlt			0x13
#define tagRGBABoth			0x14

typedef struct Key {
	unsigned short int mod;
	KeySym keysym;
	const char *arg;
} Key;

typedef struct Client Client;
struct Client {
	Client *next;
	Window win;
	char *title;
	XWMHints *hints;
	int tags;
};

typedef struct Bar {
	Window win;
	Pixmap buf;
	cairo_t *ctx;
	int opts;
} Bar;

typedef struct SBar {
	cairo_surface_t *buf;
	cairo_t *ctx;
	int x, width, height;
} SBar;

typedef struct Container Container;
struct Container {
	Container *next;
	int n, w;
	Bar bar;
	Client *top;
};

typedef struct Monitor Monitor;
struct Monitor {
	Monitor *next;
	int x, y, w, h, gap;
	int tags, occ, mode, split;
	Container *container;
	Container *focus;
	SBar sbar[2];
};

typedef struct Theme {
	double a, b, c, d, e;
} Theme;

Monitor *mons;
Monitor *m;
Client *clients;
Display *dpy;
int scr;
Window root;
Bool running;
GC gc;
FT_Library library;
FT_Face face;
Client *winmarks[10];

extern const char *status_fmt;
extern const char *string[];
extern const char *tag_names[];
extern const int tag_pad;
extern const Theme theme[];

#endif /* __ALOPEX_H__ */

