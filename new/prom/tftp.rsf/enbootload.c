/* Top level TFTP boot routine. */
/* Ross Finlayson, March 1984. */

/* Exports:	enbootload(). */

#include "../h/sunmon.h"
#include "../tftpnetboot/tftpnetboot.h"
#include "../tftpnetboot/ip.h"
#include "../tftpnetboot/enet.h"

static char *findField(from)
    register char *from;
    /* Looks in the string starting at "from" for NULL, the input delimiter,
     * or '\r'. If NULL is found, then this location is returned. If '\r' or
     * the input delimiter is found, then it is changed to NULL (to mark the
     * end of the previous string), and the next location is returned.
     */
  {
    while (*from)
      {
        if (*from == INPUT_DELIMITER || *from == '\r')
	  {
	    *from = NULL;
	    return(++from);
	  }
	++from;
      }
    return(from);
  }

static Boolean parseIpAddress(str, ipAddr)
    register char *str;
    IpAddr *ipAddr;
    /* Scans the string "str", looking for an internet address of the form
     * "x.y.z.w" (where x, y, z and w are decimal). If such an address is
     * found, then it is returned in "ipAddr".
     */
  {
    register unsigned a = 0;
    register char c;
    unsigned dotCount = 0;

    *ipAddr = 0;
    while (TRUE)
      {
	if ((c = *str++) >= '0' && c <= '9')
	    a = (a<<3) + (a<<1) + c - '0';	/* ie. a*10 +c -'0' */
	else
	    return (FALSE);
	while ((c = *str++) >= '0' && c <= '9')
	    a = (a<<3) + (a<<1) + c - '0';
	*ipAddr = (*ipAddr<<8) + a;
	a = 0;
	if (c == 0)
	    return (dotCount == 3);
	else if (c == '.')
	    ++dotCount;
	else
	    return (FALSE);
      }
  }

static Boolean parseHexAddress(str, addr)
    register char *str;
    Address *addr;
    /* Converts the string "str" (representing a hexadecimal address) into
     * numeric form, returning the result in "addr".
     */
  {
    register char c, d;

    *addr = 0;
    while ((c = *str++) != NULL && c != '\r')
	if ((d = ishex(c)) >= 0)
	    *addr = (*addr<<4) | d;
	else
	    return (FALSE);
    return (TRUE);
  }


int enbootload(strInput)
    register char *strInput;
    /* Parses the 0-terminated input string "strInput" into a file name, and
     * (optionally) a server internet address, our own internet address, and
     * a starting address. We then attempt to load the named file using TFTP.
     * 0 is returned if if the requested file cannot be loaded, otherwise the
     * entry point of the loaded program.
     */
  {
    char *hostAddrStr, *ourAddrStr, *locStr;
    IpAddr ourIpAddress, hostIpAddress, serverIpAddress;
    EnetAddr hostEnetAddress;
    Address startingAddress;
    unsigned numCharsRead;
    char *invalidIpAddressMsg;

    invalidIpAddressMsg = "Bad IP address!\n";

    /* Make sure the Ethernet controller is there, and if so, open it. */
    if (!enopen())
      {
        message("Can't open Ethernet!\n");
	return(0);
      }

    /* Break the input string into its components. */
    hostAddrStr = findField(strInput);
    ourAddrStr = findField(hostAddrStr);
    locStr = findField(ourAddrStr);

    /* Find our internet address. This is obtained from the input string, if
     * an internet address was given, otherwise 'reverse ARP' is used.
     */
    if (*ourAddrStr)
      {
        if (!parseIpAddress(ourAddrStr, &ourIpAddress))
	  {
	    message(invalidIpAddressMsg);
	    return(0);
	  }
      }
    else
      {
	if (!ReverseArp(&ourIpAddress, &serverIpAddress))
	  {
            message("Warning: RARP failed\n");
	    return(0);
	  }
      }

    /* If a host internet address was given, then use it, otherwise assume
     * that we will use the Ip addresss of the reverse arp server.
     */
    if (*hostAddrStr)
      {
        if (!parseIpAddress(hostAddrStr, &hostIpAddress))
	  {
	    message(invalidIpAddressMsg);
	    return(0);
	  }
      }
    else
        hostIpAddress = serverIpAddress;

    /* Try to find the Ethernet address to send to. If we can't find
     * this, or if the user didn't give a host internet address, then use the
     * Ethernet broadcast address instead.
     */

    if (!Arp(hostIpAddress, AddressOf(hostEnetAddress), ourIpAddress))
	SetToBroadcastAddress(hostEnetAddress);

    /* If a starting (memory) address was given, then use it, otherwise use the
     * default starting address.
     */
    if (*locStr)
      {
        if (!parseHexAddress(locStr, &startingAddress))
	  {
	    message("Bad starting address!\n");
	    return(0);
	  }
      }
    else
        startingAddress = USERCODE;

    /* Attempt to load the requested file using TFTP. */
    if (TftpRead(ourIpAddress, hostIpAddress, hostEnetAddress, strInput,
	         startingAddress, &numCharsRead))
        return (setup(startingAddress, numCharsRead));
    else
        return (0);
  }
