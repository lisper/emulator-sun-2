/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * writechar.c
 *
 * Sun ROM Monitor
 *
 * Jeffrey Mogul  @ Stanford	19 March 1982
 *
 * Indirection to select between fwritechar and busyouta.
 */

#include "../h/globram.h"
writechar(c)
char c;
{
#ifdef	NOSCREEN	/* avoids pulling in ../screen stuff */
	return (busyouta(c));
#else
	if (GlobPtr->FrameBuffer > 0)
	    return(fwritechar(c));
	else
	    return(busyouta(c));
#endif
}
