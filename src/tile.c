
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
	Client *c, *pc;
	int num, cn, numC, ord;
	for (M = mons; M; M = M->next) {
		/* calculate how many containers will be used: */
		if (M->mode == MONOCLE) numC = 1;
		else {
			c = clients;
			for (numC = 0, C = M->container; C; C = C->next, numC++) {
				cn = (M->mode == MONOCLE ? 1024 : (C->n > 0 ? C->n : 1024));
				for (num = 0, pc = c; c && num < cn; c = c->next) {
					if (tile_check(M, c)) {
						num++;
						if (!winmarks[1]) winmarks[1] = c;
						if (c == winmarks[1]) {
							M->focus = C;
							C->top = c;
						}
					}
				}
				if (num == 0) break;
			}
		}
		/* tile each used container: */
		c = clients;
		for (ord = 0, C = M->container; C && c; C = C->next, ord++) {
			cn = (M->mode == MONOCLE ? 1024 : (C->n > 0 ? C->n : 1024));
			for (num = 0, pc = c; c && num < cn; c = c->next)
				if (tile_check(M, c)) num++;
			if (!num) break; // or continue?
			resize_container(M, C, numC, ord);
			tile_container(M, C, pc, num);
		}
		for (C; C; C = C->next)
			purgatory(C->win);
		if (!numC && !(conf.bar_opts & BAR_HIDE)) {
			C = M->container;
			int y = M->y + (conf.bar_opts & BAR_BOTTOM ? M->h-C->bar->h : 0);
			XMoveResizeWindow(dpy, C->win, M->x, y, M->w, C->bar->h);
			cairo_set_source_surface(C->bar->ctx, M->bg, -C->x, -y);
			cairo_paint(C->bar->ctx);
		}
		for (c = clients; c; c = c->next) {
			// TODO: needs testing
			if (!(M->tags & c->tags))
				purgatory(c->win);
			else if (c->flags & WIN_FLOAT)
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		}
		if (!M->focus) M->focus = M->container;
	}
	/* paint bars to windows */
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
	int stack_size;
	if (numC == 1) {
		C->x = M->x + M->gap;
		C->y = M->y + M->gap;
		C->w = M->w - 2 * M->gap;
		C->h = M->h - 2 * M->gap;
	}
	else if (M->mode == RSTACK) {
		stack_size = (M->h - M->gap) / (numC - 1) - M->gap;
		C->x = M->x + M->gap + (ord ? (M->w -  M->gap) / 2 + M->split: 0);
		C->y = M->y + M->gap + (stack_size + M->gap) * (ord ? ord - 1 : 0);
		C->w = (ord ? M->w - M->gap - C->x - M->split :
				(M->w - M->gap) / 2 - M->gap + M->split);
		C->h = (ord ? stack_size : M->h - 2 * M->gap);
		if (ord == numC - 1) C->h = M->h - M->gap - C->y + M->y;
	}
	else if (M->mode == BSTACK) {
		stack_size = (M->w - M->gap) / (numC - 1) - M->gap;
		C->x = M->x + M->gap + (stack_size + M->gap) * (ord ? ord - 1 : 0);
		C->y = M->y + M->gap + (ord ? (M->h -  M->gap) / 2 + M->split: 0);
		C->w = (ord ? stack_size : M->w - 2 * M->gap);
		C->h = (ord ? M->h - M->gap - C->y - M->split :
				(M->h - M->gap) / 2 - M->gap + M->split);
		if (ord == numC - 1) C->w = M->w - M->gap - C->x + M->x;
	}
	if (!(C->bar->opts & BAR_HIDE)) {
		C->h -= C->bar->h;
		if (!(C->bar->opts & BAR_BOTTOM))
			C->y += C->bar->h;
	}
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
		if (!t) t = c;
		if (c == C->top) t = c;
		n--;
	}
	C->top = t;
}
