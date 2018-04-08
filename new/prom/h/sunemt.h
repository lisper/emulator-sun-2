/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * sunemt.h
 *
 * emulator trap definitions for the Sun MC68000 ROM monitor
 *
 * Jeffrey Mogul @ Stanford	22 May 1981
 */

/*
 * Negative EMT type codes are reserved to supervisor mode
 */

#define SUPERONLY(x)	((x) < 0)

#define SU(x)	(-(x))

#define	EMT_TICKS	0	/* ticks() */
#define	EMT_PUTCHAR	1	/* putchar(x) */
#define EMT_VERSION	2	/* version() */
#define EMT_GETCHAR	3	/* getchar() */
#define	EMT_GETMEMSIZE	4	/* getmemsize */
#define EMT_SETECHO	5	/* setecho(0/1) */
#define	EMT_GETCONFIG	6	/* getconfig() */
#define	EMT_FBMODE	7	/* fbmode(0/1/-1) */
/* The following EMT's have been reserved for the corresponding
 * functions, but remain to be incorporated into proms.
 */
#define EMT_MAYGET	8	/* mayget() (nonblocking getchar) */
#define EMT_MAYPUT	9	/* mayput() (nonblocking putchar) */
#define EMT_SCRPUT	10	/* write char to screen */
#define EMT_KEYGET	11	/* get character from parallel keyboard input */
#define EMT_INSOURCE	12	/* access to input source selector */
#define EMT_OUTSINK	13	/* access to output source selector */
#define EMT_PROCTYPE    14  	/* get the processor type */

#define	EMT_SETSEGMAP	SU(1)	/* setsegmap(cxt,segno,entry) */
#define EMT_GETSEGMAP	SU(2)	/* getsegmap(cxt,segno) */
#define EMT_GETCONTEXT	SU(3)	/* getcontext() */
#define EMT_SETCONTEXT	SU(4)	/* setcontext(x) */
