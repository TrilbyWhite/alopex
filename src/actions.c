
#include "alopex.h"
#include "actions.h"

extern Bool tile_check(Monitor *, Client *);
extern int send_message(Client *, int, int);


int command(const char *cmd) {
	if (cmd[0] == '!') return spawn(cmd+1);
	const char *wrd;
	for (wrd = cmd; wrd; wrd = strchr(wrd,' ')) {
		if (wrd[0] == ' ') word((++wrd));
		else word(wrd);
	}
	return 0;
}

int float_full(Client *t) {
	if (t) {
		if (t->flags & WIN_FLOAT) t->flags &= ~WIN_FULL;
		else t->flags |= WIN_FLOAT;
	}
	else if ( (t=winmarks[1]) ) {
		if (t->flags & WIN_FULL_TEST) t->flags &= ~WIN_FULL;
		else t->flags |= WIN_FULL;
	}
	else {
		return 1;
	}
	return 0;
}

int focus(Client *t, const char *s) {
	if (t) {
		winmarks[0] = winmarks[1];
		winmarks[1] = t;
		return 0;
	}
	Container *C;
	Client *c, *a, *b;
	if (!(c=winmarks[1])) return 1;
	switch (*s) {
		case 'f':
			for (a = winmarks[1]->next; a; a = a->next)
				if (a->flags & WIN_FLOAT) break;
			if (!a) for (a = clients; a; a = a->next)
				if (a->flags & WIN_FLOAT) break;
			if (!a) return 1;
			winmarks[0] = winmarks[1];
			winmarks[1] = a;
			break;
		case 'h':
			for (C = m->container; C && C->next; C = C->next) {
				if (C->next == m->focus) {
					winmarks[0] = winmarks[1];
					winmarks[1] = C->top;
				}
			}
			break;
		case 'j':
			for (a = c->next; a; a = a->next)
				if (tile_check(m, a)) break;
			if (a) {
				winmarks[0] = winmarks[1];
				winmarks[1] = a;
			}
			break;
		case 'k':
			for (a = clients; a && a != c; a = a->next)
				if (tile_check(m, a)) b = a;
			if (b) {
				winmarks[0] = winmarks[1];
				winmarks[1] = b;
			}
			break;
		case 'l':
			if (m->focus->next && m->focus->next->top) {
				winmarks[0] = winmarks[1];
				winmarks[1] = m->focus->next->top;
			}
			break;
		case 'p':
			if (winmarks[0] && (winmarks[0]->tags & m->tags)) {
				/* focus previous */
				a = winmarks[0];
				winmarks[0] = winmarks[1];
				winmarks[1] = a;
			}
			else { /* focus first container, or next container ... */
				C = m->container;
				if (C->top == winmarks[1]) C = C->next;
				if (C->top)
					a = C->top;
				else /* ... only one container -> focus another client */
					for (a = clients; a; a = a->next)
						if (a != winmarks[1] && tile_check(m, a)) break;
				if (a) {
					winmarks[0] = winmarks[1];
					winmarks[1] = a;
				}
			}
			break;
		default: return 1;
	}
	return 0;
}

int killclient(Client *c) {
	if (!c) return 1;
	return send_message(c, WM_PROTOCOLS, WM_DELETE_WINDOW);
}

int layout(const char *s) {
	switch (s[0]) {
		case 'r': m->mode = RSTACK; break;
		case 'b': m->mode = BSTACK; break;
		case 'm': m->mode = MONOCLE; break;
		case 'x': m->mode = conf.mode; break;
		default: if ( (++m->mode) == LAST_MODE ) m->mode = 0;
	}
}

int mark_client(Client *t, const char *s) {
	int n = atoi(s);
	if (n > 1 && n < 10) winmarks[n] = t;
	else if (n == 1) { winmarks[0] = winmarks[1]; winmarks[1] = t; }
	return 0;
}

int mod_bar(const char *s) {
	Container *C;
	Bar *b;
	if (!m->focus || !(b=m->focus->bar)) return 1;
	switch (s[0]) {
		case 's': b->opts &= ~BAR_HIDE; break;
		case 'h': b->opts |= BAR_HIDE; break;
		case 'x': b->opts ^= BAR_HIDE; break;
		case 't': b->opts &= ~BAR_BOTTOM; break;
		case 'b': b->opts |= BAR_BOTTOM; break;
		case 'S': for (C = m->container; C; C = C->next)
			C->bar->opts &= ~BAR_HIDE; break;
		case 'H': for (C = m->container; C; C = C->next)
			C->bar->opts |= BAR_HIDE; break;
		case 'X': for (C = m->container; C; C = C->next)
			C->bar->opts ^= BAR_HIDE; break;
		case 'T': for (C = m->container; C; C = C->next)
			C->bar->opts &= ~BAR_BOTTOM; break;
		case 'B': for (C = m->container; C; C = C->next)
			C->bar->opts |= BAR_BOTTOM; break;
	}
}

int mod_container(const char *s) {
fprintf(stderr,"MOD CONTAINER %s\n",s);
	if (!m->focus) return 1;
	int n, *t, r;
	switch (s[0]) {
		case 'n': t = &m->focus->n; r = m->focus->nn; break;
		case 's': t = &m->split; r = conf.split; break;
		case 'g': t = &m->gap; r = conf.gap; break;
	}
	switch (s[1]) {
		case 'i': *t += 1; break;
		case 'd': *t -= 1; break;
		case 'r': *t = r; break;
		default: n = atoi(&s[1]); *t = n; break;
	}
}

int monitor(const char *s) {
	int n1, n2, n3, n4;
	if (*s == 'm') { /* margins */
		if (s[1] == 'r')
			m->margin = conf.margin;
		else {
			sscanf(s,"%d,%d,%d,%d", &n1, &n2, &n3, &n4);
			m->margin.top = n1;
			m->margin.bottom = n2;
			m->margin.left = n3;
			m->margin.right = n4;
		}
	}
	else { /* focus another monitor */
		/* TODO needs testing */
		int n;
		if (!(n=atoi(s))) return 1;
		Monitor *M;
		for (M = mons; n > 1 && M; n--, M = M->next);
		if (M) m = M;
	}
	return 0;
}

int move(Client *t, const char *s) {
	Client *c, *a, *b;
	if (!(c=winmarks[1])) return 1;
	if (t) { /* move focused relative to t */
		if (!(m->focus && m->focus->top)) return 1;
		if (c == t) return 2;
		switch (s[0]) {
			case 'a': pull_client(c); push_client(c, t); break;
			case 'b': pull_client(c); push_client(c, t->next); break;
		}
	}
	else { /* move focused in direction */
		switch (s[0]) {
			case 'h': pull_client(c); push_client(c, clients); break;
			case 'j':
				for (a = c->next; a; a = a->next)
					if (tile_check(m, a)) break;
				if (a) for (a = a->next; a; a = a->next)
					if (tile_check(m, a)) break;
				pull_client(c); push_client(c, a);
				break;
			case 'k':
				for (a = clients; a && a != c; a = a->next)
					if (tile_check(m, a)) b = a;
				if (b) { pull_client(c); push_client(c, b); }
				break;
			case 'l': pull_client(c); push_client(c, NULL); break;
		}
	}
}

int pull_client(Client *c) {
	Client *cur, *pre = NULL;
	for (cur = clients; cur != c; cur = cur->next) pre = cur;
	if (pre) pre->next = cur->next;
	else clients = cur->next;
	cur->next = NULL;
	return 0;
}

int push_client(Client *insert, Client *before) {
	Client *cur, *pre = NULL;
	for (cur = clients; cur != before; cur = cur->next) pre = cur;
	if (pre) pre->next = insert;
	else clients = insert;
	insert->next = cur;
	return 0;
}

int spawn(const char *cmd) {
	if (cmd[0] == '\0') return 0;
	if (!fork()) {
		setsid();
		close(ConnectionNumber(dpy));
		if (conf.stat) fclose(conf.stat);
		if (conf.statfd) close(conf.statfd);
		char **arg = NULL, *tok = NULL, *tmp = strdup(cmd);
		tok = strtok(tmp," ");
		int i;
		for (i = 0; tok; i++) {
			arg = realloc(arg, (i+1) * sizeof(char *));
			arg[i] = strdup(tok);
			tok = strtok(NULL," ");
		}
		free(tmp);
		arg = realloc(arg, (i+1) * sizeof(char *));
		arg[i] = NULL;
		execvp(arg[0], (char * const *) arg);
	}
	return 0;
}

int tag(Client *t, const char *s) {
	int tag = atoi(s+1);
	if (!tag || tag > 15) return 1;
	tag = (1<<(tag-1));
	int *n;
	if (!t) n = &m->tags;
	else n = &t->tags;
	switch (*s) {
		case 'x': *n = (*n & 0xFFFF0000) | tag; break;
		case 't': *n ^= tag; break;
		case 'a': *n |= tag; break;
		case 'r': *n &= ~tag; break;
		default: return 1;
	}
}


/*
a c e  hijk   opqr  u wxy
ABCDE GHIJKLMNOP RSTUVWXYZ
*/
int word(const char *word) {
	const char *s = word;
	/* get target */
	Client *t = winmarks[1], *wt = NULL;
	int n;
	if (s[0] == 'w') {
		s++;
		if ( !(n=atoi(s)) ) n = 1;
		if (n > 0 && n < 10) wt = winmarks[n];
		if (!wt) return 1;
		while (*s >= '0' && *s <= '9') s++;
	}
	/* process command */
	switch (s[0]) {
		case 'b': mod_bar(&s[1]); break;
		case 'd': monitor(&s[1]); break;
		case 'f': focus(wt, &s[1]); break;
		case 'F': float_full(wt); break;
		case 'g': case 'n': case 's': mod_container(s);  break;
		case 'l': layout(&s[1]); break;
		case 'm': move(wt, &s[1]); break;
		case 'q': killclient(t); break;
		case 'Q': running = False; break;
		case 't': tag(wt, &s[1]); break;
		case 'v': n = (m->tags<<16) & 0xFFFF0000;
			n |= (m->tags>>16) & 0xFFFF;
			m->tags = n;
			break;
		case 'z': mark_client(t, &s[1]); break;
	}
	tile();
	return 0;
}

