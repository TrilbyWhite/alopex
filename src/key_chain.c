/********************************************************************\
* KEY_CHAIN.C - part of alopex, see alopex.c for license information
*
* Author: Jesse McClure, copyright 2012-2013
\********************************************************************/

/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

#include "alopex.h"

static const char *bar(const char *);
static const char *close_win(const char *);
static const char *command(const char *);
static const char *focus_move(const char *);
static const char *mode(const char *);
static const char *nclients(const char *);
static const char *other(const char *);
static const char *quit(const char *);
static const char *size(const char *);
static const char *tag(const char *);
static const char *toggle(const char *);
static const char *toTag(const char *);
static const char *view(const char *);
static void swap(Client *, Client *);

static int rep;
static int trigger;

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int key_chain(const char *chain) {
	trigger = 0;
	const char *c = chain;
	while (*c) {
		if ( (rep=atoi(c)) ) for (c; *c && *c > '0' && *c < ':'; c++);
		else rep = 1;
		switch (*c) {
			case 'b': c = bar(c); break;
			case 'c': c = command(c); break;
			case 't': case 's': case 'S': case 'x': case 'X':
				c = tag(c); break;
			case 'm': case 'a': case 'A': c = toTag(c); break;
			case 'v': c = view(c); break;
			case 'j': case 'k': case 'h': case 'l':
			case 'J': case 'K': case 'H': case 'L':
				c = focus_move(c); break;
			case 'o': c = other(c); break;
			case 'i': case 'd': c = size(c); break;
			case '+': case '-': case '<': case '>': c = nclients(c); break;
			case 'f': case 'F': c = toggle(c); break;
			case 'T': c = mode(c); break;
			case 'q': c = close_win(c); break;
			case 'Q': c = quit(c); break;
			default: break;
		}
	}
	return trigger;
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

const char *bar(const char *c) {return ++c;}
const char *close_win(const char *c) {return ++c;}

const char *command(const char *ch) {
	if (*(++ch)=='\0') return ch;
	int i;
	const char *arg[4];
	arg[0] = "/bin/sh"; arg[1] = "-c"; arg[3] = NULL;
	arg[2] = string[*ch];
	for (i = 0; i < rep; i++) {
		if (fork() == 0) execv(arg[0],(char * const *)arg);
	}
	return (++ch);
}

const char *focus_move(const char *ch) {
	int i;
	for (i = 0; i < rep; i++) {
		/* find next and previous containers (CN CP) */
		Container *C, *CP = NULL, *CN = NULL, *CC;
		for (C = m->container; C != m->focus; CP = C, C = C->next);
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
		else if (*ch == 'J' && CN) swap(C->top,CN->top);
		else if (*ch == 'K' && CP) swap(C->top,CP->top);
		else if (*ch == 'L' && cn) swap(C->top,cn);
		else if (*ch == 'H' && cp) swap(C->top,cp);
	}
	trigger = 2;
	return (++ch);
}

const char *mode(const char *c) {return ++c;}
const char *move(const char *c) {return ++c;}
const char *nclients(const char *c) {return ++c;}
const char *other(const char *c) {return ++c;}

const char *quit(const char *ch) {
	running = False;
	return (++ch);
}

const char *size(const char *c) {return ++c;}

const char *tag(const char *ch) {
	int t = atoi(ch+1);
	if (!t) return (++ch);
	else if ((--t) > 15) t = 15;
	else if (t < 0) t = 0;
	if (*ch == 't') m->tags ^= (1<<t);
	else if (*ch == 's') m->tags |= (1<<t);
	else if (*ch == 'S') m->tags &= ~(1<<t);
	else if (*ch == 'x') m->tags = (m->tags & 0xFF00) | (1<<t);
	else if (*ch == 'X') m->tags &= ((1<<t) ^ 0x00FF);
	for (ch++; *ch > 47 && *ch < 58; ch++);
	trigger = 2;
	return ch;
}

const char *toggle(const char *c) {return ++c;}
const char *toTag(const char *c) {return ++c;}
const char *view(const char *c) {return ++c;}

void swap(Client *a, Client *b) {
	if (!a || !b) return;
	Client t;
	t.tags = a->tags; a->tags = b->tags; b->tags = t.tags;
	t.win = a->win; a->win = b->win; b->win = t.win;
	// title, etc
}
