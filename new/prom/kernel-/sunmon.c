
/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * sunmon.c
 *
 * "kernel" of monitor for Sun MC68000 ROM
 *
 * Started way back when by Luis Trabb-Pardo
 * present state due to Jeffrey Mogul @ Stanford	18 May 1981
 *	version 2 (pc board) support added		19 May 1981
 *	fixed bug in initialization (Bus Errors)	 7 Aug 1981
 *	changed to new timer stuff; fixed bug in init such that
 *		refresh wasn't being cleared; added configuration
 *		register				17 Dec 1981
 *	general restructuring and configuration register support
 *							   Feb 1982
 *	watchdog timer support				18 Mar 1982
 *	partial bugfix in K1 command			22 Sep 1982
 *	conf. reg. control of break/abort		26 Jan 1983
 */

#include "../h/sunmon.h"
#include "../h/asmdef.h"
#include "../h/globram.h"
#include "../h/sunuart.h"
#include "../h/suntimer.h"
#include "../h/confreg.h"
#include <pcmap.h>
#ifdef	FRAMEBUF
#include <framebuf.h>
#include "../h/screen.h"
#endif	FRAMEBUF
#include "../h/prom2.h"

/* shorthand */
#define excvmake(ptr, routine) {int routine(); *(ptr) = (long)routine;}

/* Exception Vector */

	/* we need a "mini" exception vector in ROM, just to
	 * get started.  This MUST be the first data in the ROM!
	 */
	excv(INITSPa);	/* excv(/INITSP) doesn't work here */
	excv(startmon);

/* Exception Links */
	exclink(inter10,I,I);
	exclink(inter14,Z,D);
	exclink(inter18,C,h);
	exclink(inter1C,T,V);
	exclink(inter20,P,r);
	exclink(inter28,U,0);
	exclink(inter2C,U,1);
	exclink(interUN,U,n);
	exclink(interL1,L,1);
	exclink(interL2,L,2);
	exclink(interL3,L,3);
	exclink(interL4,L,4);
	exclink(interL5,L,5);
	exclink(interL6,L,6);
	exclink(interTR,T,r);
label(.enter);
	branch(interrupt);

main()
{
	register char *p;
	register long *et;
	register GlobDes *gp;
	register long msize;
	register int EraseMem;
	register unsigned short config;

label(REFRsrv);
#ifdef	WATCHDOG
	TMRLoadCmd(TCLdCnt(TSCountSelect(TIMWatchDog)));
						/* reset watchdog timer */
#endif	WATCHDOG
	rpush(d0);
	TMRLoadCmd(TCClrOutput(TIMRefresh));	/* reset refresh timer */
	RAMREFR();

#if	RAMREFTIME == MS_2
	GlobPtr->RefrCnt += 2;
#else	/* RAMREFTIME = MS_1 */
	(GlobPtr->RefrCnt)++;
#endif

#ifdef	ABORTSWITCH
	/* Abort switch - use GlobPtr->debounce to detect change of state */
	if ((StatReg(A_chan)&NECsync) && (GlobPtr->Configuration.BreakEnable))
	    GlobPtr->debounce = true;
#endif	ABORTSWITCH
#ifdef	BREAKKEY
	/* break key - use GlobPtr->debounce to detect change of state */
	if ((StatReg(A_chan)&NECbreak)
		&& (GlobPtr->Configuration.BreakEnable))
	    GlobPtr->debounce = true;
#endif	BREAKKEY

#ifdef	BREAKKEY || ABORTSWITCH
	else if (GlobPtr->debounce) {	/* changed state! */
	    UART_RESXT(A_chan);		/* reset UART */
	    GlobPtr->debounce = false & RxHldReg(A_chan);
			/* reset state flag & empty UART buffer */
	    rpop(d0);
	    ExcCause(EXC_ABORT);
	    goto ENTERmon;
	}
#endif	BREAKKEY || ABORTSWITCH

	rpop(d0);
	UART_RESXT(A_chan);	/* note???: move this inside test? */
	endsrv;

LabelGbl(HardReset);		/* branch here for hard reset */
{
	EraseMem = 0;		/* remember not to erase memory */
	goto StartSequence;
}

label(startmon);
{
	EraseMem = 1;		/* this is a true reset, erase memory */

StartSequence:
/*	asm("reset");		/* issue a reset instruction */
	config = GETCONFIG;	/* read configuration register */

	/* FIRST! reset Timers, to avoid being interrupted */
	ResetTimer;
	TMRLoadCmd(TCClrOutput(TIMRefresh));	/* clear possible interrupt */

#ifdef	WATCHDOG
	/* set up initial, long-period watchdog timer */
	TCSetModeLoad(TIMWatchDog, TModWatchDog1, TFreqWatchDog1);
	TMRLoadCmd(TCLdArmCnt(TSCountSelect(TIMWatchDog)));
#endif	WATCHDOG

	/* Next, set up memory context and mapping */
	INITMAP;	/* this must be done to provide a stack
			 * for subsequent calls (e.g., mapmem())
			 * also, leaves us in context 0
			 */
	EXITBOOT;	/* Exit boot state (allows memory reads from RAM) */

	excvmake(EVEC_BUSERR,BEcatch);	/* ignore Bus Errors for now */

	msize = mapmem()<<11;		/* map the memory "right" */
	
	/*
	 * Write all of memory in order to initialize parity bits.
	 * This can't be a "clr", since that instruction reads(!)
	 * before writing.  The use of OPC_2UN1 means that random
	 * jumps will cause immediately "U1" traps.
	 */
	if (EraseMem)	 {	/* if we want to erase memory */
	    for (et = 0; et < (long *)msize; ) *et++ = OPC_2UN1;
	}

	gp = GlobPtr;

	if (EraseMem)		/* if we erased memory */
	    gp->MemorySize = msize;	/* record for future reference */
	gp->CurContext = 0;	/* ditto */

	*((unsigned short*)&gp->Configuration) = config;
				/* store configuration code */

#ifdef	FRAMEBUF
	/* determine presence of frame buffer -
	 * this uses BEcatch which clears memory location #0 on
	 * a bus error
	 * Afterwards, we AND this with config register bit.
	 */
	excvmake(EVEC_BUSERR,BEcatch);	/* catch Bus Errors */
	*(long*)0 = 1;
	*(long*)GXBase = 0;	/* touch frame buffer */
	gp->FrameBuffer = *(long*)0;
	if (gp->Configuration.UseFB == 0)	/* asserted HIGH */
	    gp->FrameBuffer = (- gp->FrameBuffer);
#endif	FRAMEBUF

	/* determine presence of second prom - reading from where
	 * the second prom should be returns 0x4E56 if it is there
	 * (" link a6,....") and randomness if it is not.
	 * let's hope that this randomness is not 0x4E56!
	 */
	gp->Prom2 = (*(short*)ADRSPC_PROM1 == 0x4E56);

	INITUART(gp->Configuration.ConsSpeed^3,gp->Configuration.HostSpeed^7);
			/* arguments are A and B channel speed codes */

	/* make all vectors "undefined" */
	for (et = EVEC_TRAPF; et ;)
	    excvmake(et--,interUN);

	et = EVEC_BUSERR;	/* skip first two vectors */

	excvmake(et++,BusError);
	excvmake(et++,AddrError);
	excvmake(et++,inter10);
	excvmake(et++,inter14);
	excvmake(et++,inter18);
	excvmake(et++,inter1C);
	excvmake(et++,inter20);
	excvmake(et++,TraceTrap);
	excvmake(et++,inter28);
	excvmake(et++,inter2C);

	et = EVEC_LEVEL1;
	excvmake(et++,interL1);
	excvmake(et++,interL2);
	excvmake(et++,interL3);
	excvmake(et++,interL4);
	excvmake(et++,interL5);
	excvmake(et++,interL6);
	excvmake(et++,REFRsrv);
	while (et <= EVEC_TRAPF)
		excvmake(et++,interTR);
	excvmake(EVEC_TRAP1,TRAPbrek);
	excvmake(EVEC_TRAPE,TRAPexit);
	excvmake(EVEC_TRAPF,TRAPemu);


	/*
	 * Create RAM Refresh subroutine -
	 *	OPC_2NOP is a long coding two nops, OPC_NOP_RTS is a long
	 *	coding an nop followed by an rts.  We fill all
	 *	but the last long with OPC_2NOP, then tack on an OPC_NOP_RTS.
	 */
	for (et = (long*)RAMREFR;
		et < (long*)((int)RAMREFR + ((RAMREFOPS-2)*sizeof(short)));
		 *(et++) = OPC_2NOP);
	*et = OPC_NOP_RTS;

	InitRefresh;	/* Start refresh timer */

#ifdef	WATCHDOG
	/* Refresh task is ready and waiting - change to short watchdog */
	TMRLoadCmd(TCDisarmCnt(TIMWatchDog));	/* disarm temporarily */
	TCSetModeLoad(TIMWatchDog, TModWatchDog2, TFreqWatchDog2);
	TMRLoadCmd(TCLdCnt(TSCountSelect(TIMWatchDog)));
	TMRLoadCmd(TCArmCnt(TSCountSelect(TIMWatchDog)));	/* re-arm */
#endif	WATCHDOG

	GlobBase = GlobPtr;		/* store ptr to globals for users */

	gp->debounce = false;
	gp->BootLoadDef = 0;

/*temp*/ setmode('A');			/* to allow error I/O */

#ifdef BREAKTRAP
	gp->BrkInPrg = 0;	/* no break in progress */
	gp->BreakAdx = 0;	/* initialize break address */
#endif BREAKTRAP

#ifdef	FRAMEBUF
	/* clear "console" screen */
	if (gp->FrameBuffer && gp->Prom2) {	/* ... if it is there */
		gp->ScreenMode = 0;	/* initialize screen state */
		writechar('\f');
	}
#endif	FRAMEBUF

/*temp*/ if (gp->Prom2) {	/* check prom2 version */
		if (MAKEVERSION(1,1) !=
			prm2_version()) {
			message("Prom version mismatch\n");
		}
	};	/* this semi-colon is CRUCIAL */

LabelGbl(SoftReset);	/* jump here to do a soft reset */
	rpush(#0.);	/* "PC" -- maybe should be something useful? */
	wpush(sr);	/* Fake Exception Stack */
	ExcCause(EXC_RESET);
};

		ENTERmon:
			saveregs;
			setINTLV(7);
			monitor();
			restregs;
			endsrv;


label(TRAPbrek);
	ExcCause(EXC_BREAK);
	goto ENTERmon;

unusedlabel1:
label(TRAPexit);
	ExcCause(EXC_EXIT);
	goto ENTERmon;

unusedlabel2:
label(interrupt);
	goto ENTERmon;

unusedlabel3:
label(TraceTrap);
	ExcCause(EXC_TRACE);
	goto ENTERmon;

unusedlabel4:
label(TRAPemu);
	ExcCause(EXC_EMT);
	goto ENTERmon;

unusedlabel5:
label(BusError)
	SAVEACCESS;		/* save Error Access Address */
	ExcCause(EXC_BUSERR);
	goto ENTERmon;

unusedlabel6:
label(AddrError)
	SAVEACCESS;		/* save Error Access Address */
	ExcCause(EXC_ADRERR);
	goto ENTERmon;

unusedlable7:
label(BEcatch)
	FLUSHACCESS;		/* remove access address from stack */
#ifdef	FRAMEBUF
	*(long*)0 = 0;		/* signal that BE occured */
#endif	FRAMEBUF
	endsrv;
}
