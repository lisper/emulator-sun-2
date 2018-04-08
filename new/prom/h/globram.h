/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * globram.h
 *
 * Sun MC68000 ROM Monitor global data
 *
 * Jeffrey Mogul	15 May 1981
 *	created from code written by Luis Trabb-Pardo
 *
 * This defines a data structure that lives in RAM and is
 * used by the ROM monitor for its own purposes.
 *
 * There are two ways of accessing these data; the most
 * straightforward way is to say ``GlobPtr->xxx''; however,
 * if a single routine makes more than 3 accesses a global datum
 * (measured staticly), code size can be improved by first
 * loading GlobPtr into a register -- i.e.,
 * 	register GlobDes *gp = GlobPtr;
 */

#include "sunmon.h"
#include "confreg.h"

/*
 * things whose size does not actually matter are made into
 * ints; this removes the "extw; extl" sequences seen all over.
 */
typedef struct {
	union  {	/* MUST be first in Global Data */
	    struct	 {	/* how bus error info appears on stack */
	    	long os_l1;
		long os_l2;
	    } OnStack;
	    struct BusErrInfo Data;	/* structured according to contents */
	} BE;
	char linbuf[BUFSIZE+2];	/* data input buffer */
	char *lineptr;		/* next "unseen" char in linbuf */
	int linesize;		/* size of input line in linbuf */
	int BreakHit;		/* true when break key just hit */

	int recount;		/* count of S-records loaded */

	int debounce;		/* used in debounced Abort switch */

	short BreakVal;		/* instruction replaced by Break trap */
	word *BreakAdx;		/* address of current BPT */
	int BrkInPrg;		/* flag: true if we are handling BPT */

	int RefrCnt;		/* pseudo-clock counter */

	struct ConfReg Configuration;	/* copy of configuration register */
	long MemorySize;	/* size of memory in bytes */
	int CurContext;		/* current context register contents */
	char EndTransp;		/* escape character for transparent mode */
	int ConChan;		/* Control channel (A_chan or B_chan) */
	int EchoOn;		/* true if input should be echoed */
	int FrameBuffer;	/* true if frame buffer is console */
	int Prom2;		/* true if second PROM is there */
    	long DogMap0;	    	/* what was in page 0 at watchdog */
    	long DogRefr;	    	/* address of the old refresh routine */
    	int  WhyReset;	    	/* contains the reason for reset */
	
	/* following are local to second PROM */
	int Xcurr;		/* current screen x coordinate */
	int Ycurr;		/* current screen y coordinate */
	int ScreenMode;		/* mode bits for screen */

	int BootLoadDef;	/* default bootloader */
		} GlobDes;

#define GlobBase (*((GlobDes **)0))	/* set to point at base of
					 * monitor global data
				 	*/

#define GlobPtr ((GlobDes *)STRTDATA)
