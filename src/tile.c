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

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

void tile() {
	int n, nn, ncon, nlast;
	Client *c;
	Monitor *M;
	Container *C;
	for (M = mons; M; M = M->next) {
		for (n = 0, c = clients; c; c = c->next) {
			if (c->tags & M->tags) n++;
			else purgatory(c->win);
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
		if (!focus) M->focus = M->container;
		if (M->mode < 0 || !M->container->next || ncon == 1) {
			M->focus = M->container;
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
	int x = M->x + M->gap + (M->mode == RSTACK ? (con ? ms + M->gap: 0) :
			(con > 1 ? ss * (con - 1) + M->gap: 0));
	int y = M->y + M->gap + (M->mode == BSTACK ? (con ? ms + M->gap: 0) :
			(con > 1 ? ss * (con - 1) + M->gap: 0));
	int w = (M->mode == RSTACK ? (con ? ms - M->split : ms + M->split) :
			(con ? ss : mss));
	int h = (M->mode == BSTACK ? (con ? ms - M->split : ms + M->split) : 
			(con ? ss : mss));
	C->w = w;
	draw_background(C);
	/* adjust for container bar */
	XMoveResizeWindow(dpy,C->bar.win,x,y,w,BAR_HEIGHT(C->bar.opts));
	if (C->bar.opts & BAR_VISIBLE) {
		h -= BAR_HEIGHT(C->bar.opts);
		if (C->bar.opts & BAR_TOP) {
			XMoveResizeWindow(dpy,C->bar.win,x,y,w,BAR_HEIGHT(C->bar.opts));
			y += BAR_HEIGHT(C->bar.opts);
		}
		else if (C->bar.opts & BAR_BOTTOM) {
			XMoveResizeWindow(dpy,C->bar.win,x,y+h,w,BAR_HEIGHT(C->bar.opts));
		}
	}
	Client *c, *top = NULL;
	int nx;
	for (n = 0, CC = M->container; CC != C; n += CC->n, CC = CC->next);
	for (c = clients; c && n; c = c->next)
		if (c->tags & M->tags) n--;
	for (c; c; c = c->next) if (c->tags & M->tags) {
		if (C->n > 0 && (++n) > C->n) break;
		else if (C->n < 0) n++;
		if (!top) { top = c; nx = n-1; }
		if (C->top == c) top = c;
		tile_client(c,x,y,w,h);
		if (con < ncon - 1) draw_tab(C,con,c,n-1,C->n);
		else draw_tab(C,con,c,n-1,nlast);
	}
	if (C->top != top) {
		C->top = top;
		if (con < ncon - 1) draw_tab(C,con,top,nx,C->n);
		else draw_tab(C,con,top,nx,nlast);
	}
	XRaiseWindow(dpy,C->top->win);
	if (con == 0) XCopyArea(dpy,M->sbar[0].buf,C->bar.buf,gc,0,0,
			M->sbar[0].width,BAR_HEIGHT(C->bar.opts),0,0);
	else if (con == 1) XCopyArea(dpy,M->sbar[1].buf,C->bar.buf,gc,0,0,
			M->sbar[1].width,BAR_HEIGHT(C->bar.opts),
			C->w - M->sbar[1].width,0);
	XCopyArea(dpy, C->bar.buf, C->bar.win, gc, 0, 0, C->w,
			BAR_HEIGHT(C->bar.opts), 0, 0);

}

void tile_monocle(Monitor *M,int n) {
	Container *C = M->container;
	int x = M->x + M->gap, y = M->y + M->gap;
	int w = M->w - 2 * M->gap, h = M->h - 2 * M->gap;
	/* adjust for container bar */
	C->w = w;
	draw_background(C);
	XMoveResizeWindow(dpy,C->bar.win,x,y,w,BAR_HEIGHT(C->bar.opts));
	if (C->bar.opts & BAR_VISIBLE) {
		h -= BAR_HEIGHT(C->bar.opts);
		if (C->bar.opts & BAR_TOP) {
			XMoveResizeWindow(dpy,C->bar.win,x,y,w,BAR_HEIGHT(C->bar.opts));
			y += BAR_HEIGHT(C->bar.opts);
		}
		else if (C->bar.opts & BAR_BOTTOM) {
			XMoveResizeWindow(dpy,C->bar.win,x,y+h,w,BAR_HEIGHT(C->bar.opts));
		}
	}
	Client *c, *top = NULL;
	int i = 0;
	for (c = clients; c; c = c->next)
		if (c->tags & M->tags) {
			if (!top) top = c;
			if (M->container->top == c) top = c;
			tile_client(c,x,y,w,h);
			draw_tab(C,0,c,i++,n);
		}
	M->container->top = top;
	XCopyArea(dpy,M->sbar[0].buf,C->bar.buf,gc,0,0,
			M->sbar[0].width,BAR_HEIGHT(C->bar.opts),0,0);
	XCopyArea(dpy,M->sbar[1].buf,C->bar.buf,gc,0,0, M->sbar[1].width,
			BAR_HEIGHT(C->bar.opts), C->w - M->sbar[1].width,0);
	XCopyArea(dpy, C->bar.buf, C->bar.win, gc, 0, 0, C->w,
			BAR_HEIGHT(C->bar.opts), 0, 0);
}


