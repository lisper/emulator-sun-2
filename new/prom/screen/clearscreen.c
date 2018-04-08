/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * clearscreen.c
 *
 * Clears Sun Frame Buffer screen
 *
 * Jeffrey Mogul @ Stanford	30 June 1981
 *	after code by Bill Nowicki
 *
 */

#include "graphmacs.h"

/* erase display */
clearscreen()
{
	register int x;

	SETGXFUNC(ERASEPOINT);
	SETGXWIDTH(16);		/* zap 16 bits at a time */

	for (x = 0; x < GXBITMAPSIZE; x += 16) {
	    register short *p = (short *)(GXUnit0Base|GXupdate|GXselectY);
	    register short *q = &(p[GXBITMAPSIZE]);
	    
	    GXsetX(x);		/* set current x address */
	    
	    while (p <= q)
	    	*p++ = 1;
	}

	SETGXFUNC(SHOWPOINT);	/* reset things to "normal" */
	SETGXWIDTH(1);
}
