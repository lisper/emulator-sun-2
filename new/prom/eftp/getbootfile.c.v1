/* Copyright (C) 1981 by Stanford University */

/*
 * getbootfile.c
 *
 * Jeffrey Mogul @ Stanford	21 April 1981
 *
 * getbootfile -- load a bootfile into memory
 *
 * returns length (bytes) or -1 for failure
 */
/*#define DEBUG */

#include <puplib.h>
#include <pupconstants.h>
#include <pupstatus.h>
#include <m68enet.h>
#include <eftp.h>

#define BOOTTIMEOUT 5	/* timeout during boot */
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

	if (snet == 0)
	    mlkup("0#0#4",3,&shost,&snet,&ournet);
	else
	    mpuprt(snet,shost,&ournet,&immhost);

	/* if no gateway responded then try a broadcast */
	if (ournet == 0)
	    immhost = shost;

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

	pupopn(&ec.pchan,BOOTSOCKET,&ec.remport);
	
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
#ifdef	DEBUG
				printf("!");
#endif	DEBUG
				bptr += rlen;
				tlen += rlen;
				break;
			
			case EFTP_ENDOFFILE:
#ifdef	DEBUG
				printf("$\n");
#endif	DEBUG
				done++;
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
