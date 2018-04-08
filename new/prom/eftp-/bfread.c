/* Copyright (C) 1981 by Stanford University */

/*
 * bfread.c
 *
 * minimal bootstrap loader package
 *
 * bfread -- read an arbitrary buffer from an eftp channel,
 *		with minimal code-size overhead
 *
 * Jeffrey Mogul @ Stanford	19-February-1981
 */

#include <eftp.h>
#include <puplib.h>
#include <pupstatus.h>

#define min(a,b)  ( (a<b)? a:b )

bfread(EfChan,buffer,buflen,rbuflen)
struct EftpChan *EfChan;	/* eftp channel to read from */
char *buffer;			/* buffer to read */
int buflen;			/* maximum length of buffer */
int *rbuflen;			/* returned length of buffer */
{	/* */
	char	swapbuf[EFTP_MAX_PACKET];	/* for byte-swapping */
	register int	ThisChunk;	/* size of current packet */
	register int	status;		/* status from mefrecpckt */
	int	retlen;		/* returned buffer length */

	status = mefrecpckt(EfChan, swapbuf, &retlen);	/* get packet */
	retlen = min(buflen, retlen);	/* don't return more than asked for */
	
/* 	"status" now holds returned status from mefrecpckt */
	switch ((int)status) {

		case OK:	/* it worked -- return buffer to user */
#ifdef	PUP__NNSO	/* swap bytes */
			swab(swapbuf, buffer, retlen);
#else			/* don't byteswap, just move it */
			bmove(swapbuf, buffer, retlen);
#endif
			*rbuflen = retlen;	/* return len. of good buffer */
			return(status);	/* OK */

		case EFTP_ENDOFFILE:	/* end of file */
			*rbuflen = 0;
			return(EFTP_ENDOFFILE);

		default:		/* anything else, die */
			*rbuflen = 0;	/* return nothing */
			return(status);
		}	/*end switch on return from mefrecpckt */
}
