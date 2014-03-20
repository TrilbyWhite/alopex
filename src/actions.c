
#include "alopex.h"
#include "actions.h"

extern Bool tile_check(Monitor *, Client *);
extern int send_message(Client *, int, int);


int command(const char *cmd) {
	//if (cmd[0] == '!') return spawn(cmd+1);
	char *tok, *sptr, *str;
	str = strdup(cmd);
	for (tok = strtok_r(str, ";", &sptr); tok;
			tok = strtok_r(NULL, ";", &sptr))
		word(tok);
	free(str);
	return 0;
}

void conftile(Client *t, const char **arg) {
	if (!m->focus || !arg[0]) return;
	int n, *opt = NULL, r;
	switch (arg[0][0]) {
		case 'n': opt = &m->focus->n; r = m->focus->nn; break;
		case 's': opt = &m->split; r = conf.split; break;
		case 'g': opt = &m->gap; r = conf.gap; break;
	}
	if (!opt || !arg[1]) return;
	if (arg[2]) n = atoi(arg[2]);
	else n = 1;
	switch (arg[1][0]) {
		case 'i': *opt += n; break;
		case 'd': *opt -= n; break;
		case 'r': *opt = r; break;
		case 's': *opt = n; break;
	}
}

void floater(Client *t, const char **arg) {
	if (!t && !(t=winmarks[1])) return;
	if (t->flags & WIN_FLOAT) t->flags &= ~WIN_FULL;
	else t->flags |= WIN_FLOAT;
}

void fullscreen(Client *t, const char **arg) {
	if (!t && !(t=winmarks[1])) return;
	if (t->flags & WIN_FULL_TEST) t->flags &= ~WIN_FULL;
	else t->flags |= WIN_FULL;
}

void focus(Client *t, const char **arg) {
	if (t) {
		winmarks[0] = winmarks[1];
		winmarks[1] = t;
		return;
	}
	Container *C;
	Client *c, *a, *b;
	if (!(c=winmarks[1])) return;
	if (strncasecmp(arg[0],"flo",3)==0) { /* focus floating */
		for (a = winmarks[1]->next; a; a = a->next)
			if (a->flags & WIN_FLOAT) break;
		if (!a) for (a = clients; a; a = a->next)
			if (a->flags & WIN_FLOAT) break;
		if (!a) return;
		winmarks[0] = winmarks[1];
		winmarks[1] = a;
	}
	else if (strncasecmp(arg[0],"top",3)==0) { /* focus top */
		for (C = m->container; C && C->next; C = C->next) {
			if (C->next == m->focus) {
				winmarks[0] = winmarks[1];
				winmarks[1] = C->top;
			}
		}
	}
	else if (strncasecmp(arg[0],"dow",3)==0) { /* focus down */
		for (a = c->next; a; a = a->next)
			if (tile_check(m, a)) break;
		if (a) {
			winmarks[0] = winmarks[1];
			winmarks[1] = a;
		}
	}
	else if (strncasecmp(arg[0],"up",2)==0) { /* focus up */
		for (a = clients; a && a != c; a = a->next)
			if (tile_check(m, a)) b = a;
		if (b) {
			winmarks[0] = winmarks[1];
			winmarks[1] = b;
		}
	}
	else if (strncasecmp(arg[0],"bot",3)==0) { /* focus bottom */
		if (m->focus->next && m->focus->next->top) {
			winmarks[0] = winmarks[1];
			winmarks[1] = m->focus->next->top;
		}
	}
	else if (strncasecmp(arg[0],"pre",3)==0) { /* focus previous */
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
	}
}

void killclient(Client *c, const char **arg) {
	if (!c) return;
	send_message(c, WM_PROTOCOLS, WM_DELETE_WINDOW);
}

void layout(Client *c, const char **arg) {
	if (!arg[0]) { if ( (++m->mode) == LAST_MODE ) m->mode = 0; }
	else if (strncasecmp(arg[0],"rst",3)==0) m->mode = RSTACK;
	else if (strncasecmp(arg[0],"bst",3)==0) m->mode = BSTACK;
	else if (strncasecmp(arg[0],"mon",3)==0) m->mode = MONOCLE;
	else if (strncasecmp(arg[0],"def",3)==0) m->mode = conf.mode;
}

void mark_client(Client *t, const char **arg) {
	int n = atoi(arg[0]);
	if (n > 1 && n < 10) winmarks[n] = t;
	else if (n == 1) { winmarks[0] = winmarks[1]; winmarks[1] = t; }
}

void mod_bar(Client *t, const char **arg) {
	if (!arg[0]) return;
	Container *C;
	Bar *b;
	if (!m->focus || !(b=m->focus->bar)) return;
	int all = 0;
	if (arg[1] && strncasecmp(arg[1],"all",3)==0) all = 1;
	if (all) {
		if (strncasecmp(arg[0],"sho",3)==0)
			for (C = m->container; C; C = C->next)
				C->bar->opts &= ~BAR_HIDE;
		else if (strncasecmp(arg[0],"hid",3)==0)
			for (C = m->container; C; C = C->next)
				C->bar->opts |= BAR_HIDE;
		else if (strncasecmp(arg[0],"tog",3)==0)
			for (C = m->container; C; C = C->next)
				C->bar->opts ^= BAR_HIDE;
		else if (strncasecmp(arg[0],"top",3)==0)
			for (C = m->container; C; C = C->next)
				C->bar->opts &= ~BAR_BOTTOM;
		else if (strncasecmp(arg[0],"bot",3)==0)
			for (C = m->container; C; C = C->next)
				C->bar->opts |= BAR_BOTTOM;
	}
	else {
		if (strncasecmp(arg[0],"sho",3)==0) b->opts &= ~BAR_HIDE;
		else if (strncasecmp(arg[0],"hid",3)==0) b->opts |= BAR_HIDE;
		else if (strncasecmp(arg[0],"tog",3)==0) b->opts ^= BAR_HIDE;
		else if (strncasecmp(arg[0],"top",3)==0) b->opts &= ~BAR_BOTTOM;
		else if (strncasecmp(arg[0],"bot",3)==0) b->opts |= BAR_BOTTOM;
	}
}

void monitor(Client *t, const char **arg) {
//	int n1, n2, n3, n4;
//	if (*s == 'm') { /* margins */
//		if (s[1] == 'r')
//			m->margin = conf.margin;
//		else {
//			sscanf(s,"m%d,%d,%d,%d", &n1, &n2, &n3, &n4);
//			m->margin.top = n1;
//			m->margin.bottom = n2;
//			m->margin.left = n3;
//			m->margin.right = n4;
//		}
//	}
//	else if (*s == 's') { /* swap tags */
//		sscanf(s,"s%d,%d", &n1, &n2);
//		Monitor *M, *a = NULL, *b = NULL;
//		for (M = mons, n3 = 1; M; n3++, M = M->next) {
//			if (n1 == n3) a = M;
//			else if (n2 == n3) b = M;
//		}
//		if (a && b) {
//			n4 = a->tags;
//			a->tags = b->tags;
//			b->tags = n4;
//		}
//	}
//	else if (*s == 'c') { /* cycle through monitors */
//		m = m->next;
//		if (!m) m = mons;
//	}
//	else { /* focus another monitor */
//		n1 = atoi(s);
//		Monitor *M;
//		for (M = mons; n1 > 0 && M; n1--, M = M->next) m = M;
//	}
//	return 0;
}

void move(Client *t, const char **arg) {
	Client *c, *a, *b;
	if (!(c=winmarks[1]) || !arg || !arg[0]) return;
	if (t) { /* move focused window relative to t */
		if (!(m->focus && m->focus->top) || c == t) return;
		switch (arg[0][0]) {
			case 'a': pull_client(c); push_client(c, t); break;
			case 'b': pull_client(c); push_client(c, t->next); break;
		}
		return;
	}
	/* else move focused in direction */
	if (strncasecmp(arg[0],"top",3)==0) {
		pull_client(c); push_client(c, clients);
	}
	else if (strncasecmp(arg[0],"dow",3)==0) {
		for (a = c->next; a; a = a->next)
			if (tile_check(m, a)) break;
		if (a) for (a = a->next; a; a = a->next)
			if (tile_check(m, a)) break;
		pull_client(c); push_client(c, a);
	}
	else if (strncasecmp(arg[0],"up",3)==0) {
		for (a = clients; a && a != c; a = a->next)
			if (tile_check(m, a)) b = a;
		if (b) { pull_client(c); push_client(c, b); }
	}
	else if (strncasecmp(arg[0],"up",3)==0) {
		pull_client(c); push_client(c, NULL);
	}
}

void pull_client(Client *c) {
	Client *cur, *pre = NULL;
	for (cur = clients; cur != c; cur = cur->next) pre = cur;
	if (pre) pre->next = cur->next;
	else clients = cur->next;
	cur->next = NULL;
}

void push_client(Client *insert, Client *before) {
	Client *cur, *pre = NULL;
	for (cur = clients; cur != before; cur = cur->next) pre = cur;
	if (pre) pre->next = insert;
	else clients = insert;
	insert->next = cur;
}

void spawn(Client *t, const char **arg) {
	if (!arg || !arg[0]) return;
	if (!fork()) {
		setsid();
		close(ConnectionNumber(dpy));
		if (conf.stat) fclose(conf.stat);
		if (conf.statfd) close(conf.statfd);
		execvp(arg[0], (char * const *) arg);
	}
}

void tag(Client *t, const char **arg) {
	if (!arg || !arg[0] || !arg[1]) return;
	int tag = atoi(arg[1]);
	if (!tag || tag > 15) return;
	tag = (1<<(tag-1));
	int *n;
	if (!t) n = &m->tags;
	else n = &t->tags;
	switch (arg[0][0]) { /* set toggle add remove */
		case 's': *n = (*n & 0xFFFF0000) | tag; break;
		case 't': *n ^= tag; break;
		case 'a': *n |= tag; break;
		case 'r': *n &= ~tag; break;
	}
}

void word(const char *word) {
	char **arg = NULL, *tok, *sptr, *tmp;
	const char **parg;
	int i, n;
	tmp = strdup(word);
	tok = strtok_r(tmp, " ", &sptr);
	for (i = 0; tok; i++) {
		arg = realloc(arg, (i+1) * sizeof(char *));
		arg[i] = tok;
		tok = strtok_r(NULL," ", &sptr);
	}
	arg = realloc(arg, (i+1) * sizeof(char *));
	arg[i] = NULL;
	parg = (const char **) arg;
	/* get target */
	Client *t = winmarks[1], *wt = NULL;
	if ( (strncasecmp(arg[0],"win",3)==0) && arg[1] ) {
		if ( !(n=atoi(arg[1])) ) {
			n = 1;
			parg = (const char **) &arg[1];
		}
		else {
			parg = (const char **) &arg[2];
		}
		if (n > 0 && n < 10) wt = winmarks[n];
		if (!wt) return;
	}
	/* process command */
	if (strncasecmp(parg[0],"bar",3) == 0) mod_bar(wt,&parg[1]);
	else if (strncasecmp(parg[0],"mon",3) == 0) monitor(wt,&parg[1]);
	else if (strncasecmp(parg[0],"foc",3) == 0) focus(wt,&parg[1]);
	else if (strncasecmp(parg[0],"flo",3) == 0) floater(wt,&parg[1]);
	else if (strncasecmp(parg[0],"ful",3) == 0) fullscreen(wt,&parg[1]);
	else if (strncasecmp(parg[0],"spl",3) == 0) conftile(wt,&parg[0]);
	else if (strncasecmp(parg[0],"gap",3) == 0) conftile(wt,&parg[0]);
	else if (strncasecmp(parg[0],"num",3) == 0) conftile(wt,&parg[0]);
	else if (strncasecmp(parg[0],"lay",3) == 0) layout(wt,&parg[1]);
	else if (strncasecmp(parg[0],"exe",3) == 0) spawn(wt,&parg[1]);
	else if (strncasecmp(parg[0],"mov",3) == 0) move(wt,&parg[1]);
	else if (strncasecmp(parg[0],"kil",3) == 0) killclient(wt,&parg[1]);
	else if (strncasecmp(parg[0],"qui",3) == 0) running = False;
	else if (strncasecmp(parg[0],"tag",3) == 0) tag(wt,&parg[1]);
	else if (strncasecmp(parg[0],"mar",3) == 0) mark_client(wt, &parg[1]);
	else if (strncasecmp(parg[0],"vie",3) == 0) {
		n = (m->tags<<16) & 0xFFFF0000;
		n |= (m->tags>>16) & 0xFFFF;
		m->tags = n;
	}
	free(arg);
	free(tmp);
	tile();
}

