/* Copyright (C) 1981 by Stanford University */
/*#define DEBUG 1 */

/*
 * mefrecpckt..c
 *
 * minimal ethernet boot package.
 *
 *		THIS ASSUMES NOTHING ABOUT BYTE ORDER.
 * mefrecpckt -- receive the next packet of an Eftp transfer
 *
 * Jeffrey Mogul @ Stanford	19-February-1981
 *
 * Philip Almquist	January 8, 1986
 *	- Merge in changes from rsf's TFTP boot version: more debugging
 *	  code (turned off), don't record "partner"'s address if packet
 *	  wasn't an EFTPDATA packet, and treat bogus packet types as
 *	  non-fatal (ignored) errors.
 *	- Ignore EFTPEND and EFTPABORT packets that preced the first
 *	  EFTPDATA packet.
 */

#define min(a,b)  ( (a<b)? a : b )

#include <pupconstants.h>
#include <pupstatus.h>
#include <eftp.h>

mefrecpckt(Efchan, buf, blen)
register struct EftpChan *Efchan;	/* open eftp channel */
register char *buf;			/* data buffer */
register int   *blen;			/* number of bytes we got */
{	/* */
	register int retry;
	register int rstat;
	uchar rpuptype;
	ulong rpupid;
	char msgbuf[100];
	register int msgbuflen;
	register int dallying = 0;	/* true iff we are waiting for
					 * second EFTPEND */
	struct Port Sender;

	while (1) {	/* all exits from this loop are "return"s */
		
		rstat = puprd(&Efchan->pchan, buf, blen, &rpuptype,
				&rpupid, &Sender);

#ifdef CHECKSUM
		if (rstat == BADCKSUM) continue;	/* try again */
#endif
		if (rstat == TIMEOUT)
		   if (dallying)
		      return(EFTP_ENDOFFILE);
		   else {
#ifdef	DEBUG
		      printf("TIMEOUT\n");
#endif	DEBUG
		      return(TIMEOUT);
		    }
#ifdef DEBUG
printf("length read %d (@%x)\n",*blen,blen);
#endif

		/* got a packet */

		/* we now decide if the packet is one we want.  If
		 * we are past sequence #0, it must be from our partner.
		 * Otherwise, if we are waiting to start with a specific
		 * host, it must be from that host.  If it is packet #0
		 * from a host we want, remember the host address.
		 */
#ifdef DEBUG
printf("Ef %o#%o#%o ",Efchan->remport.net,Efchan->remport.host,
		  Efchan->remport.socket);
printf("Sndr %o#%o#%o ",Sender.net,Sender.host,
		  Sender.socket);
printf("PupType %o",rpuptype);
printf(", our seq %o, rpupid %o\n",Efchan->sequence,rpupid);
#endif
		if (Efchan->sequence)	{ /* we are already running */
			if (!PortEQ(&Efchan->remport,&Sender)) {
					/* somebody trying to horn in */
					/* ignore the packet */
#ifdef DEBUG
printf("..ignoring\n");
#endif
					continue;
					}
			}
		else if (rpuptype == EFTPDATA) {
			/* remember this sender for future ref. */
			PortCopy(&Sender, &Efchan->remport);
			PortCopy(&Sender,&Efchan->pchan.DstPort);
			if (Sender.net == Efchan->pchan.SrcPort.net)
			    Efchan->pchan.ImmHost = Sender.host;
			/* if host is not on our net, then we should not modify
			 * the "first-hop" host since it is a gateway.
			 */
		}
		else
			/* Ignore all packets except EFTPDATAs until we */
			/* have received an EFTPDATA packet */
			continue;
#ifdef DEBUG
printf("good packet %o\n",rpupid);
#endif
		/* packet is from our partner */
		switch ((int)rpuptype) {

		case EFTPDATA:		/* data packet */
			if (rpupid == Efchan->sequence) {  /* sequence ok */
			    EfAck(Efchan, rpupid);
			    Efchan->sequence++;
			    return(OK);
			    }
			
			if (rpupid == Efchan->sequence - 1) {
			    /* re-ack last packet */
			    EfAck(Efchan, rpupid);
			    break;	/* keep trying */
			    }
			
			/* other things could happen, but we ignore them */
			return(EFTP_OUTOFSYNCH);

		
		case EFTPEND:		/* end of transfer */
			if (rpupid == Efchan->sequence) {  /* sequence ok */
			    if (dallying) {	/* we already saw one */
				return(EFTP_ENDOFFILE);
				}
			    else	{	/* first EFTPEnd */
				EfAck(Efchan, rpupid);
				dallying++;	/* start dally */
				Efchan->pchan.timeout = ONESEC*EFTP_DALLYWAIT;
						 /* alter timeout */
				Efchan->sequence++;	/* bump seqeunce */
				}
			    }
			else if (rpupid == Efchan->sequence-1) {
			    /* if we are not dallying, this is an
			     * out-of-synch packet, but we don't
			     * care in this system
			     */
			     EfAck(Efchan, rpupid);
			     }
			else	{	/* sender out of synch */
			    return(EFTP_OUTOFSYNCH);
			    }
			break;	/* gets here after first EFTPEnd */

		case EFTPABORT:
			return(EFTP_ABORT);
		
		default:	/* we got the wrong packet type! */
			break;	/* ignore it */
/*			return(EFTP_ERROR); */
			}
		}
	/* should never get here */
}

EfAck(efc,seqnum)	/* Acknowledge an EFTPDATA or EFTPEND */
struct EftpChan *efc;	/* Eftp channel to our partner */
long seqnum;		/* sequence number for Ack */
{	/* */
	pupwrt(&(efc->pchan),EFTPACK,seqnum, NULL, 0);
}
