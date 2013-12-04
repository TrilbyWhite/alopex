/********************************************************************\
* DRAW.C - part of alopex, see alopex.c for license information
*
* Author: Jesse McClure, copyright 2012-2013
\********************************************************************/

/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

#include "alopex.h"

static void sbar_clock(SBar *, char);
static void sbar_parse(SBar *, int);
static void sbar_tags(Monitor *, SBar *, char);
static void sbar_text(SBar *, const char *);
static void set_color(cairo_t *, int);

static cairo_surface_t *icon_img;
static int icon_size;

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int icons_init(const char *fname, int size) {
return 0;
	cairo_surface_t *img = cairo_image_surface_create_from_png(fname);
	int w = cairo_image_surface_get_width(img);
	int h = cairo_image_surface_get_height(img);
	icon_img = cairo_image_surface_create(0, size * 10, size * 10);
	cairo_t *ctx = cairo_create(icon_img);
	cairo_scale(ctx, size * 10 / w, size * 10 / h);
	cairo_set_source_surface(ctx, img, 0, 0);
	cairo_paint(ctx);
	cairo_surface_destroy(img);
	cairo_destroy(ctx);
	icon_size = size;
	return 0;
}

int icons_free() {
return 0;
	cairo_surface_destroy(icon_img);
	return 0;
}

double round_rect(Bar *bar, int x, int y, int w, int h,
		int off, int bg, int brd, int txt) {
	const Theme *q = &theme[off];
	cairo_new_sub_path(bar->ctx);
if (!(bar->opts & BAR_TOP)) {
cairo_save(bar->ctx);
cairo_scale(bar->ctx,1,-1);
cairo_translate(bar->ctx,0,-1 * BAR_HEIGHT(bar->opts));
}
	x += q->a; y += q->b; w += q->c; h += q->d;
	cairo_arc(bar->ctx, x + w - q->e, y + q->e, q->e, -0.5 * M_PI, 0);
	cairo_arc(bar->ctx, x + w - q->e, y + h - q->e, q->e, 0, 0.5 * M_PI);
	cairo_arc(bar->ctx, x + q->e, y + h - q->e, q->e, 0.5 * M_PI, M_PI);
	cairo_arc(bar->ctx, x + q->e, y + q->e, q->e, M_PI, 1.5 * M_PI);
	cairo_close_path(bar->ctx);
	set_color(bar->ctx,bg);
	cairo_fill_preserve(bar->ctx);
	set_color(bar->ctx,brd);
	cairo_stroke(bar->ctx);
if (!(bar->opts & BAR_TOP))
cairo_restore(bar->ctx);
	set_color(bar->ctx,txt);
	return theme[txt].e;
}

int draw_background(Container *C) {
	XClearWindow(dpy,C->bar.win);
	XCopyArea(dpy,C->bar.win, C->bar.buf, gc, 0, 0,
			C->w, BAR_HEIGHT(C->bar.opts), 0, 0);
}

int draw_tab(Container *C, int con, Client *c, int n, int count) {
	int w = C->w;
	int x = 0;
	if (con == 0) {
		x = mons->sbar[0].width;
		w -= x;
		if (!(C->next && C->next->top)) w-= mons->sbar[1].width;
	}
	if (con == 1)
		w -= mons->sbar[1].width;
	int tw = w/count;
	int tx = x + n * tw;
	int th = BAR_HEIGHT(C->bar.opts);
	double off;
	cairo_text_extents_t ext;
	cairo_text_extents(C->bar.ctx,c->title,&ext);
/* ICONS:
c->hints->icon_pixmap
c->hints->icon_mask
*/
	//TODO need to trim long titles:
	if (m->focus == C && C->top == c)
		off = round_rect(&C->bar, tx, 0, tw, th,
				tabOffset, tabRGBAFocus, tabRGBAFocusBrd, tabRGBAFocusText);
	else if (C->top == c)
		off = round_rect(&C->bar, tx, 0, tw, th,
				tabOffset, tabRGBATop, tabRGBATopBrd, tabRGBATopText);
	else
		off = round_rect(&C->bar, tx, 0, tw, th,
				tabOffset, tabRGBAOther, tabRGBAOtherBrd, tabRGBAOtherText);
	if (off < 0) off *= -1;
	else off *= tw - ext.x_advance;
	cairo_move_to(C->bar.ctx,tx + off,th-4);
	cairo_show_text(C->bar.ctx,c->title);
	cairo_stroke(C->bar.ctx);
}

int draw_status() {
	Monitor *M;
	SBar *S;
	int i; const char *c;
	Bool trigger = False;
	char str[2]; str[1] = '\0';
	for (M = mons; M; M = M->next)
	for (c = status_fmt, i = 0; i < 2; c++, i++) {
		S = &M->sbar[i];
		cairo_save(S->ctx);
		cairo_set_operator(S->ctx,CAIRO_OPERATOR_CLEAR);
		cairo_paint(S->ctx);
		cairo_restore(S->ctx);
		S->x = 0;
		for (c; *c != '&' && *c != '\0'; c++) {
			if (*c == '%') {
				c++;
				switch (*c) {
					case 'T': case 't': case 'n': case 'i':
						sbar_tags(M,S,*c); break;
					case 'C': case 'c': 
						sbar_clock(S,*c); break;
					case 'I':
						if (ibar_text) {
							set_color(S->ctx,statRGBAInput);
							sbar_text(S,ibar_text);
							sbar_text(S," ");
						}
						break;
					case '1': case '2': case '3': case '4': case '5':
					case '6': case '7': case '8': case '9':
						sbar_parse(S,*c - 49); break;
					default:
						die("unrecognized status bar format string");
				}
			}
			else {
				str[0] = *c;
				set_color(S->ctx,statRGBAText);
				sbar_text(S,str);
			}
		}
		S->width = S->x;
	}
}

int draw() {
	draw_status();
	tile();
	if (!m->focus || !m->focus->top) m->focus = m->container;
	Client *c;
	if ( (c=m->focus->top) ) set_focus(c);
	XFlush(dpy);
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

void sbar_clock(SBar *S, char ch) {
	time_t raw = time(NULL);
	struct tm *now = localtime(&raw);
	char str[8];
	if (ch == 'C') strftime(str,8,"%H:%M",now);
	else if (ch == 'c') strftime(str,8,"%I:%M",now);
	set_color(S->ctx,statRGBAText);
	sbar_text(S,str);
}

void sbar_parse(SBar *S, int n) {
	char *c, *t1, *t2, *str;
	double r,g,b,a;
	for (c = instring; n; n --) {
		if ( !(c=strchr(c,'&')) ) return;
		c++;
	}
	for (c; c && *c != '&' && *c != '\0' && *c != '\n'; c++) {
		if (*c == '{') {
			if (*(++c) == 'i') {
				// icon
			}
			else if (sscanf(c,"%lf %lf %lf %lf",&r,&g,&b,&a) == 4)
				cairo_set_source_rgba(S->ctx,r,g,b,a);
			c = strchr(c,'}');
		}
		else {
			str = strdup(c);
			t1 = strchr(str,'{');
			t2 = strchr(str,'&');
			if (t1 && t2 && t1 > t2) t1 = t2;
			if (!t1) t1 = strchr(str,'\n');
			*t1 = '\0';
			c += t1 - str - 1;
			sbar_text(S,str);
			free(str);
		}
	}
}

void sbar_text(SBar *S, const char *str) {
	cairo_move_to(S->ctx,S->x,S->height-4);
	cairo_show_text(S->ctx,str);
	cairo_stroke(S->ctx);
	cairo_text_extents_t ext;
	cairo_text_extents(S->ctx,str,&ext);
	S->x += ext.x_advance;
}

void sbar_tags(Monitor *M, SBar *S, char ch) {
	int i;
	const char *tag;
	S->x += tag_pad;
	for (i = 0; i < ntags && (tag=tag_names[i]); i++) {
		if ( (M->tags & (1<<i)) && (M->tags & (1<<(i+16))) ) 
			set_color(S->ctx,tagRGBABoth);
		else if (M->focus && M->focus->top &&
				(M->focus->top->tags & (1<<i)))
			set_color(S->ctx,tagRGBAFoc);
		else if (M->tags & (1<<i))
			set_color(S->ctx,tagRGBASel);
		else if (M->tags & (1<<(i+16)))
			set_color(S->ctx,tagRGBAAlt);
		else if (M->occ & (1<<i))
			set_color(S->ctx,tagRGBAOcc);
		else
			continue;
		if (ch == 'T' || ch == 'i') {
			// draw icon
		}
		if (ch == 'T' || ch == 'n' || ch == 't') {
			sbar_text(S,tag);
		}
		if (ch == 't') {
			// draw icon
		}
		S->x += tag_pad;
	}
}

void set_color(cairo_t *ctx, int q) {
//fprintf(stderr,"%X: %f %f %f %f\n",
//q,theme[q].a,theme[q].b,theme[q].c,theme[q].d);
	cairo_set_source_rgba(ctx, theme[q].a, theme[q].b,
			theme[q].c, theme[q].d);
	//cairo_set_source_rgba(ctx,1.0,1.0,0.0,1.0);
}
