/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * sunscreen.c
 *
 * exports:
 *	initscreen()
 *	clearEOL(x,y)
 *	putcursor(x,y,set)
 *	putat(x,y,c)
 *
 * Jeffrey Mogul @ Stanford	6 August 1981
 *
 * Modified by Bill Nowicki August 1982
 *	- changed graphmacs.h to framebuf,
 *	- updated Macros for above
 */

#define	ASMHACK

#include "../h/screen.h"
#include <framebuf.h>
#define GXBITMAPSIZE 1024

/*
 * These are some macros which attempt to avoid multiplying
 * by using explicit shifts and adds
 */

/* YSTART - compute starting Y-coordinate (includes <<1 for word ptr) */
#if	(CHARHEIGHT+LEADING) == 17
#define YSTART(y)	( ((y)*32) + ((y)*2) )
#else
#if	(CHARHEIGHT+LEADING) == 18
#define	YSTART(y)	( ((y)*32) + ((y)*4) )
#else
#define	YSTART(y)	((y)*(2*(CHARHEIGHT+LEADING)))
#endif	18
#endif	17

/* XSTART - computer starting X-coord (includes <<1 for word ptr) */
#if	(CHARWIDTH+SPACING) == 10
#define	XSTART(x)	( ((x)*16) + ((x)*4) )
#else
#define	XSTART(x)	( (x)*(CHARWIDTH+SPACING) )
#endif

/* GXsetX2 - like GXsetX, but doesn't do a <<1 */
#define GXsetX2(x) *(short*)(GXBase|GXselectX|(x)) = 1;

initscreen()
{
	/* the order of registers here is CRUCIAL to the asm() below */
	register int x;
	register short *xstart;
	register short *xend;
	register short *ystart;
	register short *ytemp;
	register short *yend;
	
	GXcontrol = GXvideoEnable;
	GXfunction= GXclear;
	GXwidth   = 16;

#ifdef	ASMHACK
	xstart = (short *)(GXBase|GXselectX);
	xend = (short *)(GXBase|GXselectX|((GXBITMAPSIZE-1)<<1));
	
	ystart = (short *)(GXBase|GXupdate|GXsource|GXselectY);
	yend = (short *)
	   (GXBase|GXupdate|GXsource|GXselectY|((GXBITMAPSIZE-1)<<1));
		
	while (xstart < xend) {
		*xstart = 1;
		xstart += 16;
		
		ytemp = yend;

do_more:	asm(" moveml #/ffff,a2@-");
		if (ystart <= ytemp) goto do_more;
	}

#else
	for (x = 0; x < GXBITMAPSIZE; x += 16) {
		ystart = (short *)(GXBase|GXupdate|GXselectY);
		yend = &(ystart[GXBITMAPSIZE]);
		
		GXsetX(x);
		
		while (ystart <= yend)
			*ystart++ = 1;
	}
#endif	ASMHACK
}

/*
 * returns true iff a character was displayed
 */
putat(x,y,c)
register int x;
register int y;
register char c;
{
	register int h = CHARHEIGHT;
	register int ci;
	register short *yp;
#if	CHARWIDTH > 8
	register short *mp;
#else
	register char *mp;
#endif
	
	GXsetX2(XSTART(x));

	GXwidth = CHARWIDTH+SPACING;
	GXfunction = GXcopy;
	
	if ((c < MINCHAR) || (c > MAXCHAR)) return(0);

	c -= MINCHAR;

#if	CHARHEIGHT == 15
	/* avoid the multiply needed to access the array */
	ci = c;			/* force conversion to be done only once */
#if	CHARWIDTH > 8
	mp = (short*)((int)MapChar + 2*( (ci*16) - ci));
#else
	mp = (char*)((int)MapChar + (ci*16) - ci);
#endif	CHARWIDTH
#else
	mp = MapChar[c];
#endif	CHARHEIGHT == 15

	yp = (short *) (GXBase|GXupdate|GXsource|GXselectY|YSTART(y));

	while (h-- > 0) {
#if	CHARWIDTH > 8
		*yp++ = *mp++;
#else
		*yp++ = (*mp++)<<8;
#endif
	}
	return(1);	
}

putcursor(x,y,set)
register int x;
register int y;
register int set;
{
	register short *yp;
	
	GXsetX2(XSTART(x));

	GXwidth = CHARWIDTH+SPACING;

	if (set) {
	    GXfunction = GXset;
	}
	else
	    GXfunction = GXclear;
	
	yp = (short *) (GXBase|GXupdate|GXsource|GXselectY|YSTART((++y)));

	*(--yp) = 1;
}

clearEOL(x,y)
register int x;
register int y;
{
	/* the order of registers here is CRUCIAL to the asm() below */
	register short *ystartp;	/* stored in a5 */
	register short *xstartp;
	register short *xendp;
	
	GXwidth = 16;
	GXfunction = GXclear;
	
	xstartp = (short *)(GXBase|GXselectX|XSTART(x));
	xendp = (short *)(GXBase|GXselectX|(WIDTH*(CHARWIDTH+SPACING)<<1));

	ystartp = (short *)(GXBase|GXupdate|GXsource|GXselecty|YSTART(y));

	while(xstartp < xendp) {
	    *xstartp = 1;
	    xstartp += 16;
	    
#if	(CHARHEIGHT+LEADING) == 16
	    asm("	moveml #/ff,a5@");
#endif
#if	(CHARHEIGHT+LEADING) == 17
	    asm("	moveml #/1ff,a5@");
#endif
#if	(CHARHEIGHT+LEADING) == 18
	    asm("	moveml #/3ff,a5@");
#endif
	}
}
