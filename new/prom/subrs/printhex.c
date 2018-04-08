/* Copyright (C) 1982 by Jeffrey Mogul  */

/*
 * printhex.c
 *
 * common subroutines for Sun ROM monitor: print a hexadecimal number
 *
 * original author: Luis Trabb Pardo
 * largely re-written by Jeffrey Mogul	April, 1981  Stanford University
 * restructured again by Jeffrey Mogul  February, 1982
 * Edits:
 *	Philip Almquist, September 25, 1984
 *	- added printoctal
 */

#include "../h/sunmon.h"
#include "../h/asmdef.h"
#include "../h/globram.h"

char chardigs[]="0123456789ABCDEF";

/*
 * printhex prints rightmost <digs> hex digits of <val>
 */
printhex(val,digs)
register long val;
register int digs;
{
	register int i;

	i = ((--digs)&7)<<2;	/* digs == 0 => print 8 digits */
	
	for (; i >= 0; i-=4)
		putchar(chardigs[(val>>i)&0xF]);
}

/*
 * printoctal prints rightmost <digs> octal digits of <val>
 */
printoctal(val,digs)
register long val;
register int digs;
{
	register int i;

	if (digs <= 0)
		i = (11-1) * 3;		/* 11 digits is enough for 32 bits */
	else
		i = (digs-1) * 3;	/* 3 bits per digit */

	for (; i >= 0; i-=3)
		putchar(chardigs[(val>>i)&07]);
}
