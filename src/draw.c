/********************************************************************\
* DRAW.C - part of alopex, see alopex.c for license information
*
* Author: Jesse McClure, copyright 2012-2013
\********************************************************************/

/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

#include "alopex.h"

static void blit_status();
static void sbar_clock(SBar *, char);
static void sbar_icon(SBar *, int);
static void sbar_parse(SBar *, int);
static void sbar_tags(Monitor *, SBar *, char);
static void sbar_text(SBar *, const char *);
static void set_color(cairo_t *, int);

static cairo_surface_t *icon_img[100];
static int icon_size;


/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int draw(int depth) {
	if (!depth || depth > 2) return 0;
	if (depth > 1) tile();
	if (draw_status() || depth > 1) {
		tile();
		if (!m->focus || !m->focus->top) m->focus = m->container;
		Client *c;
		if ( (c=m->focus->top) ) set_focus(c);
	}
	blit_status();
	XFlush(dpy);
	return 0;
}

int bg_init(Monitor *M) {
	Pixmap bg_pix = XCreatePixmap(dpy,root,M->w,M->h,DefaultDepth(dpy,scr));
	M->bg = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,M->w,M->h);
	cairo_t *bg_ctx = cairo_create(M->bg);
	cairo_surface_t *bg_img;
	if (bg_path) {
		bg_img = cairo_image_surface_create_from_png(bg_path);
		int ww = cairo_image_surface_get_width(bg_img);
		int hh = cairo_image_surface_get_height(bg_img);
		cairo_scale(bg_ctx,M->w/(float)ww,M->h/(float)hh);
		cairo_set_source_surface(bg_ctx,bg_img,0,0);
	}
	else {
		cairo_set_source_rgba(bg_ctx, theme[statRGBA].a,
				theme[statRGBA].b, theme[statRGBA].c, 1.0);
	}
	cairo_paint(bg_ctx);
	cairo_destroy(bg_ctx);
	if (bg_path) cairo_surface_destroy(bg_img);
	bg_img = cairo_xlib_surface_create(dpy,bg_pix,DefaultVisual(dpy,scr),M->w,M->h);
	bg_ctx = cairo_create(bg_img);
	cairo_set_source_surface(bg_ctx,M->bg,0,0);
	cairo_paint(bg_ctx);
	cairo_destroy(bg_ctx);
	cairo_surface_destroy(bg_img);
	XSetWindowBackgroundPixmap(dpy,root,bg_pix);
	XFreePixmap(dpy,bg_pix);
}

int bg_free(Monitor *M) {
	cairo_surface_destroy(M->bg);
}

int icons_init(const char *fname, int size) {
	cairo_surface_t *img = cairo_image_surface_create_from_png(fname);
	double scw = (10.0 * size) / cairo_image_surface_get_width(img);
	double sch = (10.0 * size) / cairo_image_surface_get_height(img);
	int i,j;
	cairo_t *ctx;
	for (j = 0; j < 10; j++) for (i = 0; i < 10; i++) {
		icon_img[j*10+i] = cairo_image_surface_create(0, size, size);
		ctx = cairo_create(icon_img[j*10+i]);
		cairo_scale(ctx, scw, sch);
		cairo_set_source_surface(ctx, img, -1 * i * size / scw, -1 * j * size / sch);
		cairo_paint(ctx);
		cairo_destroy(ctx);
	}
	cairo_surface_destroy(img);
	icon_size = size;
	return 0;
}

int icons_free() {
	int i;
	for (i = 0; i < 100; i++)
		cairo_surface_destroy(icon_img[i]);
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
	if (!(bar->opts & BAR_TOP)) cairo_restore(bar->ctx);
	set_color(bar->ctx,txt);
	return theme[txt].e;
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
/* ICONS:
c->hints->icon_pixmap
c->hints->icon_mask
*/
	if (m->focus == C && C->top == c)
		off = round_rect(&C->bar, tx, 0, tw, th,
				tabOffset, tabRGBAFocus, tabRGBAFocusBrd, tabRGBAFocusText);
	else if (C->top == c)
		off = round_rect(&C->bar, tx, 0, tw, th,
				tabOffset, tabRGBATop, tabRGBATopBrd, tabRGBATopText);
	else
		off = round_rect(&C->bar, tx, 0, tw, th,
				tabOffset, tabRGBAOther, tabRGBAOtherBrd, tabRGBAOtherText);
	cairo_text_extents_t ext;
	cairo_text_extents(C->bar.ctx,c->title,&ext);
	if (off < 0) off *= -1;
	else off *= tw - ext.x_advance;
	if (off < 0) off = theme[tabOffset].d;
	cairo_rectangle(C->bar.ctx,tx+off,0,tw-2.0*off,th);
	cairo_clip(C->bar.ctx);
	cairo_move_to(C->bar.ctx,tx + off,th-4);
	cairo_show_text(C->bar.ctx,c->title);
	cairo_stroke(C->bar.ctx);
	cairo_reset_clip(C->bar.ctx);
}


Bool draw_status() {
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
		for (c; *c != '&' && *c != '\0' && *c != '\n'; c++) {
			if (*c == '%') {
				c++;
				switch (*c) {
					case 'T': case 't': case 'n': case 'i':
						sbar_tags(M,S,*c); break;
					case 'C': case 'c': 
						sbar_clock(S,*c);
						break;
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
		if (S->width != S->x) trigger = True;
		S->width = S->x;
	}
	return trigger;
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

void blit_status() {
	Monitor *M; Container *C;
	for (M = mons; M; M = M->next) {
		C = M->container;
		if (!(C->bar.opts & BAR_VISIBLE)) continue;
		cairo_save(C->bar.ctx);
		cairo_rectangle(C->bar.ctx,0,0,M->sbar[0].width,M->sbar[0].height);
		cairo_clip(C->bar.ctx);
		cairo_set_source_surface(C->bar.ctx,M->bg,-1 * C->bar.x, -1 * C->bar.y);
		cairo_paint(C->bar.ctx);
		cairo_set_line_width(C->bar.ctx,theme[statRGBABrd].e);
		round_rect(&C->bar,0,0,M->sbar[0].width,M->sbar[0].height,
				statOffset, statRGBA, statRGBABrd, statRGBAText);
		cairo_set_source_surface(C->bar.ctx,M->sbar[0].buf,0,0);
		cairo_paint(C->bar.ctx);
		cairo_restore(C->bar.ctx);
		//XCopyArea(dpy,C->bar.buf,C->bar.win,gc,0,0,C->w,M->sbar[0].height,0,0);
		if (C->next && C->next->top && M->mode != MONOCLE) C = C->next;
		cairo_save(C->bar.ctx);
		cairo_rectangle(C->bar.ctx,C->w - M->sbar[1].width, 0,
				M->sbar[1].width,M->sbar[1].height);
		cairo_clip(C->bar.ctx);
		cairo_set_source_surface(C->bar.ctx,M->bg,-1 * C->bar.x, -1 * C->bar.y);
		cairo_paint(C->bar.ctx);
		cairo_set_line_width(C->bar.ctx,theme[statRGBABrd].e);
		round_rect(&C->bar,C->w - M->sbar[1].width,0,M->sbar[1].width,
				M->sbar[1].height, statOffset, statRGBA, statRGBABrd,
				statRGBAText);
		cairo_set_source_surface(C->bar.ctx,M->sbar[1].buf,C->w - M->sbar[1].width,0);
		cairo_paint(C->bar.ctx);
		cairo_restore(C->bar.ctx);
		//XCopyArea(dpy,C->bar.buf,C->bar.win,gc,0,0,C->w,M->sbar[1].height,0,0);
		for (C = M->container; C; C = C->next)
			XCopyArea(dpy,C->bar.buf,C->bar.win,gc,0,0,C->w,
					BAR_HEIGHT(C->bar.opts),0,0);
	}
}

void sbar_clock(SBar *S, char ch) {
	time_t raw = time(NULL);
	struct tm *now = localtime(&raw);
	char str[8];
	if (ch == 'C') strftime(str,8,"%H:%M",now);
	else if (ch == 'c') strftime(str,8,"%I:%M",now);
	set_color(S->ctx,statRGBAText);
	sbar_text(S,str);
}

void sbar_icon(SBar *S, int n) {
	n--;
	if (n < 0 || n > 99) return;
	cairo_save(S->ctx);
	cairo_set_source_surface(S->ctx,icon_img[n],S->x,2);
	cairo_paint(S->ctx);
	S->x += icon_size;
	cairo_restore(S->ctx);
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
				sbar_icon(S,atoi((++c)));
			}
			else if (*c == 'f') cairo_set_font_face(S->ctx,cfont);
			else if (*c == 'F') cairo_set_font_face(S->ctx,cfont2);
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
		if (M->focus && M->focus->top && M->focus->top->tags & (1<<i))
			cairo_set_font_face(S->ctx,cfont2);
		else
			cairo_set_font_face(S->ctx,cfont);
		if ( (M->tags & (1<<i)) && (M->tags & (1<<(i+16))) ) 
			set_color(S->ctx,tagRGBABoth);
		else if (M->tags & (1<<i))
			set_color(S->ctx,tagRGBASel);
		else if (M->tags & (1<<(i+16)))
			set_color(S->ctx,tagRGBAAlt);
		else if (M->occ & (1<<i))
			set_color(S->ctx,tagRGBAOcc);
		else
			continue;
		if (ch == 'T' || ch == 'i') {
				sbar_icon(S,tag_icon[i]);
		}
		if (ch == 'T' || ch == 'n' || ch == 't') {
			sbar_text(S,tag);
		}
		if (ch == 't') {
				sbar_icon(S,tag_icon[i]);
		}
		S->x += tag_pad;
	}
	if (S->x == tag_pad) S->x = 0;
}

void set_color(cairo_t *ctx, int q) {
	cairo_set_source_rgba(ctx, theme[q].a, theme[q].b,
			theme[q].c, theme[q].d);
}
