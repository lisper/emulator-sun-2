/*	il.h	1.0	84/09/06	*/

/*
 * Interlan NI3210 (Intel 82586) registers (and some driver structures).
 */

/*
 * Copyright (C) 1984, Stanford SUMEX project.
 * May be used, but not sold without permission.
 */

/*
 * history
 * 09/06/84	croft	created.
 */


struct csregs {			/* command and status registers */
	u_char	cs_2;		/* command/status 2 */
	u_char	cs_1;	
	u_char	cs_xxx[6];	/* unused */
	u_short	cs_bar;		/* DMA buffer address register */
	u_short	cs_mar[2];	/* ..  memory   ..      ..     */
	u_short	cs_count;	/* DMA count */
};

/* cs_1 bits */
#define	CS_DO		0x80	/* start DMA */
#define	CS_DONE		0x40	/* DMA done */
#define	CS_TWORDS	0x20	/* DMA command, transfer words */
#define	CS_TSWAP	0x10	/*  ..  ..	swap bytes */
#define	CS_TWRITE	0x08	/*  ..  ..	write */
#define	CS_TREAD	0	/*  ..  ..	read */
#define	CS_INTA		0x07	/* DMA int level */

/* cs_2 bits */
#define	CS_CA		0x80	/* channel attention (W), interrupt (R) */
#define	CS_ONLINE	0x40	/* online */
#define	CS_SWAP		0x20	/* swap bytes */
#define	CS_IEDMA	0x10	/* enable DMA ints */
#define	CS_NRESET	0x08	/* not reset */
#define	CS_INTB		0x07	/* interrupt level */

#define	CS(bits)	CSOFF(bits|CS_ONLINE) /* bits to set in cs_2 */
#define	CSOFF(bits)	cs->cs_2 = (bits|CS_NRESET|INTIL)


/*
 * Transmit/receive buffer descriptor.
 */
struct bd {
	u_short	bd_count;	/* data count */
	u_short	bd_next;	/* link to next */
	u_short	bd_buf;		/* buffer pointer */
	u_short	bd_bufhi;
	u_short	bd_size;	/* buffer size (rbd only) */
};

/* bd flags */
/* in bd_count */
#define	BD_EOF		0x8000	/* end of frame */
#define	BD_F		0x4000	/* filled by 82586 */
#define	BD_COUNT	0x3FFF	/* count field */
/* in bd_size */
#define	BD_EL		0x8000	/* end of list */


/*
 * Command block / receive frame descriptor.
 */
struct cb {
	u_short	cb_status;	/* status */
	u_short	cb_cmd;		/* command */
	u_short	cb_link;	/* link to next */
	u_short	cb_param;	/* parameters here and following */
};

/* cb bits */
/* status */
#define	CB_C		0x8000	/* complete */
#define	CB_B		0x4000	/* busy */
#define	CB_OK		0x2000	/* ok */
#define	CB_ABORT	0x1000	/* aborted */
/* command */
#define	CB_EL		0x8000	/* end of list */
#define	CB_S		0x4000	/* suspend */
#define	CB_I		0x2000	/* interrupt (CX) when done */
/* action commands */
#define	CBC_IASETUP	1	/* individual address setup */
#define	CBC_CONFIG	2	/* configure */
#define	CBC_TRANS	4	/* transmit */


/*
 * System control block, plus some driver static structures.
 */
struct scb {
	u_short	sc_status;	/* status */
	u_short	sc_cmd;		/* command */
	u_short	sc_clist;	/* command list */
	u_short	sc_rlist;	/* receive frame list */
	u_short	sc_crcerrs;	/* crc errors */
	u_short	sc_alnerrs;	/* alignment errors */
	u_short	sc_rscerrs;	/* resource errors (lack of rfd/rbd's) */
	u_short	sc_ovrnerrs;	/* overrun errors (mem bus not avail) */
				/* end of 82586 registers */
	struct cb sc_cb;	/* general command block */
	u_char	sc_data[MAXDATA]; /* transmit buffer/general data */
	struct bd sc_tbd;	/* transmit buffer descriptor */
	struct cb sc_rfd[NRFD];	/* receive frame descriptors */
	struct bd sc_rbd[1];	/* first receive buffer descriptor */
};

/* status bits */
#define	SC_CX		0x8000	/* command executed */
#define	SC_FR		0x4000	/* frame received */
#define	SC_CNR		0x2000	/* cmd unit not ready */
#define	SC_RNR		0x1000	/* rec  .. */
#define	SC_CUS		0x700	/* command unit status field */
#define	SC_RUS		0x70	/* receive unit status field */
#define	SC_RESET	0x80	/* software reset */

/* command unit status */
#define	CUS_IDLE	0x0	/* idle */
#define	CUS_SUSP	0x100	/* suspended */
#define	CUS_READY	0x200	/* ready */

/* receive unit status */
#define	RUS_IDLE	0x0	/* idle */
#define	RUS_SUSP	0x10	/* suspended */
#define	RUS_NORES	0x20	/* no resources */
#define	RUS_READY	0x40	/* ready */

/* command unit commands */
#define	CUC_START	0x100	/* start */
#define	CUC_ABORT	0x400	/* abort */

/* receive unit commands */
#define	RUC_START	0x10	/* start */
#define	RUC_ABORT	0x40	/* abort */

#define	STOA(type,off)	((type)(((caddr_t)scb)+(off)))/* scb offset to addr */
#define	ATOS(addr)	((int)(addr))	/* addr to scb offset */

#define	SCBLEN		0x2000	/* length of scb and structures */
#define	ISCPOFF		SCBLEN-18 /* offset of ISCP/SCP */


/*
 * System configuration pointer (ISCP and SCP) at end of memory.
 */
short	iliscp[] = {
	1,			/* busy */
	0,			/* scb at offset 0, base 0 */
	0,
	0,
	0,			/* 16 bit bus */
	0,
	0,
	ISCPOFF,		/* iscp address */
	0
};


/*
 * Configuration command data (from page 4-5 of NI3210 manual).
 */
short	ilconfig[] = {
	0,	CB_EL|CBC_CONFIG,
	0,	0x080c,
	0x2e00,	0x6000,
	0xf200,	0x0000,
	0x0040
};
