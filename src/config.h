

/* STATUS FORMAT STRING
	i	tag icon
	n	tag name
	T	master tabs
	t	stack tabs
	1	status one
	2	status two
	3	status three
	...
*/

const char *status_fmt = "%i%n %T%t %1\n%2\n%3\n";
const Bool focusfollowmouse = False;

const int containers[]	= { 1, 3, -1 };
const int container_pad	= 10;
const int client_opts	= ATTACH_TOP;
const int bar_opts		= BAR_VISIBLE | BAR_TOP | SET_BAR_HEIGHT(18);

const char *string[127] = {
	[ 0 ] = "mm",
	[ 1 ] = "mm",
	['m'] = "interrobang",
	['t'] = "urxvt",
	['w'] = "firefox",
};


/* clients:
	0	currently focused
	1	last focused
	2	top master
	3	bottom master
	4	top stack
	5	bottom stack
*/

#define KEY1	Mod4Mask
#define KEY2	Mod1Mask
#define KEY3	ControlMask
#define KEY4	ShiftMask
/* tag-like */
#define TagKey(KEY,TAG)	\
	{ KEY1,			KEY,	"t" TAG }, \
	{ KEY1|KEY2,	KEY,	"x" TAG }, \
	{ KEY1|KEY3,	KEY,	"T" TAG }, \
	{ KEY1|KEY4,	KEY,	"m" TAG }

/* more "desktop-like":
#define TagKey(KEY,TAG)	\
	{ KEY1,			KEY,	"x" TAG }, \
	{ KEY1|KEY2,	KEY,	"t" TAG }, \
	{ KEY1|KEY3,	KEY,	"m" TAG }, \
	{ KEY1|KEY4,	KEY,	"T" TAG }
*/

const Key key[] = {
	{ KEY1,	XK_Return,	"ct"},
	{ KEY1,	XK_p,			"cm"},
	{ KEY1,	XK_w,			"cw"},
	{ KEY1,	XK_q,			"Q"},
	{ KEY2,	XK_F4,		"q0"},
	{ KEY2,	XK_Tab,		"v"},
	TagKey(	XK_1,			"1"),
	TagKey(	XK_2,			"2"),
	TagKey(	XK_3,			"3"),
	TagKey(	XK_4,			"4"),
	TagKey(	XK_5,			"5"),
	TagKey(	XK_6,			"6"),
};





/* keys
    e g      n pqr  u w yz
 BCDE G I    NOPQR  UVW YZ

COMMAND MODE
	b[#][sSxXtT]
		# = bar number (default = 1)
		s|S = show/hide
		x|X = show exclusively (hide others) (default)
		t|T = top/bottom
	t[#]
		toggle visibility of tag # (default = current)
	T[#][WIN]
		toggle window's presence on tag # (default = current)
	s|S|x|X[#]
		show/hide exlusive/all-but tag #
	m|a|A[#]
		move/add/remove focused window to tag #
	v
		toggle view
	c[a-z]
		launch command associated with a-z
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
	f
		toggle fullscreen
	F
		toggle floating
	M[rbm]
		Tile mode: rstack, bstack, monocle (default = cycle)
	ENTER | ESC
		return to normal mode (execute string on ENTER, ignore on ESC)
*/
