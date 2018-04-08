#ifndef TFTP
#define TFTP

/* TFTP definitions. */

/* Maximum amount of data in a TFTP packet. */
#define MAX_TFTP_DATA 512

/* Destination transfer ID (TID) to be used for the initial packet. */
#define INITIAL_DEST_TID 69

/* The 'mode' of TFTP transfer. */
#define TFTP_MODE "octet"

/* Possible TFTP opcodes (2 bytes each). */
#define RRQ	1
#define WRQ	2
#define DATA	3
#define ACK	4
#define ERROR	5

/* TFTP packet format (can be used for storage). */
typedef struct
  {
    short opcode;
    union
      {
	char fileName_and_mode[1]; /* If opcode is RRQ or WRQ. */
	struct
	  {
	    short dataBlockNum;
	    char data[MAX_TFTP_DATA];
	  } dataParams; /* If opcode is DATA. */
			/* (Note: this sets the union size.) */
	short ackBlockNum; /* If opcode is ACK. */
	struct
	  {
	    short errorCode;
	    char errMsg[1];
	  } errorParams; /* If opcode is ERROR. */
      } params;
  } TftpPacket;



#endif TFTP
