#ifndef IP
#define IP

/* IP definitions. */
/* Ross Finlayson, March 1984. */

typedef unsigned char Bit8;
typedef unsigned short Bit16;

/* IP address type. */
typedef unsigned long IpAddr;

/* IP address to be used for broadcasting on the local subnet. */
#define IP_SUBNET_BROADCAST 0xFFFFFFFF

/* IP address to be used in the source field if the real source IP address isn't
 * known (this probably won't work).
 */
#define DEFAULT_SOURCE_IP_ADDRESS 0

/* Ether-type for IP. */
#ifdef ENET10
#define IP_ETHERTYPE 0x800
#else ENET10
#define IP_ETHERTYPE 01001
#endif ENET10

#define CURRENT_IP_VERSION	4


/* (The following definition was snarfed from
 * /ng/ng/xV/servers/internet/iptcp.h)
 */
typedef struct		/* IP packet header structure.
			   See Internet handbook for defns.
			   of the various fields. */
  {
    unsigned version: 4;
    unsigned ihl: 4;		/* Internet header length */
    unsigned tos: 8;		/* Type of service */
    Bit16 tl;			/* Total length */
    Bit16 id;			/* Identifier */
    unsigned short fragmentStuff;
    Bit8 ttl;			/* Time to live */
    Bit8 prot;
    Bit16 checksum;
    IpAddr srcAdr;
    IpAddr dstAdr;
  } IpHeader;


/* IP packet format - Use for overlays only! */
typedef struct
  {
    IpHeader ipHeader;
    char ipData[1];
  } IpPacket;


#endif IP
