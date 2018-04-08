/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * mapmem.c
 *
 * Sun MC68000 memory mapper -- figures out how much
 *				there is, validates map only
 *				for extant pages.
 *
 *				Returns number of pages.
 *
 * Input assumptions:
 *	(1) Segment table is set up so that current context
 *	is contiguously mapped starting at 0.
 *	(2) an INITMAP has been done to make all
 *	mapping registers point to onboard memory.
 *	(3) someone else is catching and ignoring Bus Errors
 *
 * Jeffrey Mogul 23 March 1981
 *	modified 7 May 1981 for PC board version (2-level maps)
 */

#include <pcmap.h>

#define	ALLOCHUNK	16	/* minimum memory size quantum */
#define OFFSET		0x700	/* safe place to scribble in page 0 */

mapmem()
{
	register int i;
	register int numpages;

	/* Method used is to run down memory writing the page
	 * number into selected spots.  Due to wraparound, only
	 * the numbers that correspond to real pages will remain
	 * after the write-pass is over.  Then, read through
	 * these same words until we find one that doesn't match
	 * its page number; we have gone too far and now know
	 * how many pages there are.
	 */

	for (numpages = PAGEMAPSIZE; numpages >= 0; numpages-=ALLOCHUNK) {
		*(int *) ( (numpages << 11) + OFFSET) = numpages;
	}

	for (numpages = 0; numpages < PAGEMAPSIZE; numpages += ALLOCHUNK) {
		if ( (*(int*)((numpages<<11)+OFFSET)) != numpages) {
			break;
		}
	}

	/* numpages now = # of real pages; invalidate all others */
	for (i = numpages; i < PAGEMAPSIZE; i++) SETPAGEMAP(i,PGSPC_NXM);

	/* now initialize MultiBus IO and memory spaces */
	InitMultiBusMap;

	return(numpages);
}
