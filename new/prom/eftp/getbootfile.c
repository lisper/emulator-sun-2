/* Copyright (C) 1981 by Stanford University */

/*
 * getbootfile.c
 *
 * Jeffrey Mogul @ Stanford	21 April 1981
 *
 * getbootfile -- load a bootfile into memory
 *
 * returns length (bytes) or -1 for failure
 *
 * Edits:
 *	Philip Almquist, September 25, 1984
 *	- Be more verbose while booting
 *	- Double timeout period since some systems are heavily loaded
 *
 *	Philip Almquist, November 27, 1984
 *	- Use unique(?) socket for booting
 */
/*#define DEBUG */

#include <puplib.h>
#include <pupconstants.h>
#include <pupstatus.h>
#include <m68enet.h>
#include <eftp.h>

#define BOOTTIMEOUT 10	/* timeout during boot */
#define BOOTSOCKET 0124	/* random number */

char *index();

getbootfile(bfn,bfname,storage,inhib)
int	bfn;				/* boot file number */
register char	*bfname;		/* boot file name */
char	*storage;			/* storage buffer */
int	inhib;				/* if true, don't do a request */
{	/* */
	struct EftpChan ec;
	int rlen;
	int done;
	register int i;
	Net snet = 0;
	Host shost = 0;
	Net ournet = 0;
	Host immhost = 0;
	register int error = 0;
	register int tlen = 0;
	register char * bptr;
	register char * colptr;
	register int pcount = 1;
	char hostnm[40];
	struct Port route;

	colptr = bfname;
	while (*colptr) {
	    if (*colptr == ':') {	/* embedded host name found */
		i = (colptr - bfname);	/* length of hostname part */
		bmove(bfname,hostnm,i);	/* save host name */
		hostnm[i] = 0;		/* null-term it */
		bfname = &colptr[1];	/* move to real filename */
#ifdef DEBUG
		printf("host is %s\n",hostnm);
#endif DEBUG
		mlkup(hostnm,i,&shost,&snet,&ournet);
		break;		/* exit from while loop */
	    }
	    colptr++;
	}

	mpuprt(snet,shost,&ournet,&immhost);

#ifdef DEBUG
	printf("host is %o#%o\n",snet,shost);
	printf("our net is %o\n",ournet);
	printf("first hop is %o\n",immhost);
#endif DEBUG

	/* set up eftp channel (stolen from efrecopen, with trimming) */

	ec.sequence = 0;

	ec.remport.host = shost;
	ec.remport.net = snet;
	ec.remport.socket = MISCSERVICES;

	pupopn(&ec.pchan,BOOTSOCKET | (emt_ticks() << 9),&ec.remport);
	
	/* patch up our network number and "route" */
	ec.pchan.SrcPort.net = ournet;
	ec.pchan.ImmHost = immhost;
	
	ec.pchan.timeout = ONESEC*BOOTTIMEOUT;

	msetfilt(&ec.pchan, 64); 

	/* request the bootstrap file */

	if (!inhib) {

#ifdef DEBUG
	printf("requesting %o, %s\n",bfn, bfname);
#endif DEBUG
 
#ifdef SMALLEST
	pupwrt(&ec.pchan, SUNBOOTREQ, bfn, NULL, 0); 
#else
	pupwrt(&ec.pchan, SUNBOOTREQ, bfn, bfname, strlen(bfname));
#endif SMALLEST
	}

	/* eftp the program into memory */
	done = 0;
	bptr = storage;
	while (!done) {
#ifdef DEBUG
		switch((int)(i=bfread(&ec,bptr,EFTP_MAX_PACKET, &rlen))) {
#else
		switch((int)(bfread(&ec,bptr,EFTP_MAX_PACKET, &rlen))) {
#endif DEBUG
			case OK:	/* got buffer */
				if (tlen == 0) {
					message("\r\nBooting from ");
					printoctal(ec.remport.net, 3);
					putchar('#');
					printoctal(ec.remport.host, 3);
					message("#\r\n");
					}
				putchar('!');
				if (pcount++ >= 50) {
					message("\r\n");
					pcount = 1;
					}
				bptr += rlen;
				tlen += rlen;
				break;
			
			case EFTP_ENDOFFILE:
				done++;
				if (pcount != 0)
					message("\r\n");
				message("[done]\r\n");
				break;
			
			default:/* */
#ifdef DEBUG
				printf("Error %d\n",i);
#endif
				error++;
				done++;
			}
		}
	
	if (error)
	    return(-1);
	else
	    return(tlen);
}
