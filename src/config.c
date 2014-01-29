
#include "alopex.h"

extern int free_mons();
extern int get_mons(const char *, const char *);
extern int icons_init(const char *);
extern int icons_free();

static int config_general(XrmDatabase, const char *);
static int config_binds(XrmDatabase, const char *);
static int config_macros(XrmDatabase, const char *);
static int config_rules(XrmDatabase, const char *);
static int config_theme(XrmDatabase, const char *);

FT_Face face, bface;
static pid_t pid;
static XrmDatabase xrdb;

int config_init(const char *theme) {
	const char *pwd = getenv("PWD");
	XrmInitialize();
	xrdb = XrmGetFileDatabase("/usr/share/alopex/configdb");
	if ( (!chdir(getenv("XDG_CONFIG_HOME")) && !chdir("alopex")) ||
			(!chdir(getenv("HOME")) && !chdir(".config/alopex")) )
		XrmCombineFileDatabase("configdb", &xrdb, True);
	if (!xrdb) die("cannot find a configuration resource database");
	char *_base = "icecap";
	const char *base = _base;
	if (theme) base = theme;
	config_binds(xrdb, base);
	config_general(xrdb, base);
	config_macros(xrdb, base);
	config_rules(xrdb, base);
	config_theme(xrdb, base);
	chdir(pwd);
	return 0;
}

int config_free() {
	XrmDestroyDatabase(xrdb);
	if (conf.statfd) {
		fclose(conf.stat);
		conf.statfd = 0;
		kill(pid, SIGKILL);
		wait(pid);
	}
	icons_free();
	free_mons();
	int i;
	if (conf.tag_name) {
		for (i = 0; conf.tag_name[i]; i++)
			free(conf.tag_name[i]);
		free(conf.tag_name);
	}
	if (conf.tag_icon) free(conf.tag_icon);
	cairo_font_face_destroy(conf.font);
	cairo_font_face_destroy(conf.bfont);
	FT_Done_Face(face);
	FT_Done_Face(bface);
	free(conf.key);
	conf.key = NULL;
	conf.nkeys = 0;
	return 0;
}

int reconfigure() {
	config_free();
	config_init(NULL);
}

/**** Local functions ****/

enum {
	STR_Font, STR_BoldFont, STR_IconFile, STR_Options, STR_Background,
	STR_Containers, STR_Names, STR_Icons, STR_FollowMouse, STR_Attach,
	STR_StatIn, STR_Mode,
	STR_Last
};
enum { TYPE_S, TYPE_D };
typedef struct Res {
	const char *elem, *opt;
	int type;
	void *dest;
} Res;

int config_general(XrmDatabase xrdb, const char *base) {
	const char *s[STR_Last];
	memset(s, 0, STR_Last);
	int bar_height = 0;
	const char *RES_Bar = "Bar";
	const char *RES_Background = "Background";
	const char *RES_Tags = "Tags";
	const char *RES_Tiling = "Tiling";
	const char *RES_Padding = "Padding";
	const char *RES_Clients = "Clients";
	Res res[] = {
		{ RES_Bar,    "StatFmt",      TYPE_S, &conf.stat_fmt},
		{ RES_Bar,    "StatInput",    TYPE_S, &s[STR_StatIn]},
		{ RES_Bar,    "Font",         TYPE_S, &s[STR_Font]},
		{ RES_Bar,    "BoldFont",     TYPE_S, &s[STR_BoldFont]},
		{ RES_Bar,    "FontSize",     TYPE_D, &conf.font_size},
		{ RES_Bar,    "Height",       TYPE_D, &bar_height},
		{ RES_Bar,    "Icons",        TYPE_S, &s[STR_IconFile]},
		{ RES_Bar,    "Options",      TYPE_S, &s[STR_Options]},
		{ RES_Tiling, RES_Background, TYPE_S, &s[STR_Background]},
		{ RES_Tiling, "Split",        TYPE_D, &conf.split},
		{ RES_Tiling, RES_Padding,    TYPE_D, &conf.gap},
		{ RES_Tiling, "Containers",   TYPE_S, &s[STR_Containers]},
		{ RES_Tiling, "Mode",         TYPE_S, &s[STR_Mode]},
		{ RES_Tags,   "Names",        TYPE_S, &s[STR_Names]},
		{ RES_Tags,   "Icons",        TYPE_S, &s[STR_Icons]},
		{ RES_Tags,   RES_Padding,    TYPE_D, &conf.bar_pad},
		{ "Chains",   "Delay",        TYPE_D, &conf.chain_delay},
		{ RES_Clients,"FollowMouse",  TYPE_S, &s[STR_FollowMouse]},
		{ RES_Clients,"Attach",       TYPE_S, &s[STR_Attach]},
	};
	int i;
	char class[256], *type;
	XrmValue val;
	for (i = 0; i < sizeof(res)/sizeof(res[0]); i++) {
		strcpy(class,base);
		strcat(class,"."); strcat(class,res[i].elem);
			strcat(class,"."); strcat(class,res[i].opt);
		if (!XrmGetResource(xrdb, class, class, &type, &val)) continue;
		if (res[i].type == TYPE_S)
			*(char **) res[i].dest = val.addr;
		else if (res[i].type == TYPE_D)
			 *((int *) res[i].dest) = atoi(val.addr);
	}
	/* get fonts */
	if (FT_New_Face(ftlib, s[STR_Font], 0, &face) |
			FT_Set_Pixel_Sizes(face, 0, conf.font_size))
		die("unable to load freetype font \"%s\"",s[STR_Font]);
	if (FT_New_Face(ftlib, s[STR_BoldFont], 0, &bface) |
			FT_Set_Pixel_Sizes(bface, 0, conf.font_size))
		die("unable to load freetype font \"%s\"",s[STR_BoldFont]);
	conf.font = cairo_ft_font_face_create_for_ft_face(face, 0);
	conf.bfont = cairo_ft_font_face_create_for_ft_face(bface, 0);
	/* set options */
	conf.bar_opts = (bar_height ? bar_height : conf.font_size + 4);
	if (strcasestr(s[STR_Mode],"rstack")) conf.mode = RSTACK;
	if (strcasestr(s[STR_Mode],"bstack")) conf.mode = BSTACK;
	if (strcasestr(s[STR_Mode],"monocle")) conf.mode = MONOCLE;
	if (strcasestr(s[STR_Options],"bottom")) conf.bar_opts |= BAR_BOTTOM;
	if (strcasestr(s[STR_Options],"hide")) conf.bar_opts |= BAR_HIDE;
	if (strcasestr(s[STR_FollowMouse],"true")) conf.focus_follow = True;
	if (strcasestr(s[STR_Attach],"top")) conf.attach = ATTACH_TOP;
	if (strcasestr(s[STR_Attach],"above")) conf.attach = ATTACH_ABOVE;
	if (strcasestr(s[STR_Attach],"below")) conf.attach = ATTACH_BELOW;
	if (strcasestr(s[STR_Attach],"bottom")) conf.attach = ATTACH_BOTTOM;
	/* get tag names and icon numbers */
	conf.tag_name = NULL;
	char *name = strtok((char *) s[STR_Names]," ");
	for (i = 0; name; i++) {
		conf.tag_name = realloc(conf.tag_name, (i+1) * sizeof(char *));
		conf.tag_name[i] = strdup(name);
		name = strtok(NULL," ");
	}
	conf.tag_name = realloc(conf.tag_name, (i+1) * sizeof(char *));
	conf.tag_name[i] = NULL;
	conf.tag_icon = NULL;
	name = strtok((char *) s[STR_Icons]," ");
	for (i = 0; name; i++) {
		conf.tag_icon = realloc(conf.tag_icon, (i+1) * sizeof(int));
		conf.tag_icon[i] = atoi(name);
		name = strtok(NULL," ");
	}
	conf.tag_icon = realloc(conf.tag_icon, (i+1) * sizeof(int));
	conf.tag_icon[i] = -1;
	/* status input */
	if (s[STR_StatIn]) {
		int fd[2];
		pipe(fd);
		if ( (pid=fork()) ) {
			conf.statfd = fd[0];
			conf.stat = fdopen(fd[0],"r");
//			close(fd[1]);
		}
		else {
			close(ConnectionNumber(dpy));
			close(fd[0]);
			dup2(fd[1], 1);
			close(fd[1]);
			execlp(s[STR_StatIn], s[STR_StatIn], NULL);
		}
	}
	else {
		conf.statfd = 0;
		conf.stat = NULL;
	}
	/* external inits */
	get_mons(s[STR_Background], s[STR_Containers]);
	icons_init(s[STR_IconFile]);
	return 0;
}

#define MAX_MOD	6
int config_binds(XrmDatabase xrdb, const char *base) {
	/* get modifiers */
	int mods[MAX_MOD];
	memset(mods,0,MAX_MOD);
	const char *ord[MAX_MOD] = {
		"None", "First", "Second", "Third", "Fourth", "Fifth"
	};
	char class[256], *type;
	XrmValue val;
	int i, j, tmod;
	for (i = 1; i < MAX_MOD; i++) {
		sprintf(class,"%s.Bind.Mod.%s",base,ord[i]);
		if (XrmGetResource(xrdb, class, class, &type, &val)) {
			mods[i] = 0;
			if (strcasestr(val.addr,"super") ||
					strcasestr(val.addr, "win") ||
					strcasestr(val.addr, "mod4"))
				mods[i] |= Mod4Mask;
			if (strcasestr(val.addr, "control") ||
					strcasestr(val.addr,"ctrl"))
				mods[i] |= ControlMask;
			if (strcasestr(val.addr, "shift"))
				mods[i] |= ShiftMask;
			if (strcasestr(val.addr, "alt"))
				mods[i] |= Mod1Mask;
		}
	}
	/* loop through bindings */
	conf.key = NULL;
	conf.nkeys = 0;
	int bmax = 1000;
	sprintf(class,"%s.Bind.Max",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		bmax = atoi(val.addr) + 1;
	KeySym sym;
	for (i = 0; i < bmax; i++) {
		sprintf(class,"%s.Bind.%03d.Key",base,i);
		if (!XrmGetResource(xrdb, class, class, &type, &val)) continue;
		if ( (sym=XStringToKeysym(val.addr)) == NoSymbol ) continue;
		for (j = 0; j < MAX_MOD; j++) {
			sprintf(class,"%s.Bind.%03d.%s",base,i,ord[j]);
			if (!XrmGetResource(xrdb, class, class, &type, &val)) continue;
			conf.key = realloc(conf.key, (conf.nkeys+1) * sizeof(Key));
			conf.key[conf.nkeys].keysym = sym;
			conf.key[conf.nkeys].mod = mods[j];
			conf.key[conf.nkeys].arg = val.addr;
			conf.nkeys++;
		}
	}
	return 0;
}

int config_macros(XrmDatabase xrdb, const char *base) {
	char class[256], *type;
	XrmValue val;
	char i;
	memset(conf.macro,0,26);
	for (i = 'a'; i < '{'; i++) {
		sprintf(class,"%s.Macro.%c",base,i);
		if (XrmGetResource(xrdb, class, class, &type, &val))
		conf.macro[i - 'a'] = val.addr;
	}
	return 0;
}

int config_rules(XrmDatabase xrdb, const char *base) {
	char class[256], *type, *rn, *rc;
	XrmValue val;
	conf.rule = NULL;
	conf.nrules = 0;
	int i, j, rmax = 1000, tags, flags, n[8];
	sprintf(class,"%s.Rule.Max",base);
	if (XrmGetResource(xrdb, class, class, &type, &val))
		rmax = atoi(val.addr) + 1;
	for (i = 0; i < rmax; i++) {
		rn = rc = NULL;
		sprintf(class,"%s.Rule.%03d.Name",base,i);
		if (XrmGetResource(xrdb, class, class, &type, &val)) rn = val.addr;
		sprintf(class,"%s.Rule.%03d.Class",base,i);
		if (XrmGetResource(xrdb, class, class, &type, &val)) rc = val.addr;
		if (!rn && !rc) continue;
		conf.rule = realloc(conf.rule, (conf.nrules+1) * sizeof(Rule));
		conf.rule[conf.nrules].name = rn;
		conf.rule[conf.nrules].class = rc;
		conf.rule[conf.nrules].tags = 0;
		sprintf(class,"%s.Rule.%03d.Tags",base,i);
		if (XrmGetResource(xrdb, class, class, &type, &val)) {
			memset(n,0,8 * sizeof(int));
			sscanf(val.addr,"%d %d %d %d %d %d %d %d",
					&n[0], &n[1], &n[2], &n[3], &n[4], &n[5], &n[6], &n[7]);
			for (j = 0; j < 8; j++)
				if (n[j]) conf.rule[conf.nrules].tags |= (1<<(n[j]-1));
		}
		conf.rule[conf.nrules].flags = 0;
		sprintf(class,"%s.Rule.%03d.Flags",base,i);
		if (XrmGetResource(xrdb, class, class, &type, &val)) {
			if (strcasestr(val.addr,"fullscreen"))
				conf.rule[conf.nrules].flags |= WIN_FULL;
			if (strcasestr(val.addr,"float"))
				conf.rule[conf.nrules].flags |= WIN_FLOAT;
		}
		conf.nrules++;
	}
	return 0;
}

int config_theme(XrmDatabase xrdb, const char *base) {
	const char *RES_Tab = "Tab";
	const char *RES_TabFocus = "TabFocus";
	const char *RES_TabTop = "TabTop";
	const char *RES_Status = "Status";
	const char *RES_Tag = "Tag";
	const char *RES_Offset = "Offset";
	const char *RES_Background = "Background";
	const char *RES_Border = "Border";
	const char *RES_Text = "Text";
	Res res[] = {
		{ RES_Tab,       RES_Offset,     0, NULL},
		{ RES_Tab,       RES_Background, 0, NULL},
		{ RES_Tab,       RES_Border,     0, NULL},
		{ RES_Tab,       RES_Text,       0, NULL},
		{ RES_TabFocus,  RES_Background, 0, NULL},
		{ RES_TabFocus,  RES_Border,     0, NULL},
		{ RES_TabFocus,  RES_Text,       0, NULL},
		{ RES_TabTop,    RES_Background, 0, NULL},
		{ RES_TabTop,    RES_Border,     0, NULL},
		{ RES_TabTop,    RES_Text,       0, NULL},
		{ RES_Status,    RES_Offset,     0, NULL},
		{ RES_Status,    RES_Background, 0, NULL},
		{ RES_Status,    RES_Border,     0, NULL},
		{ RES_Status,    RES_Text,       0, NULL},
		{ RES_Status,    "Input",        0, NULL},
		{ RES_Tag,       "Occupied",     0, NULL},
		{ RES_Tag,       "View",         0, NULL},
		{ RES_Tag,       "Alt",          0, NULL},
		{ RES_Tag,       "Both",         0, NULL},
	};
	char class[256], *type;
	XrmValue val;
	int i;
	Theme *q;
	for (i = 0; i < sizeof(res)/sizeof(res[0]); i++) {
		sprintf(class,"%s.%s.%s", base, res[i].elem, res[i].opt);
		if (!XrmGetResource(xrdb, class, class, &type, &val)) continue;
		q = &conf.theme[i];
		sscanf(val.addr,"%lf %lf %lf %lf %lf",
			&q->R, &q->G, &q->B, &q->A, &q->e);
	}
	return 0;
}
