|	asm.s	1.1	01/08/86

|
| copy bytes, using movb,movw, or movl as appropriate.
|
	.globl	bcopy, bmove
bcopy:
bmove:
	movl	sp@(4),d0
	movl	d0,a0
	movl	d0,d1
	movl	sp@(8),d0
	movl	d0,a1
	orl	d0,d1
	movl	sp@(12),d0
	bles	6$
	orl	d0,d1
	btst	#0,d1
	beqs	2$
	subql	#1,d0
1$:	movb	a0@+,a1@+
	dbra	d0,1$
	rts

2$:	btst	#1,d0
	beqs	4$
	asrl	#1,d0
	subql	#1,d0
3$:	movw	a0@+,a1@+
	dbra	d0,3$
	rts

4$:	asrl	#2,d0
	subql	#1,d0
5$:	movl	a0@+,a1@+
	dbra	d0,5$
6$:	rts


|
| zero bytes
|
	.globl	bzero
bzero:	movl	sp@(4),a0
	movl	sp@(8),d0
	subql	#1,d0
	moveq	#0,d1
1$:	movb	d1,a0@+
	dbra	d0,1$
	rts


|
| compare bytes
|
	.globl	bcmp
bcmp:	movl	sp@(4),a0
	movl	sp@(8),a1
	movl	sp@(12),d0
	subql	#1,d0
1$:	cmpmb	a0@+,a1@+
	dbne	d0,1$
	bnes	2$
	moveq	#0,d0
	rts
2$:	moveq	#1,d0
	rts



|
| Return the 1's complement checksum of the word aligned buffer
| at s, for n bytes.
|
| in_cksum(s,n) 
| u_short *s; int n;
	
	.globl	in_cksum
in_cksum:
	movl	sp@(4),a0
	movl	sp@(8),d1
	asrl	#1,d1
	subql	#1,d1
	clrl	d0
1$:
	addw	a0@+,d0
	bccs	2$
	addqw	#1,d0
2$:
	dbra	d1,1$
	notw	d0
	rts
