/* Copyright (C) 1981 by Stanford University */

/*
 * puprd.c
 *
 * minimal ethernet bootstrap loader package
 *
 * Reads a pup packet and returns the information in it.
 * This is like pupread, but doesn't allow optional parameters,
 * doesn't return Destination Port, doesn't do checksums,
 * and doesn't guarantee not to alter the parameters on
 * a lossage.
 *
 * Jeffrey Mogul & Dan Kolkowitz	23-February-1981
 *
 */

#include <puppacket.h>
#include <pupconstants.h>
#include <pupstatus.h>

puprd(Pchan,buf,buflen, Ptype, ID, SrcPort)
register struct PupChan	*Pchan;			/* channel to use */
register char	*buf;				/* data buffer */
register int	*buflen;			/* data buffer length */
register uchar	*Ptype;				/* Pup type */
register ulong	*ID;				/* Pup ID */
struct Port	*SrcPort;			/* source port */
{
	short	guard;
	struct	EnPacket enpack;	/* ether encapsulation */
	register int	readstat;
	register int	rlen;		/* rounded length of pup data */

	
	readstat = enread(Pchan->ifid, & enpack, &(Pchan->filter.en),
				Pchan->timeout);

	if (readstat==OK) {	/* return all information requested */

		rlen = roundup(enpack.Pup.PupLength) - PUPACKOVER;

#ifdef CHECKSUM
		if ( (*(ushort *)(&enpack.Pup.PupData[rlen])) !=
			checksum(&enpack.Pup,rlen+PUPACKOVER-2)) {
#ifdef DEBUG
			    printf("CHECKSUM ERROR\n");
/*printf("Received checksum %o\n", *(ushort*)(&enpack.Pup.PupData[rlen]));
{int k; for (k=0;k<rlen;k++) if (enpack.Pup.PupData[k] != 'A')
	printf("%c at %d\n",enpack.Pup.PupData[k],k);}
ppacket(&enpack); */
#endif DEBUG
			    return(BADCKSUM);
		}
#endif CHECKSUM

		/*
		 * return each parameter
		 */
		
		PortUnpack(&enpack.Pup.PupSrc, SrcPort);
		
		*ID = getlong(enpack.Pup.PupID);

		*Ptype = enpack.Pup.PupType;

		*buflen = enpack.Pup.PupLength - PUPACKOVER;
			/* return unrounded length */

		/* return buffer to user */
		bmove(enpack.Pup.PupData,buf,rlen);

	}

	return(readstat);
}
