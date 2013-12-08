/********************************************************************\
* TILE.C - part of alopex, see alopex.c for license information
*
* Author: Jesse McClure, copyright 2012-2013
\********************************************************************/

/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

#include "alopex.h"

static void tile_client(Client *, int, int, int, int);
static void tile_container(Monitor *, Container *, int, int);
static void tile_monocle(Monitor *, int);

extern double round_rect(Bar *, int, int, int, int, int, int, int, int);

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int tile() {
	int n, nn, ncon, nlast;
	Client *c;
	Monitor *M;
	Container *C;
	for (M = mons; M; M = M->next) {
	// adjust M->tags for all previous M's
		M->occ = 0;
		for (n = 0, c = clients; c; c = c->next) {
			M->occ |= c->tags;
			if (c->tags & M->tags && !(c->flags & WIN_FLOAT) ) n++;
			else if (c->tags & M->tags) {
				if (c->x || c->y) {
					XMoveWindow(dpy,c->win,c->x,c->y);
					c->x = c->y = 0;
				}
				XRaiseWindow(dpy,c->win);
			}
			else {
				if (c->flags & WIN_FLOAT && !c->x && !c->y) {
					Window w; int ig;
					XGetGeometry(dpy,c->win,&w,&c->x,&c->y,&ig,&ig,&ig,&ig);
				}
				purgatory(c->win);
			}
		}
		ncon = 0;
		Container *focus = NULL;
		nn = n;
		for (C = M->container; C; C = C->next) {
			if (M->focus == C) focus = C;
			ncon++;
			nlast = n;
			if ( (n -= C->n) <= 0 || C->n < 0 ) break;
		}
		n = nn;
		//if (!focus) M->focus = M->container;
		if (M->mode < 0 || !M->container->next || ncon == 1) {
			//if (M->focus != M->floaters) M->focus = M->container;
			tile_monocle(M,n);
			for (C = M->container->next; C; C = C->next) {
				purgatory(C->bar.win);
				C->top = NULL;
				if (M->focus == C) M->focus = M->container;
			}
		}
		else {
			for (n = 0, C = M->container; C && n<ncon; n++, C=C->next)
				tile_container(M,C,ncon,nlast);
			for (C; C; C = C->next) {
				purgatory(C->bar.win);
				C->top = NULL;
				if (M->focus == C) M->focus = M->container;
			}
		}
	}
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

void tile_client(Client *c, int x, int y, int w, int h) {
	XMoveResizeWindow(dpy,c->win,x,y,w,h);
}

void tile_container(Monitor *M, Container *C, int ncon, int nlast) {
	int con, n;
	Container *CC;
	for (con = 0, CC = M->container; CC != C; CC = CC->next, con++);
	int ms = ((M->mode == RSTACK ? M->w : M->h) - 3 * M->gap)/2;
	int mss = ((M->mode == RSTACK ? M->h : M->w) - 2 * M->gap);
	int ss = ((M->mode == RSTACK ? M->h : M->w) - M->gap)/(ncon-1) - M->gap;
	int x = M->x + M->gap + (M->mode == RSTACK ?
			(con ? ms + M->gap + M->split : 0) :
			(con > 1 ? ss * (con - 1) + M->gap: 0));
	int y = M->y + M->gap + (M->mode ==  BSTACK ?
			(con ? ms + M->gap + M->split : 0) :
			(con > 1 ? ss * (con - 1) + M->gap: 0));
	int w = (M->mode == RSTACK ? (con ? ms - M->split : ms + M->split) :
			(con ? ss : mss));
	int h = (M->mode == BSTACK ? (con ? ms - M->split : ms + M->split) : 
			(con ? ss : mss));
	C->w = w;
//	draw_background(C);


	/* adjust for container bar */
	//XMoveResizeWindow(dpy,C->bar.win,x,y,w,BAR_HEIGHT(C->bar.opts));
	if (C->bar.opts & BAR_VISIBLE) {
		h -= BAR_HEIGHT(C->bar.opts);
		if (C->bar.opts & BAR_TOP) {
			XMoveResizeWindow(dpy,C->bar.win,x,y,w,BAR_HEIGHT(C->bar.opts));
			cairo_set_source_surface(C->bar.ctx,M->bg,-1 * x,-1 * y);
			y += BAR_HEIGHT(C->bar.opts);
		}
		else {
			XMoveResizeWindow(dpy,C->bar.win,x,y+h,w,BAR_HEIGHT(C->bar.opts));
			cairo_set_source_surface(C->bar.ctx,M->bg,-1 * x,-1 * (y+h));
		}
		cairo_paint(C->bar.ctx);
	}
	else purgatory(C->bar.win);
	Client *c, *top = NULL;
	int nx;
	for (n = 0, CC = M->container; CC != C; n += CC->n, CC = CC->next);
	for (c = clients; c && n; c = c->next)
		if (c->tags & M->tags) n--;
	for (c; c; c = c->next)
		if (c->tags & M->tags && !(c->flags & WIN_FLOAT)) {
			if (C->n > 0 && (++n) > C->n) break;
			else if (C->n < 0) n++;
			if (!top) { top = c; nx = n-1; }
			if (C->top == c) top = c;
			tile_client(c,x,y,w,h);
			if (con < ncon - 1) draw_tab(C,con,c,n-1,C->n);
			else draw_tab(C,con,c,n-1,nlast);
			XLowerWindow(dpy,c->win);
		}
	if (C->top != top) {
		C->top = top;
		if (con < ncon - 1) draw_tab(C,con,top,nx,C->n);
		else draw_tab(C,con,top,nx,nlast);
	}
	XWindowChanges wc;
	wc.sibling = C->bar.win;
	wc.stack_mode = Below;
	XConfigureWindow(dpy, C->top->win, CWSibling | CWStackMode, &wc);
	if (con == 0) {
		cairo_save(C->bar.ctx);
		cairo_set_line_width(C->bar.ctx,theme[statRGBABrd].e);
		round_rect(&C->bar, 0, 0, M->sbar[0].width, BAR_HEIGHT(C->bar.opts),
				statOffset, statRGBA, statRGBABrd, statRGBAText);
		cairo_set_source_surface(C->bar.ctx,M->sbar[0].buf,0,0);
		cairo_paint(C->bar.ctx);
		cairo_restore(C->bar.ctx);
	}
	else if (con == 1) {
		cairo_save(C->bar.ctx);
		cairo_translate(C->bar.ctx,C->w - M->sbar[1].width,0);
		cairo_set_line_width(C->bar.ctx,theme[statRGBABrd].e);
		round_rect(&C->bar, 0, 0, M->sbar[1].width, BAR_HEIGHT(C->bar.opts),
				statOffset, statRGBA, statRGBABrd, statRGBAText);
		cairo_set_source_surface(C->bar.ctx,M->sbar[1].buf,0,0);
		cairo_paint(C->bar.ctx);
		cairo_restore(C->bar.ctx);
	}
	XCopyArea(dpy, C->bar.buf, C->bar.win, gc, 0, 0, C->w,
			BAR_HEIGHT(C->bar.opts), 0, 0);

}

void tile_monocle(Monitor *M,int n) {
	Container *C = M->container;
	int x = M->x + M->gap, y = M->y + M->gap;
	int w = M->w - 2 * M->gap, h = M->h - 2 * M->gap;
	/* adjust for container bar */
	C->w = w;
	//draw_background(C);
	//XMoveResizeWindow(dpy,C->bar.win,x,y,w,BAR_HEIGHT(C->bar.opts));
	if (C->bar.opts & BAR_VISIBLE) {
		h -= BAR_HEIGHT(C->bar.opts);
		if (C->bar.opts & BAR_TOP) {
			XMoveResizeWindow(dpy,C->bar.win,x,y,w,BAR_HEIGHT(C->bar.opts));
			cairo_set_source_surface(C->bar.ctx,M->bg,-1 * x,-1 * y);
			y += BAR_HEIGHT(C->bar.opts);
		}
		else {
			XMoveResizeWindow(dpy,C->bar.win,x,y+h,w,BAR_HEIGHT(C->bar.opts));
			cairo_set_source_surface(C->bar.ctx,M->bg,-1 * x,-1 * (y+h));
		}
		cairo_paint(C->bar.ctx);
	}
	else purgatory(C->bar.win);
	Client *c, *top = NULL, *ftop = NULL;
	int i = 0;
	for (c = clients; c; c = c->next)
		if (c->tags & M->tags && !(c->flags & WIN_FLOAT)) {
			if (!top) top = c;
			if (C->top == c) top = c;
			if (M->focus && M->focus->top && M->focus->top == c) ftop = c;
			tile_client(c,x,y,w,h);
			draw_tab(C,0,c,i++,n);
			XLowerWindow(dpy,c->win);
		}
	if (ftop) top = ftop;
	C->top = top;
	if (C->top) {
		XWindowChanges wc;
		wc.sibling = C->bar.win;
		wc.stack_mode = Below;
		XConfigureWindow(dpy, C->top->win, CWSibling | CWStackMode, &wc);
	}
	cairo_save(C->bar.ctx);
	cairo_set_line_width(C->bar.ctx,theme[statRGBABrd].e);
	if (M->occ || M->tags) round_rect(&C->bar, 0, 0, M->sbar[0].width,
			BAR_HEIGHT(C->bar.opts), statOffset,
			statRGBA, statRGBABrd, statRGBAText);
	cairo_set_source_surface(C->bar.ctx,M->sbar[0].buf,0,0);
	cairo_paint(C->bar.ctx);
	cairo_translate(C->bar.ctx,C->w - M->sbar[1].width,0);
	round_rect(&C->bar, 0, 0, M->sbar[1].width, BAR_HEIGHT(C->bar.opts),
			statOffset, statRGBA, statRGBABrd, statRGBAText);
	cairo_set_source_surface(C->bar.ctx,M->sbar[1].buf,0,0);
	cairo_paint(C->bar.ctx);
	cairo_restore(C->bar.ctx);
	//blit(&C->bar);
	XCopyArea(dpy, C->bar.buf, C->bar.win, gc, 0, 0, C->w,
			BAR_HEIGHT(C->bar.opts), 0, 0);
}


