#include "../h/globram.h"
#include "../tftpnetboot/tftpnetboot.h"
  
/* A simple program to test TFTP booting. */
  
main(argc, argv)
    int argc;
    char **argv;
  {
    Address result;

    while (TRUE)
      {
	message("\nInput string:");
	getline();
	if (result = enbootload(GlobPtr->linbuf))
	  {
	    message("Loaded program at 0x");
	    printhex(result, 0);
	    message("\n");
	  }
	else
	    message("Couldn't load!\n");
      }
  }
