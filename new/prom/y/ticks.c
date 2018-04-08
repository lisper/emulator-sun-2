main()
{
	long NextTick;		/* value of RefrCnt at next second */
	long ThisTick;		/* temporary; used to avoid a race */
	long seconds = 0;

	ThisTick = emt_ticks();	/* initialize loop invariant */

	for (;;) {
		printf("Time is %d secs\n",seconds++);
		NextTick = ThisTick + 1000;	/* predict next second */
		while ( ThisTick < NextTick )	/* busy-wait */
			ThisTick = emt_ticks();
	}
}
