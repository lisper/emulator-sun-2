/* ARP. */
/* Ross Finlayson, March 1984. */

/* Exports:	Arp(), commonArpCode() (for use by RevArp()). */

#include "../tftpnetboot/ip.h"
#include "../tftpnetboot/enet.h"
#include "../tftpnetboot/tftpnetboot.h"
#include "../tftpnetboot/arp.h"

commonArpCode(arpPacket, etherType, ourEnetAddr)
    /* Finds our Ethernet address, sets various fields of "arpPacket", and
     * sends the packet over the net. This code is common to both ARP and
     * reverse ARP. This routine thus saves some code space.
     */
    register ArpPacket *arpPacket;
    EtherType etherType;
    EnetAddrPtr ourEnetAddr;
  {
    EnetAddr destEnetAddr;

    /* Find our own Ethernet address. */
    enetaddress(ourEnetAddr);

    arpPacket->ar_hrd = ares_hrd_Ethernet;
    arpPacket->ar_pro = IP_ETHERTYPE;
    arpPacket->ar_hln = sizeof(EnetAddr);
    arpPacket->ar_pln = sizeof(IpAddr);
    CopyEnetAddr(arpPacket->ar_sha, Deref(ourEnetAddr));

    /* Send off the request. */
    SetToBroadcastAddress(destEnetAddr);
    EnetSend((char *)arpPacket, sizeof(ArpPacket),
    	      ourEnetAddr, destEnetAddr, etherType);
  }


Boolean Arp(ipAddress, enetAddress, ourIpAddress)
    IpAddr ipAddress, ourIpAddress;
    EnetAddrPtr enetAddress;
    /* Given the internet address "ipAddress", tries to find the corresponding
     * Ethernet address "enetAddress". FALSE is returned iff this fails.
     */
  {
#ifdef NOT_DOING_ARP
    return(FALSE);
#else NOT_DOING_ARP
#ifdef ENET10
    char packet[TOTAL_PACKET_SIZE]; /* For outgoing/incoming packets. */
    register ArpPacket *arpPacket
    	 = (ArpPacket *)&packet[sizeof(EnetHeader)];
    EnetAddr ourEnetAddress, dummy;

    /* Fill in the fields of the ARP REQUEST packet. */
    arpPacket->ar_op = ares_op_REQUEST;
    arpPacket->ar_spa = ourIpAddress;
/*  arpPacket->ar_tha is undefined (since this is what we are to find). */
    arpPacket->ar_tpa = ipAddress; /* The IP address whose Ethernet address
    				      we wish to find. */
    commonArpCode(arpPacket, ARP_ETHERTYPE, AddressOf(ourEnetAddress));

    /* Loop until we receive a correct ARP reply, or timeout. */
    while (TRUE)
      {
	if (!EnetReceive(arpPacket, AddressOf(dummy),
	                 ARP_ETHERTYPE, ARP_TIMEOUT))
	    return(FALSE); /* Timeout */

        /* Make sure that the packet is the reply that we want. */
	if (arpPacket->ar_op != ares_op_REPLY)
	    continue;
	/* We needn't bother checking that the protocol is IP - if it isn't,
	 * then the following test will almost surely fail. */
	if (arpPacket->ar_spa != ipAddress)
	    continue;

        CopyEnetAddr(Deref(enetAddress), arpPacket->ar_sha);
	return(TRUE);
      }
#else ENET10
    /* on 3Mb Ethernets, we don't use RFC826. */
    return(FALSE);
#endif ENET10
#endif NOT_DOING_ARP
  }
