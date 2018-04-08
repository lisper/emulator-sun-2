/* Copyright (C) 1981 by Stanford University */

/*
 * pupwrt.c
 *
 * minimal ethernet bootstrap loader package
 *
 * Pupwrite takes a buffer and sends it in a pup packet over
 * a Pup channel.
 *
 * Jeffrey Mogul & Dan Kolkowitz	12-January-1981
 *					26-January-1981
 *					23-February-1981
 *
 */

#include <puppacket.h>
#include <pupconstants.h>
#include <pupstatus.h>

pupwrt(Pchan, Ptype, ID, buf,buflen)
register struct	PupChan	*Pchan;		/* channel to use */
uchar	Ptype;				/* Pup type */
ulong	ID;				/* Pup ID */
register char	*buf;			/* data buffer */
register int	buflen;				/* data buffer length */
{
	struct	EnPacket enpack;	/* ether encapsulation */
	register int	roundlen;	/* length rounded up to even number */

	roundlen = roundup(buflen);	/* rounds UP to even number of bytes */

	enpack.Pup.PupLength = PUPACKOVER + buflen;
					/* contains TRUE length of data */
	enpack.Pup.PupType = Ptype;
	enpack.Pup.PupTransport = 0;	/* this has NOTHING to do with
					 * the tranport medium! */
	enpack.Pup.PupID = makelong(ID);

	PortPack(&Pchan->DstPort,&enpack.Pup.PupDst);
	PortPack(&Pchan->SrcPort,&enpack.Pup.PupSrc);

	if (buflen)	/* dont move on zero length buffers */
	    bmove(buf,enpack.Pup.PupData, roundlen);	/* put data in packet */

	/* set checksum into first WORD following data */
	/* send null checksum */
	*((short *) &enpack.Pup.PupData[roundlen]) = NOCKSUM;

	return(enwrite(Pchan->ImmHost,PUP,Pchan->ofid,
			&enpack,roundlen+PUPACKOVER));
	}
