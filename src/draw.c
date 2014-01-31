
#include "alopex.h"

static int round_rect(Bar *, int, int, int, int, int, int, int, int);
static int set_color(cairo_t *, int);

// TODO draw_bar to incorporate statuses

int draw_tab(Container *C, Client *c, int n, int count) {
	Bar *b = C->bar;
	int w = C->w / count;
	int x = w * n;
	int h = C->bar->opts & BAR_HEIGHT;
	int theme = 0;
	if (m->focus == C && C->top == c) theme = 3;
	else if (C->top == c) theme = 6;
	round_rect(b, x, 0, w, h, TabOffset, TabBackground + theme,
			TabBorder + theme, TabText + theme);
	double off = conf.theme[TabText+theme].r;
	cairo_text_extents_t ext;
	cairo_text_extents(b->ctx, c->title, &ext);
	if (off < 0) off *= -1;
	else {
		off *= w - ext.x_advance;
		if (off < 0) off = 0;
	}
	// if icon, adjust off
	// TODO icons
	cairo_rectangle(b->ctx, x+off, 0, w-off, h);
	cairo_clip(b->ctx);
	cairo_move_to(b->ctx, x+off, h - 4); // - 4 ?
	cairo_show_text(b->ctx, c->title);
	cairo_reset_clip(b->ctx);
}

int icons_init(const char *fname) {
	return 0;
}

int icons_free() {
	return 0;
}

int round_rect(Bar *b, int x, int y, int w, int h,
		int off, int bg, int brd, int txt) {
	const Theme *q = &conf.theme[off];
	if (b->opts & BAR_BOTTOM) {
		cairo_save(b->ctx);
		cairo_scale(b->ctx, 1, -1);
		cairo_translate(b->ctx, 0, -1 * (b->opts & BAR_HEIGHT));
	}
	cairo_new_sub_path(b->ctx);
	x += q->x; y += q->y; w += q->w; h += q->h;
	cairo_arc(b->ctx, x + w - q->e, y + q->e, q->e, -0.5 * M_PI, 0);
	cairo_arc(b->ctx, x + w - q->e, y + h - q->e, q->e, 0, 0.5 * M_PI);
	cairo_arc(b->ctx, x + q->e, y + h - q->e, q->e, 0.5 * M_PI, M_PI);
	cairo_arc(b->ctx, x + q->e, y + q->e, q->e, M_PI, 1.5 * M_PI);
	cairo_close_path(b->ctx);
	set_color(b->ctx, bg);
	cairo_fill_preserve(b->ctx);
	set_color(b->ctx, brd);
	cairo_set_line_width(b->ctx, conf.theme[brd].r);
	cairo_stroke(b->ctx);
	if (b->opts & BAR_BOTTOM) cairo_restore(b->ctx);
	set_color(b->ctx, txt);
	return 0;
}

int set_color(cairo_t *ctx, int col) {
	cairo_set_source_rgba(ctx, conf.theme[col].R, conf.theme[col].G,
			conf.theme[col].B, conf.theme[col].A);
}
