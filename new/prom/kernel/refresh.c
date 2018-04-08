/*	refresh.c	2.3	82/10/27	*/
/*
 * refresh.c
 *
 * refresh routine for keyboard support in Sun Monitor
 *
 * This code runs 500 times a second, refreshing memory and doing other
 * assorted useful things that nobody else has the time or patience to do.
 *
 *	Keyboard support installed -- JCGilmore		21 May 1982
 *	Clear input buf on EEOF-A (abort)		16 June 1982
 */

#include "../h/sunmon.h"
#include "../h/suntimer.h"
#include "../h/sunuart.h"
#include "../h/asmdef.h"
#include "../h/globram.h"
#include "../h/keyboard.h"
#include "../h/asyncbuf.h"
#include "../h/m68vectors.h"

extern char *Control[];		/* For each InSource, its addresses in Uart */
extern char *DataReg[];
asm(".globl _AbortEn");		/* Declare abort entry pt up here, or optimizer
				    barfs. */

#define keybuf	gp->Keybuf
#define key	gp->Key
#define prevkey gp->PrevKey
#define keystate gp->KeyState
#define keybid	gp->KeybId

/*
 *	Set up the refresh routine, assuming nothing.
 *
 *	Well, assume master mode of clock chip is OK.
 *	(Maybe not a good idea, but...)
 */

FixRefresh()
{
	register GlobDes *gp;	/* For refresh code, not FixRefresh */
/**NOTE ASSUMPTION THAT GP IS IN A5!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
	register long *et;

	/*
	 * Init Refresh timer
	 *
	 * We load the timer first because it might already be running.
	 * We want to reset it so we won't get watchdogged while we are
	 * setting up the refresh routine.  Since we are running at interrupt
	 * level 7, the refresh interrupt is the only thing that could delay
	 * our setting up the refresh routine, so it will be ready by the
	 * time the interrupt occurs (2ms from now).
	 *
 	 * Actually, you could get some strange results from such things as
	 * mapping page 0 to multibus or nonexistent memory.  However,
	 * in sufficiently strange cases, we'd watchdog, which notices that
	 * page 0 is mismapped and fixes things.
	 */
	TMRLoadCmd(TCGateOff);
	TMRLoadCmd(TCClrOutput(TIMRefresh));
	TCSetModeLoad(TIMRefresh, TModRefr, TFreqRefr);
	/* Refresh task is ready - load and arm watchdog timer */
	/* Due to peculiar timing glitch, we must gate the watchdog clock
	   off while doing the load "loadreg->counter" command.  The deal
	   is that the timer can't tick within about 70 ns of when
	   we do the write.  Since that clock is a 200ns cycle, it's
	   kinda frequent that we'd screw up unless we do this.  This is
	   thanks to the wonderful folks at AMD.  Thanks fellas...you blew it.

	   All this was described by Andy Bechtohlshiem and implemented
	   by John Gilmore. */
	TMRLoadCmd(TCClrOutput(TIMRefresh));
	TCSetModeLoad(TIMWatchDog, TModWatchDog, TFreqWatchDog);
	TMRLoadCmd(TCLdArmCnt(  TSCountSelect(TIMWatchDog)
			       +TSCountSelect(TIMRefresh)  ) ); 
	TMRLoadCmd(TCGateOn);

	excvmake(EVEC_LEVEL7,KeyFrsh); /* Refresh/keyboard scanning routine */
	excvmake(EVEC_BUSERR,BusErr);  /* Bus error (watchdog recovery) rout. */

	GlobPtr->WhyReset = EXC_DOG;	/* This is used as the "reason" for
					   the next invocation of the "soft
					   reset" code in Sunmon.c.  It means
					   we took a watchdog reset. */

	/*
	 * Create RAM Refresh subroutine -
	 *	OPC_2NOP is a long coding two nops, OPC_NOP_RTS is a long
	 *	coding an nop followed by an rts.  We fill all
	 *	but the last long with OPC_2NOP, then tack on an OPC_NOP_RTS.
	 *
	 * Note that this has the effect of refreshing the memory, since we
	 * 	are writing to all combinations of low-order 7 bits...
	 */
	for (et = (long*)RAMREFR;
		et < (long*)((int)RAMREFR + ((RAMREFOPS-2)*sizeof(short)));
		 *(et++) = OPC_2NOP);
	*et = OPC_NOP_RTS;

	return;			/* End of initialization code. */


unulab1:			/* Prevents BS from compiler */

/*
 *	The refresh routine
 *
 *	Note that this refreshes memory as well as servicing the keyboard.
 *	If you trash it, the machine goes away.  Bewary!
 */
LabelGbl(KeyFrsh);
	asm("moveml #0x2303,RefrTmp");	/* Save d0,d1,a0,a1,a5 */
/**NOTE ASSUMPTION THAT GP IS IN A5!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

	TMRLoadCmd(TCClrOutput(TIMRefresh));	/* reset refresh timer */
	RAMREFR();

	gp = GlobPtr;			/* Initialize globals pointer */

	gp->RefrCnt += 2;		/* count 2 more milliseconds */

#ifdef KEYBOARD
	/* Check the parallel keyboard to see how it's doing ... */
	GETKEY (key);
	if (key != prevkey) {
		prevkey = key;
		switch (keystate) {

		NormalState:
			keystate = NORMAL;
		case NORMAL:
			if (key == ABORTKEY1) {
				keystate = ABORT1; break;
#ifdef KBDID
			} else if (key == IDLEKEY) {
				keystate = IDLE1;
#endif KBDID
			}

			bput (keybuf, key);
			break;

#ifdef KBDID
		case IDLE1:
			if (keybid != key) {
				keybid = key;
				newkeyboard();
			}
			keystate = IDLE2;
			break;

		case IDLE2:
			if (key == IDLEKEY) keystate = IDLE1;
			else goto NormalState;
			break;
#endif KBDID

		case ABORT1:
			if (key == ABORTKEY2) {
				keystate = NORMAL;
				bputclr(keybuf);  /* Clear typeahead */
				AbortFix(); /* Let our other half know that
					     these keys really did go down */
				/* Since we know the keyboard is here, and
				 * the gal is typing on it now, we oughta
				 * select it as input ('cuz InSource might
				 * be screwed up).  We
				 * make a somewhat riskier assumption that the
				 * keyboard is near the screen and she'll want
				 * her output on it.  (If there's no screen,
				 * we just punt output.  What is she doing
				 * with a keyboard and no screen anyway?)
				 */
				gp->InSource = InKEYB;  /* Take keyb inp */
				if (gp->FrameBuffer != 0)
					gp->OutSink = OutSCREEN;
				/* Break out to the monitor */
				goto TheMonitor;
			} else {
				bput (keybuf, ABORTKEY1);
				goto NormalState;
			}

		case AWAITIDLE:
			if (key == IDLEKEY) goto NormalState;
			break;

		}
	}
#endif KEYBOARD

#ifdef	BREAKKEY
	/* break key - use gp->debounce to detect change of state */
	if (gp->InSource == IoUARTA || gp->InSource == IoUARTB) {
		/* First read it, incase reg 0 is not selected */
		if (*Control[gp->InSource]) ; /* Read harmlessly */
		/* If we'd written the RESXT to some random register, various
		   havoc could prevail.  This is because the chip designeers 
		   wanted to leave 3 pins off the interface which would
		   have selected the register-of-interest in a more reasonable
		   way. */
		*Control[gp->InSource] = NECrexst;
		if (*Control[gp->InSource] & NECbreak) {
			gp->debounce = 1;
			goto ByeBye;
		}
	}
	if (gp->debounce) {	/* changed state! */
	    gp->debounce = 0 & *DataReg[gp->InSource];
			/* reset state flag & empty UART buffer */
TheMonitor:
	asm("moveml RefrTmp,#0x2303");	/* Restore d0,d1,a0,a1,a5 */
#ifdef	WATCHDOG
	/*
	   Reset the watchdog timer.  We do this last in case this routine
	   has bugs...if we never reach here, the timer won't be reset.

	   The timer chip glitches if you rewrite a counter within about
	   70 ns of the counter's input clock edge.  We therefore have to
	   gate off the source for this counter (Gate 1, tied to FOUT)
	   before reloading it, then gate it back on after the reload.
	   Thanks AMD.
	 */
	TMRLoadCmd(TCGateOff);		/* Gate off clock signal for watchdog */
	TMRLoadCmd(TCLdCnt(TSCountSelect(TIMWatchDog)
			 +(TSCountSelect(TIMRefresh))   )); /* retrigger it */
	TMRLoadCmd(TCGateOn);		/* Gate ON clock signal for watchdog */
#endif	WATCHDOG
label(AbortEn);	/* Let other alternative refresh routines call here */
	    ExcCause(EXC_ABORT);
	    asm ("jmp	_ToMon");
	}

ByeBye:
	UART_RESXT(A_chan);	/* Show current status in UART stat bits */

#endif	BREAKKEY

	asm("moveml RefrTmp,#0x2303");	/* Restore d0,d1,a0,a1,a5 */

#ifdef	WATCHDOG
	/*
	   Reset the watchdog timer.  We do this last in case this routine
	   has bugs...if we never reach here, the timer won't be reset.

	   The timer chip glitches if you rewrite a counter within about
	   70 ns of the counter's input clock edge.  We therefore have to
	   gate off the source for this counter (Gate 1, tied to FOUT)
	   before reloading it, then gate it back on after the reload.
	   Thanks AMD.
	 */
	TMRLoadCmd(TCGateOff);		/* Gate off clock signal for watchdog */
	TMRLoadCmd(TCLdCnt(TSCountSelect(TIMWatchDog)
			 +(TSCountSelect(TIMRefresh)) )); /* retrigger it */
	TMRLoadCmd(TCGateOn);		/* Gate ON clock signal for watchdog */
#endif	WATCHDOG

	endsrv;

}
