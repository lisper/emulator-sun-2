/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * busyio.c
 *
 * busywait I/O module for Sun-1 ROM monitor
 *
 * Jeffrey Mogul @ Stanford	1 May 1981
 */

#include "../h/sunmon.h"
#include "../h/asmdef.h"
#include "../h/globram.h"
#include "../h/sunuart.h"

busyouta(x)
register char x;
{
	while((char)(UARTstat(A_chan)&NECtxrdy)==0);	/* wait for ready */
	    TxHldReg(A_chan) = x;
}


busyoutb(x)
register char x;
{
	while((char)(UARTstat(B_chan)&NECtxrdy)==0);	/* wait for ready */
	TxHldReg(B_chan) = x;
}

putchar(x)
register char x;
{
	register int (*outptr)();
	int writechar();
	
	if (GlobPtr->ConChan == A_chan)
	    outptr = writechar;
	else
	    outptr = busyoutb;

	if (x == '\n') outptr('\r');

	outptr(x);

/* ???	x &= NOPARITY;	/* strip parity bit */

/*	if (x == '\r') outptr('\n'); */
}

char getchar()
{
	register char c;
	register char *rp;
	
	rp = &ContReg(GlobPtr->ConChan);	/* rp -> control reg */
	
	while (!( (*(char*)rp) & NECrxrdy));	/* busy wait for char */

	rp -= (int)(&ContReg(A_chan) - &RxHldReg(A_chan));
						/* rp -> data reg */
	c = (*(char*)rp)&NOPARITY;		/* get char, strip parity */
	
	if (GlobPtr->EchoOn && (GlobPtr->ConChan == A_chan) )
	    putchar(c);	/* echo on A side only */
	    	/* temporary, EchoOn should be parameterized */

	return(c);
}
