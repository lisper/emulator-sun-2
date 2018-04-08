/* Copyright (C) 1982 by Jeffrey Mogul  */

/*
 * gethexbyte.c
 *
 * common subroutines for Sun ROM monitor: read a byte in hex format
 *
 * original author: Luis Trabb Pardo
 * largely re-written by Jeffrey Mogul	April, 1981  Stanford University
 * restructured again by Jeffrey Mogul  February, 1982
 */

#include "../h/sunmon.h"
#include "../h/asmdef.h"
#include "../h/globram.h"

/* get one hex-coded byte */
gethexbyte()
{
	register int v = 0;
	
	v = ishex(getone())<<4;
	return(v|ishex(getone()));
}
