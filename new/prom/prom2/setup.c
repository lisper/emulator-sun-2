/* Copyright (C) 1981 by Stanford University */

/*
 * setup.c
 *
 * Jeffrey Mogul @ Stanford	21 April 1981
 *
 * setup -- interprets header, copies text & data segments,
 *		zeros bss segment, returns entry point
 */

#include <b.out.h>

setup(p,l)
register char *p;
register int l;
{	/* */
	struct bhdr hdr;
	register char *zp;
	register char *st;
	register long i;
	register long tdsize;

	bmove(p,&hdr,sizeof(hdr));

#ifdef DEBUG
	printf("tsize %d, dsize %d, bss %d\n",
	(hdr.tsize),(hdr.dsize),(hdr.bsize));
	printf("Entry %x\n",(hdr.entry));
#endif
	/* copy text and data segments */
	/* WARNING: bmove is assumed to copy from low to high,
	 * thus allowing overlapping copies */ 
	tdsize = hdr.tsize + hdr.dsize;	/* compute size of text+data */
	zp = p;
	st = &p[sizeof(hdr)];
	i = tdsize;
	while (i-- > 0) *zp++ = *st++;
/*	bmove(&p[sizeof(hdr)],p,tdsize);*/

	/* move symbol table to _end (after bss) */
/*	zp = &p[tdsize + hdr.bsize];	/* compute final addr. of symtab */
	zp += hdr.bsize;	/* compute final addr. of symtab */
/*	st = &p[sizeof(hdr) + tdsize];	/* compute current addr. of symtab */

	i = hdr.ssize;
	*((long *)zp++) = i;		/* store symbol table size */
	if (hdr.bsize < sizeof(hdr)) {	/* move symtab down */
		while (i-- > 0) *zp++ = *st++;
	}
	else {				/* move symtab up */
		zp += i;
		st += i;
		while (i-- > 0)	*zp-- = *st--;
	}

	/* initialize bss segment */
	zp = &p[tdsize];
	for (i=hdr.bsize;i>0;i--) *zp++ = 0;

	return(hdr.entry);
}
