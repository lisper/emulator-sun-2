/* Copyright (C) 1982 by Jeffrey Mogul  */

/*
 * getnum.c
 *
 * common subroutines for Sun ROM monitor: get a number from input
 *
 * original author: Luis Trabb Pardo
 * largely re-written by Jeffrey Mogul	April, 1981  Stanford University
 * restructured again by Jeffrey Mogul  February, 1982
 */

#include "../h/sunmon.h"
#include "../h/asmdef.h"
#include "../h/globram.h"

/* get a hex number */
longword getnum()
{
	register longword v = 0;
	register int hexval;

	while ((hexval= ishex(peekchar()))>=0) {
			v= (v<<4)| hexval;
			getone();
	}
	return(v);
}
