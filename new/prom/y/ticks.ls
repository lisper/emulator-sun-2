   1                                                        	.text
   2                                                        	.globl	main
   3    00 0000                                             main:
   4    00 0000  4E56  FFF4                                 	link	a6,#-_F1
   5    00 0004  48EE  0000  FFF4                           	moveml	#_S1,a6@(-_F1)
   6                                                        | A1 = 8
   7    00 000A  42AE  FFF4                                 	clrl	a6@(-12)
   8    00 000E  4EB9  0000  0000                           	jbsr	emt_ticks
   9    00 0014  2D40  FFF8                                 	movl	d0,a6@(-8)
  10    00 0018                                             .L16:
  11                                                        	.text
  12    00 0018  202E  FFF4                                 	movl	a6@(-12),d0
  13    00 001C  52AE  FFF4                                 	addql	#1,a6@(-12)
  14    00 0020  2F00                                       	movl	d0,sp@-
  15    00 0022  2F3C  0000  005C                           	movl	#.L18,sp@-
  16    00 0028  4EB9  0000  0000                           	jbsr	printf
  17    00 002E  508F                                       	addql	#8,sp
  18    00 0030  202E  FFF8                                 	movl	a6@(-8),d0
  19    00 0034  0680  0000  03E8                           	addl	#1000,d0
  20    00 003A  2D40  FFFC                                 	movl	d0,a6@(-4)
  21    00 003E                                             .L19:
  22    00 003E  202E  FFF8                                 	movl	a6@(-8),d0
  23    00 0042  B0AE  FFFC                                 	cmpl	a6@(-4),d0
  24    00 0046  6CD0                                       	jge	.L16
  25    00 0048  4EB9  0000  0000                           	jbsr	emt_ticks
  26    00 004E  2D40  FFF8                                 	movl	d0,a6@(-8)
  27    00 0052  60EA                                       	jra	.L19
  28    00 0054                                             .L20:
  29    00 0054                                             .L14:
  30    00 0054  60C2                                       	jra	.L16
  31    00 0056                                             .L15:
  32    00 0056                                             .L12:
  33    00 0056  4E5E                                       	unlk	a6
  34    00 0058  4E75                                       	rts
  35                                                        _F1 = 12
  36                                                        _S1 = 0
  37                                                        | M1 = 8
  38                                                        	.data
  39    00 005C                                             .L18:
  40    00 005C  54 69 6D 65 20 69 73 20                    	.byte	84,105,109,101,32,105,115,32
  41    00 0064  25 64 20 73 65 63 73 0A                    	.byte	37,100,32,115,101,99,115,10
  42    00 006C  00                                         	.byte	0
  43    00 006D                                             .end
