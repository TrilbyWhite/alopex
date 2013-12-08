
#ifndef __ALOPEX_H__
#define __ALOPEX_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
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

#define BAR_VISIBLE			0x0100
#define BAR_TOP				0x0200
#define BAR_HEIGHT(x)		(x & 0x00FF)
#define SET_BAR_HEIGHT(x)	x

#define RSTACK					0x01
#define BSTACK					0x02
#define MONOCLE					0x03

#define ATTACH_TOP			0x0001
#define ATTACH_BOTTOM		0x0002
#define ATTACH_ABOVE			0x0003
#define ATTACH_BELOW			0x0004

#define CMD(x)	x " &"

#define WIN_TRANSIENT		0x01
#define WIN_URGENT			0x02
#define WIN_FOCUS			0x04

/* Theme elements */
#define tabOffset				0x00
#define tabRGBAFocus			0x01
#define tabRGBAFocusBrd		0x02
#define tabRGBAFocusText	0x03
#define tabRGBATop			0x04
#define tabRGBATopBrd		0x05
#define tabRGBATopText		0x06
#define tabRGBAOther			0x07
#define tabRGBAOtherBrd		0x08
#define tabRGBAOtherText	0x09
#define statOffset			0x0a
#define statRGBA				0x0b
#define statRGBABrd			0x0c
#define statRGBAText			0x0d
#define statRGBAInput		0x0e
#define tagRGBAOcc			0x0f
#define tagRGBASel			0x10
#define tagRGBAAlt			0x11
#define tagRGBABoth			0x12
#define themeEND				0x13

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
	int tags, flags;
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
	cairo_surface_t *bg;
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
FT_Face face, face2;
Client *winmarks[10];
cairo_font_face_t *cfont, *cfont2;

extern int die(const char *);
extern int purgatory(Window);
extern int set_focus(Client *);

extern int reconfig();

extern int draw(int);
extern int draw_background(Container *);
extern int draw_status();
extern int draw_tab(Container *, int, Client *, int, int);
extern int icons_init(const char *, int);
extern int icons_free();
//extern double round_rect(cairo_t *, int, int, int, int, Bool, int, int, int, int);

extern int input(char *);

extern int key_chain(const char *);

extern int tile();

char **tag_names, **string;
char *status_fmt, *font_path, *font_path2, *icons_path, *ibar_text, *bg_path;
const char *theme_name;
char instring[256];
int font_size, container_pad, tag_pad, *containers, ncontainers;
int container_split;
unsigned short int tag_icon[16];
int client_opts, bar_opts, ntags, nkeys;
int statfd, chain_delay;
Bool focusfollowmouse;
Theme *theme;
Key *key;

#endif /* __ALOPEX_H__ */

