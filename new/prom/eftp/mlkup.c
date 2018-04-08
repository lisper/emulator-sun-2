/* Copyright (C) 1981 by Stanford University */

/*
 * mlkup.c
 *
 * minimal ethernet bootstrap loader package - lookup name
 *
 * This translates a host name to an octal host number; only
 * the host number is returned (not the net number).  Zero
 * (i.e., the broadcast host) is returned on failure.
 * CAUTION: this will not work on NNSO machines!
 *
 * Jeffrey Mogul @ Stanford	26 May 1981
 */

#include <puplib.h>
#include <pupconstants.h>

#define LOOKSOCKET	0123

mlkup(name,namelen,sh,sn,on)
register char *name;
register int namelen;
register Host *sh;
register Net *sn;
register Net *on;
{
	struct PupChan pc;
	struct Port sport;
	struct Port srcport;
	uchar buf[532];
	int dummy;
	uchar ptype;
	int retries;
	
	sport.host = 0;
	sport.net = 0;
	sport.socket = MISCSERVICES;
	
	pupopn(&pc,LOOKSOCKET,&sport);
	pc.timeout = ONESEC;
	msetfilt(&pc,64);
	
	for (retries = 0; retries < 5; retries++){
	     pupwrt(&pc, NAMEREQ, 0, name, namelen);
	
	     puprd(&pc,buf,&dummy,&ptype,&dummy,&srcport);

#ifdef DEBUG
	     printf("ptype %o\n",ptype);
	     printf("Data bytes are %o %o %o %o %o %o\n",
		  buf[0],buf[1],buf[2],buf[3],buf[4],buf[5]);
#endif DEBUG
	     if (ptype == NAMERESP) {
	         *sh = ((struct packedPort*)buf)->pk_Host;
	         *sn = ((struct packedPort*)buf)->pk_Net;
	         *on = srcport.net;
	         return( ((struct packedPort*)buf)->pk_Host);
	     }
	     if (ptype == LOOKUPERR)
	     	return(0);
	}
	return(0);
}
