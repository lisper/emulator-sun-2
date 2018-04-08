/* 'Reverse ARP'. */
/* Ross Finlayson, March 1984. */

/* Exports:	ReverseArp(). */

#include "../tftpnetboot/ip.h"
#include "../tftpnetboot/enet.h"
#include "../tftpnetboot/tftpnetboot.h"
#include "../tftpnetboot/revarp.h"

Boolean ReverseArp(ipAddress, serverIpAddress)
    IpAddr *ipAddress, *serverIpAddress;
    /* Executes the 'reverse ARP' protocol to try to find our internet
     * address. FALSE is returned iff this fails.
     */
  {
#ifdef ENET10
    char packet[TOTAL_PACKET_SIZE]; /* For outgoing/incoming packets. */
    register ArpPacket *revArpPacket
    	 = (ArpPacket *)&packet[sizeof(EnetHeader)];
    EnetAddr ourEnetAddress, dummy;

   /* Fill in the fields of the RARP REQUEST_REVERSE packet. */
    revArpPacket->ar_op = ares_op_REQUEST_REVERSE;
/*  revArpPacket->ar_spa is undefined. */
    /* The Ethernet address whose IP address we wish to find: */
    enetaddress(AddressOf(revArpPacket->ar_tha));
/*  revArpPacket->ar_tpa is undefined (since this is what we are to find). */
    commonArpCode(revArpPacket, REVARP_ETHERTYPE, AddressOf(ourEnetAddress));

    /* Loop until we receive a correct reverse ARP reply, or timeout. */
    while (TRUE)
      {
	if (!EnetReceive(revArpPacket, AddressOf(dummy),
		         REVARP_ETHERTYPE, ARP_TIMEOUT))
	    return(FALSE); /* Timeout */

        /* Make sure that the packet is the reply that we want. */
	if (revArpPacket->ar_op != ares_op_REPLY_REVERSE)
	    continue;
	if (revArpPacket->ar_pro != IP_ETHERTYPE)
	    continue;
	if (!EqualEnetAddrs(revArpPacket->ar_tha, ourEnetAddress))
	    continue;

	*ipAddress = revArpPacket->ar_tpa;
	*serverIpAddress = revArpPacket->ar_spa;
	return(TRUE);
      }
#else ENET10
    /* on 3Mb Ethernets, we don't use reverse ARP. */
    return(FALSE);
#endif ENET10
  }
