
static const char font[] =
	"-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*";
static const char *tag_name[] =
	{"one", "two", "three", "four", "five", NULL};
static const char *tile_modes[] =
	{"R_ttwm", "B_ttwm", "rstack", "bstack","monocle", NULL};

static const char colors[LASTColor][9] = {
	[Background]	= "#101010",
	[Default]		= "#686868",
	[Occupied]		= "#68B0E0",
	[Selected]		= "#FFAA48",
	[Urgent]		= "#FF8880",
	[Title]			= "#DDDDDD",
};

static const char 	ttwm_cursor			= XC_left_ptr;
static const int	borderwidth			= 1;
/* if tilegap is set to zero, this will activate the seemless tab mode
   like in ttwm-1.0 (when this is implemented)
   tilegap > 0 will make the status bar have tabs that should look better
   with a gap between the bar and the windows */
static const int	tilegap				= 4;
/* classictabs implement ttwm 1.0 style tabs in the title bar.
   These look best with a borderless and gapless configuration. */
static const Bool	classictabs			= False;
/* set to the maximum input you might feed to the stdin reader */
static const int	max_status_line		= 256;
/* smallest allowable width or height for a window
   This effects increasing and decreasing the tilebias, and can protect
   windows from getting completely "crushed" when too many are opened
   in rstack/bstack modes */
static const int	win_min				= 20;
static const Bool	focusfollowmouse	= False;
static Bool			showbar				= True;
static Bool			topbar				= True;
/* tilebias is set as a number of pixels to increase the master (and thus
   decrease the stack) from half the screen width.  Zero means the two will
   share the screen equally. */
static int			tilebias			= 0;

static const char	*video1				= "LVDS1";
static const char	*video2				= "VGA1";
static const char	*video_location		= "below";

#define DMENU		"dmenu_run -fn \"-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*\" -nb \"#101010\" -nf \"#484862\" -sb \"#080808\" -sf \"#FFDD0E\""
#define TERM		"urxvt" 		/* or "urxvtc","xterm","terminator",etc */
#define CMD(app)	app "&"

/* key definitions */
#define MOD1 Mod4Mask
#define MOD2 Mod1Mask
static Key keys[] = {
	/* modifier			key			function	argument */
	/* launchers + misc: */
	{ MOD1,				XK_Return,	spawn,		CMD(TERM)		},
	{ MOD1,				XK_p,		spawn,		CMD(DMENU)		},
	{ MOD1,				XK_w,		spawn,		CMD("luakit")	},
	{ MOD1|ShiftMask,	XK_q,		quit,		NULL			},
	{ MOD2,				XK_F4,		killclient,	NULL			},
	{ MOD1,				XK_f,		fullscreen,	NULL			},
	{ MOD1,				XK_x,		toggle,		"place bar"		},
	{ MOD1,				XK_a,		toggle,		"visible bar"	},
	{ MOD1|MOD2,		XK_f,		toggle,		"floating"		},
	/* tiling: */
	{ MOD1,				XK_space,	tile,		"cycle"			},
	{ MOD1|MOD2,		XK_t,		tile,		"R_ttwm"		},
	{ MOD1|MOD2,		XK_b,		tile,		"bstack"		},
	{ MOD1|MOD2,		XK_r,		tile,		"rstack"		},
	{ MOD1|MOD2,		XK_m,		tile,		"monocle"		},
	{ MOD1,				XK_i,		tile,		"increase"		},
	{ MOD1,				XK_d,		tile,		"decrease"		},
	/* external monitor command */
	{ MOD1|ShiftMask,	XK_a,		vga,		"activate"		},
	{ MOD1|ShiftMask,	XK_d,		vga,		"deactivate"	},
	{ MOD1|ShiftMask,	XK_s,		vga,		"send"			},
	{ MOD1|ShiftMask,	XK_r,		vga,		"return"		},
	/* tagging:
		s=set current active tag
		t=toggle toggle tag visibility
		a=assign window - toggle focused window's tag membership
		m=move win - assign focused window to a single tag		*/
	{ MOD1,				XK_1,		tag,		"s 1"			},
	{ MOD1,				XK_2,		tag,		"s 2"			},
	{ MOD1,				XK_3,		tag,		"s 3"			},
	{ MOD1,				XK_4,		tag,		"s 4"			},
	{ MOD1,				XK_5,		tag,		"s 5"			},
	{ MOD1|MOD2,		XK_1,		tag,		"t 1"			},
	{ MOD1|MOD2,		XK_2,		tag,		"t 2"			},
	{ MOD1|MOD2,		XK_3,		tag,		"t 3"			},
	{ MOD1|MOD2,		XK_4,		tag,		"t 4"			},
	{ MOD1|MOD2,		XK_5,		tag,		"t 5"			},
	{ MOD1|ShiftMask,	XK_1,		tag,		"a 1"			},
	{ MOD1|ShiftMask,	XK_2,		tag,		"a 2"			},
	{ MOD1|ShiftMask,	XK_3,		tag,		"a 3"			},
	{ MOD1|ShiftMask,	XK_4,		tag,		"a 4"			},
	{ MOD1|ShiftMask,	XK_5,		tag,		"a 5"			},
	{ MOD1|ControlMask,	XK_1,		tag,		"m 1"			},
	{ MOD1|ControlMask,	XK_2,		tag,		"m 2"			},
	{ MOD1|ControlMask,	XK_3,		tag,		"m 3"			},
	{ MOD1|ControlMask,	XK_4,		tag,		"m 4"			},
	{ MOD1|ControlMask,	XK_5,		tag,		"m 5"			},
	/* window focus/movement:
		f=focus previous, next, or alternate  window in tag(s)
		s=swap window with previous, next, or alternate  window	*/
	{ MOD1,				XK_k,		window,		"f prev"		},
	{ MOD1,				XK_j,		window,		"f next"		},
	{ MOD1,				XK_Left,	window,		"f prev"		},
	{ MOD1,				XK_Right,	window,		"f next"		},
	{ MOD1,				XK_h,		window,		"s prev"		},
	{ MOD1,				XK_l,		window,		"s next"		},
	{ MOD1,				XK_Up,		window,		"s prev"		},
	{ MOD1,				XK_Down,	window,		"s next"		},
	{ MOD1,				XK_Tab,		window,		"f alt"			},
	{ MOD1|ShiftMask,	XK_Tab,		window,		"s alt"			},
};

static Button buttons[] = {
	/* modifier			button		function 	arg */
	{MOD1,				1,			mouse,		"move"		},
	{MOD1,				2,			tile,		"cycle"		},
	{MOD1,				3,			mouse,		"resize"	},
	{MOD1,				4,			window,		"s prev"	},
	{MOD1,				5,			window,		"s next"	},
	{MOD1,				6,			window,		"f prev"	},
	{MOD1,				7,			window,		"f next"	},
};


// vim: ts=4
