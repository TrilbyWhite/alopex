/* copyright J.McClure 2012-2013, see alopex.c or COPYING for details */


static const char font[] =
	"-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*";
static const char *tile_modes[] =
	{"rstack", "bstack","monocle", NULL};

/* each tagcon entry defines and icon and/or a name for each tag.
   You may use any combination of entries for any tags including
   using names (pre/post) for some, icons for others, and both for
   yet others.  Technically you can have usable tags without any
   indicator text or icon, which may not have practical purpose
   other than "hiding" client windows.  For no icon use NO_ICON
   or a -1, for no text use NULL. */
static const Tagcon tagcons[] = {
	/* pre		icon		post */
	{ NULL,		20,			"one"},
	{ NULL,		21,			"two" },
	{ NULL,		22,			"three" },
	{ "4:",		NO_ICON,	"four" },
	{ "5:",		NO_ICON,	"five" },
	{ "6:",		NO_ICON,	"six" },
};

static const char	alopex_cursor		= XC_left_ptr;
static const int	borderwidth			= 0;
/* set gap between the windows */
static const int	tilegap				= 0;
/* set a minimum space on the left of the bar that the master tab will
   stay to the right of.  Setting to zero will keep the master tab flush
   against the last displayed tag name */
static const int	tagspace			= 0;
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
/* attachmode determines where new windows will be placed.
   0 = master, 1 = aside (top of stack), 2 = bottom */
static const int	attachmode			= 0;
/* stackcount sets how many clients can me visible in the stack region.
   For "ttwm" tiling mode, set stackcount to 1.  The value can be changed
   via a key binding (default Mod+ or Mod-). */
static int			stackcount			= 3;

#define DMENU		"dmenu_run -fn \"-misc-fixed-medium-r-normal--13-120-75-75-c-70-*-*\" -nb \"#101010\" -nf \"#484862\" -sb \"#080808\" -sf \"#FFDD0E\""

#ifdef THEME_NAME
#define TERM		"urxvt -name " THEME_NAME
#else
#define TERM		"urxvt" 		/* or "urxvtc","xterm","terminator",etc */
#endif

#define CMD(app)	app "&"

/* these are just examples - be sure to edit for your setup.
   Uncomment and define WALLPAPER suitably to have the wallpaper */

#define XRANDR_CMD		"xrandr --output LVDS1 --auto --output VGA1 --auto --left-of LVDS1"
//#define WALLPAPER		"feh --bg-scale ~/images/bg.jpg"

/* key definitions */
#define MOD1 Mod4Mask
#define MOD2 Mod1Mask
#define MOD3 ControlMask
#define MOD4 ShiftMask
#define TagKey(KEY,TAG) \
	{ MOD1,				KEY,		tag,	"s " TAG }, \
	{ MOD1|MOD2,		KEY,		tag,	"t " TAG }, \
	{ MOD1|MOD3,		KEY,		tag,	"m " TAG }, \
	{ MOD1|MOD4,		KEY,		tag,	"a " TAG },

static Key keys[] = {
	/* modifier			key			function	argument */
	/* launchers + misc: */
	{ MOD1,				XK_Return,	spawn,		CMD(TERM)		},
	{ MOD1,				XK_p,		spawn,		CMD(DMENU)		},
	{ MOD1,				XK_w,		spawn,		CMD("luakit")	},
	{ MOD1|MOD4,		XK_q,		quit,		NULL			},
	{ MOD2,				XK_F4,		killclient,	NULL			},
	{ MOD1,				XK_f,		fullscreen,	NULL			},
	{ MOD1|MOD2,		XK_f,		toggle,		"floating"		},
	{ MOD1,				XK_x,		toggle,		"place bar"		},
	{ MOD1,				XK_a,		toggle,		"visible bar"	},
	{ 0,				0x1008ff59,	spawn,		XRANDR_CMD		},
	/* tiling:
		tile modes, increase/decrease master region size,
		increment or decrement (+/-) the number of stack clients,
		select for all or one ("ttwm tiling") stack clients */
	{ MOD1,				XK_space,	tile,		"cycle"			},
	{ MOD1,				XK_b,		tile,		"bstack"		},
	{ MOD1,				XK_r,		tile,		"rstack"		},
	{ MOD1,				XK_m,		tile,		"monocle"		},
	{ MOD1,				XK_i,		tile_conf,	"increase"		},
	{ MOD1,				XK_d,		tile_conf,	"decrease"		},
	{ MOD1,				XK_equal,	tile_conf,	"+"				},
	{ MOD1,				XK_minus,	tile_conf,	"-"				},
	{ MOD1,				XK_period,	tile_conf,	"all"			},
	{ MOD1,				XK_comma,	tile_conf,	"one"			},
	/* tagging:
		flip between alternate views, set tags */
	{ MOD2,				XK_Tab,		tag,		"flip"			},
		TagKey(			XK_1,					"1"		)
		TagKey(			XK_2,					"2"		)
		TagKey(			XK_3,					"3"		)
		TagKey(			XK_4,					"4"		)
		TagKey(			XK_5,					"5"		)
		TagKey(			XK_6,					"6"		)
	/* window focus/movement:
		f=focus previous, next, or alternate  window in tag(s)
		s=swap window with previous, next, or alternate  window
		capital "Next" or "Prev" include floating windows
		+/- adjust a windows monitor number */
	{ MOD1,				XK_k,		window,		"f prev"		},
	{ MOD1,				XK_j,		window,		"f next"		},
	{ MOD1|MOD4,		XK_k,		window,		"f Prev"		},
	{ MOD1|MOD4,		XK_j,		window,		"f Next"		},
	{ MOD1,				XK_Left,	window,		"f prev"		},
	{ MOD1,				XK_Right,	window,		"f next"		},
	{ MOD1,				XK_h,		window,		"s prev"		},
	{ MOD1,				XK_l,		window,		"s next"		},
	{ MOD1,				XK_Up,		window,		"s prev"		},
	{ MOD1,				XK_Down,	window,		"s next"		},
	{ MOD1,				XK_Tab,		window,		"f alt"			},
	{ MOD1|MOD4,		XK_Tab,		window,		"s alt"			},
	{ MOD1|MOD4,		XK_equal,	window,		"+"				},
	{ MOD1|MOD4,		XK_minus,	window,		"-"				},
	{ MOD1|MOD4,		XK_space,	toggle,		"monitor focus"	},
};

static Button buttons[] = {
	/* modifier			button		function 	arg */
	{ MOD1,				1,			mouse,		"move"		},
	{ MOD1,				2,			toggle,		"floating"	},
	{ MOD1,				3,			mouse,		"resize"	},
	{ MOD1,				4,			window,		"s prev"	},
	{ MOD1,				5,			window,		"s next"	},
	{ MOD1,				6,			window,		"f prev"	},
	{ MOD1,				7,			window,		"f next"	},
};

static Rule rules[] = {
	/* name				class		tags		flags */
	/* tags/flags=0 means no change to default */
	{ NULL,				"float",	0,			FLAG_FLOATING },
	{ "float",			NULL,		0,			FLAG_FLOATING },
};

// vim: ts=4
