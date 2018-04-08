/*
 * confreg.c
 *
 * Shows SUN processor configuration register
 *
 * Jeffrey Mogul	17 February 1982
 */

#include "../h/confreg.h"
#include "../h/sunmon.h"

/*
 * kludge: do this so that monitor's I/O used instead
 * of going directly to UART
 */

putchar(c)
char c;
{
	return(emt_putchar(c));
}

getchar()
{
	return(emt_getchar());
}

main()
{
	int version;
	int majversion;
	int minversion;
	long x;
	
	version = emt_version();
	
	majversion = MAJ_VERSION(version);
	minversion = MIN_VERSION(version);
	
	printf("Configuration of Sun MC68000, ROM Monitor Version %d.%d\n",
			majversion, minversion);

	x = emt_getconfig();
	
	if (x < 0)
	    printf(
	    "Monitor version too low, configuration register not supported.\n"
	    );
	else
	    interpret(x);
}

interpret(x)
int x;
{
	struct ConfReg config;
	
	*((short *)&config) = x;
	
	printf("Configuration register value is %4x\n",x);

	printf("Host speed: ");
	printspeed(config.HostSpeed);
	printf(", Console speed: ");
	printspeed(config.ConsSpeed|4);
	printf("\n");
	
	if (config.UseFB)	/* asserted high */
	    printf("Using Frame buffer by default\n");
	else
	    printf("Not using Frame buffer by default\n");

	if (config.ErrorRestart)	/* asserted low */
	    printf("No restart on error\n");
	else
	    printf("Restart on error\n");

	printf("Boot action is: ");
	printboot(config.Bootfile);
	printf("\n");

	if (config.FBType)
	    printf("Frame buffer is landscape format\n");
	else
	    printf("Frame buffer is portrait format\n");

	if (config.BreakEnable)
	    printf("Break/Abort enabled\n");
	else
	    printf("Break/Abort disabled\n");

	printf("\n");
}

printspeed(s)
int s;
{
	switch (s) {
	case BRATE_9600:
		printf("9600 baud");
		break;

	case BRATE_4800:
		printf("4800 baud");
		break;

	case BRATE_1200:
		printf("1200 baud");
		break;

	case BRATE_300:
		printf("300 baud");
		break;

	case BRATE_2400:
		printf("2400 baud");
		break;

	case BRATE_600:
		printf("600 baud");
		break;

	case BRATE_110:
		printf("110 baud");
		break;

	case 0:
		printf("9600 baud");
		break;

	default:
		printf("[Internal error, unknown speed!]");
		break;
	}
}

printboot(b)
int b;
{
	switch (b) {
	case BOOT_TO_MON:
		printf("Boot into console monitor");
		break;

	case BOOT_SELF_TEST:
		printf("Do self test, then enter console monitor.");
		break;

	default:
		printf("Load boot file #%d",b);
		printf("; do 'W ff' to list autoboot files");
		break;
	}
}
