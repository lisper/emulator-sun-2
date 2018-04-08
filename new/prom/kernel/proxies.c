/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * proxies.c
 *
 * Sun ROM Monitor
 *
 * Jeffrey Mogul  @ Stanford	25 January 1982
 *
 * "Proxy" routines - used to redirect calls to routines
 * that have been moved into the second pair of proms.
 */

#include "../h/globram.h"
#include "../h/prom2.h"

char NoBootProm[] = "No bootload prom.\n";

bootload(fname)
char *fname;
{
	if (GlobPtr->Prom2)
	    return(prm2_bootload(fname));
	else {
	    message(NoBootProm);
	    return(0);
	}
}

boottype(bt)
int bt;
{
	if (bt >= 0xff) {	/* minor kludge - 'W ff' = 'list autoboots' */
	    return(listboots());
	}
	else if (GlobPtr->Prom2) {
	    return(prm2_boottype(bt));
	}
	else {
	    message(NoBootProm);
	    return(0);
	}
}

writechar(c)
char c;
{
	if (GlobPtr->Prom2)
	    return(prm2_writechar(c));
	else
	    return(busyouta(c));
}
