/* UDP I/O routines. */
/* Ross Finlayson, March 1984. */

/* Exports:	UdpSend(), UdpReceive(). */

#include "../tftpnetboot/tftpnetboot.h"
#include "../tftpnetboot/udp.h"
#include "../tftpnetboot/enet.h"


Boolean UdpSend(udpData, udpDataLength,
		sourceIpAddr, destIpAddr, destEnetAddr, sourcePort, destPort)
    char *udpData;
    int udpDataLength; /* number of bytes of UDP data (excl. UDP hdr). */
    IpAddr sourceIpAddr, destIpAddr;
    EnetAddr destEnetAddr;
    unsigned short sourcePort, destPort;
/*
 * Sends the UDP packet containing "udpData" to "destIpAddr". The
 * source and destination UDP ports are "sourcePort" and "destPort".
 * This routine returns FALSE if an error occurs.
 */
  {
    register UdpPacket *udpPacket = (UdpPacket *)(udpData - sizeof(UdpHeader));

    udpPacket->udpHeader.sourcePort = sourcePort;
    udpPacket->udpHeader.destPort = destPort;
    udpPacket->udpHeader.length
		= udpDataLength + sizeof(udpPacket->udpHeader);
    udpPacket->udpHeader.checksum = 0; /* don't bother with a real checksum. */

    /* Do an IP send to the specified address. */
    return(IpSend((char *)udpPacket,
    		  udpPacket->udpHeader.length, /* length of IP data */
		  sourceIpAddr, destIpAddr, destEnetAddr,
		  UDP_PROTOCOL_NUM));
   }


Boolean UdpReceive(udpData, udpDataLength,
	   	   sourceIpAddr, destIpAddr, sourceEnetAddr,
        	   sourcePort, destPort)
    char *udpData;
    unsigned *udpDataLength;
    IpAddr *sourceIpAddr, destIpAddr;
    EnetAddrPtr sourceEnetAddr;
    unsigned short *sourcePort, *destPort;
/*
 * (Attempts to) receive a well-formed UDP packet, putting the data in the area
 * pointed to by "udpData". The source address (the sender of the packet) is
 * returned in "sourceIpAddr" (and the Ethernet address in "sourceEnetAddr).
 * The source and destination UDP ports are  returned in "sourcePort" and
 * "destPort". The number of data bytes in the packet (excluding the UDP
 * header) is returned in "udpDataLength".
 * FALSE is returned iff a fatal error (timeout) occurs.
 */
  {
    register UdpPacket *udpPacket = (UdpPacket *)(udpData - sizeof(UdpHeader));
    unsigned udpPacketLength; /* Total length of received UDP packet. */
    unsigned char protocolNum; /* The IP protocol number of the packet. */

while (TRUE) /* loop until we receive a well-formed UDP packet, or timeout. */
  {
    /* (Attempt to) receive an IP packet. */
    if (!IpReceive((char *)udpPacket, &udpPacketLength,
	     	    sourceIpAddr, destIpAddr, sourceEnetAddr, &protocolNum))
        return(FALSE); /* Timeout. */

    /* Check for some possible errors. */
    if (protocolNum != UDP_PROTOCOL_NUM)
	continue; /* Wrong protocol. */
    /* Note: to save code space we have omitted the following checks:
     *		- that the length of the received packet is consistent with the
     *		  value of the UDP header length field.
     *		- that the UDP checksum is correct.
     */

    *udpDataLength = udpPacket->udpHeader.length - sizeof(udpPacket->udpHeader);
    *sourcePort = udpPacket->udpHeader.sourcePort;
    *destPort = udpPacket->udpHeader.destPort;
    return(TRUE);
  }
  }
