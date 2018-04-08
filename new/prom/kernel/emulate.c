/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * emulate.c
 *
 * Emulator trap handler for Sun MC68000 ROM monitor
 *
 * Philip Almquist	January 10, 1986
 *	- Remove break statements following return statements to shut up lint
 */

#ifdef EMULATE
#include "../h/sunmon.h"
#include "../h/globram.h"
#include <pcmap.h>
#include "../h/sunemt.h"
#include <statreg.h>

#define	ARG_TRAPTYPE	0	/* position of trap type on arg list */
#define	ARG_ARG1	1	/* position of argument 1 */
#define	ARG_ARG2	2	/* position of argument 2 */
#define	ARG_ARG3	3	/* position of argument 3 */

Emulate(pcadr,usrsr,usrsp)
register long *pcadr;	/* address of saved pc */
register short usrsr;	/* user's status register */
register long *usrsp;	/* user stack pointer */
{
	register int traptype;
	register int traparg1;
	register int traparg2;
	register int traparg3;
	register GlobDes *gp = GlobPtr;
	
	if (usrsr&SR_SUPERVISOR) {	/* super-mode trap */
	    usrsp = &(pcadr[1]);	/* arguments follow saved pc */
	    }

	traptype = usrsp[ARG_TRAPTYPE];
	traparg1 = usrsp[ARG_ARG1];	/* this might be a bug! suppose
					 * traptype is at bottom of stack! */
	traparg2 = usrsp[ARG_ARG2];
	traparg3 = usrsp[ARG_ARG3];

	/* Check if a user-mode trap is reserved to supervisor mode */

	if ( (SUPERONLY(traptype)) && ((usrsr&SR_SUPERVISOR) == 0) ) {
		/* nasty, nasty */
		return(-1);	/* return failure */
	}

	switch (traptype) {
	
	case EMT_PUTCHAR:
		putchar(traparg1);
		break;

	case EMT_VERSION:
		return((1*0x100)+1);

	case EMT_GETCHAR:
		return(getchar());
	
	case EMT_SETECHO:
		gp->EchoOn = traparg1;
		return(0);

	case EMT_SETSEGMAP:
		/* this must be done in registers since changing
		 * the context may lose us our ram!
		 */
		traptype = GETCONTEXT;		/* remember current
						 * context (traptype
						 * is free) */
		SETCONTEXT(traparg1);
		SETSEGMAP(traparg2,traparg3);
		SETCONTEXT(traptype);
		return(0);

	case EMT_GETSEGMAP:
		/* this must be done in registers since changing
		 * the context may lose us our ram!
		 */
		traptype = GETCONTEXT;		/* remember current
						 * context (traptype
						 * is free) */
		SETCONTEXT(traparg1);
		traparg3 = GETSEGMAP(traparg2);
		SETCONTEXT(traptype);
		return(traparg3);

	case EMT_GETCONTEXT:
		return(GETCONTEXT);
		
	case EMT_SETCONTEXT:
		SETCONTEXT(traparg1);
		return(0);
		
	case EMT_GETMEMSIZE:
		return(gp->MemorySize);

	case EMT_TICKS:
		return(gp->RefrCnt);

	case EMT_GETCONFIG:
		return(GETCONFIG);

	case	EMT_FBMODE:
		traparg2 = gp->FrameBuffer;	/* save old value */
		if ((traparg1 == 1) || (traparg1 == -1))	/* if legal */
		    if (gp->FrameBuffer)	/* if not now zero */
			gp->FrameBuffer = traparg1;	/* set new */
		return(traparg2);		/* return OLD value */

    	case    EMT_PROCTYPE:
#ifdef SMI8
    	        return(0);
#endif SMI8
#ifdef CADLINC
    	        return(0);
#endif CADLINC
#ifdef SMI10
    	        return(1);
#endif SMI10
#ifdef FTI10
    	        return(2);
#endif FTI10

	default:
		return(-1);
	}
}
#endif EMULATE
