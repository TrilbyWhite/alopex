/* copyright J.McClure 2012-2013, see alopex.c or COPYING for details */

static const char colors[LASTColor][9] = {

#ifdef SummerCoat
	[Background]	= "#201814",
	[Default]		= "#686858",
	[Occupied]		= "#DDBB88",
	[Selected]		= "#EE8844",
	[Urgent]		= "#FFDD0E",
	[Title]			= "#EECCAA",
	[TabFocused]	= "#D0B072",
	[TabFocusedBG]	= "#242424",
	[TabTop]		= "#BB8844",
	[TabTopBG]		= "#242424",
	[TabDefault]	= "#686858",
	[TabDefaultBG]	= "#181818",
	[TagLine]		= "#FF4400",

#else /* default WinterCoat */
	[Background]	= "#101010",
	[Default]		= "#686868",
	[Occupied]		= "#68A0DD",
	[Selected]		= "#BBE0EE",
	[Urgent]		= "#FF8880",
	[Title]			= "#DDDDDD",
	[TabFocused]	= "#68B0E0",
	[TabFocusedBG]	= "#242424",
	[TabTop]		= "#486488",
	[TabTopBG]		= "#242424",
	[TabDefault]	= "#686868",
	[TabDefaultBG]	= "#181818",
	[TagLine]		= "#2436AA",

#endif
};

