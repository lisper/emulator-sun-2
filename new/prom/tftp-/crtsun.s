| C run time startoff.

	.text
	.globl	_start,_end,_edata,ddtinit,main

_start:	jra	__st1
	.long	_end+4			| ddt uses this to find symtab
__st1:
	movl	#_edata,a0		| clear bss area
	clrl	d0			|  (why doesnt C startup do this??)
1$:
	movw	d0,a0@+
	cmpl	#_end,a0
	blts	1$

	movl	#_start,sp
	movl	sp,a6
	jsr	ddtinit
	jsr	main
	jra	_start
