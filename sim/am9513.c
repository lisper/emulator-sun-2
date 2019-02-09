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
int trace_irq;

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
unsigned int am9513_armed_bits;

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
  return -1;
}

void am9513_update(void)
{
  int i, bit;

  trace_am9513 = 0;

#if 0
  if (++am9513_prescale < 10)
    return;
  am9513_prescale = 0;
#endif

  //xxx count must be armed before it can commence counting

  for (i = 1; i < 6; i++) {
    bit = 1 << i;

    if ((am9513_armed_bits & bit) == 0)
      continue;

    if (am9513_ctr[i].mode) {

      if (am9513_ctr[i].mode & 1)
	am9513_ctr[i].cntr++;
      else
	am9513_ctr[i].cntr--;

      if (0) printf("am9513: ctr[%d] cntr=%x\n", i, am9513_ctr[i].cntr);

      if (am9513_ctr[i].cntr == 0) {
	if (trace_am9513) printf("am9513: ctr[%d] cntr==0 output_bits %x\n", i, am9513_output_bits);

	if ((am9513_ctr[i].mode & 0x40) == 0)
	  am9513_ctr[i].cntr = am9513_ctr[i].load;

	if ((am9513_output_bits & (1<<1)) == 0 && (bit & (1<<1))) {
	  if (trace_am9513) printf("am9513: ctr[%d] cntr==0 IRQ!\n", i);
	  am9513_irq_t1 = 1;
#if 0
	  enable_trace(2);
#endif
	}

	if ((am9513_output_bits & (1<<2)) == 0 && (bit & (1<<2))) {
	  if (trace_am9513) printf("am9513: ctr[%d] cntr==0 IRQ!\n", i);
	  am9513_irq_t2 = 1;
	}

	am9513_output_bits |= bit;
      }
    }
  }

  if (am9513_irq_t1) {
    am9513_irq_t1 = 0;
    if (0) printf("am9513: irq t1\n");
    int_controller_set(IRQ_9513_TIMER1);
  }

  if (am9513_irq_t2) {
    am9513_irq_t2 = 0;
    if (trace_irq) printf("am9513: irq t2\n");
    int_controller_set(IRQ_9513_TIMER2);
  }
}

void am9513_wr_data(unsigned int value)
{
  int e = (am9513_data_ptr & 0x18)>>3;
  int g = (am9513_data_ptr & 0x7);

  if (trace_am9513) printf("am9513_wr_data: e%d g%d b%d <- %02x\n", e, g, am9513_data_ptr_byte, value);
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

  if (trace_am9513) {
    switch (e) {
    case 0: printf("am9513: mode[%d] = %04x\n", g, am9513_ctr[g].mode); break;
    case 1: printf("am9513: load[%d] = %04x\n", g, am9513_ctr[g].load); break;
    case 2: printf("am9513: hold[%d] = %04x\n", g, am9513_ctr[g].hold); break;
    }
  }

  if (am9513_data_ptr_byte) {
    am9513_data_ptr_byte = 0;
  }
}

void am9513_wr_cmd(unsigned int value)
{
  unsigned int n, mask, bits;
  int i;

  //if ((value & 0xff) != 0xe1) printf("am9513_wr_cmd; cmd %x ", value);
  switch (value & 0xff) {
  case 0xff: if (trace_am9513) printf("am9513: reset\n"); return;
  case 0xe7: if (trace_am9513) printf("am9513: clr mm13\n"); return;
  case 0xe6: if (trace_am9513) printf("am9513: clr mm12\n"); return;
  case 0xe0: if (trace_am9513) printf("am9513: clr mm14\n"); return;
  case 0xef: if (trace_am9513) printf("am9513: set mm13\n"); return;
  case 0xee: if (trace_am9513) printf("am9513: set mm12\n"); return;
  case 0xe8: if (trace_am9513) printf("am9513: set mm14\n"); return;
  }

  switch (value & 0xf8) {
  case 0xf0:
    printf("am9513: step counter\n"); return;

  case 0xe0:
    n = value & 0x7;
    mask = ~(1 << n);
    am9513_output_bits &= mask;
    if (trace_am9513) printf("am9513: clr output bit %d %x\n", n, am9513_output_bits);
//    am9513_ctr[n].cntr = 0;
    return;

  case 0xe8:
    printf("am9513: set output bit\n");
    return;
  }

  switch (value & 0xe0) {
  case 0xc0:
    if (trace_am9513) printf("am9513: disarm\n");
    bits = value & 0x1f;
    am9513_armed_bits &= ~(bits << 1);
    return;
  case 0xa0:
    if (trace_am9513) printf("am9513: save hold\n");

    for (i = 1; i < 6; i++) {
      if (value && (1 << i))
	am9513_ctr[i].load = am9513_ctr[i].cntr;
    }
    return;
  case 0x80:
    if (trace_am9513) printf("am9513: disarm and save\n");

    for (i = 1; i < 6; i++) {
      if (value && (1 << i))
	am9513_ctr[i].load = am9513_ctr[i].cntr;
    }
    bits = value & 0x1f;
    am9513_armed_bits &= ~(bits << 1);
    return;
  case 0x60:
    if (trace_am9513) {
      printf("am9513: load and arm ");
      if (value & 0x10) { printf("s5 "); }
      if (value & 0x08) { printf("s4 "); }
      if (value & 0x04) { printf("s3 "); }
      if (value & 0x02) { printf("s2 "); }
      if (value & 0x01) { printf("s1 "); }
      printf("\n");
    }

    for (i = 1; i < 6; i++) {
      if (value && (1 << i))
	am9513_ctr[i].cntr = am9513_ctr[i].load;
    }
    bits = value & 0x1f;
    am9513_armed_bits |= bits << 1;
    return;
  case 0x40:
    if (trace_am9513) {
      printf("am9513: load ");
      if (value & 0x10) { printf("s5 "); }
      if (value & 0x08) { printf("s4 "); }
      if (value & 0x04) { printf("s3 "); }
      if (value & 0x02) { printf("s2 "); }
      if (value & 0x01) { printf("s1 "); }
      printf("\n");
    }

    for (i = 1; i < 6; i++) {
      if (value && (1 << i))
	am9513_ctr[i].cntr = am9513_ctr[i].load;
    }
    return;
  case 0x20:
    if (trace_am9513) {
      printf("am9513: arm ");
      if (value & 0x10) { printf("s5 "); }
      if (value & 0x08) { printf("s4 "); }
      if (value & 0x04) { printf("s3 "); }
      if (value & 0x02) { printf("s2 "); }
      if (value & 0x01) { printf("s1 "); }
      printf("\n");
    }

    bits = value & 0x1f;
    am9513_armed_bits |= bits << 1;
    return;
  case 0x00:
    if (trace_am9513) printf("am9513: load data ptr e%d g%d\n", (value & 0x18)>>3, (value & 0x7));
    am9513_data_ptr = value;
    am9513_data_ptr_byte = 1;
    return;
  }

  if (trace_am9513) printf("am9513: unknown?\n");
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

  value &= 0x00ff;

  switch (offset) {
  case 0: am9513_wr_data(value); break;
  case 2: am9513_wr_cmd(value); break;
  }
}

/* Local Variables:  */
/* mode: c           */
/* c-basic-offset: 2 */
/* End:              */
