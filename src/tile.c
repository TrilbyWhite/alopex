
#include "alopex.h"

Bool tile_check(Monitor *, Client *);

static int resize_container(Monitor *, Container *, int, int);
static int tile_container(Monitor *, Container *, Client *, int);

int tile() {
	if ( !winmarks[1] || !(winmarks[1]->tags & m->tags) ) {
		winmarks[0] = winmarks[1];
		winmarks[1] = NULL;
	}
	Monitor *M;
	Container *C;
	Client *c, *pc, *t;
	int num, cn, numC, ord;
	for (M = mons; M; M = M->next) {
		/* calculate how many containers will be used: */
		if (M->mode == MONOCLE) {
			M->focus = M->container;
			numC = 1;
			for (c = clients; c; c = c->next) if (tile_check(M, c)) {
				if (!winmarks[1]) winmarks[1] = c;
				if (c == winmarks[1]) M->container->top = c;
			}
		}
		else { /* {r/b}stack: */
			c = clients;
			for (numC = 0, C = M->container; C; C = C->next, numC++) {
				cn = (M->mode == MONOCLE ? 1024 : (C->n > 0 ? C->n : 1024));
				t = NULL;
				for (num = 0, pc = c; c && num < cn; c = c->next) {
				/* fill C up to max clients */
					if (tile_check(M, c)) {
						num++;
						/* set C->top */
						if (!winmarks[1]) winmarks[1] = c;
						if (c == winmarks[1]) {
							M->focus = C;
							C->top = c;
						}
						if (!t) t = c;
						if (c == C->top) t = c;
					}
				}
				if (num == 0) break;
				C->top = t;
			}
		}
		/* tile each used container, hide others */
		c = clients;
		for (ord = 0, C = M->container; C && c; C = C->next, ord++) {
			cn = (M->mode == MONOCLE ? 1024 : (C->n > 0 ? C->n : 1024));
			for (num = 0, pc = c; c && num < cn; c = c->next)
				if (tile_check(M, c)) num++;
			if (!num) break;
			resize_container(M, C, numC, ord);
			tile_container(M, C, pc, num);
		}
		for (C; C; C = C->next) purgatory(C->win);
		/* show bar if no containers are visible */
		if (!numC && !(conf.bar_opts & BAR_HIDE)) {
			C = M->container;
			int y = M->y + (conf.bar_opts & BAR_BOTTOM ? M->h-C->bar->h : 0);
			XMoveResizeWindow(dpy, C->win, M->x, y, M->w, C->bar->h);
			cairo_set_source_surface(C->bar->ctx, M->bg, -C->x, -y);
			cairo_paint(C->bar->ctx);
		}
		/* sort floating windows */
		for (c = clients; c; c = c->next) {
			// TODO: needs testing
			if (!(M->tags & c->tags))
				purgatory(c->win);
			else if (c->flags & WIN_FLOAT)
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		}
		if (!M->focus) M->focus = M->container;
	}
	/* paint bar buffers to windows */
	for (M = mons; M; M = M->next) {
		for (numC = 0, C = M->container; C; C = C->next, numC++) {
			cairo_set_source_surface(C->ctx, C->bar->buf, 0, 0);
			cairo_paint(C->ctx);
		}
	}
	set_focus();
	return 0;
}


int resize_container(Monitor *M, Container *C, int numC, int ord) {
	/* calcualte size based on mode */
	int x = M->x + M->margin.left;
	int y = M->y + M->margin.top;
	int w = M->w - M->margin.left - M->margin.right;
	int h = M->h - M->margin.top - M->margin.bottom;
	int stack_size;
	if (numC == 1) { /* only one container */
		C->x = x + M->gap;
		C->y = y + M->gap;
		C->w = w - 2 * M->gap;
		C->h = h - 2 * M->gap;
	}
	else if (M->mode == RSTACK) {
		stack_size = (h - M->gap) / (numC - 1) - M->gap;
		C->x = x + M->gap + (ord ? (w -  M->gap) / 2 + M->split: 0);
		C->y = y + M->gap + (stack_size + M->gap) * (ord ? ord - 1 : 0);
		C->w = (ord ? x + w - M->gap - C->x :
				(w - M->gap) / 2 - M->gap + M->split);
		C->h = (ord ? stack_size : h - 2 * M->gap);
		if (ord == numC - 1) C->h = h - M->gap - C->y + y;
	}
	else if (M->mode == BSTACK) {
		stack_size = (w - M->gap) / (numC - 1) - M->gap;
		C->x = x + M->gap + (stack_size + M->gap) * (ord ? ord - 1 : 0);
		C->y = y + M->gap + (ord ? (h -  M->gap) / 2 + M->split: 0);
		C->w = (ord ? stack_size : w - 2 * M->gap);
		C->h = (ord ? y + h - M->gap - C->y :
				(h - M->gap) / 2 - M->gap + M->split);
		if (ord == numC - 1) C->w = w - M->gap - C->x + x;
	}
	/* adjust for bar space */
	if (!(C->bar->opts & BAR_HIDE)) {
		C->h -= C->bar->h;
		if (!(C->bar->opts & BAR_BOTTOM))
			C->y += C->bar->h;
	}
	/* hide/show the bar */
	if (C->bar->opts & BAR_HIDE) purgatory(C->win);
	else {
		int y = (C->bar->opts & BAR_BOTTOM ? C->y+C->h : C->y-C->bar->h);
		XMoveResizeWindow(dpy, C->win, C->x, y, C->w, C->bar->h);
		cairo_set_source_surface(C->bar->ctx, M->bg, -C->x, -y);
		cairo_paint(C->bar->ctx);
	}
}

Bool tile_check(Monitor *M, Client *c) {
	return ((M->tags & c->tags) && !(c->flags & WIN_FLOAT));
}

int tile_container(Monitor *M, Container *C, Client *pc, int num) {
	Client *c, *t = NULL;
	int n;
	for (c = pc, n = num; c && n; c = c->next) if (tile_check(M, c)) {
		XMoveResizeWindow(dpy, c->win, C->x, C->y, C->w, C->h);
		if (!(C->bar->opts & BAR_HIDE))
			draw_tab(C, c, num - n, num, C->bar->opts & BAR_BOTTOM);
		n--;
	}
}
