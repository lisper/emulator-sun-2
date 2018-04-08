/* Copyright (C) 1982 by Stanford University */
/*#define	DEBUG */

/*
 * mpuprt.c
 *
 * minimal ethernet bootstrap loader package - find Pup route
 *
 * Given a destination, this returns (by reference)
 * our net number and the first-hop host number
 * for the host.
 * Zero is returned on failure.
 *
 * Even if we cannot find a route, we should still try as hard
 * as possible to fill in "our network number", since this may
 * be a "broadcast" request.
 *
 * CAUTION: this will not work on NNSO machines!
 *
 * Jeffrey Mogul @ Stanford	8 July 1982
 *
 * Philip Almquist	September 29, 1984
 *	- Increase max nets from 20 to 256
 *
 * Philip Almquist	January 8, 1986
 *	- Use symbolic MAXNUMNETS rather than 256 now that puproute.h
 *	  has been fixed.
 *	- Merge fixes from rsf's TFTP version: check return code from puprd,
 *	  better debugging code.  Note: did not merge change which uses
 *	  responding gateway as the gateway host rather than searching the
 *	  routing table, since the old code has always worked fine for me
 *	  when the routing table buffer was large enough (his wasn't).
 */

#include <puplib.h>
#include <pupconstants.h>
#include <puproute.h>
#include <pupstatus.h>

#define ROUTESOCKET	0127	/* a "random" number */

mpuprt(destnet, desthost,routenet, routehost)
register Net	destnet;
register Host	desthost;
register Net	*routenet;
register Host	*routehost;
{
	struct PupChan pc;
	struct Port sport;
	struct Port srcport;
	struct GatewayInfo buf[MAXNUMNETS];	/* @ 4 bytes/entry */
	int dummy;
	int buflen;
	uchar ptype;
	register int retries;
	
	sport.host = 0;
	sport.net = 0;
	sport.socket = GATEWAYINFO;
	
	pupopn(&pc,ROUTESOCKET,&sport);
	pc.timeout = ONESEC;
	msetfilt(&pc,64);
	
	for (retries = 0; retries < 5; retries++){
	     pupwrt(&pc,GWINFREQ, 0, NULL, 0);
	
	     if (puprd(&pc,buf,&buflen,&ptype,&dummy,&srcport) != OK)
		continue;

#ifdef DEBUG
	     PortPrint(&srcport);
	     printf(" sends %d bytes, ptype %o\n",buflen, ptype);
	     { char *d = (char *)&(buf[0]);
	     printf("Data bytes are %o %o %o %o %o %o %o %o %o %o\n",
		  d[0],d[1],d[2],d[3],d[4],d[5], d[6], d[7], d[8], d[9]);
	     }
#endif DEBUG
		
	    *routenet = srcport.net;	/* if this isn't right, it never
	    				 * will be */

	    if (srcport.net == destnet) {	/* on local net */
		*routehost = desthost;
		return(1);
	    }

	    if (ptype == GWINFREP) {
		register int i = (buflen >> 2);	/* 4 bytes/entry */

		while (i-- > 0) {
#ifdef	DEBUG
		    printf("buf[%d]: %o## via %o\n", i,
		    	buf[1].TargetNet, buf[i].GatewayHost);
#endif	DEBUG
		    if (destnet == buf[i].TargetNet) {
			*routehost = buf[i].GatewayHost;
#ifdef	DEBUG
			printf("Route to %o#%o# is via host %o on net %o\n",
				destnet, desthost, *routehost, *routenet);
#endif	DEBUG
			return(1);
		    }
		}
		return(0);	/* No Route */
	    }
	}
	return(0);	/* Timeout */
}
