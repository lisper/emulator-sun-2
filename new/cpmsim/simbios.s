	cpu	68000
	title	"simbios"
	;;
	;; Rump BIOS for use with cpm400.bin produced from S-records
	;; 
_ccp:	equ	$4b8
	
DISK_DMA	equ	$ff0000
DISK_DRIVE	equ	$ff0004
DISK_SECTOR	equ	$ff0008
DISK_READ	equ	$ff000c
DISK_WRITE	equ	$ff0010
DISK_STAT	equ	$ff0014
DISK_FLUSH	equ	$ff0018
	
ACIASTAT	equ	$ff1000
ACIADATA	equ	$ff1002

	segment	code
	org 0
	dc.l	initstack
	dc.l	$400
	
	org	$6000		; BIOS start point

_init:	move.l	#traphndl,$8c	; setup trap #3 handler
	bsr 	charcold
	move.l	#initmsg,a6
	bsr	prtstr
	move.l	#$2,d0		; default disk is drive C
	rts

traphndl:
	move.l	#biosbase,a0
	cmpi	#nfuncs,d0
	bcc	trapng
	lsl	#2,d0
	move.l	(a0,d0),a0
	jsr	(a0)
trapng:	rte

biosbase:
	dc.l	_init
	dc.l	wboot
	dc.l	coninstat
	dc.l	conin
	dc.l	conout
	dc.l	lstout
	dc.l	auxout
	dc.l	auxin
	dc.l	home
	dc.l	seldsk
	dc.l  settrk
        dc.l  setsec
        dc.l  setdma
        dc.l  read  
        dc.l  write 
        dc.l  lststat
        dc.l  sectran
        dc.l  conoutstat
        dc.l  getseg    
        dc.l  getiob    
        dc.l  setiob    
        dc.l  flush     
        dc.l  setexc    
        dc.l  auxinstat 
        dc.l  auxoutstat

nfuncs	equ	(*-biosbase)/4
	
wboot:
	bsr	charwarm
	jmp	_ccp

getseg:	move.l	#memrgn,d0
	rts
	
setexc:
        andi.l  #$ff,d1         ; do only for exceptions 0 - 255
        lsl     #2,d1           ; multiply exception nmbr by 4
        movea.l d1,a0
        move.l  (a0),d0         ; return old vector value
        move.l  d2,(a0)         ; insert new vector
noset:  rts

prtstr: move.b  (a6)+,d1	; simple null terminated string printer
        bsr     conout
        cmp.b   #0,d1
        bne     prtstr
        rts

setiob:	move.w	d1,iobyte
	rts
getiob:	move.w	iobyte,d0
	rts

charcold:
charwarm:
	move.b	#$0,ACIASTAT
	rts

coninstat:
	btst.b	#0,ACIASTAT	; test receive buffer full flag
	bne	conrdy
	move.w	#0,d0
	rts
conrdy:
	move.w	#$ff,d0
	rts
conoutstat:
	btst.b	#1,ACIASTAT
	bne	conrdy
	move.w	#0,d0
	rts
	
conin:	btst.b	#0,ACIASTAT
	beq	conin
	clr	d0
	move.b	ACIADATA,d0
        rts
	
conout:	btst.b	#1,ACIASTAT
	beq	conout
	move.b	d1,ACIADATA
	rts

flush:	move.l	d0,DISK_FLUSH
	clr.l	d0
	rts
	
auxinstat:
auxoutstat:	
lstout:
auxout:
auxin:	clr.l	d0
	rts

home:	clr.w	track
	rts

seldsk:	and.l	#$f,d1
	move.w	d1,drive	; save drive number for later
	asl.l	#2,d1		; index into dph table
	move.l	#dphtab,a0
	move.l	(a0,d1),d0
	rts

settrk:	move.w	d1,track
	rts
setsec:	move.w	d1,sector
	rts

sectran:
	clr.l	d0
	movea.l	d2,a0
	ext.l	d1
	tst.l	d2
	beq	notran
	asl	#1,d1
	move.w	0(a0,d1),d0
	rts
notran:	move.l	d1,d0
	rts

setdma:	move.l	d1,DISK_DMA
	rts

read:	move.l	#DISK_READ,a1	; load read command
	bra	diskio
write:	move.l	#DISK_WRITE,a1	; load write command
	
diskio:	clr.l	d0
	move.w	drive,d0	; get drive number
	move.l	d0,DISK_DRIVE
	asl.l	#2,d0		; index into dph table
	lea.l	dphtab,a0
	move.l	(a0,d0),d0	; get dph address
	beq	diskerr
	move.l	d0,a0
	move.l	14(a0),a0	; load dpb address
	move.w	(a0),d1		; and finally, sectors/track
	move.w	track,d0	; 
	mulu	d1,d0
	clr.l	d1
	move.w	sector,d1
	add.l	d1,d0

	move.l	d0,DISK_SECTOR	; set sector
	move.l	d0,(a1)		; send command
	move.l	DISK_STAT,d0
	rts
diskerr:
	move.l	#1,d0
	rts

	
lststat:
	rts

initmsg:
        dc.b   13,10,"CP/M-68K BIOS Version 0.1",13,10,0

	;; This memory region table allows for testing of the CPM loader
	;; which puts the loaded system at $fe0000

memrgn: dc.w   1       ; 1 memory region
        dc.l   $8000 ; starts at 8000 hex
        dc.l   $f00000  ; goes until f08000 hex

	;; table of dph structures
	;; zero indicates that the drive doesn't exist
dphtab:	dc.l	dph0
	dc.l	dph1
	dc.l	dph2
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0
	dc.l	0

	;; dph0 is for a 8" SS SD compatible file system in drive A
dph0:	dc.l	xlt0
	dc.w	0
	dc.w	0
	dc.w	0
	dc.l	dirbuf
	dc.l	dpb0
	dc.l	ckv0
	dc.l	alv0

dph1:	dc.l	xlt1
	dc.w	0
	dc.w	0
	dc.w	0
	dc.l	dirbuf
	dc.l	dpb1
	dc.l	ckv1
	dc.l	alv1

	;; dph2 is for a fixed disk of 16MB
dph2:	dc.l	0		; no translate
	dc.w	0
	dc.w	0
	dc.w	0
	dc.l	dirbuf
	dc.l	dpb2
	dc.l	0
	dc.l	alv2

	;; NOTE: Because asl automagically enforces alignment after a
	;; dc.b, do not split the values onto separate lines here. That
	;; really screws things up.
dpb0:	dc.w	26		;sectors/track
	dc.b	3,7		; bsh, blm
	dc.b	0,0		; null extent mask, fill
	dc.w	242		; disk size in blocks
	dc.w	63		; directory mask
	dc.w	0
	dc.w	16		; check vector size
	dc.w	2		; track offset
	
dpb1:	dc.w	40		;sectors/track
	dc.b	4,15		; bsh, blm
	dc.b	0,0		; null extent mask, fill
	dc.w	392		; disk size in blocks
	dc.w	191		; directory mask
	dc.w	0
	dc.w	48		; check vector size
	dc.w	2		; track offset
	

dpb2:	dc.w	256		;sectors/track
	dc.b	4,15		; bsh,blm  (2K blocks)
	dc.b	0,0		; extent mask, fill
	dc.w	8176		; disk size in blocks (minus 1)
	dc.w	4095		; directory mask
	dc.w	0
	dc.w	0		; check vector size (none)
	dc.w	1		; track offset

xlt0:	dc.w	0,6,12,18,24,4,10,16,22,2,8,14,20
	dc.w	1,7,13,19,25,5,11,17,23,3,9,15,21
	
xlt1:   dc.w   0,1,2,3,4,5,6,7
        dc.w   16,17,18,19,20,21,22,23
        dc.w   32,33,34,35,36,37,38,39
        dc.w   8,9,10,11,12,13,14,15
        dc.w   24,25,26,27,28,29,30,31
	

dirbuf:	ds.b	128

ckv0:	ds.b	18
ckv1:	ds.b	48
alv0:	ds.b	64
alv1:	ds.b	50
alv2:	ds.b	1024

track:	ds.w	1
sector:	ds.w	1
drive:	ds.w	1

iobyte:	ds.w	1
	

	ds.l	128
initstack:
	ds.l	1
        end
