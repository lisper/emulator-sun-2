// mm58167

#include <stdio.h>

static unsigned counter[8];
static unsigned ms;
static unsigned delay;

void mm58167_update(void)
{
	unsigned m, t, th, tl, s;

	if (++delay < 1000) {
		return;
	}
	delay = 0;

	ms++;

	t = (ms / 10) % 100;
	th = t / 10;
	tl = t % 10;

	m = (ms % 10) << 4;
	t = (th << 4) | tl;
	s = 0;

	counter[0] = m;  // milliseconds
	counter[1] = t;  // tenths
	counter[2] = s;  // seconds
	counter[3] = 0x00; // min
	counter[4] = 0x12; // hours
	counter[5] = 0x01; // day of week
	counter[6] = 0x21; // day of mon
	counter[7] = 0x06; // mon
}

void mm58167_write(unsigned pa, unsigned value, int size)
{
    printf("io: write %x <- %x (%d) tod?\n", pa, value, size);
}


unsigned mm58167_read(unsigned pa, int size)
{
    unsigned v;

    v = 0xff;
    switch (pa & 0xff) {
    case 0: v = counter[0]; break;
    case 2: v = counter[1]; break;
    case 4: v = counter[2]; break;
    case 6: v = counter[3]; break;
    case 8: v = counter[4]; break;
    case 10: v = counter[5]; break;
    case 12: v = counter[6]; break;
    case 14: v = counter[7]; break;

    case 0x28:
	    v = 0;
	    break;
    }

    printf("io: read %x -> %x (%d) tod?\n", pa, v, size);

    return v;
}


