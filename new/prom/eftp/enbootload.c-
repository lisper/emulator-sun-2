/* Copyright (C) 1981 by Stanford University */

/*
 * netboot.c
 *
 * Sun ROM monitor - bootload from ethernet
 *
 * Jeffrey Mogul @ Stanford	18 August 1981
 *
 * defines: 
 *	bootload(name)
 * which tries to load the named program and return its start
 * address, or 0 on failure.
 */

#include "../h/sunmon.h"

#ifndef	MAXBOOTTRIES
#ifdef	AUTOBOOT
#define	MAXBOOTTRIES 20
#else
#define MAXBOOTTRIES 5
#endif	AUTOBOOT
#endif	MAXBOOTTRIES

/*
 * get and setup a program via the network.  Keeps trying
 * until it succeeds or MAXBOOTTRIES is exceeded.
 */
bootload(bfname)
register char *bfname;
{
	register int bflen = -1;
	register int tries = 0;

	while ((bflen <= 0) && (tries < MAXBOOTTRIES)) {
	    if (tries++) putchar('.');
	    bflen = getbootfile(1,bfname,USERCODE,0);
	}
	if (bflen > 0)
	    return(setup(USERCODE,bflen));
	else {
	    message("Timeout\n");
	    return(0);
	}
}
