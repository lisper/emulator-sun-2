#ifndef UDP
#define UDP

/* UDP definitions. */


/* Assigned IP protocol number. */
#define UDP_PROTOCOL_NUM 17

/* UDP header format. */
typedef struct
  {
    unsigned short sourcePort, destPort;
    unsigned short length;
    unsigned short checksum;
  } UdpHeader;


/* UDP packet format - Use for overlays only! */
typedef struct
  {
    UdpHeader udpHeader;
    char udpData[1];
  } UdpPacket;


#endif UDP
