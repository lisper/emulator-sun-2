/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * tables.c
 *
 * used to define data tables for UART control
 */

#include <nec7201.h>

/*
 * Initial Register definitions
 */

char	UARTInitSeq[] = {
	NECchres, NECchres,	/* 2 channel resets */
    /* reg,   initial value */
	2,	0,			/* default */
	4,	(NECrx1sb|NEC16clk),	/* 1 stop bit, 16x clock mode */
#ifdef FTI10
	3,	(NECrxena|NECrx8bt),
#else
	3,	(NECrxena|NECrx8bt|NECautoe),
#endif					/* rx enable, 8 bit, auto enable */
	5,	(NECtxrts|NECtxena|NECtx8bt|NECdtr), 
					/* RTS, tx enable, 8 bit, DTR */
	1,	0			/* no interrupts */
    };

char *UARTInitEnd = &UARTInitSeq[sizeof(UARTInitSeq)];

/*#define	MAKEBAUD(bd)	(13*(9600/bd))*/
#ifdef FTI10
#define	MAKEBAUD(bd)	((8*9600)/(bd))
#endif FTI10

#ifdef SMI10
#define	MAKEBAUD(bd)	((16*9600)/(bd))
#endif SMI10

#ifdef SMI8
#define	MAKEBAUD(bd)	((13*9600)/(bd))
#endif SMI8

#ifdef CADLINC
#define	MAKEBAUD(bd)	((13*9600)/(bd))
#endif CADLINC

unsigned short UARTSpeeds[] = {
    	MAKEBAUD(9600),
#ifdef	CADLINC
	MAKEBAUD(811),
#else
#ifdef	SMI
	MAKEBAUD(110),
#else
	MAKEBAUD(4800),
#endif	SMI
#endif	CADLINC
	MAKEBAUD(1200),
	MAKEBAUD(300),
	MAKEBAUD(2400),
	MAKEBAUD(600),
	MAKEBAUD(110),
	MAKEBAUD(9600)	/* could be something else */
};

