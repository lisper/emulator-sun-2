/*
 * sun-2 emulator
 * 10/2014  Brad Parker <brad@heeltoe.com>
 *
 * scsi controller
 */

#include <stdio.h>

#include "sim68k.h"
#include "m68k.h"
#include "sim.h"
#include "scsi.h"

extern int trace_mmu;
extern int trace_sc;
extern int trace_irq;
extern int trace_armed;

static unsigned short sc_dma_count;
static unsigned int sc_dma_addr;
static unsigned short sc_data;
static unsigned short sc_icr;
static unsigned short sc_cmd;
static unsigned int sc_word_mode;


/* interface */
#define SC_ICR_PARITY_ERROR     0x8000  /* parity error */
#define SC_ICR_BUS_ERROR        0x4000  /* bus error */
#define SC_ICR_ODD_LENGTH       0x2000  /* odd length */
#define SC_ICR_INT_REQUEST      0x1000  /* interrupt request */
#define SC_ICR_REQUEST          0x0800  /* request */
#define SC_ICR_MESSAGE          0x0400  /* message */
#define SC_ICR_COMMAND_DATA     0x0200  /* 1=command, 0=data */
#define SC_ICR_INPUT_OUTPUT     0x0100  /* 1=input (initiator should read), 0=output */
#define SC_ICR_PARITY           0x0080  /* parity */
#define SC_ICR_BUSY             0x0040  /* busy */
#define SC_ICR_SELECT           0x0020  /* select */
#define SC_ICR_RESET            0x0010  /* reset */
#define SC_ICR_PARITY_ENABLE    0x0008  /* enable parity */
#define SC_ICR_WORD_MODE        0x0004  /* word mode */
#define SC_ICR_DMA_ENABLE       0x0002  /* enable DMA */
#define SC_ICR_INT_ENABLE       0x0001  /* enable interrupts */

void _sc_scsi_update(void)
{
  unsigned int in, out, irq, old_icr;

  out =
    ((sc_icr & SC_ICR_RESET) ? SCSI_BUS_RST : 0) |
/*    ((sc_icr & SC_ICR_BUSY) ? SCSI_BUS_BSY : 0) | */
    ((sc_icr & SC_ICR_SELECT) ? SCSI_BUS_SEL : 0);

  if (trace_sc) {
    printf("sc:\n");
    printf("sc: OUT %x\n", out);
  }

  in = 0;
  scsi_update(&sc_data, out, &in, &irq);

  if (trace_sc) {
    printf("sc: IN %x DATA %02x\n", in, sc_data);
  }

  old_icr = sc_icr;

  sc_icr &= ~(SC_ICR_INT_REQUEST |
	      SC_ICR_REQUEST |
	      SC_ICR_MESSAGE |
	      SC_ICR_COMMAND_DATA |
	      SC_ICR_INPUT_OUTPUT |
	      SC_ICR_PARITY |
	      SC_ICR_BUSY |
	      SC_ICR_SELECT);

  if (in & SCSI_BUS_BSY) sc_icr |= SC_ICR_BUSY;
  if (in & SCSI_BUS_SEL) sc_icr |= SC_ICR_SELECT;

  if (in & SCSI_BUS_MSG) sc_icr |= SC_ICR_MESSAGE;
  if (in & SCSI_BUS_CD) sc_icr |= SC_ICR_COMMAND_DATA;
  if (in & SCSI_BUS_REQ) sc_icr |= SC_ICR_REQUEST;
  if (in & SCSI_BUS_IO) sc_icr |= SC_ICR_INPUT_OUTPUT;

  if (irq) {
    sc_icr |= SC_ICR_INT_REQUEST;
    if (sc_icr & SC_ICR_INT_ENABLE) {
      if ((old_icr & SC_ICR_INT_REQUEST) == 0) {
	if (trace_irq) printf("sc: irq %d\n", IRQ_SC);
	int_controller_set(IRQ_SC);
      }
    }
  }

  if (0) printf("sc: scsi bus in %x -> icr %x\n", in, sc_icr);
}

int sc_device_ack()
{
  int_controller_clear(IRQ_SC);
  return M68K_INT_ACK_AUTOVECTOR;
}

void sc_set_cmd_reg(unsigned int v)
{
  sc_cmd = v;
}

unsigned int sc_read(unsigned address, int size)
{
  unsigned int value;

  /* do initiator side of req/ack handshake */
  if (address <= /*0x2*/0x04)
    _sc_scsi_update();

  address &= 0xf;
  switch (address) {
    /* data */
  case 0x0:
    value = sc_data;
    if (trace_sc) printf("sc: read data0 %02x\n", value);
    break;
  case 0x1:
    value = sc_data;
    if (trace_sc) printf("sc: read data1 %02x\n", value);
    break;

    /* command/status */
  case 0x2:
    scsi_read_cmd_byte();
    value = sc_cmd;
    if (trace_sc) printf("sc: read cmd %02x\n", value);
    break;
    /* interface */
  case 0x4:
    value = sc_icr;
    if (trace_sc) printf("sc: read icr %02x\n", value);
    break;

  case 0x8:
    switch (size) {
    case 2: value = sc_dma_addr >> 16; break;
    case 4: value = sc_dma_addr; break;
    }
    if (0) printf("sc: read dma_addr %x\n", sc_dma_addr);
    break;
  case 0xa:
    value = sc_dma_addr & 0xffff;
    if (0) printf("sc: read dma_addr %x\n", sc_dma_addr);
    break;

  case 0xc:
    value = sc_dma_count;
    if (trace_sc) printf("sc: read dma_count %x\n", sc_dma_count);
    break;

  default:
    ;
  }

  _sc_scsi_update();

  return value;
}

void sc_write(unsigned int address, int size, unsigned int value)
{
  unsigned int cmd;

  address &= 0xf;
  switch (address) {
    /* data */
  case 0x0:
    sc_data = value;
    if (trace_sc) printf("sc: write data0 %02x\n", value);
    break;
  case 0x1:
    if (trace_sc) printf("sc: write data1 %02x\n", value);
    sc_data = value;
    break;

    /* command/status */
  case 0x2:
    if (trace_sc) printf("sc: write cmd %02x\n", value);
    sc_cmd = value;
    scsi_write_cmd_byte(value);
    break;
    /* interface */
  case 0x4:
    //sc_icr = value;
    cmd = value;
    sc_icr &= ~0x3f;
    sc_icr |= cmd & 0x3f;
    if (trace_sc) printf("sc: write icr %02x (%02x)\n", sc_icr, value);
    break;

  case 0x8:
    switch (size) {
    case 2: sc_dma_addr = (value << 16) | (sc_dma_addr & 0x0000ffff); break;
    case 4: sc_dma_addr = value; break;
    }
    if (trace_sc) printf("sc: write dma_addr %x\n", sc_dma_addr);
    break;
  case 0xa:
    sc_dma_addr = (sc_dma_addr & 0xffff0000) | (value & 0x0000ffff);
    if (trace_sc) printf("sc: write dma_addr %x\n", sc_dma_addr);
    break;

  case 0xc:
    sc_dma_count = value;
    if (trace_sc) printf("sc: write dma_count %x (%d)\n", sc_dma_count, sc_dma_count);
    break;

  case 0xf:
    if (trace_sc) printf("sc: write intvec %x (%d)\n", value, value);
    break;

  default:
    ;
  }

#if 0
  if (address == 0xc) {
    if (sc_dma_count == 0xffff) {
      sc_icr |= SC_ICR_INT_REQUEST;
      sc_data = 0;
    }
  }
#endif

  _sc_scsi_update();
}

void sc_dma_read_data(unsigned char *buf, int bufsiz)
{
  int i;
  extern unsigned char g_ram[];

  if (trace_armed) {
    unsigned int va, pa, mtype, fault, pte;

    va = 0xf00000 + sc_dma_addr;
    pa = cpu_map_address(va, 5, 1, &mtype, &fault, &pte);
    printf("sc: dma %d bytes to 0x%x; va %x pa %x mtype %d\n", bufsiz, sc_dma_addr, va, pa, mtype);
  }

#if 0
  if (0) {
    printf("sc: dma %d byte to 0x%x\n", bufsiz, sc_dma_addr);
    dumpbuffer(buf, bufsiz);
  }
#endif

  for (i = 0; i < bufsiz; i++) {
    unsigned int mtype, fault;
    unsigned int va, pa, pte;

    va = 0xf00000 + sc_dma_addr;
    pa = cpu_map_address(va, 5, 1, &mtype, &fault, &pte);

    g_ram[pa] = buf[i];
    sc_dma_addr++;
    sc_dma_count++;
  }

  if (bufsiz & 1) {
    sc_icr |= SC_ICR_ODD_LENGTH;
    sc_data = buf[bufsiz-1];
    printf("sc: setting odd length %d\n", bufsiz);
  } else
    sc_icr &= ~SC_ICR_ODD_LENGTH;
}

void sc_reset_odd_len(void)
{
  sc_icr &= ~SC_ICR_ODD_LENGTH;
}

void sc_dma_write_data(unsigned char *buf, int bufsiz)
{
  int i;
  extern unsigned char g_ram[];

  if (0) {
    unsigned int va, pa, mtype, fault, pte;

    printf("sc: dma %d bytes to 0x%x\n", bufsiz, sc_dma_addr);
    va = 0xf00000 + sc_dma_addr;
    pa = cpu_map_address(va, 5, 1, &mtype, &fault, &pte);
    printf("sc: dma va %x pa %x mtype %d\n", va, pa, mtype);
  }

  for (i = 0; i < bufsiz; i++) {

    unsigned int mtype, fault;
    unsigned int va, pa, pte;

    va = 0xf00000 + sc_dma_addr;
    pa = cpu_map_address(va, 5, 1, &mtype, &fault, &pte);

    buf[i] = g_ram[pa];
    sc_dma_addr++;
    sc_dma_count++;
  }

  if (bufsiz & 1)
    sc_icr |= SC_ICR_ODD_LENGTH;
  else
    sc_icr &= ~SC_ICR_ODD_LENGTH;
}

unsigned int sc_get_data(void)
{
  return sc_data;
}

/* Local Variables:  */
/* mode: c           */
/* c-basic-offset: 2 */
/* End:              */
