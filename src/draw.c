
#include "alopex.h"

static int round_rect(Bar *, int, int, int, int, int, int, int, int);
static int set_color(cairo_t *, int);
static int sbar_icon(Bar *, int, Bool);
static int sbar_text(Bar *, const char *);

static cairo_surface_t *icon_img[100];
static cairo_format_t icon_fmt = 0;

int draw_bar_sub(Monitor *M, Container *C, Bar *S, int x, Bool bg) {
	if (C->bar->opts & BAR_HIDE) return 0;
	if (S->w < 1) return 0;
	cairo_rectangle(C->bar->ctx, x, 0, S->w, C->bar->h);
	cairo_clip(C->bar->ctx);
	if (bg) { /* draw background as needed */
		int y = (C->bar->opts & BAR_BOTTOM ? C->y+C->h : C->y-C->bar->h);
		if (C->top)
			cairo_set_source_surface(C->bar->ctx, M->bg, -C->x, -y);
		else
			cairo_set_source_surface(C->bar->ctx, M->bg, -C->x, 0);
		cairo_paint(C->bar->ctx);
	}
	round_rect(C->bar, x, 0, S->w, S->h, StatusOffset,
			StatusBackground, StatusBorder, StatusText);
	cairo_set_source_surface(C->bar->ctx, S->buf, x, 0);
	cairo_paint(C->bar->ctx);
	cairo_reset_clip(C->bar->ctx);
	return 0;
}

int draw_bars(Bool bg) {
	Monitor *M; Container *C;
	int n, count;
	for (M = mons; M; M = M->next) {
		count = n = 0;
		for (C = M->container; C; C = C->next) if (C->top) count++;
		for (C = M->container; C; C = C->next) {
			/* paint status to buf */
			if (n == 0) {
				draw_bar_sub(M, C, &sbar[0], 0, bg);
				draw_bar_sub(M, C, &M->tbar, sbar[0].w, bg);
				draw_bar_sub(M, C, &sbar[1], sbar[0].w + M->tbar.w, bg);
				if (count < 2) {
					draw_bar_sub(M, C, &sbar[2], C->w-sbar[2].w-sbar[3].w, bg);
					draw_bar_sub(M, C, &sbar[3], C->w - sbar[3].w, bg);
				}
			}
			else if (n == 1) {
				if (count == 2) {
					draw_bar_sub(M, C, &sbar[2], C->w-sbar[2].w-sbar[3].w, bg);
					draw_bar_sub(M, C, &sbar[3], C->w - sbar[3].w, bg);
				}
				else  {
					draw_bar_sub(M, C, &sbar[2], C->w - sbar[2].w, bg);
				}
			}
			else if (n == count - 1) {
				draw_bar_sub(M, C, &sbar[3], C->w - sbar[3].w, bg);
			}
			/* paint buf to win */
			cairo_set_source_surface(C->ctx, C->bar->buf, 0, 0);
			cairo_paint(C->ctx);
			if ( (++n) >= count ) break;
		}
	}
}

int draw_tab(Container *C, Client *c, int n, int count) {
	Bar *b = C->bar;
	int w = (C->w - C->left_pad - C->right_pad)/ count;
	int x = w * n + C->left_pad;
	int h = C->bar->opts & BAR_HEIGHT;
	int theme = 0;
	if (m->focus == C && C->top == c) theme = 3;
	else if (C->top == c) theme = 6;
	round_rect(b, x, 0, w, h, TabOffset, TabBackground + theme,
			TabBorder + theme, TabText + theme);
	double off = conf.theme[TabText+theme].r;
	cairo_text_extents_t ext;
	cairo_text_extents(b->ctx, c->title, &ext);
if (c->icon) ext.x_advance += conf.font_size + conf.bar_pad;
	if (off < 0) off *= -1;
	else {
		off *= w - ext.x_advance;
		if (off < 0) off = 0;
	}
if (c->icon) {
	cairo_save(b->ctx);
	cairo_set_source_surface(b->ctx, c->icon, x + off,
			(b->h - conf.font_size) / 2.0);
	cairo_paint(b->ctx);
	off += conf.font_size + conf.bar_pad;
	cairo_restore(b->ctx);
}
	cairo_rectangle(b->ctx, x+off, 0, w-off, h);
	cairo_clip(b->ctx);
	cairo_move_to(b->ctx, x+off, h - 4); // - 4 ?
	cairo_show_text(b->ctx, c->title);
	cairo_reset_clip(b->ctx);
}

int icons_init(const char *fname) {
	double iw, ih;
	int i, j, fs = conf.font_size;
	cairo_surface_t *img = cairo_image_surface_create_from_png(fname);
	icon_fmt = cairo_image_surface_get_format(img);
	iw = cairo_image_surface_get_width(img) / 10.0;
	ih = cairo_image_surface_get_height(img) / 10.0;
	cairo_t *ctx;
	for (j = 0; j < 10; j++) for (i = 0; i < 10; i++) {
		icon_img[j*10+i] = cairo_image_surface_create(icon_fmt, fs, fs);
		ctx = cairo_create(icon_img[j*10+i]);
		cairo_scale(ctx, fs/iw, fs/ih);
		cairo_set_source_surface(ctx, img, -1 * i * iw, -1 * j * ih);
		cairo_paint(ctx);
		cairo_destroy(ctx);
	}
	cairo_surface_destroy(img);
	return 0;
}

int icons_free() {
	int i;
	for (i = 0; i < 100; i++)
		cairo_surface_destroy(icon_img[i]);
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

int sbar_icon(Bar *S, int n, Bool col) {
// TODO fix the "2"
	if ( (--n) < 0 || n > 99) return 1;
	cairo_save(S->ctx);
	if (col && icon_fmt != CAIRO_FORMAT_A1) {
		cairo_set_source_surface(S->ctx, icon_img[n], S->xoff, 2);
		cairo_paint(S->ctx);
	}
	else {
		cairo_mask_surface(S->ctx, icon_img[n], S->xoff, 2);
	}
	S->xoff += conf.font_size;
	cairo_restore(S->ctx);
	return 0;
}

int sbar_parse(Bar *S, const char *s) {
	cairo_save(S->ctx);
	cairo_set_operator(S->ctx, CAIRO_OPERATOR_CLEAR);
	cairo_paint(S->ctx);
	cairo_restore(S->ctx);
	S->xoff = 0;
	set_color(S->ctx, StatusText);
	const char *c; char *t1, *t2, *str;
	double r, g, b, a;
	for (c = s; c && *c != '&' && *c != '\0' && *c != '\n'; c++) {
		if (*c == '{') {
			if (*(++c) == 'i') sbar_icon(S, atoi((++c)), True);
			else if (*c == 'I') sbar_icon(S, atoi((++c)), False);
			else if (*c == 'f') cairo_set_font_face(S->ctx, conf.font);
			else if (*c == 'F') cairo_set_font_face(S->ctx, conf.bfont);
			else if (sscanf(c, "%lf %lf %lf %lf", &r, &g, &b, &a) == 4)
				cairo_set_source_rgba(S->ctx, r, g, b, a);
			c = strchr(c, '}');
		}
		else {
			str = strdup(c);
			t1 = strchr(str,'{');
			t2 = strchr(str,'&');
			if (t1 && t2 && t1 > t2) t1 = t2;
			else if (!t1 && t2) t1 = t2;
			if (!t1) t1 = strchr(str,'\n');
			*t1 = '\0';
			c += t1 - str - 1;
			sbar_text(S, str);
			free(str);
		}
	}
	if (S->w != S->xoff) {
		S->w = S->xoff;
		return 1;
	}
	else return 0;
}

static int sbar_tag_icon(Monitor *M, Bar *S, int i) {
	if (conf.tag_icon[i] == -1) return 1;
	else return sbar_icon(S, conf.tag_icon[i], True); //TODO: True -> conf
}

static int sbar_tag_text(Monitor *M, Bar *S, int i) {
	if (conf.tag_name[i][0] == '-') return 1;
	sbar_text(S, conf.tag_name[i]);
	S->xoff += conf.bar_pad;
	return 1;
}

int sbar_tags(Monitor *M) {
	if (M->container->bar->opts & BAR_HIDE) return 0;
	Bar *S = &M->tbar;
	cairo_save(S->ctx);
	cairo_set_operator(S->ctx, CAIRO_OPERATOR_CLEAR);
	cairo_paint(S->ctx);
	cairo_restore(S->ctx);
	S->xoff = conf.bar_pad;
	int i, count = 0;
	for (i = 0; conf.tag_name[i]; i++) {
		set_color(S->ctx, TagOccupied);
		if (M->occ & (1<<i)) cairo_set_font_face(S->ctx, conf.bfont);
		else cairo_set_font_face(S->ctx, conf.font);
		if ( (M->tags & (1<<i)) && (M->tags & (1<<(1+16))) )
			set_color(S->ctx, TagBoth);
		else if (M->tags & (1<<i)) set_color(S->ctx, TagView);
		else if (M->tags & (1<<(i+16))) set_color(S->ctx, TagAlt);
		else if (!(M->occ & (1<<i))) continue;
		switch (conf.tag_mode) {
			case TAG_ICON_TEXT: count += sbar_tag_icon(M, S, i);
			case TAG_TEXT: count += sbar_tag_text(M, S, i); break;
			case TAG_TEXT_ICON: count += sbar_tag_text(M, S, i);
			case TAG_ICON: count += sbar_tag_icon(M, S, i); break;
		}
	}
	if (!count) S->xoff = 0;
	S->w = S->xoff;
	return 0;
}

int sbar_text(Bar *S, const char *str) {
	cairo_move_to(S->ctx, S->xoff, S->h - 4);
	cairo_show_text(S->ctx, str);
	cairo_text_extents_t ext;
	cairo_text_extents(S->ctx, str, &ext);
	S->xoff += ext.x_advance;
	return 0;
}

int set_color(cairo_t *ctx, int col) {
	cairo_set_source_rgba(ctx, conf.theme[col].R, conf.theme[col].G,
			conf.theme[col].B, conf.theme[col].A);
}
