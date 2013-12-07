/********************************************************************\
* KEY_CHAIN.C - part of alopex, see alopex.c for license information
*
* Author: Jesse McClure, copyright 2012-2013
\********************************************************************/

/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

#include "alopex.h"

static const char *bar(int, const char *);
static const char *command(int, const char *);
static const char *focus_move(int, const char *);
static const char *killclient(int, const char *);
static const char *loop(int, const char *);
static const char *mode(int, const char *);
static const char *nclients(int, const char *);
static const char *other(int, const char *);
static const char *quit(int, const char *);
static const char *size(int, const char *);
static const char *ternary(int, const char *);
static const char *tag(int, const char *);
static const char *toTag(int, const char *);
static const char *view(int, const char *);
static void swap(Client *, Client *);

static int trigger;
static Client *target;

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int key_chain(const char *chain) {
	trigger = 0;
	const char *c = chain; int n;
	target = winmarks[0];
	while (*c) {
		n = atoi(c);
		for (c; *c && *c > '/' && *c < ':'; c++);
		switch (*c) {
			case 'j': case 'k': case 'h': case 'l':
			case 'J': case 'K': case 'H': case 'L':
				c = focus_move(n,c); break;
			case 'o': c = other(n,c); break;
			case 't': case 's': case 'S': case 'x': case 'X':
				c = tag(n,c); break;
			case 'T': case 'm': case 'a': case 'A': c = toTag(n,c); break;
			case 'v': c = view(n,c); break;
			case 'b': c = bar(n,c); break;
			case 'i': case 'd': c = size(n,c); break;
			case '+': case '-': case '<': case '>': c = nclients(n,c); break;
			case 'M': case 'g': c = mode(n,c); break;
			case ';': case ':': c = command(n,c); break;
			case 'q': c = killclient(n,c); break;
			case 'Q': case 'R': c = quit(n,c); break;
			case '(': c = ternary(n,c); break;
			case '{': c = loop(n,c); break;
			default: break;
		}
	}
	return trigger;
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

#define OP_EQUALS		0x03
#define OP_GREATER	0x02
#define OP_LESS		0x01

static Bool condition(const char *exp) {
	int op = OP_EQUALS;
	switch (*exp) {
		case '<': op--; case '>': op--;
		case '=': exp++; default: break;
	}
	int n;
	n = atoi(exp);
	for (exp; *exp && *exp > '/' && *exp < ':'; exp++);
	switch (*exp) {
		case 'w':
			return winmarks[n] == winmarks[2];
			break;
		case 'g':
			if (op == OP_EQUALS) return m && m->gap == n;
			if (op == OP_GREATER) return m && m->gap > n;
			if (op == OP_LESS) return m && m->gap < n;
			break;
		case 'M':
			return m && m->mode == n;
			break;
		default: return False;
	}
}

const char *bar(int n, const char *ch) {
	Bar *b;
	if (n) {
		Container *C;
		C = m->container;
		int i;
		for (i = 1; i < n && C->next; i++, C = C->next);
		b = &C->bar;
	}
	else if (m->focus) b = &m->focus->bar;
	else b = &m->container->bar;
	if (!b) return (++ch);
	if (*(++ch) == 's') b->opts |= AR_VISIBLE;
	lse if (*ch == 'h') b->opts &= ~BAR_VISIBLE;
	else if (*ch == 't') b->opts |= BAR_TOP;
	else if (*ch == 'b') b->opts &= ~BAR_TOP;
	else if (*ch == 'x') b->opts ^= BAR_VISIBLE;
	trigger = 2;
	return (++ch);
}

const char *command(int n, const char *ch) {
	if (*(++ch)=='\0') return ch;
	int i; const char *s = string[*ch];
	if (!s) return (++ch);
	for (i = 0; i < (n ? n : 1); i++) {
		if (s[strlen(s)-1] == '&') system(string[*ch]);
		else trigger = key_chain(s);
	}
	return (++ch);
}

const char *focus_move(int n, const char *ch) {
	int i;
	for (i = 0; i < (n ? n : 1); i++) {
		Container *C, *CP = NULL, *CN = NULL, *CC;
		/* find next and previous containers (CN CP) */
		for (C = m->container; C && C != m->focus; CP = C, C = C->next);
		C = m->focus; CN = C->next;
		if (CN && !CN->top) CN = NULL;
		/* find next and previous clients (cn cp) */
		Client *c, *cp = NULL, *cn = NULL;
		int n = 0;
		/* count up to container */
		for (CC = m->container; CC != C; CC = CC->next) n += CC->n;
		for (c = clients; n; c = c->next) if (c->tags & m->tags) n--;
		/* get previous and next */
		for (c; c != C->top; c = c->next)
			if (c->tags & m->tags) { cp = c; n++; }
		for (c = c->next; c && !(c->tags & m->tags); c = c->next);
		if (n < C->n || C->n == -1) cn = c;
		else cn = NULL;
		if (*ch == 'j' && CN) m->focus = CN;
		else if (*ch == 'k' && CP) m->focus = CP; 
		else if (*ch == 'l' && cn) C->top = cn; 
		else if (*ch == 'h' && cp) C->top = cp;
		else if (*ch == 'J' && CN) swap(target,CN->top);
		else if (*ch == 'K' && CP) swap(target,CP->top);
		else if (*ch == 'L' && cn) swap(target,cn);
		else if (*ch == 'H' && cp) swap(target,cp);
	}
	trigger = 2;
	return (++ch);
}

const char *killclient(int n, const char *ch) {
	Client *c = target;
	if (n && !(c=winmarks[n])) return (++ch);
	XEvent ev;
	ev.type = ClientMessage; ev.xclient.window = c->win;
	ev.xclient.message_type = XInternAtom(dpy,"WM_PROTOCOLS",True);
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = XInternAtom(dpy,"WM_DELETE_WINDOW",True);
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(dpy,c->win,False,NoEventMask,&ev);
	winmarks[n] = NULL;
	trigger = 2;
	return (++ch);
}

const char *loop(int n, const char *ch) {
	return (++ch);
}

const char *mode(int n, const char *ch) {
	if (*ch == 'M') {
		if (n == m->mode || (n == MONOCLE && m->mode < 0)) return (++ch);
		if (n == MONOCLE) {
			m->mode = -1 * m->container->n;
			m->container->n = -1;
		}
		else {
			if (m->mode < 0) m->container->n = -1 * m->mode;
			m->mode = n;
		}
	}
	else if (*ch == 'g') m->gap = n;
	trigger = 2;
	return (++ch);
}

const char *nclients(int n, const char *ch) {
	if (!m->focus || !m->focus->next) return;
	if (*ch == '+') m->focus->n += (n ? n : 1);
	if (*ch == '-') m->focus->n -= (n ? n : 1);
	if (*ch == '<') m->focus->n = -1;
	if (*ch == '>') m->focus->n = 1;
	if (m->focus->n == 0) m->focus->n = 1;
	trigger = 2;
	return (++ch);
}

const char *other(int n, const char *ch) {
	int i;
	if (m->focus == m->container) {
		for (i = 0; i < (n ? n : 1); i++)
			if (m->focus->next) m->focus = m->focus->next;
	}
	else {
		m->focus = m->container;
	}
	trigger = 2;
	return (++ch);
}

const char *quit(int n, const char *ch) {
	if (*ch == 'Q') running = False;
	else if (*ch == 'R')
		reconfig();
	trigger = 2;
	return (++ch);
}

const char *size(int n, const char *ch) {
	if (*ch == 'i') m->split += (n ? n : 1);
	else if (*ch == 'd') m->split -= (n ? n : 1);
	trigger = 2;
	return (++ch);
}

const char *tag(int n, const char *ch) {
	if (!n) return (++ch);
	else if ((--n) >= ntags) return (++ch);
	else if (n < 0) n = 0;
	if (*ch == 't') m->tags ^= (1<<n);
	else if (*ch == 's') m->tags |= (1<<n);
	else if (*ch == 'S') m->tags &= ~(1<<n);
	else if (*ch == 'x') m->tags = (m->tags & 0xFF00) | (1<<n);
	else if (*ch == 'X') m->tags &= ((1<<n) ^ 0x00FF);
	trigger = 2;
	return (++ch);
}

const char *ternary(int n, const char *ch) {
	char *str, *t, start, end;
	ch++;
	if (condition(ch)) { start = '?'; end = ':'; }
	else { start = ':'; end = ')'; }
	for (ch; *ch && *ch != start; ch++);
	str = strdup( (++ch) );
	if ( (t=strchr(str,end)) ) *t = '\0';
	key_chain(str);
	free(str);
	t = strchr(ch,')');
	return (++t);
}

const char *toTag(int n, const char *ch) {
	if (!n || !target) return (++ch);
	else if ((--n) >= ntags) return (++ch);
	else if (n < 0) n = 0;
	if (*ch == 'T') target->tags ^= (1<<n);
	if (*ch == 'm') target->tags = (1<<n);
	if (*ch == 'a') target->tags |= (1<<n);
	if (*ch == 'A') target->tags &= ~(1<<n);
	trigger = 2;
	return (++ch);
}

const char *view(int n, const char *ch) {
fprintf(stderr,"TAGS=%X\n",m->tags);
	int alt = (m->tags>>16) & 0xFF;
	m->tags = (m->tags<<16) | alt;
fprintf(stderr,"  TAGS=%X\n",m->tags);
	trigger = 2;
	return (++ch);
}

void swap(Client *a, Client *b) {
	if (!a || !b) return;
	Client t;
	t.tags = a->tags; a->tags = b->tags; b->tags = t.tags;
	t.win = a->win; a->win = b->win; b->win = t.win;
	// title, etc
}
