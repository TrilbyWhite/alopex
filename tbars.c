
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <X11/Xlib.h>

enum {Background, BarsNorm, BarsFull, BarsCharge, BarsWarn, BarsAlarm, LASTColor};

static void buttonpress(XEvent *);
static void expose(XEvent *);

static void draw();
static void updatestatus();

static Display *dpy;
static Window root,win;
static Drawable bar;
static GC *gc;
static Bool running=True;
static long j1,j2,j3,j4;
static char *aud_file;
static FILE *in;
static uint8_t cpu,vol,bat,cpu_col,vol_col,bat_col;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress]		= buttonpress,
//	[ButtonRelease]		= buttonrelease,
	[Expose]			= expose,
//	[MapRequest]		= maprequest,
//	[PropertyNotify]	= propertynotify,
//	[MotionNotify]		= motionnotify,
//	[UnmapNotify]		= unmapnotify
};

#define CPU_FILE        "/proc/stat"
#define BATT_NOW        "/sys/class/power_supply/BAT1/charge_now"
#define BATT_FULL       "/sys/class/power_supply/BAT1/charge_full"
#define BATT_STAT       "/sys/class/power_supply/BAT1/status"

static const char colors[LASTColor][9] = {
	[Background]	= "#101010",
	[BarsNorm]		= "#303030",
	[BarsFull]		= "#122864",
	[BarsCharge]	= "#308030",
	[BarsWarn]		= "#997700",
	[BarsAlarm]		= "#990000",
};

void buttonpress(XEvent *ev) {
}

void expose(XEvent *ev) { draw(); }

void draw() {
	XFillRectangle(dpy,bar,gc[Background],0,0,50,13);
	XFillRectangle(dpy,bar,gc[cpu_col],0,3,cpu,2);
	XFillRectangle(dpy,bar,gc[vol_col],0,6,vol,2);
	XFillRectangle(dpy,bar,gc[bat_col],0,9,bat,2);
	XCopyArea(dpy,bar,win,gc[0],0,0,50,13,0,0);
	XFlush(dpy);
}

void updatestatus() {
	static long ln1,ln2,ln3,ln4;
	static int n;
	static char c;
	if ( (in=fopen(CPU_FILE,"r")) ) {       /* CPU MONITOR */
		fscanf(in,"cpu %ld %ld %ld %ld",&ln1,&ln2,&ln3,&ln4);
		fclose(in);
		if (ln4>j4) n=(int)100*(ln1-j1+ln2-j2+ln3-j3)/(ln1-j1+ln2-j2+ln3-j3+ln4-j4);
		else n=0;
		j1=ln1; j2=ln2; j3=ln3; j4=ln4;
		if (n > 85) cpu_col = BarsAlarm;
		else if (n > 60) cpu_col = BarsWarn;
		else cpu_col = BarsNorm;
		cpu = (n > 100 ? 100 : n) / 2;
	}
	if ( (in=fopen(aud_file,"r")) ) {       /* AUDIO VOLUME MONITOR */
		fscanf(in,"%d",&n);
		fclose(in);
		if (n == -1) vol_col = BarsAlarm;
		else if (n == 100) vol_col = BarsFull;
		else if (n < 15) vol_col = BarsWarn;
		else vol_col = BarsNorm;
		if (n > -1) vol = n / 2;
	}
	if ( (in=fopen(BATT_NOW,"r")) ) {       /* BATTERY MONITOR */
		fscanf(in,"%ld\n",&ln1); fclose(in);
		if ( (in=fopen(BATT_FULL,"r")) ) { fscanf(in,"%ld\n",&ln2); fclose(in); }
		if ( (in=fopen(BATT_STAT,"r")) ) { fscanf(in,"%c",&c); fclose(in); }
		n = (ln1 ? ln1 * 100 / ln2 : 0);
		if (c == 'C') bat_col = BarsCharge;
		else if (n < 10) bat_col = BarsAlarm;
		else if (n < 20) bat_col = BarsWarn;
		else if (n > 95) bat_col = BarsFull;
		else bat_col = BarsNorm;
		bat = n / 2;
	}
	draw();
}

int main(int argc, const char **argv) {
	dpy = XOpenDisplay(0x0);
	int scr = DefaultScreen(dpy);
	root = RootWindow(dpy,scr);
	win = XCreateSimpleWindow(dpy,root,74,0,50,13,0,0,0);
	bar = XCreatePixmap(dpy,root,50,13,DefaultDepth(dpy,scr));
	XStoreName(dpy,win,"88 Keys");
	unsigned int i;
	gc = (GC *) calloc(LASTColor, sizeof(GC));
	Colormap cmap = DefaultColormap(dpy,scr);
	XColor color;
	XGCValues val;
	for (i = 0; i < LASTColor; i++) {
		XAllocNamedColor(dpy,cmap,colors[i],&color,&color);
		val.foreground = color.pixel;
		gc[i] = XCreateGC(dpy,root,GCForeground,&val);
	}
	XSetWindowAttributes wa;
	wa.event_mask = ExposureMask|KeyPressMask|ButtonPressMask|StructureNotifyMask;
	wa.override_redirect = True;
	XChangeWindowAttributes(dpy,win,CWEventMask|CWOverrideRedirect,&wa);
	XMapWindow(dpy,win);

	/* prepare "previous" values for cpu jiffies, and create aud_file variable */
	in = fopen(CPU_FILE,"r");
	fscanf(in,"cpu %ld %ld %ld %ld",&j1,&j2,&j3,&j4);
	fclose(in);
	char *homedir = getenv("HOME");
	if (homedir != NULL) {
		aud_file = (char *) calloc(strlen(homedir)+15,sizeof(char));
		strcpy(aud_file,homedir);
		strcat(aud_file,"/.audio_volume");
	}
	else aud_file = NULL;
	updatestatus();

	/* get connection to read from */
	XEvent ev;
	int fd,r;
	struct timeval tv;
	fd_set rfds;
	fd = ConnectionNumber(dpy);
	running = True;
	while (running) { /* main loop */
		memset(&tv,0,sizeof(tv));
		tv.tv_usec = 250000; /* update status bar every 0.25 seconds */
		FD_ZERO(&rfds);
		FD_SET(fd,&rfds);
		r = select(fd+1,&rfds,0,0,&tv);
		if (r == 0) /* timed out, update status */
			updatestatus();
		else /* received event before timeout */
			while (XPending(dpy)) {
				XNextEvent(dpy,&ev);
				if (handler[ev.type]) handler[ev.type](&ev);
			}
	}

	/* clean up */
	XFlush(dpy);
	XCloseDisplay(dpy);
	return 0;
}
