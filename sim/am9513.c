/*
 * sun-2 emulator
 * 10/2014  Brad Parker <brad@heeltoe.com>
 *
 * am9513 timer emulation
 */

#include <stdio.h>

#include "sim68k.h"
#include "m68k.h"
#include "sim.h"

int trace_am9513;

struct am9513_ctr_s {
  unsigned short mode;
  unsigned short load;
  unsigned short hold;
  unsigned short cntr;
} am9513_ctr[6];

unsigned int am9513_status = 0x0b00;
unsigned int am9513_data_ptr;
unsigned int am9513_data_ptr_byte;
unsigned int am9513_irq_t1;
unsigned int am9513_irq_t2;
unsigned int am9513_prescale;
unsigned int am9513_output_bits;

int am9513_device_ack(int which)
{
  if (trace_am9513) printf("am9513: irq ack %d\n", which);
  switch (which) {
  case 1:
    int_controller_clear(IRQ_9513_TIMER1);
    return M68K_INT_ACK_AUTOVECTOR;
  case 2:
    int_controller_clear(IRQ_9513_TIMER2);
    return M68K_INT_ACK_AUTOVECTOR;
  }
}

void am9513_update(void)
{
  int i, mask;

#if 0
  if (++am9513_prescale < 100)
    return;
  am9513_prescale = 0;
#endif

  for (i = 1; i < 6; i++) {
    if (am9513_ctr[i].mode) {
      am9513_ctr[i].cntr++;
      if (0) printf("am9513: ctr[%d] %x\n", i, am9513_ctr[i].cntr);
      if (am9513_ctr[i].cntr == 0) {
	mask = 1 << i;
	if (trace_am9513) printf("am9513: ctr[%d] bits %x\n", i, am9513_output_bits);

	if ((am9513_output_bits & (1<<1)) == 0 && (mask & (1<<1))) {
	  if (trace_am9513) printf("am9513: ctr[%d] IRQ!\n", i);
	  am9513_irq_t1 = 1;
#if 0
	  enable_trace(2);
#endif
	}
	if ((am9513_output_bits & (1<<2)) == 0 && (mask & (1<<2))) {
	  if (trace_am9513) printf("am9513: ctr[%d] IRQ!\n", i);
	  am9513_irq_t2 = 1;
	}

	am9513_output_bits |= mask;
      }
    }
  }

  if (am9513_irq_t1) {
    am9513_irq_t1 = 0;
    int_controller_set(IRQ_9513_TIMER1);
    if (0) printf("am9513: irq t1\n");
  }

  if (am9513_irq_t2) {
    am9513_irq_t2 = 0;
    int_controller_set(IRQ_9513_TIMER2);
    if (0) printf("am9513: irq t2\n");
  }
}

void am9513_wr_data(unsigned int value)
{
  int e = (am9513_data_ptr & 0x18)>>3;
  int g = (am9513_data_ptr & 0x7);

  if (trace_am9513) printf("am9513_wr_data: e%d g%d b%d\n", e, g, am9513_data_ptr_byte);
  switch (g) {
  case 1:
  case 2:
  case 3:
  case 4:
  case 5:
    value &= 0xff;
    if (am9513_data_ptr_byte) {
      switch (e) {
      case 0: am9513_ctr[g].mode = (value << 8) | (am9513_ctr[g].mode & 0xff); break;
      case 1: am9513_ctr[g].load = (value << 8) | (am9513_ctr[g].load & 0xff); break;
      case 2: am9513_ctr[g].hold = (value << 8) | (am9513_ctr[g].hold & 0xff); break;
      case 3: ;
	break;
      case 7:
	break;
      }
    } else {
      switch (e) {
      case 0: am9513_ctr[g].mode = value | (am9513_ctr[g].mode & 0xff00); break;
      case 1: am9513_ctr[g].load = value | (am9513_ctr[g].load & 0xff00); break;
      case 2: am9513_ctr[g].hold = value | (am9513_ctr[g].hold & 0xff00); break;
      case 3: ;
	break;
      case 7:
	break;
      }
    }
  }

  if (am9513_data_ptr_byte) {
    am9513_data_ptr_byte = 0;
  }
}

void am9513_wr_cmd(unsigned int value)
{
  unsigned int n, mask;

  //if ((value & 0xff) != 0xe1) printf("am9513_wr_cmd; cmd %x ", value);
  switch (value & 0xff) {
  case 0xff: printf("am9513: reset\n"); return;
  case 0xe7: printf("am9513: clr mm13\n"); return;
  case 0xe6: printf("am9513: clr mm12\n"); return;
  case 0xe0: printf("am9513: clr mm14\n"); return;
  case 0xef: printf("am9513: set mm13\n"); return;
  case 0xee: printf("am9513: set mm12\n"); return;
  case 0xe8: printf("am9513: set mm14\n"); return;
  }

  switch (value & 0xf8) {
  case 0xf0: printf("am9513: step counter\n"); return;
  case 0xe0:
    n = value & 0x7;
    mask = ~(1 << n);
    am9513_output_bits &= mask;
    if (trace_am9513) printf("am9513: clr output bit %d %x\n", n, am9513_output_bits);
    am9513_ctr[n].cntr = 0;
    return;
  case 0xe8: printf("am9513: set output bit\n"); return;
  }

  switch (value & 0xe0) {
  case 0xc0: printf("am9513: disarm\n");    return;
  case 0xa0: printf("am9513: save hold\n");    return;
  case 0x80: printf("am9513: disarm and save\n");    return;
  case 0x60:
    printf("am9513: load and arm\n");
    if (value & 0x10) { printf("am9513: s5 "); }
    if (value & 0x08) { printf("am9513: s4 "); }
    if (value & 0x04) { printf("am9513: s3 "); }
    if (value & 0x02) { printf("am9513: s2 "); }
    if (value & 0x01) { printf("am9513: s1 "); }
    printf("am9513: \n");
    return;
  case 0x40:
    printf("am9513: load source\n");
    if (value & 0x10) { printf("am9513: s5 "); }
    if (value & 0x08) { printf("am9513: s4 "); }
    if (value & 0x04) { printf("am9513: s3 "); }
    if (value & 0x02) { printf("am9513: s2 "); }
    if (value & 0x01) { printf("am9513: s1 "); }
    printf("am9513: \n");
    return;
  case 0x20: printf("am9513: arm\n");    return;
  case 0x00:
    printf("am9513: load data ptr e%d g%d\n", (value & 0x18)>>3, (value & 0x7));
    am9513_data_ptr = value;
    am9513_data_ptr_byte = 1;
    return;
  }

  printf("am9513: unknown?\n");
}

unsigned int am9513_read(unsigned int pa, int size)
{
  unsigned int offset = pa & 0xff;
  unsigned int value;

  switch (offset) {
  case 0: value = am9513_status; break;
  case 2: value = am9513_status; break;
  }

  if (trace_am9513)
    printf("am9513: read %x (%d) -> %x\n", pa, size, value);

  return value;
}

void am9513_write(unsigned int pa, unsigned int value, int size)
{
  unsigned int offset = pa & 0xff;

  if (trace_am9513)
    printf("am9513: write %x (%d) <- %x\n", pa, size, value);

  switch (offset) {
  case 0: am9513_wr_data(value); break;
  case 2: am9513_wr_cmd(value); break;
  }
}

/* Local Variables:  */
/* mode: c           */
/* c-basic-offset: 2 */
/* End:              */
