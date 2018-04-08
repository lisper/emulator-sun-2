/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * screen.h
 *
 * Jeffrey Mogul @ Stanford	5 August 1981
 */

#define	LINES 24
#define	WIDTH 80

#define	CHARWIDTH	8
#define	CHARHEIGHT	15

#define	SPACING 2
#define	LEADING 2

#define	MINCHAR		' '
#define	MAXCHAR		127
#define	CURSORCHAR	0177

#if	CHARWIDTH > 8
extern short MapChar[MAXCHAR+1-MINCHAR][CHARHEIGHT];
#endif

#if	CHARWIDTH <= 8
extern char MapChar[MAXCHAR+1-MINCHAR][CHARHEIGHT];
#endif

/*
 * Entries for GlobPtr->ScreenMode
 */
#define	SM_NORMAL	0	/* quiescent state */
#define	SM_ESCSEEN	1	/* one <esc> just seen */
#define	SM_NEEDY	2	/* waiting for y coordinate */
#define	SM_NEEDX	3	/* waiting for x coordinate */

#define	GXBase	GXDefaultBase
