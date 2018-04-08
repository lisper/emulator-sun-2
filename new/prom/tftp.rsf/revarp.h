#ifndef REVARP
#define REVARP

/* ARP definitions. */
/* Ross Finlayson, March 1984. */

#include "../tftpnetboot/ip.h"
#include "../tftpnetboot/enet.h"

/* Since reverse ARP packets have the same format as regular ARP... */
#include "../tftpnetboot/arp.h"

/* Ethertype of 'reverse ARP' packets. */
#define REVARP_ETHERTYPE 0x8035

/* Possible opcodes (values of ar_op). */
#define ares_op_REQUEST_REVERSE 3
#define ares_op_REPLY_REVERSE 4

#endif REVARP
