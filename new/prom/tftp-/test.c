#include "../tftp/defs.h"

main()
{
	char buf[128];

	for (;;) {
		printf("Test of TFTP boot.\n>N ");
		gets(buf);
			tftpboot(buf);
	}
}

