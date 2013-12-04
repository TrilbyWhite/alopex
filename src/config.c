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
static pid_t stat_open(const char *);

static pid_t pid;

/********************************************************************/
/*  GLOBAL FUNCTIONS                                                */
/********************************************************************/

int config() {
	const char *pwd = getenv("PWD");
	FILE *rc = NULL;
	if ( (!chdir(getenv("XDG_CONFIG_HOME")) && !chdir("alopex")) ||
			(!chdir(getenv("HOME")) && !chdir(".config/alopex")) )
		rc = fopen("config","r");
	if (!rc)
		rc = fopen("/usr/share/alopex/config","r");
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
	return 0;
}

int deconfig() {
	int i;
	if (statfd) {
		fclose(inpipe); statfd = 0;
		kill(pid,SIGKILL);
		wait(pid);
		instring[0] = '\0';
	}
	if (tag_names) {
		for (i = 0; i < ntags; i++) free(tag_names[i]);
		free(tag_names); tag_names = NULL;
	}
	if (key) {
		free(key); key = NULL;
	}
	if (containers) {
		free(containers); containers = NULL;
	}
	if (string) {
		for (i = 0; i < 128; i++)
			if (string[i]) free(string[i]);
		free(string); string = NULL;
	}
	if (theme) {
		free(theme); theme = NULL;
	}
}

int reconfig() {
// TODO: detect bar height change and get_monitors() if needed
	deconfig();
	config();
}

/********************************************************************/
/*  LOCAL FUNCTIONS                                                 */
/********************************************************************/

static void safe_dup(const char *src,char **dest) {
	if (*dest) free(*dest);
	*dest = strdup(src);
}

void conf_bars(FILE *rc) {
	char line[LINE_LEN], str[LINE_LEN];
	char **toks = NULL, *tok;
	int n, ntok = 0;
	bar_opts = 0;
	while (fgets(line,LINE_LEN,rc)) {
		if (line[0] != '\t') break;
		else if (sscanf(line," status format = \"%[^\"]\"\n",str) == 1)
			safe_dup(str,&status_fmt);
		else if (sscanf(line," status input = \"%[^\"]\"\n",str) == 1)
			pid = stat_open(str);
		else if (sscanf(line," font path = %[^\n]\n",str) == 1)
			safe_dup(str,&font_path);
		else if (sscanf(line," icons path = %[^\n]\n",str) == 1)
			safe_dup(str,&icons_path);
		else if (sscanf(line," options = %[^\n]\n",str) == 1) {
			if (strstr(str,"visible")) bar_opts |= BAR_VISIBLE;
			if (strstr(str,"top")) bar_opts |= BAR_TOP;
			if ( (tok=strstr(str,"height=")) ) SET_BAR_HEIGHT(atoi(tok+7));
		}
		else if (sscanf(line," font size = %d",&n) == 1)
			font_size = n;
		else if (sscanf(line," tag padding = %d",&n) == 1)
			tag_pad = n;
		else if (sscanf(line," tag names = %[^\n]\n",str) == 1 ) {
			tok = strtok(str," ");
			while (tok) {
				toks = realloc(toks,(++ntok) * sizeof(char *));
				toks[ntok-1] = strdup(tok);
				tok = strtok(NULL," ");
			}
		}
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
			strcat(str," &"); safe_dup(str,&string[c]);
		}
		else if ( sscanf(line," %c %[^\n]\n",&c,str) == 2)
			safe_dup(str,&string[c]);
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

pid_t stat_open(const char *cmd) {
	int fd[2];
	pipe(fd);
	pid_t in;
	if ( (in=fork()) ) {
		statfd = fd[0];
		inpipe = fdopen(fd[0],"r");
		close(fd[1]);
		return in;
	}
	else {
		close(fd[0]); dup2(fd[1],1); close(fd[1]);
		const char *arg[2]; arg[0] = cmd; arg[1] = NULL;
		execv(arg[0],(char * const *)arg);
	}
}

