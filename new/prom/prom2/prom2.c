/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * prom2.c
 *
 * Code for high prom in Sun ROM monitor
 *
 * Jeffrey Mogul @ Stanford	12 August 1981
 *
 * This file dispatches functions called by low prom
 * into the high prom.  The prom2() routine MUST be the
 * very first code in the high prom for this to work!
 */

#include "../h/prom2.h"
#include "../h/sunmon.h"

prom2(code, arg)
register long code;
register long arg;
{
	switch(code) {

	case PRM2_WRITECHAR:
		return(writechar(arg));

	case PRM2_BOOTLOAD:
		return(bootload(arg));

	case PRM2_VERSION:
		return(MAKEVERSION(1,1));

	case PRM2_BOOTTYPE:
		return(boottype(arg));

	default:
		message("monitor error: prom2\n");
		return(0);
	}
}
