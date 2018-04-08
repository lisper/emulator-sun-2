/* Ethernet (high-level) I/O routines. */
/* Ross Finlayson, March 1984. */

/* Exports:	EnetSend(), EnetReceive(). */

#include "../tftpnetboot/enet.h"
#include "../tftpnetboot/tftpnetboot.h"

EnetSend(enetData, enetDataLength, sourceEnetAddr, destEnetAddr, etherType)
    char *enetData;
    unsigned enetDataLength; /* Number of bytes of ethernet data. */
    EnetAddr sourceEnetAddr, destEnetAddr;
    EtherType etherType;
    /* Sends the ethernet packet containing "enetData", from ethernet
     * address "sourceEnetAddr" to "destEnetAddr".
     */
  {
    register EnetPacket *enetPacket
    			 = (EnetPacket *)(enetData - sizeof(EnetHeader));
    unsigned numShorts;

    /* Figure out how long the packet will be. */
    numShorts = (enetDataLength + sizeof(EnetHeader) + 1) >> 1;
    if (numShorts < ENET_MIN_PACKET)
        numShorts = ENET_MIN_PACKET;

    CopyEnetAddr(enetPacket->enetHeader.sourceAddr, sourceEnetAddr);
    CopyEnetAddr(enetPacket->enetHeader.destAddr, destEnetAddr);
    enetPacket->enetHeader.etherType = etherType;

    /* Write out to the Ethernet. */
    mc68kenwrite((unsigned short *)enetPacket,
		 numShorts);
  }


Boolean EnetReceive(enetData, sourceEnetAddr, etherType, timeoutCount)
    char *enetData;
    EnetAddrPtr sourceEnetAddr;
    EtherType etherType;
    unsigned timeoutCount;
    /* (Attempts to) receive a well-formed ethernet packet (of type
     * "etherType"), putting the data in the area pointed to "enetData".
     * The sender's Ethernet address is returned in "sourceEnetAddr".
     * FALSE is returned iff a fatal error (timeout) occurs.
     */
  {
    register EnetPacket *enetPacket
    			 = (EnetPacket *)(enetData - sizeof(EnetHeader));

    /* loop until we receive a well-formed ethernet packet of type
     * "etherType", or until we timeout. */
    while (TRUE)  
    {
        /* (Attempt to) read an ethernet packet. */
	if (mc68kenread((unsigned short *)enetPacket, &timeoutCount,
		    TOTAL_PACKET_SIZE>>1) <= 0)
	    return(FALSE); /* Timeout */

	/* Reject packets that don't have the ethertype that we want. */
	if (enetPacket->enetHeader.etherType != etherType)
	    continue;

	/* We have a good packet - return. */
	CopyEnetAddr(Deref(sourceEnetAddr), enetPacket->enetHeader.sourceAddr);
	return(TRUE);
      }
  }
