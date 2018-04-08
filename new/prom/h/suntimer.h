/*
 * suntimer.h
 *
 * Sun Rom Monitor Timer definitions
 *
 * First version by Luis Trabb-Pardo
 *
 * restructured by Jeffrey Mogul	17 December 1981
 */

#include "../h/am9513.h"

/*
 * Sun Processor board modes and frequencies
 */
/* Input frequency is 2.46MHz for the Forward Technology board  FTI10 */
/* Input frequency is 4.92Mhz for the SMI board SMI10 */
/* Input frequency is 4MHz  SMI8, CADLINC */

/* Uart Timing:  the timer functions in "MODE D"... */
#define	TModUART (unsigned short) \
	(TCMFall|TCMNoGa|TCMDivBy1|TCMDiSpG|TCMRldLd|\
		TCMCntRep|TCMBinCnt|TCMDownCnt|TCMTCTog)
#ifdef FTI10
#define TFreqUART 8	/* with a square wave of 16 * 9615.4Hz */
#endif FTI10

#ifdef SMI10
#define TFreqUART 16	/* with a square wave of 16 * 9615.4Hz */
#endif SMI10

#ifdef SMI8
#define TFreqUART 13	/* with a square wave of 16 * 9615.4Hz */
#endif SMI8

#ifdef CADLINC
#define TFreqUART 13	/* with a square wave of 16 * 9615.4Hz */
#endif CADLINC

/* Refresh Timing: also "MODE D" */
#ifdef WATCHDOG
#define	TModRefr  (unsigned short) \
	(TCMFall|TCMNoGa|TCMGate1|TCMDiSpG|TCMRldLd|\
		TCMCntRep|TCMBinCnt|TCMDownCnt|TCMTCTog)
#else WATCHDOG
#define	TModRefr TModUART
#endif WATCHDOG

#ifdef FTI10
#define TFreqRefr 4915	/* frequency: 500Hz */
#endif FTI10

#ifdef SMI10
#define TFreqRefr 4915	/* frequency: 500Hz */
#endif SMI10

#ifdef SMI8
#define TFreqRefr 8000	/* frequency: 500Hz */
#endif SMI8

#ifdef CADLINC
#define TFreqRefr 8000	/* frequency: 500Hz */
#endif CADLINC

/*
 * Init Refresh timer
 */
#define	InitRefresh\
	TCSetModeLoad(TIMRefresh, TModRefr, TFreqRefr);\
	TMRLoadCmd(TCLdArmCnt(TSCountSelect(TIMRefresh)));


/*
 * Watchdog timer -- two phases:
 *	Phase 1: from beginning of Boot to initialization of
 *		refresh cycle, one long interval.
 *	Phase 2: once refresh is running, watchdog should
 *		be just slightly slower than refresh
 */

/* Also "MODE D", but with long (ca. 1/60 second) period */
/*#define	TModWatchDog1 \
	(TCMFall|TCMNoGa|TCMDivBy64k|TCMDiSpG|TCMRldLd|\
		TCMCntRep|TCMBinCnt|TCMDownCnt|TCMTCTog) */

/* #ifdef FTI10
 #define	TFreqWatchDog1	369
 #endif FTI10

 #ifdef SMI10
 #define	TFreqWatchDog1	738
 #endif SMI10

 #ifdef SMI8
 #define	TFreqWatchDog1	600
 #endif SMI8

 #ifdef CADLINC
 #define	TFreqWatchDog1	600
 #endif CADLINC
*/

#define	TModWatchDog2	  (unsigned short) \
	(TCMFall|TCMNoGa|TCMGate1|TCMDiSpG|TCMRldLd|\
		TCMCntRep|TCMBinCnt|TCMDownCnt|TCMHighTC)

/* 50% longer than Refresh timer. minus 1% slop */
#define	TFreqWatchDog2	( ((TFreqRefr*3)/2) + (TFreqRefr/25) )
