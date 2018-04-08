/* Copyright (C) 1982 by Jeffrey Mogul */


/*
 * asmdef.h
 *
 * Assembler Hacks for Sun MC68000 Monitor
 *
 * Jeffrey Mogul @ Stanford	15 May 1981
 *	after code written by Luis Trabb-Pardo
 */

/* branch to a label */
#define branch(a) asm (" .text;");\
		 asm("	jmp a")

/* push a word (short) */
#define wpush(a) asm ("	.text");\
	asm("	movw a,sp@-")

/* push a register */
#define rpush(a) asm ("	.text");\
	asm("	movl a,sp@-")

/* pop a register */
#define rpop(a) asm ("	.text");\
	asm("	movl sp@+,a")

/* define an entry in a vector of longs (e.g., exception vector) */
#define excv(a)	asm("	.text")\
		asm("	.long a")

/*
 * exclink - used to push a word describing an exception,
 *	then jump to ".enter".  The word contains two bytes,
 *	which form a two-character message.
 */
#define exclink(n,a,b) asm("	.text");\
	asm("n:	movw	#'a*256.+'b,sp@-");\
	asm("	jra	.enter")

/*
 * exclinkn - like exclink but pushes a numeric code
 */
#define exclinkn(n,x) asm("	.text");\
	asm("n: movw	#'x,sp@-");\
	asm("	jra	.enter")

/* return from interrupt/exception */
#define endsrv asm("	rte")

/* label a point for assembler jumps */
#define label(a)	asm("a:")

/* label a point globally for assembler jumps */
#define LabelGbl(a)	asm("	.globl a"); asm ("a:");

/* set processor interrupt level */
#define setINTLV(a)\
 {extern unsigned short sr; sr= (unsigned short)(0x2000+(a<<8));}

/* Register save area allocation */
#define _reg_d0		0
#define _reg_a0		8
#define _reg_ss		15
#define _reg_us		16
#define _reg_sr		17
#define _reg_pc		18
#define _reg_MX		18
asm("_disp_us=64.");

/* Auxiliary functions to save and restore things */
/* Allocation of saved registers on the stack:
	sp@(72.)...	PC
	sp@(70.)...	SR
	sp@(68.)...	 0
	sp@(64.)...	US
	sp@(62.)...	SS
	sp@(58.)...	A6
		.......
	sp@(32.)...	A0
	sp@(28.)...	D7
		.......
	sp@........	D0
*/

/* The Monitor sees the Register Save Area as 19 long parameters:
	monitor(r_pc, r_sr, r_us, r_ss,
		r_a6, r_a5, r_a4, r_a3, r_a2, r_a1, r_a0,
		r_d7, r_d6, r_d5, r_d4, r_d3, r_d2, r_d1, r_d0)
*/

/* Storing the exception cause after an exception: */
#define ExcCause(a)  asm("	.text");\
	asm("	movw	#a,sp@-");\

/* Saving registers after an exception: */
#define saveregs asm("	.text");\
	asm("	subql	#4.,sp");\
	asm("	moveml	#/FFFF,sp@-");\
	asm("	movl	usp,a0");\
	asm("	movl	a0,sp@(_disp_us)")

/* Restoring registers after a call to the monitor: */
#define restregs	asm("	.text");\
	asm("	movl	sp@(_disp_us),a0");\
	asm("	movl	a0,usp");\
	asm("	moveml	sp@+,#/FFFF");\
	asm("	addql	#6.,sp")

/*
 * Save Access Address etc. after BusErr or AdrErr; the assumption
 * is that 0 contains the address of the global data region, and
 * the first two longs of global data store the top two longs from
 * the exception stack frame.
 */
#define SAVEACCESS	\
	asm("	movl	a0,4.");	/* temporarily store a0 */\
	asm("	movl	0.,a0");	/* get global base pointer */\
	asm("	movl	sp@+,a0@+");	/* pop first long */\
	asm("	movl	sp@+,a0@");	/* pop second long */\
	asm("	movl	4.,a0");	/* restore a0 */

/*
 * Flush Access Address etc. after BusErr or AdrErr; we don't
 * need them and cannot rte until they are gone
 */
#define	FLUSHACCESS	asm("	addql #8.,sp");	/* pop two longs */

/*
 * Opcode definitions -
 *	The monitor occasionally stuffs some opcodes into
 *	memory.  These are defined here.
 */

#define	OPC_2NOP	0x4E714E71	/* TWO nops */
#define	OPC_NOP_RTS	0x4E714E75	/* nop followed by an rts */
#define	OPC_TRAP1	0x4E41		/* trap 1 */
#define	OPC_2UN1	0xFFFFFFFF	/* TWO "unimplemented 1111"s */
