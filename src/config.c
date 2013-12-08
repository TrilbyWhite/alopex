/********************************************************************\
* CONFIG.C - part of alopex, see alopex.c for license information
*
* Author: Jesse McClure, copyright 2012-2013
\********************************************************************/

/********************************************************************/
/*  LOCAL DATA                                                      */
/********************************************************************/

#include "alopex.h"
#define LINE_LEN	128

static void conf_bars(FILE *);
static void conf_bindings(FILE *);
static void conf_clients(FILE *);
static void conf_containers(FILE *);
static void conf_strings(FILE *);
static void conf_theme(FILE *);
static void free_mons();
static void get_mons();
static pid_t stat_open(const char *);

static pid_t pid;

#define SAFE_DUP(a,b)	{ if (b) free(b); b = strdup(a); }
#define SAFE_FREE(x)	{ if (x) free(x); x = NULL; }

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int config() {
	const char *pwd = getenv("PWD");
	FILE *rc = NULL;
	if ( (!chdir(getenv("XDG_CONFIG_HOME")) && !chdir("alopex")) ||
			(!chdir(getenv("HOME")) && !chdir(".config/alopex")) )
		rc = fopen("config","r");
	if (!rc) {
		char path[LINE_LEN];
		if (theme_name && theme_name[0] != '/')
			snprintf(path,LINE_LEN,"/usr/share/alopex/config.%s",theme_name);
		else
			strncpy(path,theme_name,LINE_LEN);
		rc = fopen(path,"r");
	}
	if (!rc) die("cannot open configuration file");
	char line[LINE_LEN];
	while (fgets(line,LINE_LEN,rc)) {
		if (line[0] == '#' || line[0] == '\n') continue;
		else if (!strncmp(line,"bars:",5)) conf_bars(rc);
		else if (!strncmp(line,"containers:",11)) conf_containers(rc);
		else if (!strncmp(line,"clients:",8)) conf_clients(rc);
		else if (!strncmp(line,"theme:",6)) conf_theme(rc);
		else if (!strncmp(line,"strings:",8)) conf_strings(rc);
		else if (!strncmp(line,"bindings:",8)) conf_bindings(rc);
		else fprintf(stderr,"ALOPEX: unrecognized configuration entry: "
				"%s\n",line);
	}
	fclose(rc);
	if (pwd) chdir(pwd);
	if (FT_New_Face(library,font_path,0,&face) | FT_Set_Pixel_Sizes(face,0,font_size))
		die("unable to load free type font");
	if (font_path2 && (FT_New_Face(library,font_path2,0,&face2) |
			FT_Set_Pixel_Sizes(face2,0,font_size)))
		die("unable to load free type font");
	cfont = cairo_ft_font_face_create_for_ft_face(face,0);
	if (font_path2) cfont2 = cairo_ft_font_face_create_for_ft_face(face2,0);
	get_mons();
	return 0;
}

int deconfig() {
	free_mons();
	SAFE_FREE(bg_path)
	SAFE_FREE(containers)
	SAFE_FREE(icons_path)
	SAFE_FREE(key)
	SAFE_FREE(status_fmt)
	SAFE_FREE(theme)
	int i;
	memset(tag_icon,0,16 * sizeof(unsigned short int));
	if (statfd) {
		statfd = 0;
		kill(pid,SIGKILL);
		wait(pid);
		instring[0] = '\0';
	}
	if (tag_names) {
		for (i = 0; i < ntags; i++) free(tag_names[i]);
		free(tag_names); tag_names = NULL;
	}
	if (string) {
		for (i = 0; i < 128; i++)
			if (string[i]) free(string[i]);
		free(string); string = NULL;
	}
	if (font_path) {
		cairo_font_face_destroy(cfont);
		FT_Done_Face(face);
		free(font_path); font_path = NULL;
	}
	if (font_path2) {
		cairo_font_face_destroy(cfont2);
		FT_Done_Face(face2);
		free(font_path2); font_path2 = NULL;
	}
}

int reconfig() {
//TODO reset C->top for all C
	int i, j; Monitor *M; Container *C, *CC = NULL;
	int *foc = NULL, *t = NULL, *o = NULL, *md = NULL;
	for (M = mons, i = 0; M; M = M->next, i++) {
		foc = realloc(foc,(i+1) * sizeof(int));
		t = realloc(t,(i+1) * sizeof(int));
		o = realloc(o,(i+1) * sizeof(int));
		md = realloc(md,(i+1) * sizeof(int));
		for (C = M->container, j = 0; C; C = C->next, j++)
			if (M->focus == C) foc[i] = j;
		t[i] = M->tags;
		o[i] = M->occ;
		md[i] = M->mode;
	}
	deconfig();
	config();
	for (M = mons, i = 0; M; M = M->next, i++) {
		for (C = M->container, j = foc[i]; j && C; C = C->next, j--);
		if (C) M->focus = C;
		M->tags = t[i];
		M->occ = o[i];
		M->mode = md[i];
	}
	free(foc); free(t); free(o); free(md);
	return 0;
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

void conf_bars(FILE *rc) {
	char line[LINE_LEN], str[LINE_LEN];
	char **toks = NULL, *tok;
	int n, ntok = 0;
	bar_opts = 0;
	while (fgets(line,LINE_LEN,rc)) {
		if (line[0] != '\t') break;
		else if (sscanf(line," status format = \"%[^\"]\"\n",str) == 1)
			SAFE_DUP(str,status_fmt)
		else if (sscanf(line," status input = %[^\n]\n",str) == 1)
			pid = stat_open(str);
		else if (sscanf(line," font path = %[^\n]\n",str) == 1)
			SAFE_DUP(str,font_path)
		else if (sscanf(line," background = %[^\n]\n",str) == 1)
			SAFE_DUP(str,bg_path)
		else if (sscanf(line," alt font path = %[^\n]\n",str) == 1)
			SAFE_DUP(str,font_path2)
		else if (sscanf(line," icons path = %[^\n]\n",str) == 1)
			SAFE_DUP(str,icons_path)
		else if (sscanf(line," options = %[^\n]\n",str) == 1) {
			if (strstr(str,"visible")) bar_opts |= BAR_VISIBLE;
			if (strstr(str,"top")) bar_opts |= BAR_TOP;
			if ( (tok=strstr(str,"height=")) ) bar_opts |= atoi(tok+7);
		}
		else if (sscanf(line," font size = %d",&n) == 1)
			font_size = n;
		else if (sscanf(line," tag padding = %d",&n) == 1)
			tag_pad = n;
		else if (sscanf(line," chain delay = %d",&n) == 1)
			chain_delay = n;
		else if (sscanf(line," tag names = %[^\n]\n",str) == 1 ) {
			tok = strtok(str," ");
			while (tok) {
				toks = realloc(toks,(++ntok) * sizeof(char *));
				toks[ntok-1] = strdup(tok);
				tok = strtok(NULL," ");
			}
		}
		else if (sscanf(line," tag icons = "
		"%hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu",
		&tag_icon[0], &tag_icon[1], &tag_icon[2], &tag_icon[3],
		&tag_icon[4], &tag_icon[5], &tag_icon[6], &tag_icon[7],
		&tag_icon[8], &tag_icon[9], &tag_icon[10], &tag_icon[11],
		&tag_icon[12], &tag_icon[13], &tag_icon[14], &tag_icon[15]));
		else fprintf(stderr,"ALOPEX: unrecognized configuration entry: "
				"%s\n",line);
	}
	tag_names = toks;
	ntags = ntok;
}

void conf_bindings(FILE *rc) {
	char line[LINE_LEN], mods[24], ch[12], str[LINE_LEN];
	KeySym sym;
	int n = 0;
	while (fgets(line,LINE_LEN,rc)) {
		if (line[0] != '\t') break;
		mods[0] = '\0';
		if (sscanf(line," %s %s = %s",mods,ch,str) == 3) {
			if ( (sym=XStringToKeysym(ch)) == NoSymbol )
				fprintf(stderr,"ALOPEX: config binding error: %s\n", line);
			else {
				key = realloc(key,(n+1) * sizeof(Key));
				key[n].keysym = sym;
				key[n].arg = strdup(str);
				key[n].mod = 0;
				if (strcasestr(mods,"super")) key[n].mod |= Mod4Mask;
				if (strcasestr(mods,"alt")) key[n].mod |= Mod1Mask;
				if (strcasestr(mods,"shift")) key[n].mod |= ShiftMask;
				if (strcasestr(mods,"control")) key[n].mod |= ControlMask;
				if (strcasestr(mods,"ctrl")) key[n].mod |= ControlMask;
				n++;
			}
		}
		else fprintf(stderr,"ALOPEX: unrecognized configuration entry: "
				"%s\n",line);
	}
	nkeys = n;
}

void conf_clients(FILE *rc) {
	char line[LINE_LEN], str[LINE_LEN];
	focusfollowmouse = False;
	client_opts = 0;
	while (fgets(line,LINE_LEN,rc)) {
		if (line[0] != '\t') break;
		if (sscanf(line," mouse focus = %s",str)) {
			if (!strncasecmp(str,"true",4)) focusfollowmouse = True;
		}
		if (sscanf(line," attach = %s",str)) {
			if (!strncasecmp(str,"top",3)) client_opts |= ATTACH_TOP;
			if (!strncasecmp(str,"bot",3)) client_opts |= ATTACH_BOTTOM;
		}
	}
};

void conf_containers(FILE *rc) {
	int i, n, c[10]; char line[LINE_LEN];
	while (fgets(line,LINE_LEN,rc)) {
		if (line[0] != '\t') break;
		if ( (n=sscanf(line," counts = %d %d %d %d %d %d %d %d %d %d",
				&c[0], &c[1], &c[2], &c[3], &c[4], &c[5],
				&c[6], &c[7], &c[8], &c[9])) ) {
			n++;
			containers = calloc(n,sizeof(int));
			for (i = 0; i < n-1; i++) containers[i] = c[i];
			containers[i] = -1;
			ncontainers = i+1;
		}
		else if (sscanf(line," padding = %d", &n))
			container_pad = n; 
		else if (sscanf(line," split = %d", &n))
			container_split = n; 
		else fprintf(stderr,"ALOPEX: unrecognized configuration entry: "
				"%s\n",line);

	}
};

void conf_strings(FILE *rc) {
	int i; char line[LINE_LEN], str[LINE_LEN], c;
	string = calloc(128, sizeof(char *));
	while (fgets(line,LINE_LEN,rc)) {
		if (line[0] != '\t') break;
		if ( sscanf(line," %c! %[^\n]\n",&c,str) == 2) {
			strcat(str," &"); SAFE_DUP(str,string[c])
		}
		else if ( sscanf(line," %c %[^\n]\n",&c,str) == 2)
			SAFE_DUP(str,string[c])
		else fprintf(stderr,"ALOPEX: unrecognized configuration entry: "
					"%s\n",line);
	}
}

void conf_theme(FILE *rc) {
	int i = 0; Theme *q; char line[LINE_LEN], str[LINE_LEN];
	theme = calloc(themeEND, sizeof(Theme));
	double a,b,c,d,e;
	while (fgets(line,LINE_LEN,rc)) {
		if (line[0] != '\t') break;
		q = &theme[i];
		if ( sscanf(line," %*[^=]= %lf %lf %lf %lf %lf\n",
				&q->a,&q->b,&q->c,&q->d,&q->e) == 5 ) i++;
		else fprintf(stderr,"ALOPEX: unrecognized configuration entry: "
					"%s\n",line);
		if (i == themeEND) break;
	}
}

void free_mons() {
	int i; Monitor *M; Container *C, *CC = NULL;
	for (M = mons; M; M = M->next) {
		for (i = 0; i < 2; i++) {
			cairo_surface_destroy(M->sbar[i].buf);
			cairo_destroy(M->sbar[i].ctx);
		}
		for (C = M->container; C; CC = C, C = C->next) {
			if (CC) free(CC);
			cairo_destroy(C->bar.ctx);
			XFreePixmap(dpy,C->bar.buf);
			XDestroyWindow(dpy,C->bar.win);
		}
		free(C);
		bg_free(M);
	}
	free(mons);
}

void get_mons() {
	int i, bh = (BAR_HEIGHT(bar_opts) ? BAR_HEIGHT(bar_opts) : font_size + 4);
	Monitor *M; Container *C, *CC = NULL;
	XSetWindowAttributes wa;
	wa.override_redirect = True;
	wa.event_mask = ExposureMask;
	cairo_surface_t *t;
// TODO get actual monitor data
mons = calloc(1,sizeof(Monitor));
	for (M = mons; M; M = M->next) {
// TODO get actual monitor data
M->x = 0; M->y = 0;
M->w = DisplayWidth(dpy,scr);
M->h = DisplayHeight(dpy,scr);
bg_init(M);
		M->split = container_split;
		M->gap = container_pad;
		for (i = 0; i < ncontainers; i++) {
		//for (C = M->container; C; C = C->next) {
			C = calloc(1,sizeof(Container));
			C->n = containers[i];
			C->bar.opts = bar_opts | bh;
			C->bar.win = XCreateSimpleWindow(dpy, root, M->x,
					(bar_opts & BAR_TOP ? M->y:  M->y + M->h - bh),
					M->w, bh, 0, 0, 0);
			XChangeWindowAttributes(dpy,C->bar.win,CWOverrideRedirect |
					CWEventMask, &wa);
			C->bar.buf = XCreatePixmap(dpy,root,M->w,bh,DefaultDepth(dpy,scr));
			t = cairo_xlib_surface_create(dpy,C->bar.buf,DefaultVisual(dpy,scr),
					M->w,bh);
			C->bar.ctx = cairo_create(t);
			cairo_surface_destroy(t);
			cairo_set_font_face(C->bar.ctx,cfont);
			cairo_set_font_size(C->bar.ctx,font_size);
			cairo_set_line_width(C->bar.ctx,theme[tabRGBAFocusBrd].e);
			cairo_set_line_join(C->bar.ctx,CAIRO_LINE_JOIN_ROUND);
			XMapWindow(dpy,C->bar.win);
			if (!M->container) M->container = C;
			if (CC) CC->next = C;
			CC = C;
		}
		for (i = 0; i < 2; i++) {
			M->sbar[i].width = M->w/2; M->sbar[i].height = bh;
			M->sbar[i].buf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,M->w/2,bh);
			M->sbar[i].ctx = cairo_create(M->sbar[i].buf);
			cairo_set_font_face(M->sbar[i].ctx,cfont);
			cairo_set_font_size(M->sbar[i].ctx,font_size);
			cairo_set_line_width(M->sbar[i].ctx,theme[statRGBABrd].e);
			cairo_set_line_join(M->sbar[i].ctx,CAIRO_LINE_JOIN_ROUND);
		}
		M->mode = RSTACK; //TODO
		M->focus = M->container;
	}
	m = mons;
}

pid_t stat_open(const char *cmd) {
	int fd[2];
	pipe(fd);
	pid_t in;
	if ( (in=fork()) ) {
		statfd = fd[0];
		//inpipe = fdopen(fd[0],"r");
		close(fd[1]);
		return in;
	}
	else {
		close(fd[0]); dup2(fd[1],1); close(fd[1]);
		const char *arg[2]; arg[0] = cmd; arg[1] = NULL;
		execv(arg[0],(char * const *)arg);
	}
}

