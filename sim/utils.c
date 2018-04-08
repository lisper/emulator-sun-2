/*
 * sun-2 emulator
 * 10/2014  Brad Parker <brad@heeltoe.com>
 *
 * utiity routines
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void dumpbuffer(char *buf, int len)
{
    unsigned char *b = (unsigned char *)buf;
    int offset = 0;
    int i;
    char lb[17];

    while (len > 0) {
        printf("%03x: ", offset);
        for (i = 0; i < 8; i++) {
            if (i < len) {
                printf("%02x ", b[i]);
                lb[i] = (b[i] >= ' ' && b[i] <= '~') ? b[i] : '.';
            } else {
                printf("xx ");
                lb[i] = 'x';
            }
        }
        lb[8] = 0;
        
        printf(" %s\n", lb);
        len -= 8;
        b += 8;
        offset += 8;
    }
}

void abortf(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  exit(1);
}


/* Local Variables:  */
/* mode: c           */
/* c-basic-offset: 2 */
/* End:              */
