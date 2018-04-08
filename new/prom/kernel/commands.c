/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * commands.c
 *
 * Sun ROM monitor: command interpreter and execution
 *
 * original author: Luis Trabb Pardo
 * largely re-written by Jeffrey Mogul	April, 1981  Stanford University
 * restructured again by Jeffrey Mogul  February, 1982
 *
 * John Seamons	  April 19, 1982
 *
 * Modified to pass along file name from command line as argument to
 * booted program.  This allows more specialized boot programs loaded
 * by PROM to further parse the command line information.
 *
 * Philip Almquist	January 10, 1986
 *	- Add ARGSUSED before monitor routine
 *	- Add final else to openitem so we never fall thru and return
 *	  garbage
 */

#include "../h/sunmon.h"
#include "../h/asmdef.h"
#include "../h/globram.h"
#include "../h/sunuart.h"
#include "../h/confreg.h"
#include <statreg.h>
#include <pcmap.h>

/* Display/change one memory or map item
 * returns true iff another item coming.
 */
openitem(num,adrs,digs,dsize)
register int num;	/* either address or map number */
register word *adrs;	/* object address */
register int digs;	/* digits in address or map number */
int dsize;		/* 1 == byte, 2 == short, 4 == long */
{
	register char c;

	printhex(num,digs);
	if (dsize == 2)
	   queryval(": ",*adrs,4);
	else	/* dsize == 1 */
	   queryval(": ",*((char*)adrs),2);
	/* dsize == 4 not implemented */
	c = peekchar();
	if (ishex(c)>=0) {
		if (dsize == 2)
		    *adrs = getnum();
		else	/* dsize == 1 */
		    *((char *)adrs) = getnum();
		/* dsize == 4 not implemented */
		return(1);
		}
	else if (c!='\r')
		return(0);
	else
		return(1);
}


#define SREC_DATA	'2'	/* Data-carrying S-record */
#define	SREC_TRAILER	'8'	/* Trailer S-record */

/*
 * get an S-record.  Returns character used for handshaking.
 * pcadx is set to address part of S-record for non-data records.
 */
char getrecord(pcadx)
longword *pcadx;
{
	register GlobDes *gp = GlobPtr;
	register int bytecount;
	register int x;
	register longword adx;
	char rtype;
	int w;
	short checksum;
	char *savptr;
	int EchoState = gp->EchoOn;
	
	gp->EchoOn = 0;

	printhex(gp->recount,2);

	/*
	 * Format of 'S' record:
	 *	S<type><count><address><datum>...<datum><checksum>
	 *	  digit  byte   3bytes  byte      byte    byte
 	 */

	rtype = getone();
	if ( (rtype != SREC_DATA) && (rtype != SREC_TRAILER)) {
    	    gp->EchoOn = EchoState;
	    return('T');
	}

	bytecount = gethexbyte();
	/*
	 * checksum = 0; checksum += bytecount
	 */
	checksum = bytecount;	

	/* check to see that the bytecount agrees with line length */
	if ( (bytecount < 3) ||	(gp->linesize != (bytecount<<1)+4)) {
    	    gp->EchoOn = EchoState;
	    return('L');
	}

	/* accumulate address - 3 bytes */
	adx = 0;
	for (w = 0; w++ < 3; ) {
		 adx = (adx<<8)|(x = gethexbyte());
		 checksum += x;
		 bytecount--;
	}

	/* verify checksum */
	savptr = gp->lineptr;	/* save line ptr for rescanning */
	for (x = bytecount; x > 0 ; x--)
		checksum += gethexbyte();

	if ((++checksum)&0xFF) {
       	    gp->EchoOn = EchoState;
	    return('K');
        }

	/* checksum worked - now rescan input and read data */
	if (rtype == SREC_DATA) {
		gp->lineptr = savptr;
		while (--bytecount) {	/* predecrement to ignore checksum */
			*(char *)adx++ = (char)gethexbyte();
		}
	} else *pcadx = adx;

	(gp->recount)++;

	if (!(gp->recount&0x3))
	    writechar('.');  /* let user know that good records are coming */

    	gp->EchoOn = EchoState;
	return('Y');
}

/* Register names */
char _reg_nam[]= 
"D0\0D1\0D2\0D3\0D4\0D5\0D6\0D7\0A0\0A1\0A2\0A3\0A4\0A5\0A6\0SS\0US\0SR\0PC";

openreg(r,radx)
word r;
longword *radx;
	/* Display reg r, get new value (if any)
	If right answer:increment r and repeat until last reg
	*/
{
	register char c;

	r+=(getnum()&0xF);	/* only use last hex digit */
	radx+=r;
	for(;r<=_reg_MX;r++,radx++)  {
		message(&_reg_nam[r+r+r]);	
		queryval(": ",*radx,8);
		c = peekchar();
		if (ishex(c) >= 0) {
		    if (r != _reg_ss)	/* You toucha SS, I breaka you face */
		    	*radx = getnum();
		}
		else if (c != '\r') break;
	}
}

doload()
	/* Sends the rest of the line to channel B, and
	enters load mode.  To avoid a "shouting match" with
	the host, we don't echo until we see a CSTLD.
	*/
{
	register GlobDes *gp = GlobPtr;
	register int EchoState = gp->EchoOn;

	setmode('B');
	gp->EchoOn = 0;
	gp->linbuf[0] = (char)'\r';
	message(gp->linbuf);
	gp->recount = 0;
	while (getchar() != CSTLD);
	gp->EchoOn = EchoState;
}

#ifdef BREAKTRAP
dobreak()
	/* Sets up a break point */
{
	register GlobDes *gp = GlobPtr;
	register word *bp= gp->BreakAdx;
	register word *bpa;

	if (bp && (!gp->BrkInPrg) && (*bp != OPC_TRAP1))
	    /* oops? got bashed */
	    bp = 0;

	queryval("Break: ",(int)bp,6);

	if (peekchar()!='\r') then {
		gp->BrkInPrg = 0;	/* clear any pending BPT */
		bpa= (word*)getnum();
		if (bp) then *bp = gp->BreakVal;
		gp->BreakVal= *bpa;
		if (bpa) then *bpa= OPC_TRAP1;
		gp->BreakAdx= bpa;
	};
}

/*
 * TakeBreak - handle breakpoint trap
 */
TakeBreak(pcp)
register long *pcp;	/* pointer to user's PC */
{
	register GlobDes *gp = GlobPtr;

	message("\nBreak at ");
	*pcp -= 2;		/* back up to broken word */
	printhex(*pcp,6);
	putchar('\n');
	gp->BrkInPrg++;		/* remember that we are breaking */
	*(gp->BreakAdx) = gp->BreakVal;	/* restore old instruction */
}
#endif BREAKTRAP

/*
 * Sets terminal operation mode: A, B, or T
 */
setmode(c)
register char c;
{
	register GlobDes *gp = GlobPtr;
	register char former = gp->ConChan;

	gp->EchoOn = 1;
	if (c=='A') {
		gp->ConChan = A_chan;
		/* if already on channel A, don't putchar('\n') */
		if (former != A_chan)
		    putchar('\n');
	} else if (c=='B') then {
		gp->ConChan = B_chan;
#ifdef	FRAMEBUF
	} else if (c=='S') then {
		gp->FrameBuffer = -(gp->FrameBuffer);
#endif	FRAMEBUF
	} else if (c=='T') then {
		transparent();
		putchar('\n');
	} else message("Unknown mode\n");
}

/*ARGSUSED*/
monitor(	r_d0, r_d1, r_d2, r_d3, r_d4, r_d5, r_d6, r_d7,
		r_a0, r_a1, r_a2, r_a3, r_a4, r_a5, r_a6,
		r_ss, r_us, r_sr, r_pc)
long	r_d0, r_d1, r_d2, r_d3, r_d4, r_d5, r_d6, r_d7,
	r_a0, r_a1, r_a2, r_a3, r_a4, r_a5, r_a6,
	r_ss, r_us, r_sr, r_pc;

	/* This mini-monitor does only a few things:
		a <octal dig>	open A regs
		b		set breakpoint
		c [<address>]	continue [at <address>]
		d <octal dig>	open D regs
		e <hex number> 	open address (as word)
		f <filename>	bootload file but don't start
		g [<address>]	start (call) [at <address>]
		h		help
		i [A|B|T|S]	terminal operation mode A,B or T;
					I S toggles Frame Buffer usage
		k		reset monitor stack [k1 hard reset]
		l <host-cmd>	get in LOAD mode
		m <hex number>	open segment map
		n <filename>	bootload file and start it
		o <hex number>	open address (as byte)
		p <hex number>	open page map
		r	 	open PC,SR,SS,US
		s		load S-record
		w		set/show boot device
		x <char>	set transparent-mode escape to <char>
	*/
{
	register GlobDes *gp = GlobPtr;
	register char c;
	register int goadx;
	int	confbootfile;
	long cause;	/* must NOT be in register */
	int (*calladx)();	/* ditto */

	cause = r_sr>>16;

	if (cause != EXC_EMT)	/* don't reset UART for EMT! */
	    setmode('A');		/* revert to normal UART operation */
		/* probably we should save mode and restore it later? */

	switch(cause) {	/* what caused entry into monitor? */

	case EXC_RESET:
		message(SunIdent);
		printhex(gp->MemorySize,5);
		message(" bytes of memory\n");
		r_us = gp->MemorySize;	/* reset User Stack pointer */
		gp->EndTransp = CENDTRANSP;	/* default escape */
		confbootfile = gp->Configuration.Bootfile;
		if (confbootfile == BOOT_SELF_TEST) {
			/* just did self-test */
		}
		else if (confbootfile != BOOT_TO_MON) {
			autoboot(confbootfile);
		}
		/* else just enter monitor */
		break;

	case EXC_ABORT:
		message("\nAbort");
		goto PCPrint;

	case EXC_BREAK:
#ifdef	BREAKTRAP
		TakeBreak(&r_pc);
		break;
#else
		message("BREAK\n");
		break;
#endif	BREAKTRAP

	case EXC_EXIT:
		break;

	case EXC_TRACE:
#ifdef	BREAKTRAP
		if (gp->BrkInPrg) {
		    /* this is single step past	broken instruction */
		    r_sr &= ~SR_TRACE;
		    gp->BrkInPrg = 0;
		    if ((*(gp->BreakAdx)) == gp->BreakVal) {
		    	/* good - not bashed since we set it */
			*(gp->BreakAdx) = OPC_TRAP1;	/* reset break */
		    }
		    return;	/* in either case, return to user code */
		}
#endif	BREAKTRAP
		message("\nTrace trap");
		goto PCPrint;

	case EXC_EMT:	/* emulator trap */
#ifdef EMULATE
		r_d0 = Emulate(&r_pc,r_sr,r_us);
#else
		r_d0 = -1;	/* return -1 if no emulator */
#endif EMULATE
		return;

	case EXC_BUSERR:
		message(bedecode(&(gp->BE), r_sr));
		goto AccAddPrint;

	case EXC_ADRERR:
		message("Address");
AccAddPrint:	message(" Error, addr: ");
		printhex(*(long*)(&(gp->BE.Data.AccAddrLo)),8);
PCPrint:	message(" at ");
		printhex(r_pc,6);
		putchar('\n');
		break;
		
	default: cause = r_sr&0xFFFF0000;
		message("\nException: ");
		message((char *)&cause);    /* high word of r_sr is message */
		putchar('\n');
		break;
	}

	r_sr &= 0xFFFF;		/* clear extraneous high word */

	/*
	 * Reboot on error unless we got here by a reset with no
	 * autoboot specified.
	 * Note: this depends on EXC_RESET being <> any Exception message.
	 */
	if ((gp->Configuration.ErrorRestart == 0) &&
	      ( (cause != EXC_RESET) ||
		((confbootfile != BOOT_TO_MON) &&
		 (confbootfile != BOOT_SELF_TEST)
		)
	      )
	    ) {
	    extern int sp;
	    sp = INITSP;	/* restore initial SSP */
	    branch(HardReset);
	}

	loop {
		message(">");
		getline();

		while(c= getone()) 
		    if (c >= '0') break;

		while (peekchar() == ' ')
		    getone();	/* remove any spaces before argument */

		if (c == '\0') continue;

		switch(c&UPCASE) {	/* slight hack, force upper case */

		case 'A':	/* open A register */
			openreg(_reg_a0,&r_d0);
			break;
#ifdef BREAKTRAP
		case 'B':	/* set breakpoint */
			dobreak();
			break;
#endif BREAKTRAP
		case 'C':	/* Continue */
			if(goadx = getnum())
			    then r_pc = goadx;
#ifdef BREAKTRAP
			else if (gp->BrkInPrg) {
			    /* breakpoint trap taken */
			    r_sr |= SR_TRACE;	/* set trace trap */
			}
#endif BREAKTRAP
			return;
		case 'D':	/* open D register */
			openreg(_reg_d0,&r_d0);
			break;
		case 'E':	/* open memory (short word) */
			for(goadx = getnum()&0x0FFFFFE; ;goadx+=2)
			    if (!openitem(goadx,(word *)goadx,6,2)) break;
			break;
		case 'F':	/* fetch */
			gp->linbuf[gp->linesize] = 0;
			r_pc = bootload(gp->lineptr);
			break;			
		case 'G':	/* Go */
			if (!(*((long*)&calladx) = getnum()))
				*((long*)&calladx) = r_pc;
			calladx();
			break;
		case 'H':	/* Help */
			givehelp();
			break;
		case 'I':	/* change mode */
			setmode(getone()&UPCASE);	/* force upper case */
			break;
		case 'K':	/* reset stack */
				/* if non-zero argument given, hard reset */
			{extern int sp;
			sp = INITSP;
			if (getnum()) {
			    putchar('\n');
			    branch(HardReset);
			}
			else
			    {branch(SoftReset);}
			}
			break;			
		case 'L':	/* enter Load mode */
			doload();
			break;
		case 'M':	/* open segment map entry */
			for (goadx = getnum()&(CONTEXTSIZE-1);
				goadx < CONTEXTSIZE ;goadx++)  {
			    message("SegMap ");
			    if (!openitem(goadx,(word *)SEGMAPADR(goadx),4,2))
				break;
			}
			break;
		case 'N':	/* boot a file */
			gp->linbuf[gp->linesize] = 0;
			*((long*)&calladx) = bootload(gp->lineptr);
			if (calladx) calladx(gp->lineptr);
			break;
		case 'O':	/* open memory (byte) */
			for(goadx = getnum()&0x0FFFFFF; ;goadx++)
			    if (!openitem(goadx,(word *)goadx,6,1)) break;
			break;
		case 'P':	/* set page map */
			for (goadx = getnum()&(PAGEMAPSIZE-1);
				goadx < PAGEMAPSIZE ;goadx++)  {
			    message("PageMap ");
			    if (!openitem(goadx,(word*)PAGEMAPADR(goadx),4,2))
				break;
			}
			break;
		case 'R':	/* open registers */
			openreg(_reg_ss,&r_d0);
			break;
		case 'S':	/* accept S record */
			putchar(getrecord(&r_pc));
			break;
		case 'W':
			boottype(getnum());
			break;
		case 'X':	/* set transparent-mode escape */
			gp->EndTransp = getone();
			break;
		default:
			message("What?\n");
			break;
		}	/* end of switch */

	}
}
