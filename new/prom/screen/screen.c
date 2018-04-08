/* Copyright (C) 1982 by Jeffrey Mogul */

/*
 * screen.c
 *
 * hardware-independent part of cheap screen package.
 *
 * exports:
 *	fwritechar(c)
 *
 * imports:
 *	initscreen()
 *	putat(x,y,c)
 *	putcursor(x,y,set)
 *	clearEOL(x,y)
 *
 * A termcap entry for use with Emacs/Vi:
 * # Sun ROM-resident emulator
 *
 * sr|sunrom|SunRom|Sun Rom:\
 * 	am:ns:pt:bs:co#80:li#24:ce=\EK:up=\EA:cl=\f:nd=\EC:\
 * 	nl=\ED:cm=\EY%+ %+ :
 * 
 */

#include "../h/screen.h"
#ifndef	STANDALONE
#include "../h/globram.h"
#include "../h/sunmon.h"
#endif	STANDALONE

#ifdef	STANDALONE
typedef struct {
	int Xcurr;
	int Ycurr;
	int ScreenMode;
} GlobDes;

GlobDes Globals;
GlobDes *GlobPtr = &Globals;

#define	NOPARITY	0x7F

#endif	STANDALONE
	
fwritechar(c)
register char c;
{
	register GlobDes *gp = GlobPtr;
	register int i;
	register int clear = 0;
	register int tx;
	register int ty;

#ifdef	STANDALONE
#ifdef	DEBUG
printf("mode %o c %o x %d y %d\n",gp->ScreenMode,c,gp->Xcurr,gp->Ycurr);
#endif
#endif
	/* cache gp->Xcurr and gp->Ycurr in registers to reduce code space */
	tx = gp->Xcurr;
	ty = gp->Ycurr;

	putcursor(tx,ty,0);	/* clear cursor */
	
	switch (gp->ScreenMode) {	/* what state are we in now? */
	
	    case SM_ESCSEEN:
		switch (c) {		/* what command is comming? */
		
		    case 'A':		/* up line */
		    	if (--ty < 0) ty = 0;
			break;
			
		    case 'C':		/* non-destructive space */
		    	tx++;
			break;

		    case 'D':		/* non-destructive newline */
		    	ty++;
			break;

		    case 'K':		/* clear to EOL */
		    	clearEOL(tx,ty);
			break;
			
		    case 'Y':		/* cursor motion */
		    	gp->ScreenMode = SM_NEEDY;
			return;		/* no more to do now */
		}
		break;

	    case SM_NEEDY:
	    	gp->Ycurr = (int)(c - ' ');
		gp->ScreenMode = SM_NEEDX;
		return;

	    case SM_NEEDX:
	    	tx = (int)(c - ' ');
		break;

	    case SM_NORMAL:	/* put a character */

		c &= NOPARITY;
	
		switch(c) {
	
		case 033:	/* escape! */
			gp->ScreenMode = SM_ESCSEEN;
			return;

		case '\n':	/* linefeed, not newline */
			ty++;
			clear++;
			break;
		case '\r':	/* carriage return */
			tx = 0;
			break;
		case '\b':	/* backspace */
			if (tx > 0) tx--;
			break;
		case '\t':	/* non-destructive tab */
			tx = (tx+8)&(~7);
			break;
		case '\f':	/* form-feed/clear screen */
			initscreen();
			tx = 0;
			ty = 0;
			break;
		case 0177:	/* <rubout> doesn't print */
			break;
		default:	/* printing character */
			if (putat(tx,ty,c))
			    tx++;	/* bump x-pos iff printing char */
		}
		break;
	}

	if (tx >= WIDTH) {
		tx = 0;
		ty++;
		clear++;
	}
	if (ty > LINES)
		ty = 0;

	if (clear)
		clearEOL(tx,ty);	/* clear rest of line */

	putcursor(tx,ty,1);

	gp->Xcurr = tx;	/* write cache to real storage */
	gp->Ycurr = ty;
	gp->ScreenMode = SM_NORMAL;
}
