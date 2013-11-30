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
static void sbar_tags(Monitor *, SBar *, char);
static void sbar_text(SBar *, const char *);

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int draw_background(Container *C) {
	cairo_set_source_rgba(C->bar.ctx,0.5,0.5,0.5,1);
	cairo_rectangle(C->bar.ctx,0,0,C->w,BAR_HEIGHT(C->bar.opts));
	cairo_fill(C->bar.ctx);
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
	if (m->focus == C && C->top == c)
		cairo_set_source_rgba(C->bar.ctx,1,1,0,1);
	else if (C->top == c)
		cairo_set_source_rgba(C->bar.ctx,0,1,1,1);
	else
		cairo_set_source_rgba(C->bar.ctx,0,0,0.5,1);
	cairo_rectangle(C->bar.ctx,tx+2,2,tw-4,th-4);
	cairo_fill(C->bar.ctx);

	//TODO need to trim long titles:
	cairo_set_source_rgba(C->bar.ctx,0,0,0,1);
	cairo_move_to(C->bar.ctx,tx+4,th-4);
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
cairo_set_source_rgba(S->ctx,0.5,0.5,0.5,1);
cairo_rectangle(S->ctx,0,0,S->width*2,S->height);
cairo_fill(S->ctx);
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
	if (m->focus->top)
		XSetInputFocus(dpy,m->focus->top->win,
				RevertToPointerRoot,CurrentTime);
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
	cairo_set_source_rgba(S->ctx,0,0,1,1);
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

