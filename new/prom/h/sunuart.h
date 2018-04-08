/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * sunuart.h
 *
 * Defines semi-hardware-independent constants/macros for
 * sun on-board UART.
 *
 * Jeffrey Mogul @ Stanford
 *	after "necuart.h" by Luis Trabb-Pardo
 */

#include "nec7201.h"

/*  Address definitions for the nec7201 on the SUN board */

/* Channel definitions */

#define A_chan 	0
#define B_chan 	4

#define ContReg(ch)	*(char*)(0x600002+ch)
#define StatReg(ch)	*(char*)(0x600002+ch)	/* Read only */
#define TxHldReg(ch)	*(char*)(0x600000+ch)	/* Write only */
#define RxHldReg(ch)	*(char*)(0x600000+ch)	/* Read only */


/*  Programming the NEC PD7201 for the SUN M68000 board:  */
/*	Assumes External clock frequency 16 * baud rate */

/*
 * Set a NEC uart control register;
 *	ch == channel
 *	r == register #
 *	v == value
 */
#define NECset(ch,r,v) \
	ContReg(ch)= (char)r;\
	ContReg(ch)= (char)v

/*
 * Reset External/Status Interrupts
 */
#define	UART_RESXT(ch)	ContReg(ch) = NECrexst

extern	char UARTInitSeq[];
extern	char *UARTInitEnd;
#define	INITBEGIN	&UARTInitSeq[0]
/*#define	INITEND		&UARTInitSeq[sizeof(UARTInitSeq)]*/
#define	INITEND		UARTInitEnd

/*
 * initialize both uarts; Aspeed and Bspeed are indices into
 * UARTSpeeds[]
 *
 * It's not clear why this should be a macro (saves a few bytes)
 */

extern short UARTSpeeds[];

#define	INITUART(Aspeed, Bspeed) {\
	register char* p;\
	TCSetModeLoad(TIMAClk, TModUART, UARTSpeeds[Aspeed]);\
	TCSetModeLoad(TIMBClk, TModUART, UARTSpeeds[Bspeed]);\
	TMRLoadCmd(TCArmCnt(TSCountSelect(TIMAClk) + TSCountSelect(TIMBClk)));\
	for (p = INITBEGIN; p < INITEND;) {\
	    ContReg(A_chan) = *p;\
	    ContReg(B_chan) = *p++;\
	}\
	TxHldReg(A_chan) = '\0';\
	TxHldReg(B_chan) = '\0';\
    }

#define ReadUART(ch) RxHldReg(ch)

#define UARTstat(ch) StatReg(ch)


