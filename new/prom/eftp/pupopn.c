/* Copyright (C) 1981 by Stanford University */


/*
 * pupopn.c
 *
 * minimal ethernet bootstrap loader package
 *
 * pupopn returns an open Pup Channel between two
 * a socket on this host, and a host/net/socket over
 * some network.  
 *
 *
 * Jeffrey Mogul & Dan Kolkowitz	12-January-1981
 *					25-january-1981
 *					24-february-1981
 *
 */

#include <puppacket.h>
#include <pupconstants.h>
#include <pupstatus.h>

pupopn(Pchan,srcsock,Dst)
register struct	PupChan	*Pchan;		/* channel data structure */
register Socket	srcsock;		/* source socket */
register struct	Port	*Dst;		/* destination host/net/socket */
{	/* */
	Pchan->ImmHost = Dst->host;

	PortCopy(Dst,&Pchan->DstPort);

	/* FOLLOWING IS UNIX SPECIFIC (ifid and ofid get same fid) !!! */
	Pchan->ifid = enopen();
	Pchan->ofid = Pchan->ifid;
	Pchan->SrcPort.host = engethost(Pchan->ifid);
/*	Pchan->SrcPort.net = getnet(Pchan->ifid); */
	Pchan->SrcPort.net = 0;	/* caller must fix this if necessary!! */
	Pchan->SrcPort.socket = srcsock;

}
