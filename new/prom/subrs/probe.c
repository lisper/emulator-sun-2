/* Copyright (c) 1981 by Jeffrey Mogul */

/*
 * probe.c
 *
 * Determine if an interface is present on the bus. 
 * Jeffrey Mogul	25 March 1982
 */

/*static int	OKFlag;*/

/*
 * Temporarily re-routes Bus Errors, and then tries to
 * read a short from the specified address (for interfaces
 * that do not respond kindly to short reads, something
 * will have to be arranged.)  If a Bus Error occurs,
 * then we presume that the address is unfilled and return
 * false (0); otherwise, return true.
 */
ProbeAddress(adr)
short *adr;
{
#ifdef	NOTREG
	short dummy;
#else
	register short	dummy;		/* MUST be a register */
#endif	NOTREG
	register long OldBE=0;		/* used in asm()s only */
				/* set here to force compiler to save it */
	register int OKFlag;
	
	asm("	movl	8,d6");		/* save old BE vector */
	asm("	movl	#_PAlabel,8");	/* re-direct Bus Errors */
	
	OKFlag = 1;			/* assume no error */
	
	dummy = *adr;			/* try access */
	asm("	movl	d6,8");		/* restore normal BE vectoring */
	
	return(OKFlag);
	
	/* Bus Errors Come Here */
	asm("_PAlabel:");
	asm("	addql	#8.,sp");	/* pop garbage */
#ifdef	NOTREG
	asm("	addql	#2.,sp@(-2)");	/* offending instruction is 2 words */
#endif	NOTREG
	OKFlag = 0;			/* BE means "not ok" */
	asm("	rte");
}
