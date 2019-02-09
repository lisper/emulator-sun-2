/*
 * sun-2 emulator
 * 10/2014  Brad Parker <brad@heeltoe.com>
 *
 * SCC serial chip emulation
 */

#include <stdio.h>

#include "sim68k.h"
#include "m68k.h"
#include "sim.h"

int trace_scc = 0;

unsigned int scc_init[4];
unsigned int scc_cmd[4];
unsigned int scc_cmd_primed[4];
unsigned int scc_wr[4][16];
unsigned int scc_rr[4][16];
unsigned int scc_ints[4];
unsigned int scc_int_pending;

struct scc_fifo_s {
  int count;
  int wptr;
  int rptr;
  unsigned char fifo[16];
  int ints_enabled;
};

struct scc_fifo_s scc_ififo[4];
struct scc_fifo_s scc_ofifo[4];


#define RR0_RX_READY	0x01
#define RR0_TX_READY	0x04

#define RR1_ALL_SENT	0x01

#define WR1_EXT_INT_EN 0x01
#define WR1_TX_INT_EN  0x02
#define WR1_RX_INT_EN0 0x08
#define WR1_RX_INT_EN1 0x10

#define WR1_RX_INT_DIS 0
#define WR1_RX_INT_FC  1
#define WR1_RX_INT_ALL 2
#define WR1_RX_INT_SC  3

#define WR9_VIS   0x01
#define WR9_NV    0x02
#define WR9_DLC   0x04
#define WR9_MIE   0x08
#define WR9_ST_HI 0x10


int scc_device_ack(int which)
{
  if (trace_scc) printf("scc: irq ack %d\n", which);
  int_controller_clear(IRQ_SCC);
  scc_int_pending = 0;
  scc_rr[0][2] = scc_rr[1][2] = scc_rr[2][2]= scc_rr[3][2] = 0;
  return M68K_INT_ACK_AUTOVECTOR;
}

void scc_throw_interrupt(int ch, int which)
{
  int st_hi = scc_wr[ch][9] & WR9_ST_HI;
  if (scc_int_pending)
    return;

  if (scc_int_pending)
    return;
  scc_int_pending = 1;
  if (trace_scc) printf("scc%d: throw interrupt %d\n", ch, which);

  if (ch == 2) {
    switch (which) {
    case 1: scc_rr[2][2] = st_hi ? 0x00 : 0x00; break;
    case 2: scc_rr[2][2] = st_hi ? 0x20 : 0x04; break;
    }
  }
  if (ch == 3) {
    switch (which) {
    case 1: scc_rr[2][2] = st_hi ? 0x10 : 0x08; break;
    case 2: scc_rr[2][2] = st_hi ? 0x30 : 0x0c; break;
    }
  }
  int_controller_set(IRQ_SCC);
}

void scc_in_push(int ch, int v)
{
  scc_ififo[ch].count++;
  scc_ififo[ch].fifo[ scc_ififo[ch].wptr++ ] = v;
  scc_ififo[ch].wptr &= 0xf;
}

int scc_in_pop(int ch, unsigned int *pv)
{
  if (scc_ififo[ch].count <= 0) {
    *pv = -1;
    return 0;
  }

  scc_ififo[ch].count--;
  *pv = scc_ififo[ch].fifo[ scc_ififo[ch].rptr++ ];
  scc_ififo[ch].rptr &= 0xf;
  return 1;
}

void scc_chk_init(int ch)
{
  if (!scc_init[ch]) {
    scc_init[ch] = 1;
    scc_cmd[ch] = 0;
    scc_cmd_primed[ch] = 0;
    scc_rr[ch][0] = RR0_TX_READY;
    scc_rr[ch][1] = RR1_ALL_SENT;
  }
}

/* ------------- */

unsigned int scc_rd_ctl(int ch, int size)
{
  unsigned int value = 0xffff;
  int reg, cmd;

  scc_chk_init(ch);

  reg = scc_cmd[ch] & 0x7;
  cmd = (scc_cmd[ch] & 0x38) >> 3;
  if (cmd == 1)
    reg += 8;

  value = scc_rr[ch][reg];
  scc_cmd_primed[ch] = 0;
  scc_cmd[ch] = 0;

  if (trace_scc) {
    if (reg == 0 && value == 4)
      ;
   else
      printf("scc%d: read ctl; read %d -> %x (%d)\n", ch, reg, value, size);
  }
  return value;
}

unsigned int scc_rd_data(int ch, int size)
{
  unsigned int value = 0xffff;
  scc_in_pop(ch, &value);
  if (trace_scc) printf("scc%d: read data %x (%d)\n", ch, value, size);
  return value;
}

void scc_wr_ctl(int ch, int value, int size)
{
  scc_chk_init(ch);
  if (!scc_cmd_primed[ch]) {
    scc_cmd[ch] = value;
    scc_cmd_primed[ch] = 1;
    if (trace_scc) printf("scc%d: write ctl %x (%d)\n", ch, value, size);

    // ??
    if (((scc_cmd[ch] & 0x38) >> 3) > 1)
      scc_cmd_primed[ch] = 0;
  } else {
    int reg = scc_cmd[ch] & 0x7;
    int cmd = (scc_cmd[ch] & 0x38) >> 3;
    int rst = (scc_cmd[ch] & 0xc0) >> 6;

    scc_cmd_primed[ch] = 0;
    scc_cmd[ch] = 0;

    if (cmd == 1) {
      /* point hi */
      reg += 8;
    }

    scc_wr[ch][reg] = value;
    if (trace_scc) printf("scc%d: write ctl; reg %d <- %x (%d)\n", ch, reg, value, size);

    switch (reg) {
    case 0:
      switch (value & 0xff) {
      case 0x10:
	break;
      case 0x38:
	break;
      }
      break;
    case 1:
      if (trace_scc) printf("scc%d: wr1 <- %02x (%d)\n", ch, value, size);
      break;
    case 2:
      scc_rr[ch][reg] = value;
      break;
    case 9:
      printf("scc%d: wr9 <- %02x (%d)\n", ch, value, size);

      if (scc_wr[ch][9] & WR9_MIE) {
	if ((scc_ints[ch] & 0x80) == 0) {
	  printf("scc%d: MIE enabled\n", ch);
	  scc_ints[ch] |= 0x80;
	}
      } else {
	if (scc_ints[ch] & 0x80) {
	  printf("scc%d: MIE disabled\n", ch);
	  scc_ints[ch] &= ~0x80;
	}
      }
      break;
    case 12:
      scc_rr[ch][reg] = value;
      break;
    case 13:
      scc_rr[ch][reg] = value;
      scc_rr[ch][9] = value;
      break;
    case 14:
      scc_rr[ch][reg] = value;
      break;
    case 15:
      scc_rr[ch][reg] = value & ~5;
      scc_rr[ch][11] = scc_rr[ch][reg];
      break;
    }

    if (scc_wr[ch][1] & WR1_TX_INT_EN) {
      if ((scc_ints[ch] & 0x01) == 0) {
	  printf("scc%d: TX int enabled\n", ch);
	  scc_ints[ch] |= 0x01;
      }
    } else {
      if (scc_ints[ch] & 0x01) {
	printf("scc%d: TX int disabled\n", ch);
	scc_ints[ch] &= ~0x01;
      }
    }

    if (scc_wr[ch][1] & (WR1_RX_INT_EN0|WR1_RX_INT_EN1)) {
      if ((scc_ints[ch] & 0x02) == 0) {
	  printf("scc%d: RX int enabled\n", ch);
	  scc_ints[ch] |= 0x02;
      }
    } else {
      if (scc_ints[ch] & 0x02) {
	printf("scc%d: RX int disabled\n", ch);
	scc_ints[ch] &= ~0x02;
      }
    }

  }

}

void scc_wr_data(int ch, int value, int size)
{
  char dc, ds;
  int r;

  value &= 0xff;
  dc = value;
  ds = (value >= ' ' && value <= '~') ? value : '.';
  if (trace_scc) printf("scc%d: write data %02x %c\n", ch, value, ds);

    switch (ch) {
    case 0:
    case 1:
//    r = write(0, &ds, 1);
      break;

    case 3:
      sun2_kb_write(value, size);
      break;
    }
}

unsigned int scc_read(unsigned int pa, int size)
{
  unsigned int offset = pa & 0x0f;
  unsigned int value;
  int w;

  w = 0;
  if ((pa & 0x00ffff00) == 0x780000)
    w = 2;

  switch (offset) {
  case 0:
    value = scc_rd_ctl(w+0, size);
    break;
  case 2:
    value = scc_rd_data(w+0, size);
    break;
  case 4:
    value = scc_rd_ctl(w+1, size);
    break;
  case 6:
    value = scc_rd_data(w+1, size);
    break;
  default:
    value = 0;
    break;
  }

  if (0) printf("scc: read pa=%x -> %x (%d)\n", pa, value, size);

  return value;
}

void scc_write(unsigned int pa, unsigned int value, int size)
{
  unsigned int offset = pa & 0xf;
  int w;

  if (0) printf("scc: write pa=%x <- %x (%d)\n", pa, value, size);

  w = 0;
  if ((pa & 0x00ffff00) == 0x780000)
    w = 2;

  switch (offset) {
  case 0:
    scc_wr_ctl(w+0, value, size);
    break;
  case 2:
    scc_wr_data(w+0, value, size);
    break;
  case 4:
    scc_wr_ctl(w+1, value, size);
    break;
  case 6:
    scc_wr_data(w+1, value, size);
    break;
  }
}

void scc_update(void)
{
  int i;

//broken - incomplete probe?
scc_ints[2] |= 0x80;
scc_ints[3] |= 0x80;

  for (i = 0; i < 4; i++) {
    if (scc_ififo[i].count > 0) {
      scc_rr[i][0] |= RR0_RX_READY;

      if ((scc_ints[i] & 0x80) && (scc_ints[i] & 0x02)) {
	scc_throw_interrupt(i, 2);
      }
    } else {
      scc_rr[i][0] &= ~RR0_RX_READY;
    }
  }

#if 0
  for (i = 0; i < 4; i++) {
    if (scc_ofifo[i].count == 0) {
      scc_rr[i][0] |= RR0_TX_READY;

      if ((scc_ints[i] & 0x80) && (scc_ints[i] & 0x01)) {
	scc_throw_interrupt(i, 1);
      }
    } else {
      scc_rr[i][0] &= ~RR0_TX_READY;
    }
  }
#endif
}

/* Local Variables:  */
/* mode: c           */
/* c-basic-offset: 2 */
/* End:              */
