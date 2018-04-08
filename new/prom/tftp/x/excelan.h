#ifndef EXCELAN.H
#define EXCELAN.H
/*
 *  Ethernet Header for Excelan board:  manifests, structs local to
 *  this Ethernet code.
 * (derived from /usr/sun/src/ipwatch/excelan.h)
 * Note: most of this stuff isn't used in the boot PROM code.
 */

#define	  TRUE	1
#define	  FALSE	0

typedef struct {
   short addrlow, addrmid, addrhigh;
   } NetAddress;

# define ENET_MAX_PACKET 1500

# define NULL	0

#define	  MBIO_VA	( (long) 0x1f0000 )	/* Multibus I/O Virtual Addr. */
#define   DEVADDR	( MBIO_VA + 0 )

/*
 * Ports for interrupting and resetting the Excelan board.  Note the address
 * swap due to byte ordering differences.
 */
#define   PORTA		*(unsigned char *)( DEVADDR + 1 )
#define   PORTB		*(unsigned char *)( DEVADDR + 0 )

/*
 * PORTB bits when read. (Bits 7-4 and 2 are undefined.
 */
#define   X_NOT_READY	0x8		/* '0' means Exos is ready. */
#define   X_INT		0x2		/* '1' means Exos is interrupting. */
#define   X_ERROR	0x1		/* '0' means error condition--or

					       hasn't responded to reset. */

#define   ENET_INT_LEVEL	INT3

/*
 * Link Level Controller Commands and Messages.
 */

#define   NET_MODE	0x8	/* Get and/or Set mode of connection. */
#define   NET_ADDRS	0x9	/* Get and/or Set contents of a slot. */
#define	  NET_RECV	0xa	/* Enable/disable reception on a slot. */
#define   NET_STATS	0xb	/* Get and/or reset board statistics. */
#define   NET_XMIT	0xc	/* Transmit a packet. */
#define   NET_READ	0xd	/* Request an incoming packet. */
#define	  NET_XMIT_SELF	0xe	/* Transmit and receive our own packets. */

/*
 * Request Mask Bits for use with MODE, ADDRS, RECV, and STATS commands.
 */

#define   WRITEREQ	1
#define   READREQ	2
#define   ENABLE_RECV	4

/*
 * Numbers of request and reply buffers.  Note that three buffers for each
 * allows a read, a write, and a stats command all to be outstanding at
 * the same time.  At the current time, other commands are done only at
 * initialization, and busy-waiting is done for their replies.  Because
 * we open only one slot for reading, and provide only one data buffer
 * for incoming packets, there is not yet any reason to have more than
 * one read request outstanding.
 */

/* Actually, for booting one request buffer and one reply buffer suffices. */
#define   NREQBUFS	1
#define   NREPBUFS	1

/* 
 * 50 millisecond wait before tossing an unclaimed incoming packet.
 */

#define   ENET_READER_TIMEOUT	5

/*
 * Buffer status descriptions.
 */

#define   EXOS_OWNER	1	/* Bit 0 of status is owner bit. */
#define   HOST_OWNER	0
#define   EXOS_DONE	0	/* Bit 1 is whether previous owner has */
#define   HOST_DONE	2	/* finished what he had to do. */



/* Physical Address Slot--see documentation for other slots. */
#define	  PHYS_ADDRS_SLOT	253

/* Time allowed for response by board to a busy-waiting request. */ 
#define   X_TIMEOUT	100000

/* Packets smaller than this are messed up--the true size is almost
 * definitely larger, but this is what I inherited.--EJB
 */
#define   MAGIC_MIN_PACKET_SIZE		20

/* Number of values returned by a NET_STATS call. */
#define   NUM_STATS_OBJS	8


/*
 * Test pattern.  Used by Exos to determine byte order.
 */

struct testp
  {
	char	b0,b1,b2,b3;
	short	w0,w1;
	long	l0;
  };

/*
 * Exos configuration message--pages 24 and 25 of our manual.
 */

struct Init
  {
	short	res0;		/* reserved */
	char	vers[4];	/* Exos version */
	char	code;		/* completion code */
	char	mode;		/* operating mode */
	char	format[2];	/* data format option */
	char	res1[3];	/* reserved */
	char	amode;		/* address mode */
	char	res2;		/* reserved */
	char	mapsize;	/* map size */
	char	mmap[32];		/* memory map, in reply message. */
	long 	mvblk;		/* movable block addresses */
	char	nproc;		/* number of processes */
	char	nmbox;		/* number of mailboxes */
	char	naddr;		/* number of address slots */
	char	nhost;		/* number of hosts */
	long	hxseg;		/* Host to Exos request queue base address */
	short	hxhead;		/* Host to Exos request queue header */
	char	hxitype;	/* Host to Exos interrupt type */
	char	hxival;		/* Host to Exos interrupt value */
	long	hxiaddr;	/* Host to Exos interrupt address */
	long	xhseg;		/* Exos to Host request queue base address */
	short	xhhead;		/* Exos to Host request queue header */
	char	xhitype;	/* Exos to Host interrupt type */
	char	xhival;		/* Exos to Host interrupt value */
	long	xhiaddr;	/* Exos to Host interrupt address */
  };



/*
 * Host to Exos Message Formats--Requests.  Note that each starts with
 * a common set of fields:  link, status, res, and length.  These fields
 * are maintained in all buffers, and adjusted depending on the message
 * and data to be stored in the buffer.
 */

struct ModeMsg				/* NET_MODE. */
  {
	unsigned short	link;		/* Link to next buffer. */
	char		res;		/* Reserved. */
	char		status;		/* Buffer status--see manifests above.*/
	unsigned short	length;		/* Length of message itself. */
	short		pad;
	long		id;		/* Host set message identifier. */
	char		request;	/* Command. */
	char		reply;		/* Reply and error indicator. */
	char		reqmask;	/* Request mask. */
	char		errmask;	/* Options mask--error packets wanted?*/
	char		mode;		/* Connection mode. */
	char		nop;
  };

struct AddrsMsg				/* NET_ADDRS. */
  {
	unsigned short	link;		/* Link to next buffer. */
	char		res;		/* Reserved. */
	char		status;		/* Buffer status--see manifests above.*/
	unsigned short	length;		/* Length of message itself. */
	short		pad;
	long		id;		/* Host set message identifier. */
	char		request;	/* Command. */
	char		reply;		/* Reply and error indicator. */
	char		reqmask;	/* Request mask. */
	char		slot;		/* Slot to fill or read. */
	NetAddress	addr;		/* Contents of slot. */
  };

struct RecvMsg				/* NET_RECV. */
  {
	unsigned short	link;		/* Link to next buffer. */
	char		res;		/* Reserved. */
	char		status;		/* Buffer status--see manifests above.*/
	unsigned short	length;		/* Length of message itself. */
	short		pad;
	long		id;		/* Host set message identifier. */
	char		request;	/* Command. */
	char		reply;		/* Reply and error indicator. */
	char		reqmask;	/* Request mask. */
	char		slot;		/* Slot to enable or disable. */
  };

struct StatsMsg				/* NET_STATS. */
  {
	unsigned short	link;		/* Link to next buffer. */
	char		res;		/* Reserved. */
	char		status;		/* Buffer status--see manifests above.*/
	unsigned short	length;		/* Length of message itself. */
	short		pad;
	long		id;		/* Host set message identifier. */
	char		request;	/* Command. */
	char		reply;		/* Reply and error indicator. */
	char		reqmask;	/* Request mask. */
	char		res1;		/* Reserved. */
	short		nobj;		/* # of stats to read/reset. */
	short		index;		/* First stat to read/reset. */
	long		bufaddr;	/* Buffer to store stats read. */
  };


/*
 * Transmit and receive can specify from 1 to 8 data blocks from/to which
 * the packet is to be read/written.
 */

struct	Block
  {
	short	len;			/* Length of block. */
	long	addr;			/* Address of block. */
  };

struct XmitMsg				/* NET_XMIT and NET_XMIT_SELF. */
  {
	unsigned short	link;		/* Link to next buffer. */
	char		res;		/* Reserved. */
	char		status;		/* Buffer status--see manifests above.*/
	unsigned short	length;		/* Length of message itself. */
	short		pad;
	long		id;		/* Host set message identifier. */
	char		request;	/* Command. */
	char		reply;		/* Reply and error indicator. */
	char		slot;		/* Where to receive this packet. */
	char		nblocks;	/* Number of blocks holding packet. */
	struct Block	block[8];	/* The blocks. */
  };

struct ReadMsg				/* NET_READ. */
  {
	unsigned short	link;		/* Link to next buffer. */
	char		res;		/* Reserved. */
	char		status;		/* Buffer status--see manifests above.*/
	unsigned short	length;		/* Length of message itself. */
	short		pad;
	long		id;		/* Host set message identifier. */
	char		request;	/* Command. */
	char		reply;		/* Reply and error indicator. */
	char		slot;		/* Where this packet was received. */
	char		nblocks;	/* Number of blocks to hold packet. */
	struct Block	block[8];	/* The blocks. */
  };


/*
 * Host's view of the request and reply queues.
 */

struct MsgBuf
  {
	struct XmitMsg	msg;		/* Each buffer has a message--no */
					/* longer than the transmit message, */
	struct MsgBuf	*next;		/* and a pointer to the next buffer. */
  };

struct MsgQs
  {
	unsigned short	xreqhdr;	/* Exos's request queue head--next */
					/* request will be read from here. */
	unsigned short	xreptail;	/* Exos's reply queue tail--next   */
					/* reply will be stored here.      */
	struct MsgBuf	*hreqtail;	/* Host's request queue tail--next */
					/* request will be stored here.    */
	struct MsgBuf	*hrephdr;	/* Host's reply queue head--next   */
					/* reply will be read from here.   */
	struct MsgBuf	requestbufs[NREQBUFS];
					/* Request buffers.                */
	struct MsgBuf	replybufs[NREPBUFS];
					/* Reply buffers.                  */
  };

/*
 * Areas of Multibus memory for buffering incoming packets, outgoing
 * packets, and command replies.
 */

struct StatsArray		/* fr means Frames. */
  {
    long  frSent;
    long  frAbortExcessCollisions;
    long  undef;
    long  timeDomainReflec;
    long  frRecvdNoError;
    long  frRecvdAlignError;
    long  frRecvdCRCError;
    long  frLostNoBuffers;
  };


struct SystemBuffers
  {
	char		readbuffer[ENET_MAX_PACKET];
	char		writebuffer[ENET_MAX_PACKET];
	char		statsbuffer[sizeof(struct StatsArray)];
  };



/*
 * Part of a MsgBuf common to all messages. (Declared for the convenience
 * of MSGLEN.)
 */

struct common
  {
	unsigned short	link;		/* Link to next buffer. */
	char		res;		/* Reserved. */
	char		status;		/* Buffer status--see manifests above.*/
	unsigned short	length;		/* Length of message itself. */
  };

#define   MSGLEN	( sizeof( struct XmitMsg ) - sizeof( struct common ) )
	

/*
 * Macro definitions--for converting virtual to physical addresses, and
 * for calculating offsets from the queue base addresses for Exos.
 */

#define   VirToPhys( VA )	( (long) ( ((unsigned) VA - (unsigned)\
				VMultibusMemAddr) + MultibusMemAddr ) )
#define   HostToExAddr( a )	((unsigned int)a - (unsigned int)MsgQVirSegAddr)
#define   ExToHostAddr( a )	((unsigned int)a + (unsigned int)MsgQVirSegAddr)

/*
 * Error status bits on read (return codes)
 * Also used in the option mask field
 */
# define PacketTooLong 0x04
# define AlignmentError 0x10
# define CRCError 0x20

/*
 * Net mode field
 */
# define Promiscuous 3
# define CONNECT_TO_NET	2


#endif EXCELAN.H


/* NOTE:
 * The following global variable declarations are omitted, being replaced by
 * the manifest definitions that follow. (These definitions replace variable
 * assignments that originally appeared in "enopen"). This was done to
 * reduce code space, and because this code is intended to be put in ROM (we
 * can't assign to a global variable in ROM!).
 */

#ifdef USE_GLOBALS
/*
 * Significant addresses referring to Multibus memory.
 */

unsigned	MultibusMemAddr;	/* Base address of my area of */
					/* Multibus memory. */
char		*VMultibusMemAddr;	/* Virtual address of above. */

char		*ReadDataBuf;		/* Buffer for incoming packets. */
char		*WriteDataBuf;		/* Buffer for outgoing packets. */
char		*StatsBuffer;		/* Buffer for statistics replies. */

struct MsgQs	*MsgQPtr;		/* Start of request/reply queues. */
unsigned	MsgQVirAddr;		/* Virtual address of same. */
unsigned	MsgQPhysAddr;		/* Physical address of same. */
unsigned	MsgQVirSegAddr;		/* Start of segment of same--Exos's */
					/*  16 bit offsets are calculated */
					/*  from here.  Luckily, the */
					/*  AllocateMultibusMemory routine */
					/*  returns areas which begin or end*/
					/*  on a segment boundary. */
#endif USE_GLOBALS

#define MultibusMemAddr 0x0000
#define VMultibusMemAddr 0x100000
#define systembuf ( (struct SystemBuffers *) VMultibusMemAddr )
#define ReadDataBuf ( systembuf->readbuffer )
#define WriteDataBuf ( systembuf->writebuffer )
#define StatsBuffer ( systembuf->statsbuffer )
#define MsgQVirAddr ( (long)(VMultibusMemAddr+sizeof(struct SystemBuffers)) )
#define MsgQPhysAddr ( VirToPhys(MsgQVirAddr) )
#define MsgQVirSegAddr ( VMultibusMemAddr )
#define MsgQPtr ( (struct MsgQs *) MsgQVirAddr )
