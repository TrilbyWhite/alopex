
#include "alopex.h"

/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int draw() {
	tile();
	if (!m->focus) m->focus = m->container;
	if (m->focus->top)
		XSetInputFocus(dpy,m->focus->top->win,
				RevertToPointerRoot,CurrentTime);
	XFlush(dpy);
}

int draw_background(Container *C) {
	cairo_set_source_rgba(C->bar.ctx,0.5,0.5,0.5,1);
	cairo_rectangle(C->bar.ctx,0,0,C->w,BAR_HEIGHT(C->bar.opts));
	cairo_fill(C->bar.ctx);
}

int draw_tab(Container *C, int con, Client *c, int n, int count) {
	//if (con < 2) {
		//status bar
	//}
	int tw = C->w/count;
	int tx = n * tw;
	int th = BAR_HEIGHT(C->bar.opts);
fprintf(stderr,"tx: %d tw: %d\n",tx,tw);
	if (m->focus == C && C->top == c)
		cairo_set_source_rgba(C->bar.ctx,1,1,0,1);
	else if (C->top == c)
		cairo_set_source_rgba(C->bar.ctx,0,1,1,1);
	else
		cairo_set_source_rgba(C->bar.ctx,0,0,0.5,1);
	cairo_rectangle(C->bar.ctx,tx+2,2,tw-4,th-4);
	cairo_fill(C->bar.ctx);
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

