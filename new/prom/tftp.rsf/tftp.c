/* TFTP I/O routines. */
/* Ross Finlayson, March 1984. */

/* Exports:	TftpRead(). */

#include "../tftpnetboot/tftpnetboot.h"
#include "../tftpnetboot/tftp.h"
#include "../tftpnetboot/enet.h"

static copy(dst, src, length)
    /* Does the obvious thing. */
    register char *dst, *src;
    register length;
  {
    while (length-- > 0)
      {
	*dst++ = *src++;
      }
  }

static char *strcopy(to, from)
    register char to[], from[];
/*
 * Copies the string "from" into "to", returning the address of the next
 * empty position in "to".
 */
  {
    while (*to++ = *from++)
        ;
    return(to);
  }

/*
 * The following 3 routines are used to send packets of various kinds. In each
 * case "sourceIpAddress" is our internet address, "destIpAddress" is the
 * address to send to, and "sourceTID" and "destTID" are the source and
 * destination transfer ids to include in the UDP packet. "tftpPacket" is a
 * pointer to the TFTP packet to be sent.
 */
static int tftpSendRRQ(sourceIpAddress, destIpAddress, destEnetAddress,
		       sourceTID, destTID,
		       fileName, mode, tftpPacket)
    IpAddr sourceIpAddress, destIpAddress;
    EnetAddr destEnetAddress;
    unsigned short sourceTID, destTID;
    char fileName[], mode[];
    TftpPacket *tftpPacket;
/*
 * A "read request" packet. "filename" is the name of the file to be read,
 * in "mode" mode.
 */
  {
    char *modePos, *endOfPacket;

    tftpPacket->opcode = RRQ;

    /* Copy the file name into the packet. */
    modePos = strcopy(tftpPacket->params.fileName_and_mode, fileName);
	
    /* Copy the 'mode' string parameter into the packet. */
    endOfPacket = strcopy(modePos, mode);

    /* Do a UDP send. */
    return(UdpSend((char *)tftpPacket,
    		   endOfPacket-(char *)tftpPacket, /* length of UDP data */
		   sourceIpAddress, destIpAddress, destEnetAddress,
		   sourceTID, destTID));
  }

static int tftpSendERROR(sourceIpAddress, destIpAddress,  destEnetAddress,
			 sourceTID, destTID,
			 errorCode, tftpPacket)
    IpAddr sourceIpAddress, destIpAddress;
    EnetAddr destEnetAddress;
    unsigned short sourceTID, destTID;
    short errorCode;
    TftpPacket *tftpPacket;
/* An "error" packet. */
  {

    tftpPacket->opcode = ERROR;
    tftpPacket->params.errorParams.errorCode = errorCode;
    /* Don't bother inserting an error message: */
    tftpPacket->params.errorParams.errMsg[0] = NULL;
    return(UdpSend((char *)tftpPacket,
		   sizeof(tftpPacket->opcode)
		    + sizeof(tftpPacket->params.errorParams.errorCode)
		    + 1,
		   sourceIpAddress, destIpAddress, destEnetAddress,
		   sourceTID, destTID));
  }

static int tftpSendACK(sourceIpAddress, destIpAddress,  destEnetAddress,			       sourceTID, destTID,
		       blockNum, tftpPacket)
    IpAddr sourceIpAddress, destIpAddress;
    EnetAddr destEnetAddress;
    unsigned short sourceTID, destTID;
    short blockNum;
    TftpPacket *tftpPacket;
/* An "acknowledgement" packet. */
  {

    tftpPacket->opcode = ACK;
    tftpPacket->params.ackBlockNum = blockNum;
    return(UdpSend((char *)tftpPacket,
		   sizeof(tftpPacket->opcode)
		    + sizeof(tftpPacket->params.ackBlockNum),
		   sourceIpAddress, destIpAddress, destEnetAddress,
		   sourceTID, destTID));
  }


Boolean TftpRead(ourIpAddress, hostIpAddress, hostEnetAddress,
	 	 fileName, readBuffer, numCharsRead)
    IpAddr ourIpAddress, hostIpAddress;
    EnetAddr hostEnetAddress;
    char fileName[], readBuffer[];
    int *numCharsRead;
/* Uses TFTP to read the file named "fileName" from host "hostIpAddress"
 * into the buffer "readBuffer". The number of characters read is returned
 * in "numCharsRead". FALSE is returned iff a fatal error occurs.
 */
  {
    unsigned short remoteHostTID = INITIAL_DEST_TID; /* Initially */
    unsigned short OUR_TID;
    Boolean firstTime = TRUE; 
    Boolean timedOut = FALSE; 
    short expectedNextBlockNum = 1;
    short waster = 0;	/* *JEP* */
    char packet[TOTAL_PACKET_SIZE];
    /* The storage for the packets that will be sent/received. */
    register TftpPacket *tftpPacket
    	 = (TftpPacket *)&packet[TOTAL_PACKET_SIZE - sizeof(TftpPacket)];

    /* first establish a random number for OUR_TID */

    OUR_TID = (enRandomI() && 0xffff);

    /* Send a Read Request (RRQ) packet to the remote host. */


    if (!tftpSendRRQ(ourIpAddress, hostIpAddress, hostEnetAddress,
		     OUR_TID, remoteHostTID,
		     fileName, TFTP_MODE, tftpPacket))
      {
	return(FALSE);
      }

    /* At this stage, we haven't transferred anything. */
    *numCharsRead = 0;

    /* Now loop, receiving UDP packets, until done. */
    while (TRUE)
      {
	unsigned short sourcePort, destPort;
	unsigned udpDataLength;
	IpAddr sourceIpAddress;
	EnetAddr sourceEnetAddress;

        if (!UdpReceive((char *)tftpPacket, &udpDataLength,
		   	&sourceIpAddress, ourIpAddress,
			AddressOf(sourceEnetAddress), &sourcePort, &destPort))
	    /* Timeout - if we have not yet received any data, or have
	     * timed out previously on the same data block, then exit
	     * immediately, otherwise re-Ack the last data block that we
	     * received.
	     */
	  {
	    if (firstTime || timedOut)
	      {
		message("Timeout!\n");
		return(FALSE);
	      }
	    timedOut = TRUE;
	    /* Acknowledge the last block correctly received. */
	    tftpSendACK(ourIpAddress, hostIpAddress, hostEnetAddress, OUR_TID,
			remoteHostTID, expectedNextBlockNum-1, tftpPacket);
	  }

	if (destPort != OUR_TID)
	  {
	    /* Send back an ERROR packet. (Code 5 = "unknown transfer ID".) */
	    tftpSendERROR(ourIpAddress, sourceIpAddress, sourceEnetAddress,
	    		  OUR_TID, remoteHostTID, 5, tftpPacket);
	    continue;
	  }

	/* Now act according to the TFTP opcode in the packet. */

	switch(tftpPacket->opcode)
	  {
	    case DATA:
		/*
		 * If this is the first DATA packet received, remember the
		 * source port, the source IP address, and the souurce Ethernet
		 * address (we allow them to be different from those to which
		 * we sent the initial RRQ).
		 */
		if (firstTime)
		  {
		    remoteHostTID = sourcePort;
		    hostIpAddress = sourceIpAddress;
		    hostEnetAddress = sourceEnetAddress;
		    firstTime = FALSE;
		  }
		else if (sourceIpAddress != hostIpAddress)
		  {
		    /* Send back an ERROR packet. (Code 0) */
		    tftpSendERROR(ourIpAddress, sourceIpAddress,
				  sourceEnetAddress, OUR_TID, sourcePort,
				  0, tftpPacket);
		    break;
		  }
		else if (sourcePort != remoteHostTID)
		  {
		    /* Send an ERROR packet. (Code 5 = "unknown transfer ID") */
		    tftpSendERROR(ourIpAddress, sourceIpAddress,
				  sourceEnetAddress, OUR_TID, sourcePort,
				  5, tftpPacket);
		    break;
		  }

		if (tftpPacket->params.dataParams.dataBlockNum
						== expectedNextBlockNum)
		  {
		    /* We have received the packet that we expected. */
		    putchar('!');
		    /* Determine the amount of received data. */
		    udpDataLength
			-= sizeof(tftpPacket->opcode)
			 + sizeof(tftpPacket->params.dataParams.dataBlockNum);

		    ++expectedNextBlockNum; /* Since we got this block OK. */

		    timedOut = FALSE;

		    /* Send back an acknowledgement for this block. */
		    /* Note that by sending the ACK before copying the data to
		     * the buffer, we get some extra parallelism. We can do this
		     * because the parameters for an ACK do not overwrite the
		     * previously received data.
		     */
		    tftpSendACK(ourIpAddress, hostIpAddress, hostEnetAddress,
				OUR_TID, remoteHostTID,
				expectedNextBlockNum - 1, tftpPacket);

		    /* Copy the received data to the buffer. */
		    copy(&readBuffer[*numCharsRead],
			 tftpPacket->params.dataParams.data,
			 udpDataLength);
		    *numCharsRead += udpDataLength;

		    /*
		     * If the size of the block was less than the maximum
		     * possible, then we are done (this was the last block).
		     */
		    if (udpDataLength < MAX_TFTP_DATA)
		      {
			putchar('\n');
		        return(TRUE);
		      }
		  }
		else
		  {
		    /* We received an unexpected block. */
		    /* Acknowledge the last block correctly received. */
		    tftpSendACK(ourIpAddress, hostIpAddress, hostEnetAddress,
				OUR_TID, remoteHostTID,
				expectedNextBlockNum-1, tftpPacket);
		  }
		break;

	    case ERROR:
		/* Throw ERROR packets away (we can't abort here, since many
		 * hosts may receive the initial RRQ, but only some may be able
		 * to respond with a DATA packet).
		 */
		break;

	    default:
		/*
	         * Other packets are also erroneous. If we see such a packet,
		 * we send back an error packet, and continue.
	         */

		/*
		 * Send back an ERROR packet. (Code 4 = "Illegal TFTP
		 * operation".)
		 */
		tftpSendERROR(ourIpAddress, sourceIpAddress, sourceEnetAddress,
			      OUR_TID, sourcePort, 4, tftpPacket);
		break;
	  }
      }
  }
