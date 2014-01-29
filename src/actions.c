
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

int focus(Client *t, const char *s) {
	if (t) {
		winmarks[0] = winmarks[1];
		winmarks[1] = t;
		return 0;
	}
	if (!m->focus || !m->focus->top) return 1;
	Container *C;
	Client *c = m->focus->top, *a, *b;
	switch (*s) {
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
			else {
				/* focus first container, or next container */
				C = m->container;
				if (C->top == winmarks[1]) C = C->next;
				if (C->top) {
					winmarks[0] = winmarks[1];
					winmarks[1] = C->top;
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

int move(Client *t, const char *s) {
	if (t) { /* move focused relative to t */
		if (!(m->focus && m->focus->top)) return 1;
		Client *f = m->focus->top;
		if (f == t) return 2;
		switch (s[0]) {
			case 'a': pull_client(f); push_client(f, t); break;
			case 'b': pull_client(f); push_client(f, t->next); break;
			case 's': /* TODO swap */ break;
		}
	}
	else { /* move focused in direction */
		switch (s[0]) {
			case 'h': break;
			case 'j': break;
			case 'k': break;
			case 'l': break;
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
		close(ConnectionNumber(dpy));
		if (conf.stat) fclose(conf.stat);
		if (conf.statfd) close(conf.statfd);
		setsid();
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
		case 'x': *n = tag; break;
		case 't': *n ^= tag; break;
		case 'a': *n |= tag; break;
		case 'r': *n &= ~tag; break;
		default: return 1;
	}
}

/*
Command:
	word [word [word [word...]]]
Word:
	[target]verb[options]
	(expression?word:word)
	{expression?word}
Target:
	[0-9]+(h|j|k|l|w)
Verb:  (TODO add monitor controls)
	(f|m|t|q|Q|n|s|g|b|v|z)
Options:
	<depend on verb>
Expression:
	getter(=<>)value
Getter:
	(F|N|S|G|B|Z)
Value:
	w[0-9]+

a cde  -i---  op r  u -xy
A CDE  HIJKLM OP R TUV XY

target: [0-9](h|j|k|l|w)
	[target] verb [options]

	verb
f focus		target focus
f focus		focus (left|right|up|down|prev/alt) (h|j|k|l|p|a)
m move		target move-focused (before|after|swap) (b|a|s)
m move		move-focused (left|right|up|down) (h|j|k|l)
t tag		target tag (move|add|remove|toggle) (x|a|r|t)#
t tag		tag (single|add|remove|toggle) (x|a|r|t)#
		mode (mono,rstack,bstack)
q quit		target kill
q quit		kill-focused
Q Quit		kill wm (quit)
n nclient		nclients (up|down|one|many)
s split		split (up|down|reset)
g gap		gap (up|down|reset)
b bar		bar (show|hide|top|bottom)
v view		view-swap
z Windowmark num
		monitor ...

F get-focused
N get-nclients
S get-split
G get-gap
B get-bar (show|top)
Z get-window-num
*/

int word(const char *word) {
	const char *s = word;
	/* get target */
	Client *t = NULL, *dt = NULL;
	if (m->focus && m->focus->top) dt = m->focus->top;
	int n = atoi(s);
	if (!n) n = 1;
	while (*s >= '0' && *s <= '9') s++;
	switch (s[0]) {
		// TODO
		case 'j': s++; break;
		case 'k': s++; break;
		case 'h': s++; break;
		case 'l': s++; break;
		case 'w': t = winmarks[n]; s++; break;
	}
	if (t) dt = t;
	/* process command */
	switch (s[0]) {
		case 'f': focus(t, &s[1]); break;
		case 'm': move(t, &s[1]); break;
		case 't': tag(t, &s[1]); break;
		case 'q': killclient(dt); break;
		case 'Q': running = False; break;
		case 'n': break;
		case 's': break;
		case 'g': break;
		case 'b': break;
		case 'v': break;
		case 'z': break;
	}
	tile();
	return 0;
}

