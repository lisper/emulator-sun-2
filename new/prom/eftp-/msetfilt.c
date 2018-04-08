/* Copyright (C) 1981 by Stanford University */

/*
 * msetfilt.c
 *
 * minimal ethernet bootstrap loader package
 *
 * Builds and sets a packet filter for a pup channel.
 * 	Filters on destination socket and type= PUP.
 *
 * Jeffrey Mogul @ Stanford	19-February-1981
 *
 */

#define offset(b,a) ( (int)&(b->a) - (int)(b) )

#ifdef VAX

#include <puplib.h>
#include <pupconstants.h>
#include <pupstatus.h>
#include <puppacket.h>

msetfilt(Pchan,priority)
struct PupChan *Pchan;		/* open pup channel */
uchar	priority;		/* filter priority */
{	/* */
	register struct enfilter *ef;	/* used as a temporary */
	struct PupPacket *pp;	/* used as a placeholder */
  	struct EnPacket *ep;	/* used as a placeholder */
	register int	i;		/* # of words in filter */

	/*
	 * Here we would switch on Pchan->transport to
	 * decide what sort of filter mechanism to build;
	 * currently, the only choice is an enfilter.
	 */
	
	ef = &(Pchan->filter.en);	/* get address of filter */

	ef->Priority = priority;
	i = 0;

	/* always insure that the ethernet packet type is Pup */

	ef->Filter[i++] = PUSHWORD + (offset(ep,PacketType)/2);
		/* filters use word offsets in packet */
	ef->Filter[i++] = PUSHLIT | EQ;
		/* always an equality test */
	ef->Filter[i++] = PUP;

	/* always filter on destination socket */
	ef->Filter[i++] = PUSHWORD +
		 ((ENPACKOVER+offset(pp,PupDst.pk_FSocket))/2);
		/* first word of longword */
	ef->Filter[i++] = PUSHLIT | EQ;
		/* always an equality test */
	ef->Filter[i++] = 
		getLoWord(makelong(Pchan->SrcPort.socket));
		/* low word of literal */

	ef->Filter[i++] = PUSHWORD +
		 ((ENPACKOVER+offset(pp,PupDst.pk_FSocket))/2) + 1;
		/* second word of longword */
	ef->Filter[i++] = PUSHLIT | EQ;
		/* always an equality test */
	ef->Filter[i++] = 
		getHiWord(makelong(Pchan->SrcPort.socket));
		/* high word of literal */

	ef->Filter[i++] = AND;
		/* create the conjunction of the two terms */
	
	ef->Filter[i++] = AND;
	/* between terms in the filter, include an AND operation. */
	ef->FilterLen = i;
}
#endif

#ifdef MC68000

#include <puplib.h>
#include <pupconstants.h>
#include <pupstatus.h>
#include <puppacket.h>

msetfilt(Pchan,priority)
register struct PupChan *Pchan;		/* open pup channel */
uchar	priority;		/* filter priority */
{	/* */
	register struct enfilter *ef;	/* used as a temporary */

	/*
	 * Here we would switch on Pchan->transport to
	 * decide what sort of filter mechanism to build;
	 * currently, the only choice is an enfilter.
	 */
	
	ef = &(Pchan->filter.en);	/* get address of filter */

	/* always insure that the ethernet packet type is Pup, and
	 * to right destination socket */
	ef->mask = ENPTYPE|DSTSOCK;		/* indicate in mask */
	ef->enptype = PUP;		/* remember the packet type */
	ef->dstsock = Pchan->SrcPort.socket;	/* and socket */

}

#endif MC68000
