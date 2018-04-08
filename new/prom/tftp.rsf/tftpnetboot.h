#ifndef TFTPNETBOOT.H
#define TFTPNETBOOT.H

/* Misc definitions. */

#include "tftp.h"
#include "udp.h"
#include "ip.h"
#include "enet.h"

typedef int Boolean;
typedef unsigned long Address;

#define TRUE 1
#define FALSE 0
#define NULL 0

/* The character used to delimit fields in the console input string. */
#define INPUT_DELIMITER ','

/* Timeout intervals (in (very roughly) 1/60 second units) on
 * Ethernet receives. */
#define ARP_TIMEOUT 500
#define TFTP_TIMEOUT 3000

/* The maximum number of unwanted Ethernet packets that we shall receive
 * before timing out. We use this constant in addition to one of the above
 * timeout counts because the Ethernet reader may get 'woken up' by lots of
 * broadcast packets that we're not interested in.
 */
#define MAX_BAD_PACKETS 15

/* The total size of a TFTP packet, including IP header and UDP header. */
#define TOTAL_PACKET_SIZE sizeof(EnetHeader) \
			     + sizeof(IpHeader) \
			     + sizeof(UdpHeader) \
			     + sizeof(TftpPacket)


#endif TFTPNETBOOT.H
