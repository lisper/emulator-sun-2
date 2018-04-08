/*
 *
 * Excelan 10 Mbit Ethernet driver
 * (derived from /usr/sun/src/ipwatch/excelan.c, which in turn was derived from
 *  the V-kernel Excelan driver.)
 *
 * Ross Finlayson, April 1984.
 */

/* Exports:	enetaddress(), enopen(), mc68kenread(), mc68kenwrite(). */

#include "../enet10/excelan.h"


static clear(ptr, n)
    register char *ptr;
    register n;
  {
    while (n-- > 0)
        *ptr++ = 0;
  }


static InterruptBoard()
  {
    register counter = 10000;

    while ((PORTB & X_NOT_READY) && (--counter > 0))
        ; /* Wait until it's OK to write to PORTB. */
    PORTB = 0;			/* Interrupt the board by writing to PORTB. */
  }


int SetMode( newmode )
	unsigned newmode;
  {
    /*
     * Set the mode of the interface.
     */
    register struct ModeMsg *modemsg, *replymsg;
    register counter;

    modemsg = (struct ModeMsg *) MsgQPtr->hreqtail;
    modemsg->request = NET_MODE;
    modemsg->reqmask = WRITEREQ;
    modemsg->mode = newmode;
    modemsg->errmask = AlignmentError+CRCError;
    modemsg->status = EXOS_OWNER|HOST_DONE;
    InterruptBoard();

    replymsg = (struct ModeMsg *) MsgQPtr->hrephdr;
    counter = 100000;
    while (replymsg->status != HOST_OWNER|EXOS_DONE)
        if (--counter == 0)
	    return(0);
    replymsg->status = EXOS_OWNER | HOST_DONE;
    InterruptBoard();
    return(1);
  }


int enopen()
 {
    /*
     *  Powerup and initialize the Ethernet Interface Board. 0 is returned if
     *  the board couldn't be initialized.
     */
    register i;
    long addr;
    register struct testp *testpat;
    /* The following variables are replaced by constants. */
/*  struct MsgBuf *reqbufptr, *repbufptr;
    struct Init *configmsg;*/

    /* Reset board, by reading from PORTA. If PORTA cannot be read (eg. because
     * there's no Excelan board at all), then ProbeAddress will return 0).
     */
    if (!ProbeAddress((&PORTA)&~1)) /* NB: ProbeAddress tries to read a short*/
        return (0); /* No board? */

    i = 0;
    while( ( PORTB & X_ERROR ) == 0 )	/* Test status--wait for response. */
      if( i++ > 100000 )
	  return(0); /* Ethernet interface didn't reset. */

    /* Set up Multibus Memory-the shared memory between board and processor.*/
    clear( MsgQVirAddr, sizeof( struct MsgQs ) );

    /* Initialize message queues. */
    MsgQPtr->xreqhdr = HostToExAddr( MsgQPtr->requestbufs );
    MsgQPtr->xreptail = HostToExAddr( MsgQPtr->replybufs );
    MsgQPtr->hreqtail = MsgQPtr->requestbufs;
    MsgQPtr->hrephdr = MsgQPtr->replybufs;

    /* Initialize (the single) request buffer--Host to Exos buffer. */
#define reqbufptr ( MsgQPtr->requestbufs )
	reqbufptr->next = reqbufptr;
	reqbufptr->msg.link = HostToExAddr( reqbufptr );
	reqbufptr->msg.length = MSGLEN;
	reqbufptr->msg.status = HOST_OWNER | EXOS_DONE;

    /* Initialize (the single) reply buffer--Exos to Host buffer. */
#define	repbufptr ( MsgQPtr->replybufs )
	repbufptr->next = repbufptr;
	repbufptr->msg.link = HostToExAddr( repbufptr );
	repbufptr->msg.length = MSGLEN;
	repbufptr->msg.status = EXOS_OWNER | HOST_DONE;

    /* Construct configuration message. */

#define configmsg ( (struct Init *)ReadDataBuf )/* Beginning of buffer space. */
    clear( configmsg, sizeof *configmsg );	/* Defaults all values to 0. */

    configmsg->res0 = 1;			/* Excelan doc: pgs. 24-5. */
    configmsg->code = 0xff;
    configmsg->mode = 0;			/* Link level cntrlr mode. */
    configmsg->format[0] = 1;			/* Look at test pattern. */
    configmsg->format[1] = 1;
    testpat = (struct testp *)configmsg->mmap;
    testpat->b0 = 1;
    testpat->b1 = 3;
    testpat->b2 = 7;
    testpat->b3 = 0xf;
    testpat->w0 = 0x103;
    testpat->w1 = 0x70f;
    testpat->l0 = 0x103070f;
    configmsg->amode = 3;			/* Absolute address mode. */
    configmsg->mvblk = -1L;			/* 0xffffffff  */
    configmsg->nproc = 0xff;
    configmsg->nmbox = 0xff;
    configmsg->naddr = 0xff;
    configmsg->nhost = 1;
    configmsg->hxitype = 0;			/* no Interrupts. */
    configmsg->xhitype = 0;
    configmsg->hxseg = VirToPhys( MsgQVirSegAddr );
				/* Request Q offsets calculated from here. */
    configmsg->xhseg = VirToPhys( MsgQVirSegAddr );
				/* Reply Q offsets calculated from here. */
    configmsg->hxhead = ( (int) &MsgQPtr->xreqhdr - MsgQVirSegAddr );
    configmsg->xhhead = ( (int) &MsgQPtr->xreptail - MsgQVirSegAddr );

    addr = 0x0000ffff;
    for( i = 0; i < 4; i++ )
      {
	while( PORTB & X_NOT_READY );		/* Wait for device ready. */
	PORTB = addr & 0xff;
	addr >>= 8;
      }

    /* Now send physical address of configmsg. */

    addr = VirToPhys( configmsg );
    for( i = 0; i < 4; i++ )
      {
	while( PORTB & X_NOT_READY );		/* Wait for device ready. */
	PORTB = addr & 0xff;
	addr >>= 8;
      }

    /* Wait for Exos to change code. */
    i = 100000;
    while( configmsg->code == 0xff )
	if (--i == 0)
	    return (0); /* Board didn't configure. */

    return (SetMode( CONNECT_TO_NET ));
  }



enetaddress(enetAddr)
    char *enetAddr;
    /* returns our Ethernet address in "enetAddr". */
  {
    register struct AddrsMsg *addrmsg, *replymsg;
    register counter;
    register char *src;

    addrmsg = (struct AddrsMsg *) MsgQPtr->hreqtail;
    addrmsg->request = NET_ADDRS;
    addrmsg->reqmask = READREQ;
    addrmsg->slot = PHYS_ADDRS_SLOT;
    addrmsg->status = EXOS_OWNER|HOST_DONE;
    InterruptBoard();

    replymsg = (struct AddrsMsg *) MsgQPtr->hrephdr;
    counter = 100000;
    while (replymsg->status != HOST_OWNER|EXOS_DONE)
        if (--counter == 0)
	    return;
    src = (char *) &( replymsg->addr );
    for( counter = 0; counter < sizeof( NetAddress ); counter++ )
	*enetAddr++ = *src++;
    replymsg->status = EXOS_OWNER | HOST_DONE;
    InterruptBoard();
  }


mc68kenread(buf, timeout, maxShorts)
      unsigned short *buf;
      long *timeout; /* Read timeout, in (very roughly) 1/100s units. */
      int maxShorts; /* Maximum number of shorts to read in. */
   /*
    * Read a packet from the ethernet interface. 
    * We copy the packet from Multibus memory into the indicated buffer,
    */
  {
    register struct ReadMsg *readmsg, *replymsg;
    register count;
    unsigned shortsRead;
    register unsigned short *src;
    register long counter;
    
    /* Construct and send a read request--allowing reception of any packets
     * which might come in.
     */
    readmsg = (struct ReadMsg *) MsgQPtr->hreqtail;
    readmsg->request = NET_READ;
    readmsg->nblocks = 1;
    readmsg->block[0].len = ENET_MAX_PACKET;
    readmsg->block[0].addr = VirToPhys( ReadDataBuf );
    readmsg->status = EXOS_OWNER | HOST_DONE;
    InterruptBoard();
 
    /* Wait for the board to reply. */
    replymsg = (struct ReadMsg *)MsgQPtr->hrephdr;
    counter = (*timeout) << 9; /* ie. * 512 */
    while (--counter > 0)
        if (replymsg->status == HOST_OWNER|EXOS_DONE)
	  {
	    src =  (unsigned short *) ReadDataBuf;
	    shortsRead = (replymsg->block[0].len)>>1;
	    if (shortsRead > maxShorts)
	        shortsRead = maxShorts;
	    for (count=shortsRead; count>0; count--)
		     *buf++ = *src++;
	    break;
	  }
    *timeout = counter >> 9; /* ie. / 512 */
    if (counter == 0)
        return(0);
    replymsg->status = EXOS_OWNER | HOST_DONE;
    InterruptBoard();
    return(shortsRead);
  }


mc68kenwrite(buffer, count)
    unsigned short *buffer;
    int count; /* number of shorts to write. */
  {
    register struct XmitMsg *xmitmsg, *replymsg;
    register unsigned short *dest;
    register counter;
    
   /* Copy the data bytes into the write buffer. */
    dest = (unsigned short *) WriteDataBuf;
    for (counter=count; counter-- > 0; *dest++ = *buffer++)
        ;

    /* Set up the request message. */
    xmitmsg = (struct XmitMsg *) MsgQPtr->hreqtail;
    xmitmsg->request = NET_XMIT;
    xmitmsg->nblocks = 1;
    xmitmsg->block[0].len = (count<<1); /* Number of bytes. */
    xmitmsg->block[0].addr = (long) VirToPhys( WriteDataBuf );
    xmitmsg->status = EXOS_OWNER|HOST_DONE;
    InterruptBoard();

    replymsg = (struct XmitMsg *) MsgQPtr->hrephdr;
    counter = 100000;
    while (replymsg->status != HOST_OWNER|EXOS_DONE)
        if (--counter == 0)
	    return(0);
    replymsg->status = EXOS_OWNER | HOST_DONE;
    InterruptBoard();
    return(1);
  }
