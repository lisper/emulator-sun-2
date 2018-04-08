/* Copyright (C) 1982 by Jeffrey Mogul  */

/*
 * getline.c
 *
 * common subroutines for Sun ROM monitor: read input line
 *
 * original author: Luis Trabb Pardo
 * largely re-written by Jeffrey Mogul	April, 1981  Stanford University
 * restructured again by Jeffrey Mogul  February, 1982
 */

#include "../h/sunmon.h"
#include "../h/asmdef.h"
#include "../h/globram.h"


/*
 * read the next input line into a global buffer
 */
getline()
{
	register char *p = GlobPtr->linbuf;
	register int linelength = 0;
	register int erase;	/* number of characters to erase */

	GlobPtr->lineptr = p;	/* reset place-in-line pointer */

	while ( (*p = getchar()) != '\r') {
		erase = 0;
		if (*p == CERASE1) {
		    putchar(' ');	/* un-echo the \b */
		    erase = 1;	/* erase one character */
		}
		else if (*p == CERASE2)	/* <del> key */
		    erase = 1;
		else if (*p == CKILL)
		    erase = linelength;	/* erase all character on line */
		else if (linelength < BUFSIZE) {    /* ok to accept chars */
		    p++;
		    linelength++;
		/* here we could ding for line-too-long */
		}
		while(linelength && erase--) {
			p--;
			linelength--;
			message("\b \b");
		    }
	}
			
	putchar('\n');	/* echo a linefeed to follow the return */
	*(++p)= '\0';	/* the ++ protects the \r to make getone() work */
	GlobPtr->linesize = linelength;
}

/*
 * gets one character from the input buffer; returns are
 * converted to nulls
 */
char getone()
{
	return ((*(GlobPtr->lineptr)=='\r')?
		'\0':
		 *((GlobPtr->lineptr)++));
}

/*
 * indicates the next character that getone() would return;
 * does NOT convert \r to \0.
 */
char peekchar()
{
	return(*(GlobPtr->lineptr));
}

