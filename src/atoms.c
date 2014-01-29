
#include "alopex.h"

static inline Atom make_atom(const char *a) {
	return XInternAtom(dpy, a, False);
}

int atoms_init() {
	atom[WM_PROTOCOLS] = make_atom("WM_PROTOCOLS");
	atom[WM_DELETE_WINDOW] = make_atom("WM_DELETE_WINDOW");
	atom[WM_STATE] = make_atom("WM_STATE");
	atom[WM_TAKE_FOCUS] = make_atom("WM_TAKE_FOCUS");
//	atom[NET_SUPPORTED] = make_atom("_NET_SUPPORTED");
//	atom[NET_WM_NAME] = make_atom("_NET_WM_NAME");
//	atom[NET_WM_STATE] = make_atom("_NET_WM_STATE");
//	atom[NET_WM_STATE_FULLSCREEN] = make_atom("_NET_WM_STATE_FULLSCREEN");
//	atom[NET_ACTIVE_WINDOW] = make_atom("_NET_ACTIVE_WINDOW");
//	atom[NET_WM_WINDOW_TYPE] = make_atom("_NET_WM_WINDOW_TYPE");
//	atom[NET_WM_TYPE_DIALOG] = make_atom("_NET_WM_TYPE_DIALOG");
//	atom[NET_CLIENT_LIST] = make_atom("_NET_CLIENT_LIST");
	return 0;
}

Atom get_atom(Client *c, int type) {
	int i;
	unsigned long ul;
	unsigned char *uc = NULL;
	Atom aa, at = None;
	if (!XGetWindowProperty(dpy, c->win, atom[type], 0L, sizeof(at),
			False, XA_ATOM, &aa, &i, &ul, &ul, &uc) == Success && uc)
		return at;
	at = *(Atom *)uc;
	XFree(uc);
	return at;
}

int send_message(Client *c, int type, int msg) {
	XEvent ev;
	ev.type = ClientMessage;
	ev.xclient.window = c->win;
	ev.xclient.message_type = atom[type];
	//ev.xclient.message_type = XInternAtom(dpy, "WM_PROTOCOLS", True);
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = atom[msg];
	//ev.xclient.data.l[0] = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	return 0;
}
