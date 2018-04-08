/*	ether.h	6.3	84/03/20	*/

/*
 * Ethernet address - 6 octets
 */
struct ether_addr {
	u_char	ether_addr_octet[6];
};

/*
 * Structure of a 10Mb/s Ethernet header.
 */
struct	ether_header {
	struct	ether_addr ether_dhost;
	struct	ether_addr ether_shost;
	u_short	ether_type;
};

#define	ETHERTYPE_PUPTYPE	0x0200		/* PUP protocol */
#define	ETHERTYPE_IPTYPE	0x0800		/* IP protocol */
#define ETHERTYPE_ARPTYPE	0x0806		/* Addr. resolution protocol*/

#define	ETHERMTU	1500
#define	ETHERMIN	(60-14)

/*
 * Ethernet Address Resolution Protocol.
 *
 * See RFC 826 for protocol description.  Structure below is adapted
 * to resolving internet addresses.  Field names used correspond to 
 * RFC 826.
 */
struct	ether_arp {
	u_short	arp_hrd;	/* format of hardware address */
#define ARPHRD_ETHER 	1	/* ethernet hardware address */
	u_short	arp_pro;	/* format of proto. address (ETHERPUP_IPTYPE) */
	u_char	arp_hln;	/* length of hardware address (6) */
	u_char	arp_pln;	/* length of protocol address (4) */
	u_short	arp_op;
#define	ARPOP_REQUEST	1	/* request to resolve address */
#define	ARPOP_REPLY	2	/* response to previous request */
#define RARPOP_REQUEST	3	/* Reverse ARP request (RARP packets only!) */
#define RARPOP_REPLY	4	/* Reverse ARP reply (RARP packets only!) */
	u_char	arp_xsha[6];	/* sender hardware address */
	u_char	arp_xspa[4];	/* sender protocol address */
	u_char	arp_xtha[6];	/* target hardware address */
	u_char	arp_xtpa[4];	/* target protocol address */
};
#define	arp_sha(ea)	(*(eaddr_t *)(ea)->arp_xsha)
#define	arp_spa(ea)	(*(iaddr_t *)(ea)->arp_xspa)
#define	arp_tha(ea)	(*(eaddr_t *)(ea)->arp_xtha)
#define	arp_tpa(ea)	(*(iaddr_t *)(ea)->arp_xtpa)
