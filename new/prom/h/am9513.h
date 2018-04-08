/*
 * am9513.h
 *
 * AMD Am9513 Timer definitions
 *
 * First version by Luis Trabb-Pardo
 *
 * restructured by Jeffrey Mogul	24 September 1981
 */

/* Timer addresses on the SUN system */

#define SunTimerCom	*(unsigned short*)0x800002	/* for Commands */
#define SunTimerReg	*(unsigned short*)0x800000	/* for Registers */

/* Command codes */

/*
 * all commands should have bits 15:8 set
 */
#define	TCCmd(c)	(0xFF00|(c))

/*
 * Commands that refer to all counters bit-encode the counters
 * so that bit 0 corresponds to counter 1, bit 4 to counter 5.
 *
 * Commands that refer to one counter use 1-based index (1,2,3,4,5).
 */

#define TCLdDataPtr(g)	TCCmd(g)	/* Load Data Pointer Register */
#define TCArmCnt(c)	TCCmd(c|0x20)	/* Arm counters (bit x = ctr x+1) */
#define TCLdCnt(c)	TCCmd(c|0x40)	/* Load source into counters */
#define TCLdArmCnt(c)	TCCmd(c|0x60)	/* Load and Arm counters */
#define	TCDisSaveCnt(c)	TCCmd(c|0x80)	/* Disarm and Save Counters */
#define	TCSaveCnt(c)	TCCmd(c|0xA0)	/* Save counters in Hold Register */
#define TCDisarmCnt(c)	TCCmd(c|0xC0)	/* Disarm Counters */
#define	TCSetOutput(n)	TCCmd(n|0xE8)	/* Set Output of counter #n */
#define	TCClrOutput(n)	TCCmd(n|0xE0)	/* Clear Output of counter #n */
#define	TCStep(n)	TCCmd(n|0xF0)	/* Step counter #n */
#define	TCDisDPSeq	TCCmd(0xE8)	/* Disable DPR sequencing */
#define	TCEnaDPSeq	TCCmd(0xE0)	/* Enable DPR sequencing */
#define TCGateOff	TCCmd(0xEE)	/* Gate Off FOUT signal */
#define TCGateOn	TCCmd(0xE6)	/* Gate On FOUT signal */
#define TC16BitBus	TCCmd(0xEF)	/* Enter 16bit bus mode */
#define TC8BitBus	TCCmd(0xE7)	/* Enter 8bit bus mode */
#define TCReset		TCCmd(0xFF)	/* Master Reset */

/* Register Selection (Element Field) */
#define TSModeRg	0		/* Mode Register */
#define TSLoadRg	0x08		/* Load Register */
#define TSHoldRg	0x10		/* Hold Register */
#define TSMasterMode	0x17		/* Master Mode Register */

/* Counter Selection */

#define	TSCountSelect(c)	(2<<((c)-2))
#define TSSelectAll	0x1F

/* Master Mode Register settings (partial list) */

#define TMMBus16	0x2000		/* Data bus is 16 wide */
#define TMMFoutOff	0x1000		/* Fout gated off */
#define TMMFoutDiv2	0x0200		/* Fout is F1/2 */

/* Counter Mode Register settings (Partial List) */

#define	TCMRise		0x0100	/* Count on Rising edge */
#define	TCMFall		0	/* Count on Falling edge */

#define	TCMNoGa		0	/* No gating */

#define	TCMF1		0x0B00	/* Select freq F1 (=input freq) */
#define	TCMDivBy1	TCMF1
#define	TCMF2		0x0C00	/* Select freq F2 (=input freq/16) */
#define	TCMDivBy16	TCMF2
#define	TCMDivBy10	TCMF2	/* or /10 in decimal mode */
#define	TCMF3		0x0D00	/* Select freq F3 (=input freq/256) */
#define TCMDivBy256	TCMF3
#define	TCMDivBy100	TCMF3	/* or /100 in decimal mode */
#define	TCMF4		0x0E00	/* Select freq F4 (=input freq/4096) */
#define	TCMDivBy4k	TCMF4
#define	TCMDivBy1k	TCMF4	/* or /1000 in decimal mode */
#define	TCMF5		0x0F00	/* Select freq F5 (=input freq/65536) */
#define	TCMDivBy64k	TCMF5
#define	TCMDivBy10k	TCMF5	/* or /10000 in decimal mode */

#define TCMSource1	0x0100	/* Select input from Source 1 pin */
#define TCMGate1	0x0600	/* Select input from Gate 1 pin */

#define	TCMDiSpG	0	/* Disable Special Gate */
#define	TCMEnaSpG	0x0080	/* Enable Special Gate */

#define	TCMRldLd	0	/* Reload from Load Register */
#define	TCMRldLdHld	0x0040	/* Reload from Load or Hold */

#define	TCMCntRep	0x0020	/* Count Repeatedly */
#define	TCMCntRp	0x0020	/* Count Repeatedly */	/* DGN extra*/
#define	TCMCntOnce	0	/* Count once */

#define	TCMBinCnt	0	/* Count in binary */
#define	TCMBinCt	0	/* Count in binary */	/* DGN extra*/
#define	TCMDecCnt	0x0010	/* Count in decimal */

#define	TCMDownCnt	0	/* Count downwards */
#define	TCMDwnCt	0	/* Count downwards */	/* DGN extra*/
#define	TCMUpCnt	0x0008	/* Count upwards */

#define TCMLowTC    	0x0005	/* Active Low Terminal Count Pulse */
#define TCMOutTS	0x0004  /* Output inactive Tri-state */
#define	TCMTCTog	0x0002	/* Toggle output at TC */
#define TCMHighTC   	0x0001	/* Active High Terminal Count Pulse */
#define	TCMOutLo	0	/* Inactive output, low */

/*
 ****************************************************************
 * Timer control macros
 ****************************************************************
 */

/*
 * Load Command Register
 */
#define TMRLoadCmd(x)	SunTimerCom = (x)

/*
 * Load Data Register
 */
#define LoadReg(x)	SunTimerReg = (x)

/*
 * Read Data Register
 */
#define ReadReg(x)	x = SunTimerReg

/*
 * Reset all timers
 */
#define	ResetTimer \
	TMRLoadCmd(TCReset);		/* Master Reset */\
	TMRLoadCmd(TCLdCnt(TSSelectAll));	/* Reset all counters */\
	TMRLoadCmd(TC16BitBus);		/* Enter 16-bit Bus mode */

/*
 * Set Mode and Load registers for one timer
 */
#define TCSetModeLoad(n,mode,divide)\
	TMRLoadCmd(TCLdDataPtr(n));\
	LoadReg(mode);\
	LoadReg(divide);

/*  Sun processor Channel Assignment is */

#define	TIMRefresh	3	/* for refresh task */
#define TIMAClk		4	/* Ch.A clock */
#define TIMBClk		5	/* Ch.B clock */
#define TIMInter	2	/* Interrupt Clock, Level 6 */
#define TIMWatchDog	1	/* Watchdog Timer Clock */


/* Register Transfer */

#define TRLoad(val)	SunTimerReg= (unsigned short)(val)


/* Set Mode and Load registers for one channel */
#define TCSetGrp(group,mode,divid)\
	TMRLoadCmd(TCCmd(group));\
	TRLoad(mode);\
	TRLoad(divid)

