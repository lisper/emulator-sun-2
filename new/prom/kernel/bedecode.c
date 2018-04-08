/* Copyright (c) 1982 by Jeffrey Mogul */

/*
 * bedecode.c
 *
 * decode a Sun bus error, and return a string giving the cause
 *
 * Jeffrey Mogul	24 February 1982
 *
 * Philip Almquist	January 10, 1986
 *	- Remove break statements following return statements to shut lint up.
 *	- Return a value from bedecode if we fall through all the cases (this
 *	  is probably impossible, but lint doesn't know that).
 */

#include <buserr.h>
#include <pcmap.h>
#include <statreg.h>

/* codes for Read/Write field */
#define	ACC_WRITE	0x00
#define	ACC_READ	0x20

/* codes for function field */
#define	FUNC_DATA	1
#define	FUNC_PROG	2
#define	FUNC_USER	0
#define	FUNC_SUPER	4

/* codes for operations */
#define	MAKE_SUPER(o)	((o)<<3)

#define	UR	0x01
#define	UW	0x02
#define	UE	0x04
#define	SR	MAKE_SUPER(UR)
#define	SW	MAKE_SUPER(UW)
#define	SE	MAKE_SUPER(UE)

/*
 * table of legal operations, indexed by protection code
 */
char legal[16] = {
	/* 0  */	0,
	/* 1  */	SE,
	/* 2  */	SR,
	/* 3  */	SR|SE,
	/* 4  */	SR|SW,
	/* 5  */	SR|SW|SE,
	/* 6  */	SR|UR,
	/* 7  */	SR|SW|UR,
	/* 8  */	SR|UR|UW,
	/* 9  */	SR|SW|UR|UW,
	/* 10 */	SR|SW|UR|UE,
	/* 11 */	SR|SW|UR|UW|UE,
	/* 12 */	SR|SE|UR|UE,
	/* 13 */	SR|SW|SE|UR|UE,
	/* 14 */	SR|SW|SE|UE,
	/* 15 */	SR|SW|SE|UR|UW|UE
};

char *bedecode(bei, r_sr)
register struct BusErrInfo *bei;	/* bus error information block */
register int r_sr;		/* status register */
{
	register long addr = *((long *)(&(bei->AccAddrLo)));
	register int func = bei->FuncCode;

	register int sprot;
	register int access;
	register int pspace;
	
#ifdef	DEBUG
	printf("Addr %x, func %x, read %x, sr %x\n",
		addr, func, bei->Read, r_sr);
#endif	DEBUG

	if (addr > (ADRSPC_RAM + ADRSPC_SIZE)) {
	    /* address out of mapped range */
	    if (r_sr & SR_SUPERVISOR) {
	    	/* but in supervisor mode; weird error */
		return("Spurious Bus");
	    }
	    else
		return("System Space");
	}

	/* if it gets to here, access address is mapped */
	
	/* determine the segment protection for the access address */
	sprot = legal[(GETSEGMAP(addr>>15)>>8)&0xF];
	
#ifdef	DEBUG
	printf("GETSEGMAP(%x) = %x\n",addr>>15,GETSEGMAP(addr>>15));
	printf("sprot %x\n",sprot);
#endif	DEBUG

	/* Now, determine what the access really is */
	
	if (func & FUNC_PROG) {
	    /* Program access, must be execute */
	    access = UE;
	}
	else {	/* Data access, must be read or write */
	    if (bei->Read) {
	    	access = UR;
	    }
	    else {
	    	access = UW;
	    }
	}
	if (func & FUNC_SUPER) {
	    /* supervisor access */
	    access = MAKE_SUPER(access);
	}
	
	/* Now test protection code to see if access is allowed */

#ifdef	DEBUG
	printf("access %x\n",access);
#endif	DEBUG

	if ((access & sprot) == 0) {
	    return("Protection");
	}
	
	/* if we get here, segment protection OK */
	
	/* determine Page address space */
	pspace = GETPAGEMAP(addr>>11) & 0x3000;

#ifdef	DEBUG
	printf("pspace %x\n", pspace);
#endif	DEBUG

	switch (pspace) {

	case PGSPC_NXM:
		return("Page Invalid");

	case PGSPC_MBMEM:
	case PGSPC_MBIO:
		return("Timeout");

	case PGSPC_MEM:
		return("Parity");
	}
	return("Unknown???");	
}
