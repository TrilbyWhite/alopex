/********************************************************************\
* DRAW.C - part of alopex, see alopex.c for license information
*
* Author: Jesse McClure, copyright 2012-2013
\********************************************************************/

/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

#include "alopex.h"

static void round_rect(cairo_t *, int, int, int, int, int);
static void sbar_clock(SBar *, char);
static void sbar_tags(Monitor *, SBar *, char);
static void sbar_text(SBar *, const char *);
static void set_color(cairo_t *, int);

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int draw_background(Container *C) {
	XClearWindow(dpy,C->bar.win);
	XCopyArea(dpy,C->bar.win,C->bar.buf,gc,0,0,C->w,BAR_HEIGHT(C->bar.opts),0,0);
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
	//TODO need to trim long titles:
	if (m->focus == C && C->top == c) {
		set_color(C->bar.ctx,tabRGBAFocus);
		round_rect(C->bar.ctx,tx,0,tw,th,tabOffset);
		cairo_fill_preserve(C->bar.ctx);
		set_color(C->bar.ctx,tabRGBAFocusBrd);
		cairo_stroke(C->bar.ctx);
		set_color(C->bar.ctx,tabRGBAFocusText);
		off = theme[tabRGBAFocusText].e;
	}
	else if (C->top == c) {
		set_color(C->bar.ctx,tabRGBATop);
		round_rect(C->bar.ctx,tx,0,tw,th,tabOffset);
		cairo_fill_preserve(C->bar.ctx);
		set_color(C->bar.ctx,tabRGBATopBrd);
		cairo_stroke(C->bar.ctx);
		set_color(C->bar.ctx,tabRGBATopText);
		off = theme[tabRGBATopText].e;
	}
	else {
		set_color(C->bar.ctx,tabRGBAOther);
		round_rect(C->bar.ctx,tx,0,tw,th,tabOffset);
		cairo_fill_preserve(C->bar.ctx);
		set_color(C->bar.ctx,tabRGBAOtherBrd);
		cairo_stroke(C->bar.ctx);
		set_color(C->bar.ctx,tabRGBAOtherText);
		off = theme[tabRGBAOtherText].e;
	}
	if (off < 0) off *= -1;
	else off *= tw - ext.x_advance;
	cairo_move_to(C->bar.ctx,tx + off,th-4);
	cairo_show_text(C->bar.ctx,c->title);
	cairo_stroke(C->bar.ctx);
}

void draw_status() {
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
//cairo_set_source_rgba(S->ctx,0.5,0.5,0.5,0.2);
//cairo_rectangle(S->ctx,0,0,S->width*2,S->height);
//cairo_fill_preserve(S->ctx);
//cairo_set_source_rgba(S->ctx,0.5,0.5,1,0.5);
//cairo_stroke(S->ctx);
		S->x = 0;
		for (c; *c != '\n'; c++) {
			if (*c == '%') {
				c++;
				switch (*c) {
					case 'T': case 't': case 'n': case 'i':
						sbar_tags(M,S,*c); break;
					case 'C': case 'c': 
						sbar_clock(S,*c); break;
					default:
						die("unrecognized status bar format string");
				}
			}
			else {
				str[0] = *c;
				cairo_set_source_rgba(S->ctx,0,0,1,1);
				sbar_text(S,str);
			}
		}
		if (S->width != S->x) {
			S->width = S->x;
			trigger = True;
		}
		else {
			// copy from S to the right C->bar.win
			// XFlush(dpy);
		}
	}
	//if (trigger) draw();
}

int draw() {
	draw_status();
	tile();
	if (!m->focus) m->focus = m->container;
	if (m->focus->top) {
		XSetInputFocus(dpy,m->focus->top->win,
				RevertToPointerRoot,CurrentTime);
		if (m->focus->top != winmarks[0]) {
			winmarks[1] = winmarks[0];
			winmarks[0] = m->focus->top;
		}
	}
	XFlush(dpy);
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

void round_rect(cairo_t *ctx, int x, int y, int w, int h, int qn) {
	const Theme *q = &theme[qn];
	cairo_new_sub_path(ctx);
	x += q->a; y += q->b; w += q->c; h += q->d;
	cairo_arc(ctx, x + w - q->e, y + q->e, q->e, -0.5 * M_PI, 0);
	cairo_arc(ctx, x + w - q->e, y + h - q->e, q->e, 0, 0.5 * M_PI);
	cairo_arc(ctx, x + q->e, y + h - q->e, q->e, 0.5 * M_PI, M_PI);
	cairo_arc(ctx, x + q->e, y + q->e, q->e, M_PI, 1.5 * M_PI);
	cairo_close_path(ctx);
}

void sbar_clock(SBar *S, char ch) {
	time_t raw = time(NULL);
	struct tm *now = localtime(&raw);
	char str[8];
	if (ch == 'C') strftime(str,8,"%H:%M",now);
	else if (ch == 'c') strftime(str,8,"%I:%M",now);
	cairo_set_source_rgba(S->ctx,1,1,0,1);
	sbar_text(S,str);
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
	for (i = 0; (tag=tag_names[i]); i++) {
		if (M->focus && M->focus->top &&
				(M->focus->top->tags & (1<<i)))
			cairo_set_source_rgba(S->ctx,1,1,0,1);
		else if (M->tags & (1<<i))
			cairo_set_source_rgba(S->ctx,0,1,1,1);
		else if (M->occ & (1<<i))
			cairo_set_source_rgba(S->ctx,0,0,1,1);
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
	cairo_set_source_rgba(ctx,theme[q].a,theme[q].b,theme[q].c,theme[q].d);
}
