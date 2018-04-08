/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * transparent.c
 *
 * Transparent-mode module for Sun ROM monitor
 *
 * Jeffrey Mogul @ Stanford	 1 May 1981
 *  split into separate file	17 Feb 1982
 */

#include "../h/sunmon.h"
#include "../h/asmdef.h"
#include "../h/globram.h"
#include "../h/sunuart.h"

/*
 * transparent mode - cross connect UARTs A and B.
 * Since it is quite possible that the two UARTs are running
 * at different speeds, there is a potential flow control
 * problem here.  We leave this up to the user to solve;
 * this routine runs at the speed of the slower side.
 */
transparent()
{
	register char c;
	register int EscPending = 0;	/* true if we just saw an escape */

	loop {
	    /* move char from Host to here */
	    if (UARTstat(B_chan)&NECrxrdy)
		writechar(ReadUART(B_chan));

	    /* has user typed a character? */
	    if (UARTstat(A_chan)&NECrxrdy) {
		c = ReadUART(A_chan);	/* get the char */
		if (EscPending)
		    if ((c&UPCASE) == 'C') {
			break;	/* esc-c: end transparent mode */
		    }
		    else EscPending = 0;	/* clear flag */
		else if ((c&NOPARITY)==(GlobPtr->EndTransp)) {
			EscPending++;
			continue;
		}
		busyoutb(c);	/* send char to host */
	    }
	}
}
