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

struct scc_fifo_s {
  int count;
  int wptr;
  int rptr;
  unsigned char fifo[16];
};

struct scc_fifo_s scc_ififo[4];
struct scc_fifo_s scc_ofifo[4];


#define ZSRR0_RX_READY	0x01
#define ZSRR0_TX_READY	0x04

#define ZSRR1_ALL_SENT	0x01

void scc_in_push(int ch, int v)
{
  scc_ififo[ch].count++;
  scc_ififo[ch].fifo[ scc_ififo[ch].wptr++ ] = v;
  scc_ififo[ch].wptr &= 0xf;
}

int scc_in_pop(int ch, int *pv)
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
    scc_rr[ch][0] = ZSRR0_TX_READY;
    scc_rr[ch][1] = ZSRR1_ALL_SENT;
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
//value = scc_rr[ch][0];
  scc_cmd_primed[ch] = 0;
  scc_cmd[ch] = 0;

  if (trace_scc) printf("scc%d: read ctl; read %d -> %x (%d)\n", ch, reg, value, size);
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
  } else {
    int reg = scc_cmd[ch] & 0x7;
    int cmd = (scc_cmd[ch] & 0x38) >> 3;
    int rst = (scc_cmd[ch] & 0xc0) >> 6;
    scc_cmd_primed[ch] = 0;
    if (cmd == 1) {
      /* point hi */
      reg += 8;
    }
    if (trace_scc) printf("scc%d: write ctl; reg %d <- %x (%d)\n", ch, reg, value, size);
    scc_wr[ch][reg] = value;
    switch (reg) {
    case 2:
      scc_rr[ch][reg] = value;
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
  }
}

void scc_wr_data(int ch, int value, int size)
{
  char dc, ds;
  int r;

  value &= 0xff;
  dc = value;
  ds = (value >= ' ' && value <= '~') ? value : '.';
    printf("scc%d: write data %02x %c\n", ch, value, ds);

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
#if 0
  // va eec004 pa 78004
//	if (m68k_get_reg(NULL, M68K_REG_PC) == 0xef045c)
  if (m68k_get_reg(NULL, M68K_REG_PC) == 0xef03d4)
  {
    enable_trace(1);
    printf("scc: XXX\n");
  }
#endif
  int i;

  for (i = 0; i < 4; i++) {
    if (scc_ififo[i].count > 0) {
      scc_rr[i][0] |= ZSRR0_RX_READY;
    } else {
      scc_rr[i][0] &= ~ZSRR0_RX_READY;
    }
  }
}

/* Local Variables:  */
/* mode: c           */
/* c-basic-offset: 2 */
/* End:              */
