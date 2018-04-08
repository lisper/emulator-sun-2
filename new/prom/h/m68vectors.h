/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * m68vectors.h
 *
 * Defines addresses for MC68000 exception vectors
 *	Addresses are of type "long *"
 *
 * Jeffrey Mogul	Stanford University	22 April 1981
 *
 */

#define	EVEC_RESETSSP	((long *)0x000)
#define EVEC_RESETPC	((long *)0x004)
#define EVEC_BUSERR	((long *)0x008)
#define EVEC_ADDERR	((long *)0x00C)
#define EVEC_ILLINST	((long *)0x010)
#define EVEC_DIVZERO	((long *)0x014)
#define EVEC_CHK	((long *)0x018)
#define EVEC_TRAPV	((long *)0x01C)
#define EVEC_PRIV	((long *)0x020)
#define EVEC_TRACE	((long *)0x024)
#define EVEC_LINE1010	((long *)0x028)
#define EVEC_LINE1111	((long *)0x02C)

/*
 * vectors 0x30 - 0x5C are "reserved to Motorola")
 */

#define EVEC_SPURINT	((long *)0x060)
#define EVEC_LEVEL1	((long *)0x064)
#define EVEC_LEVEL2	((long *)0x068)
#define EVEC_LEVEL3	((long *)0x06C)
#define EVEC_LEVEL4	((long *)0x070)
#define EVEC_LEVEL5	((long *)0x074)
#define EVEC_LEVEL6	((long *)0x078)
#define EVEC_LEVEL7	((long *)0x07C)

#define EVEC_TRAP0	((long *)0x080)
#define EVEC_TRAP1	((long *)0x084)
#define EVEC_TRAP2	((long *)0x088)
#define EVEC_TRAP3	((long *)0x08C)
#define EVEC_TRAP4	((long *)0x090)
#define EVEC_TRAP5	((long *)0x094)
#define EVEC_TRAP6	((long *)0x098)
#define EVEC_TRAP7	((long *)0x09C)
#define EVEC_TRAP8	((long *)0x0A0)
#define EVEC_TRAP9	((long *)0x0A4)
#define EVEC_TRAPA	((long *)0x0A8)
#define EVEC_TRAPB	((long *)0x0AC)
#define EVEC_TRAPC	((long *)0x0B0)
#define EVEC_TRAPD	((long *)0x0B4)
#define EVEC_TRAPE	((long *)0x0B8)
#define EVEC_TRAPF	((long *)0x0BC)

/*
 * vectors 0xC0 - 0xFC are "reserved to Motorola"
 */

/*
 * vectors 0x100 - 0x3FC are User Interrupt Vectors
 */
#define EVEC_USERINT(v)	((long *)(0x100 + ((v)<<2)))

#define EVEC_LASTVEC	((long *)(0x3FC))
#define EVEC_AFTER	((long *)(0x400))
#define NUM_EVECS	256	/* number of exception vectors */

