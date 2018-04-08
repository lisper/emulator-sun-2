#ifndef ENET
#define ENET

/* Ethernet definitions. */
/* Ross Finlayson, March 1984. */

/* WARNING: don't read this without an airsick bag handy. */
#ifdef ENET10
typedef unsigned short EnetAddr[3];	/* 48 bits */
typedef EnetAddr EnetAddrPtr;
#define AddressOf(enetaddr) enetaddr
#define Deref(enetaddrptr) enetaddrptr
#define CopyEnetAddr(to, from) (to[0]=from[0], to[1]=from[1], to[2]=from[2])
#define EqualEnetAddrs(e1, e2) (e1[0]==e2[0] && e1[1]==e2[1] && e1[2]==e2[2])
#define SetToBroadcastAddress(e) (e[0]=0xffff, e[1]=0xffff, e[2]=0xffff)

#else ENET10
typedef char EnetAddr;		/* 8 bits. */
typedef EnetAddr *EnetAddrPtr;
#define AddressOf(enetaddr) &enetaddr
#define Deref(enetaddrptr) *enetaddrptr
#define CopyEnetAddr(to, from) to=from
#define EqualEnetAddrs(e1, e2) (e1==e2)
#define SetToBroadcastAddress(e) e=0
#endif ENET10

typedef unsigned short EtherType;

/* Minimum Ethernet packet size (in shorts - this is somewhat arbitrary). */
#define ENET_MIN_PACKET 50

/* Ethernet header format. */
typedef struct
  {
    EnetAddr destAddr, sourceAddr;
    EtherType etherType;
  } EnetHeader;

/* Ethernet packet format - Use for overlays only! */
typedef struct
  {
    EnetHeader enetHeader;
    char enetData[1];
  } EnetPacket;


#endif ENET
