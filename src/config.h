

/* STATUS FORMAT STRING
	%T	tags (icon + name)
	%t tags (name + icon)
	%n tags (name)
	%i tags (icon)
	%C clock (24 hour)
	%c clock (12 hour)
STATUS INPUT NOT IMPLEMENTED JUST YET
	%1 first line of status input
	%2 second (or previous) line of status input
	%3 ...
	...
*/

const char *status_fmt = "%T\n %C \n";
//const char *status_fmt = "%T\n %1 %C \n";
const char *font_path = "/usr/share/fonts/TTF/DroidSansMono.ttf";
const char *icons_path = "/usr/share/alopex/icons.png";
//const char *font_path = "/usr/share/fonts/TTF/comic.ttf";
const int font_size = 14;

const Bool focusfollowmouse = False;
const int containers[]	= { 1, 3, 3, -1 };
//const int container_pad	= 4;
const int container_pad	= 4;
const int client_opts	= ATTACH_TOP;
//const int bar_opts		= BAR_VISIBLE | BAR_TOP | SET_BAR_HEIGHT(18);
const int bar_opts		= BAR_VISIBLE | BAR_TOP;
const char *tag_names[] = { "one", "two", "three", "four", NULL };
const int tag_pad			= 5;

const Theme theme[] = {
								/*  X     Y     W     H     Radius [1] */
	[tabOffset]				= { 2.00, 2.00, -4.0, 10.0, 5.0 },
	[statOffset]			= { 2.00, 2.00, -4.0, -8.0, 5.0 },
								/*  R     G     B     A     Ignore [2] */
	[tabRGBAFocus]			= { 0.14, 0.14, 0.14, 1.00, 2.0 },
	[tabRGBATop]			= { 0.14, 0.14, 0.14, 0.85, 0.0 },
	[tabRGBAOther]			= { 0.04, 0.04, 0.04, 0.50, 0.0 },
	[tabRGBAFocusBrd]		= { 0.60, 0.90, 1.00, 0.95, 0.0 },
	[tabRGBATopBrd]		= { 0.00, 0.40, 1.00, 0.80, 0.0 },
	[tabRGBAOtherBrd]		= { 0.00, 0.20, 1.00, 0.45, 0.0 },
	[statRGBA]				= { 0.14, 0.14, 0.14, 0.40, 1.0 },
	[statRGBABrd]			= { 1.00, 1.00, 1.00, 0.60, 0.0 },
	[statRGBAText]			= { 0.80, 0.80, 0.80, 1.00, 0.0 },
	[tagRGBAOcc]			= { 0.00, 0.00, 1.00, 1.00, 0.0 },
	[tagRGBAFoc]			= { 1.00, 1.00, 1.00, 1.00, 0.0 },
	[tagRGBASel]			= { 0.60, 0.90, 1.00, 1.00, 0.0 },
	[tagRGBAAlt]			= { 0.80, 0.00, 0.00, 1.00, 0.0 },
	[tagRGBABoth]			= { 0.80, 0.00, 0.80, 1.00, 0.0 },
								/*  R     G     B     A     Alignment [3] */
	[tabRGBAFocusText]	= { 0.90, 0.95, 1.00, 0.90, -10 },
	[tabRGBATopText]		= { 0.25, 0.65, 1.00, 0.70, 0.5 },
	[tabRGBAOtherText]	= { 0.25, 0.45, 1.00, 0.40, 0.5 },
/***  THEME NOTES:
[1]: Offset values are added to the calculated space for the
     item.  Y and H values are inverted for bottom bars.

[2]: The last values of tabRGBAFocus and statGRBA are used for
     to set the line width for all tab or status borders.

[3]: positive alignment numbers are a fraction tab space.
     0.0 is left aligned,
     0.5 is centered,
     1.0 is right aligned.
     Negative alignment numbers are used for a number of 
     pixels to offset from the start of the tab.
****/
};


const char *string[127] = {
	[ 0 ] = "3M0g1bS",	// FULLSCREEN
/* macro values 0 - 31 can only be accessed by key bindings as there
   are no printable (typeable) characters for those values */
	['f'] = "3M0g1bh",	// FULLSCREEN
	['F'] = "1M4g1bs",	// UNFULLSCREEN change "4" in 3rd place to your gap size
	['m'] = CMD("interrobang"),
	['t'] = CMD("urxvt"),
	['w'] = CMD("firefox"),
};


/* clients:
	0	currently focused
	1	last focused
	2	user marks
	3	''
	...
*/

#define KEY1	Mod4Mask
#define KEY2	Mod1Mask
#define KEY3	ControlMask
#define KEY4	ShiftMask

/* tag-like: */
#define TagKey(KEY,TAG)	\
	{ KEY1,			KEY,	TAG "t"}, \
	{ KEY1|KEY2,	KEY,	TAG "x"}, \
	{ KEY1|KEY3,	KEY,	TAG "T"}, \
	{ KEY1|KEY4,	KEY,	TAG "m"}

/* more "desktop-like": */
/*
#define TagKey(KEY,TAG)	\
	{ KEY1,			KEY,	TAG "x"}, \
	{ KEY1|KEY2,	KEY,	TAG "t"}, \
	{ KEY1|KEY3,	KEY,	TAG "m"}, \
	{ KEY1|KEY4,	KEY,	TAG "T"}
*/

const Key key[] = {
	{ KEY1,	XK_Return,	";t"},
	{ KEY1,	XK_p,			";m"},
	{ KEY1,	XK_w,			";w"},
	{ KEY1,	XK_q,			"Q"},
	{ KEY2,	XK_F4,		"0q"},
	{ KEY1,	XK_Tab,		"o"},
	{ KEY2,	XK_Tab,		"v"},
	TagKey(	XK_1,			"1"),
	TagKey(	XK_2,			"2"),
	TagKey(	XK_3,			"3"),
	TagKey(	XK_4,			"4"),
	TagKey(	XK_5,			"5"),
	TagKey(	XK_6,			"6"),
};





/* keys  
  c ef       n p r  uvw yz
 BCDEFG I    NOP R  UVW YZ

COMMAND MODE
	[#]b(s|h|t|b|x)
		# = bar number (default = 1)
		s|h = show/hide
		t|b = top/bottom
		x   = toggle show/hide
	#t
		toggle visibility of tag # 
	#T
		toggle window's presence on tag #
	#(s|S|x|X)
		show/hide exlusive/all-but tag #
	#(m|a|A)
		move/add/remove focused window to tag #
	v
		toggle view
	[#](;|:)(a-z)
		launch command/macro associated with string a-z # times
			w = firefox
			t = urxvt
			m = interrobang
	[#]j|k
		move focus up/down in stack # times
	h|l
		move focus to top/bottom of stack
	o
		foccus "other"
	[#]J|K
		move window up/down in stack # times
	H|L
		move window to top/bottom of stack
	[#]i|d
		increase/decrease master size by # steps
	[#]+|-
		increase/decrease nclients by #
	>|<
		nclients = 1/all
	[#]M
		Tile mode: rstack (default), bstack 1, monocle 2
	#(w|W)
		target = winmarks # / set winmarks #
	#g
		set gap size
	([#]q|Q)
		kill client (mark #) / quit
	ENTER | ESC
		return to normal mode (execute string on ENTER, ignore on ESC)
*/
