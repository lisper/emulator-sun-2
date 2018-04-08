/* IP I/O routines. */
/* Ross Finlayson, March 1984. */

/* Exports:	IpSend(), IpReceive(). */

#include "../tftpnetboot/ip.h"
#include "../tftpnetboot/tftpnetboot.h"
#include "../tftpnetboot/enet.h"

static Bit16 computeIpCksum(ipPacket)
    IpPacket *ipPacket;
    /*
     * Returns the checksum of the header of the IP packet pointed to by
     * "ipPacket".
     */
  {
    register unsigned workingCksum = 0;
    register unsigned short *sptr;
    register int nshorts;

    /* Checksum the IP header, in 16-bit pieces. */
    sptr = (unsigned short *)ipPacket;
    nshorts = ipPacket->ipHeader.ihl*2; /* Number of shorts in the header. */
    while (nshorts-->0)
        workingCksum += *sptr++;

    /* Subtract out the current value of the checksum field, which is not taken
     * into consideration. */
    workingCksum -= (unsigned short)ipPacket->ipHeader.checksum;

    /* Add the overflows to the low order short. */
    workingCksum += workingCksum>>16;

    /* Return the one's complement of what we have. */
    return(~workingCksum);
  }


Boolean IpSend(ipData, ipDataLength, sourceIpAddr, destIpAddr, destEnetAddr,
	       protocol)
    char *ipData;
    unsigned ipDataLength; /* number of bytes of IP data (excl. IP hdr). */
    IpAddr sourceIpAddr, destIpAddr;
    EnetAddr destEnetAddr;
    Bit8 protocol;
/*
 * Sends the IP packet containing "ipData" to "destIpAddr". "protocol"
 * is the protocol number to be inserted in the IP header.
 * Other options in the IP header are given 'default' values.
 * This routine returns FALSE if an error occurs.
 */
  {
    register IpPacket *ipPacket = (IpPacket *)(ipData - sizeof(IpHeader));
    int byteCount; 
    EnetAddr ourEnetAddress;

    /* Fill in the IP header. */
    ipPacket->ipHeader.version = CURRENT_IP_VERSION;
    ipPacket->ipHeader.ihl = 5;		/* No options => 5 word header. */
    ipPacket->ipHeader.tos = 0;		/* No special service. */
    ipPacket->ipHeader.tl = ipDataLength + 20;
    ipPacket->ipHeader.id = 1;	/* packet id (we won't bother changing this)*/
    ipPacket->ipHeader.fragmentStuff = 0;	/* ignore fragmentation. */
    ipPacket->ipHeader.ttl = 255;	/* 255 seconds "time to live". */
    ipPacket->ipHeader.prot = protocol;
    ipPacket->ipHeader.srcAdr = sourceIpAddr;
    ipPacket->ipHeader.dstAdr = destIpAddr;

    /* Now compute the header checksum. */
    ipPacket->ipHeader.checksum = computeIpCksum(ipPacket);

    /* Write the IP packet out to the net. */
    enetaddress(AddressOf(ourEnetAddress));
    EnetSend((char *)ipPacket, ipPacket->ipHeader.tl,
		  ourEnetAddress, destEnetAddr, IP_ETHERTYPE);
    return(TRUE);
  }


Boolean IpReceive(ipData, ipDataLength,
	          sourceIpAddr, destIpAddr, sourceEnetAddr, protocol)
    char *ipData;
    unsigned *ipDataLength;
    IpAddr *sourceIpAddr, destIpAddr;
    EnetAddrPtr sourceEnetAddr;
    Bit8 *protocol;
/*
 * (Attempts to) receive a well-formed IP packet, putting the data in the area
 * pointed to by "ipData". The source address (the sender of the packet) is
 * returned in "sourceIpAddr" (and the Ethernet address in "sourceEnetAddr").
 * The number of data bytes in the packet (excluding the IP header) is returned
 * in "ipDataLength". The protocol field of the header is returned in
 * "protocol".
 * FALSE is returned iff a fatal error (timeout) occurs.
 */
  {
    register IpPacket *ipPacket = (IpPacket *)(ipData - sizeof(IpHeader));

while (TRUE) /* loop until we receive a well-formed IP packet, or timeout. */
  {
    /* (Attempt to) receive an IP packet. */
    if (!EnetReceive(ipPacket, sourceEnetAddr, IP_ETHERTYPE, TFTP_TIMEOUT))
        return(FALSE); /* Timeout. */

    /* Check for some possible errors. */
    if (ipPacket->ipHeader.dstAdr != destIpAddr)
	continue;
    if (computeIpCksum(ipPacket) != ipPacket->ipHeader.checksum)
        continue; /* bad IP header checksum */
    /* Note: to save code space we have omitted the following checks:
     *		- that the IP version number is correct.
     *		- that the packet isn't part of a fragment.
     */

    *ipDataLength = ipPacket->ipHeader.tl - 4*ipPacket->ipHeader.ihl;
    *sourceIpAddr = ipPacket->ipHeader.srcAdr;
    *protocol = ipPacket->ipHeader.prot;
    if (ipPacket->ipHeader.ihl > 5)
      {
	/* There are 'options' in the IP header.  We ignore these, but we must
	 * move the data accordingly.  (Ideally, we should just be able to
	 * adjust the data pointer parameter.)  With luck, this code will
	 * seldom, if ever, be used.
	 */
	char *from = &(ipData[4 * (ipPacket->ipHeader.ihl - 5)]);
	int i;
	
	for (i = *ipDataLength; i > 0; --i)
	    *ipData++ = *from++;
      }
    return(TRUE);
  }
  }
