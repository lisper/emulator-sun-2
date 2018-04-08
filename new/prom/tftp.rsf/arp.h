#ifndef ARP
#define ARP

/* ARP definitions. */
/* Ross Finlayson, March 1984. */

#include "../tftpnetboot/ip.h"
#include "../tftpnetboot/enet.h"


/* Ethertype of ARP packets. */
#ifdef ENET10
#define ARP_ETHERTYPE 0x806
#else ENET10
#define ARP_ETHERTYPE 01006
#endif ENET10

/* KLUDGE!! (for 3Mb Ethernet only). */
#ifndef ENET10
#define EnetAddr Bit16
#endif ENET10

/* ARP packet format. We are assuming "hardware" is Ethernet and "protocol" is
 * IP.
 */
typedef struct
  {
    Bit16 ar_hrd;	/* See RFC826 for the meaning of each field. */
    Bit16 ar_pro;
    Bit8 ar_hln;	/* = 6 for 10Mbps Ethernet, 2 for 3Mbps Ethernet (since
			 for alignment it needs to be a multiple of 16-bits). */
    Bit8 ar_pln;	/* = 4 for IP. */
    Bit16 ar_op;
    EnetAddr ar_sha;
    IpAddr ar_spa;
    EnetAddr ar_tha;
    IpAddr ar_tpa;
  } ArpPacket;


/* ar_hrd value for Ethernet. */
#define ares_hrd_Ethernet 1

/* Possible opcodes (values of ar_op). */
#define ares_op_REQUEST 1
#define ares_op_REPLY 2

#endif ARP
