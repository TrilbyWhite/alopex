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
static const char *mode(int, const char *);
static const char *nclients(int, const char *);
static const char *other(int, const char *);
static const char *quit(int, const char *);
static const char *size(int, const char *);
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
			case 'J': case 'K': case 'H': case 'L': case '@':
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
			case 'Q': c = quit(n,c); break;
			default: break;
		}
	}
	return trigger;
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

const char *bar(int n, const char *c) {return ++c;}

const char *command(int n, const char *ch) {
	if (*(++ch)=='\0') return ch;
	int i; const char *s = string[*ch];
	if (!s) return (++ch);
	for (i = 0; i < (n ? n : 1); i++) {
		if (s[strlen(s)-1] == '&') system(string[*ch]);
		else key_chain(s);
	}
	return (++ch);
}

const char *focus_move(int n, const char *ch) {
	int i;
	for (i = 0; i < n; i++) {
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

const char *mode(int n, const char *ch) {
	if (*ch == 'M') m->mode = n;
	else if (*ch == 'g') m->gap = n;
	trigger = 2;
	return (++ch);
}

const char *move(int n, const char *c) {return ++c;}
const char *nclients(int n, const char *c) {return ++c;}

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
	running = False;
	return (++ch);
}

const char *size(int n, const char *c) {return ++c;}

const char *tag(int n, const char *ch) {
	if (!n) return (++ch);
	else if ((--n) > 15) n = 15;
	else if (n < 0) n = 0;
	if (*ch == 't') m->tags ^= (1<<n);
	else if (*ch == 's') m->tags |= (1<<n);
	else if (*ch == 'S') m->tags &= ~(1<<n);
	else if (*ch == 'x') m->tags = (m->tags & 0xFF00) | (1<<n);
	else if (*ch == 'X') m->tags &= ((1<<n) ^ 0x00FF);
	trigger = 2;
	return (++ch);
}

const char *toTag(int n, const char *ch) {
	if (!n || !target) return (++ch);
	else if ((--n) > 15) n = 15;
	else if (n < 0) n = 0;
	if (*ch == 'T') target->tags ^= (1<<n);
	if (*ch == 'm') target->tags = (1<<n);
	if (*ch == 'a') target->tags |= (1<<n);
	if (*ch == 'A') target->tags &= ~(1<<n);
	return (++ch);
}

const char *view(int n, const char *c) {return ++c;}

void swap(Client *a, Client *b) {
	if (!a || !b) return;
	Client t;
	t.tags = a->tags; a->tags = b->tags; b->tags = t.tags;
	t.win = a->win; a->win = b->win; b->win = t.win;
	// title, etc
}
