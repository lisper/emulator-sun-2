/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * confreg.h
 *
 * Bit definitions for Sun configuration register
 *
 * Jeffrey Mogul	16 November 1981
 *
 * Note that bits will float HIGH (i.e., 1) so that codings are assigned
 * to minimize jumpers needed for a "normal" configuration.  Further,
 * the probable failure modes of all high or all low should yield
 * semi-useful results!
 *
 * MODIFIED:
 *	25 January 1983	Jeffrey Mogul
 *	- created BreakEnable (asserted HIGH) by reassigning a "reserved"
 *		bit.
 */

#ifndef	CONFREG_DEFS
#define	CONFREG_DEFS

#define	uint unsigned int

struct ConfReg {
	/* ROM Monitor options */
	uint HostSpeed:3;		/* host UART speed code */
	uint ConsSpeed:2;		/* console UART speed code */
			/* see encodings below */
	uint UseFB:1;		/* use frame buffer if present */
			/* asserted HIGH */

	uint ErrorRestart:1;		/* hard restart on error */
			/* asserted LOW */
	uint BreakEnable:1;		/* break/abort has effect */
			/* asserted HIGH */

	uint Bootfile:4;		/* bootfile number */
			/* see encodings below */
	
	/* Frame Buffer Options; see note below */
	uint FBType:1;		/* type of display */
			/* 1 = landscape, 0 = portrait */
	uint FBother:1;		/* reserved */
	
	/* User-assigned */
	uint UserAssigned:2;
};


/*
 * Speed Encodings: 
 * This table is arranged to provide usable codings in either
 * 2 or 3 bits.  The idea here is that if we want to use a 2-bit
 * coding for the console, and a three-bit coding for the host line, we
 * can use the same table and get the most likely speeds for the
 * console into 2 bits.
 *
 * Implementation notes:
 *	(1) "no jumpers" gives 9600 baud
 *	(2) these values are complemented and then used as indices
 *		into the UART speed table
 *	(3) Unfortunately, there does not seem to be a reasonable
 *		way of enforcing a correspondence between
 *		"tables.c" and this file; be careful!
 */

/* Console and Host: */
/*	     speed		  bits */
#define	BRATE_9600	7	/* (1)11 */	/* table entry 0 */
#define	BRATE_4800	6	/* (1)10 */
#define	BRATE_1200	5	/* (1)01 */
#define	 BRATE_300	4	/* (1)00 */

/* Host only: */
/*	     speed		  bits */
#define	BRATE_2400	3	/* 011 */
#define	 BRATE_600	2	/* 010 */
#define	 BRATE_110	1 	/* 001 */ /* (approx/this may not work) */
#ifndef	BRATE_9600
#define	BRATE_9600	0	/* 000 */ /* or something more useful? */
#endif	BRATE_9600


/*
 * Bootfile number encodings:
 *
 * Note: "normal" mode requires all four jumpers in
 */

/*	symbol			bits	action */
/*	======			====	====== */
#define	BOOT_TO_MON	0	/* 0000 */
				/* don't autoboot,  enter Console Monitor */
#define	BOOT_SELF_TEST	0xF	/* 1111 */
				/* don't autoboot, run self-test code */
				/* when this code is set, all other
				 * options are ignored.
				 */
/* all other codes reserved for autobooting programs by number */

/*
 * Note: Frame Buffer Options
 * Although I am allocating two bits here, this function really
 * should be made part of the Frame buffer interface, if possible.
 * When this is done, these two bits can revert to "user-assigned".
 */

/*
 * Note: failure mode
 *
 * The most likely failure mode (according to AVB) is that
 * the configuration register will read all 1's.  This gives the following
 * results:
 *	Host speed: 9600
 *	Console speed: 9600
 *	Don't restart on error
 *	Use Framebuffer
 *	run self-test code
 */
#endif	CONFREG_DEFS
