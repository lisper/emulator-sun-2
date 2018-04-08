/*
  MC68000 simulation taylored to support CP/M-68K. It includes:

  16MB of memory. (Flat, function codes and address space types ignored.)

  Console I/O using a MC6850 like serial port with interrupts.

  Simulated disk system:

  I was going to just read a file system image into memory and map it into
  the unusable (in a MC68000) address space above the 16MB physical limit.
  Alas, the simulator is simulating that physical limit and a quick check of
  the code hasn't revealed a simple way to defeat that. So plan B.

  Since the intent is to support CP/M-68K and it does disk I/O in 128 byte
  chunks, so will this. Control is via several registers mapped into memory:

  Offset       function
   0           DMA             address to read/write data to
   4           drive           select disk drive
   8           read sector     sector (128 byte) offset on disk
   12          write sector    sector (128 byte) offset on disk
   16          status          read status of operation

   Operation is simple: set the drive and DMA address and then write the
   sector number to the sector register. This write triggers the requested
   operation. The status of the operation can be determined by reading the
   status register.
   A zero indicates that no error occured.

   Note that these operations invoke read() and write() system calls directly
   so that they will alter the image on the hard disk. KEEP BACKUPS!

   In addition Linux will buffer the writes so they may note be really complete
   for a while. The BIOS flush function invokes a fsync on all open files.

   There are two options for booting CPM:

   S-records: This loads CPM in two parts. The first is in cpm400.bin which
   is created from the srecords in cpm400.sr. The second is in simbios.bin
   which contains the BIOS. Both of these files must be binaries and not
   srecords.
   
   If you want to alter the bios, rebuild simbios.bin using:

   asl simbios.s
   p2bin simbios.p

   This option is requested using "-s" on the command line.

   Boot track: A CPM loader is in the boot track of simulated drive C. 32K of
   data is loaded from that file to memory starting at $400. This is the
   default option.

  Uses the example that came with the Musashi simulator as a skeleton to
  build on.

 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#define __USE_GNU
#include <unistd.h>
#include <errno.h>
#include "sim.h"
#include "m68k.h"


/* Memory-mapped IO ports */

/* 6850 serial port like thing
   Implements a reduced set of functionallity.
 */
#define MC6850_STAT  0xff1000L     // command/status register
#define MC6850_DATA  0xff1002L     // receive/transmit data register

/* Memory mapped disk system */

#define DISKC_FILENAME "diskc.cpm.fs"
#define DISK_BASE       0xff0000L
#define DISK_SET_DMA   (DISK_BASE)
#define DISK_SET_DRIVE (DISK_BASE+4)
#define DISK_SET_SECTOR (DISK_BASE+8)
#define DISK_READ      (DISK_BASE+12)
#define DISK_WRITE     (DISK_BASE+16)
#define DISK_STATUS    (DISK_BASE+20)
#define DISK_FLUSH     (DISK_BASE+24)

#define RAMDISK                   // create a simulated 16MB RAM disk on M:
#define RAM_DRIVE   ('M'-'A')
#define RAMDISK_SIZE  0x1000000

#define CPM_IMAGE  "cpm400.bin"
#define BIOS_IMAGE "simbios.bin"

/* Miscilaneous */

#define S_TIME          (0xff7ff8)  // read long to get time in seconds
#define CPM_EXIT        (0xff7ffc)

/* IRQ connections */
#define IRQ_NMI_DEVICE 7
#define IRQ_MC6850     5


/* ROM and RAM sizes */
#define MAX_ROM 0           // all RAM
#define MAX_RAM 0xffffff    // 16MB of RAM


/* Read/write macros */
#define READ_BYTE(BASE, ADDR) (BASE)[ADDR]
#define READ_WORD(BASE, ADDR) (((BASE)[ADDR]<<8) |	\
			       (BASE)[(ADDR)+1])
#define READ_LONG(BASE, ADDR) (((BASE)[ADDR]<<24) |	\
			       ((BASE)[(ADDR)+1]<<16) |	\
			       ((BASE)[(ADDR)+2]<<8) |	\
			       (BASE)[(ADDR)+3])

#define WRITE_BYTE(BASE, ADDR, VAL) (BASE)[ADDR] = (VAL)&0xff
#define WRITE_WORD(BASE, ADDR, VAL) (BASE)[ADDR] = ((VAL)>>8) & 0xff;	\
  (BASE)[(ADDR)+1] = (VAL)&0xff
#define WRITE_LONG(BASE, ADDR, VAL) (BASE)[ADDR] = ((VAL)>>24) & 0xff;	\
  (BASE)[(ADDR)+1] = ((VAL)>>16)&0xff;					\
  (BASE)[(ADDR)+2] = ((VAL)>>8)&0xff;					\
  (BASE)[(ADDR)+3] = (VAL)&0xff


/* Prototypes */
void exit_error(char* fmt, ...);
int osd_get_char(void);

unsigned int cpu_read_byte(unsigned int address);
unsigned int cpu_read_word(unsigned int address);
unsigned int cpu_read_long(unsigned int address);
void cpu_write_byte(unsigned int address, unsigned int value);
void cpu_write_word(unsigned int address, unsigned int value);
void cpu_write_long(unsigned int address, unsigned int value);
void cpu_pulse_reset(void);
void cpu_set_fc(unsigned int fc);
int cpu_irq_ack(int level);

void nmi_device_reset(void);
void nmi_device_update(void);
int nmi_device_ack(void);

void int_controller_set(unsigned int value);
void int_controller_clear(unsigned int value);

void get_user_input(void);


/* Data */
unsigned int g_quit = 0;			/* 1 if we want to quit */
unsigned int g_nmi = 0;				/* 1 if nmi pending */

int g_MC6850_receive_data = -1;		/* Current value in input device */
int g_MC6850_status = 2;                /* MC6850 status register */
int g_MC6850_control = 0;               /* MC6850 control register */

int g_disk_fds[16];
int g_disk_size[16];
int srecord = 0;
int g_trace = 0;

unsigned int g_int_controller_pending = 0;	/* list of pending interrupts */
unsigned int g_int_controller_highest_int = 0;	/* Highest pending interrupt */

unsigned char g_rom[MAX_ROM+1];					/* ROM */
unsigned char g_ram[MAX_RAM+1];					/* RAM */

#ifdef RAMDISK
unsigned char g_ramdisk[RAMDISK_SIZE];
#endif

unsigned int g_fc;       /* Current function code from CPU */


/* OS-dependant code to get a character from the user.
 */

#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
 
struct termios oldattr;

int kbhit(void)
{
  struct timeval timeout;
  fd_set rdset;

  FD_ZERO(&rdset);
  FD_SET(STDIN_FILENO, &rdset);
  timeout.tv_sec  = 0;
  timeout.tv_usec = 0;

  return select(STDIN_FILENO + 1, &rdset, NULL, NULL, &timeout);  
}

void memdump(int start, int end)
{
  int i = 0;
  while(start < end)
    {
      if((i++ & 0x0f) == 0)
	fprintf(stderr, "\r\n%08x:",start);
      fprintf(stderr, "%02x ", g_ram[start++]);
    }
}


void termination_handler(int signum)
{
  int i;

  tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);  // restore terminal settings

  for(i = 0; i < 16; i++)
    {
      if(g_disk_fds[i] != -1)
	{
	  lseek(g_disk_fds[i], 0, 0);
	  lockf(g_disk_fds[i], F_ULOCK, 0);
	  close(g_disk_fds[i]);
	}
    }
  //  fprintf(stderr, "\nFinal PC=%08x\n",m68k_get_reg(NULL, M68K_REG_PC));

  exit(0);
}

/* Exit with an error message.  Use printf syntax. */
void exit_error(char* fmt, ...)
{
  va_list args;

  tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);  // restore terminal settings


  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");

  exit(EXIT_FAILURE);
}


/* Implementation for the MC6850 like device

   Only those bits of the control register that enable/disable receive and
   transmit interrupts are implemented.

   In the status register, the Receive Data Register Full, Transmit Data
   Register Empty, and IRQ flags are implemented. Although the transmit
   data register is always empty.
 */

void MC6850_reset(void)
{
  g_MC6850_control = 0;
  g_MC6850_status = 2;
  int_controller_clear(IRQ_MC6850);
}

void input_device_update(void)
{
  if(kbhit())
    {
      g_MC6850_status |= 1;
      if((g_MC6850_control & 0x80) && !(g_MC6850_status & 0x80))
	{
	  int_controller_set(IRQ_MC6850);
	  g_MC6850_status |= 0x80;
	}
    }
}

int input_device_ack(void)
{
  return M68K_INT_ACK_AUTOVECTOR;
}

unsigned int MC6850_data_read(void)
{
  char ch;

  int_controller_clear(IRQ_MC6850);
  g_MC6850_status &= ~0x81;          // clear data ready and interrupt flag

  if(read(STDIN_FILENO, &ch, 1) == 1)
    return ch;

  else 
    return -1;
}

int MC6850_status_read()
{
  return g_MC6850_status;
}


/* Implementation for the output device */

void output_device_update(void)
{
  
}

int MC6850_device_ack(void)
{
  return M68K_INT_ACK_AUTOVECTOR;
}

void MC6850_data_write(unsigned int value)
{
  char ch;

  ch = value;
  write(STDOUT_FILENO, &ch, 1);
  //  putc(value, stdout);

  if((g_MC6850_control & 0x60) == 0x20)   // transmit interupt enabled?
    {
      int_controller_clear(IRQ_MC6850);
      int_controller_set(IRQ_MC6850);
    }
}

void MC6850_control_write(unsigned int val)
{
  g_MC6850_control = val;
}

/*
  Disk devices
 */

int g_disk_dma;
int g_disk_drive;
int g_disk_status;
int g_disk_sector;

int g_track;
int g_sector;


void disk_read(int sector)
{
  int i, fd;

  g_disk_status = -1;

#ifdef RAMDISK
  if(g_disk_drive == RAM_DRIVE)
    {
      if(sector > (sizeof(g_ramdisk)/128 - 128))
	return;
      g_disk_status = 0;
      for(i = 0; i < 128; i++)
	WRITE_BYTE(g_ram, g_disk_dma+i, g_ramdisk[sector*128 + i]);
      return;
    }
#endif

  fd = g_disk_fds[g_disk_drive];

  if(fd == -1)                   // verify file is opened
    return;

   // seek to set sector
  if(lseek(fd, sector * 128, SEEK_SET) != sector *128)
    return;

  i = read(fd, &g_ram[g_disk_dma], 128); // read the data

  if(i == 128)
    g_disk_status = 0;

  if(g_trace)
    {
      if(sector == 7536)
	memdump(0x500,0x600);
      fprintf(stderr, "\r\n%08x:%dR\r\n", g_disk_dma, sector);
    }

}
/*
  While refreshing my memory of the write() system call I noticed that it
  mentioned that an interrupt could prevent a write from completing and that
  errno should be checked for the value EINTR to see if a retry is in order.
  Or to use the macro TEMP_FAILURE_RETRY. Which isn't provided by all UNIX
  like systems. This substitute was suggested on comp.os.cpm:
 */
#ifdef __FreeBSD__
#define TEMP_FAILURE_RETRY(expr) \
     ({ long int _res; \
         do _res = (long int) (expr); \
         while (_res == -1L && errno == EINTR); \
         _res; })
#endif 

void disk_write(int sector)
{
  int i, fd, count;

  //  fprintf(stderr, "%d\r\n", sector);

  fd = g_disk_fds[g_disk_drive];
  g_disk_status = -1;

#ifdef RAMDISK
  if(g_disk_drive == RAM_DRIVE)
    {
      if(sector > (sizeof(g_ramdisk)/128 - 128))
	return;
      g_disk_status = 0;
      for(i = 0; i < 128; i++)
	g_ramdisk[sector*128 + i] = READ_BYTE(g_ram, g_disk_dma+i);

      return;
    }
#endif

  if(fd == -1 || sector*128 > g_disk_size[g_disk_drive])
    return;

  if(lseek(fd, sector * 128, SEEK_SET) != sector*128)   // seek to set sector
    return;

  count = 128;
  do {
    i = TEMP_FAILURE_RETRY(write(g_disk_fds[g_disk_drive],
				 &g_ram[g_disk_dma+128-count], count)); 
    if(i == -1)
      return;
    count -= i;
  } while(count > 0);

  g_disk_status = 0;
}

void disk_flush(void)
{
  int i;

  for(i = 0; i < 16; i++)
    if(g_disk_fds[i] != -1)
      fsync(g_disk_fds[i]);
}

/*
  Open a file for use as a CP/M file system. Must specify the drive number,
  filename, and mode.
 */
void open_disk(int fn, char *fname, mode_t flags)
{
  if((g_disk_fds[fn] = open(fname, flags)) == -1)
    {
      fprintf(stderr, "Disk image %s doesn't exist! \n", fname);
      g_disk_size[fn] = 0;
      return;
    }
  lseek(g_disk_fds[fn], 0, 0);         // make sure at start of file
  if(lockf(g_disk_fds[fn], F_TLOCK, 0) == -1)
    {
      close(g_disk_fds[fn]);
      fprintf(stderr, "File %s locked\nOpening read only.\n", fname);
      g_disk_fds[fn] = open(fname, O_RDONLY);
    }
  g_disk_size[fn] = lseek(g_disk_fds[fn], 0, SEEK_END);
}

void init_disks(void)
{
  int i;

  for(i = 0; i < 16; i++)
    g_disk_fds[i] = -1;
}



/* Read data from RAM */
unsigned int cpu_read_byte(unsigned int address)
{
  switch(address)
    {
    case MC6850_DATA:
      return MC6850_data_read();
    case MC6850_STAT:
      return MC6850_status_read();
    default:
      break;
    }

  return READ_BYTE(g_ram, address);
}

unsigned int cpu_read_word(unsigned int address)
{
  switch(address)
    {
    case DISK_STATUS:
      return g_disk_status;
    default:
      break;
    }

  return READ_WORD(g_ram, address);
}

unsigned int cpu_read_long(unsigned int address)
{
  switch(address)
    {
    case DISK_STATUS:
      return g_disk_status;
    case S_TIME:
      return time(NULL);
    default:
      break;
    }
  return READ_LONG(g_ram, address);
}


/* Write data to RAM or a device */
void cpu_write_byte(unsigned int address, unsigned int value)
{
  switch(address)
    {
    case MC6850_DATA:
      MC6850_data_write(value&0xff);
      return;
    case MC6850_STAT:
      MC6850_control_write(value&0xff);
      return;
    default:
      break;
    }

  WRITE_BYTE(g_ram, address, value);
}

void cpu_write_word(unsigned int address, unsigned int value)
{
  WRITE_WORD(g_ram, address, value);
}

void cpu_write_long(unsigned int address, unsigned int value)
{
  switch(address)
    {
    case DISK_SET_DRIVE:
      g_disk_drive = value;
      return;
    case DISK_SET_DMA:
      g_disk_dma = value;
      return;
    case DISK_SET_SECTOR:
      g_disk_sector = value;
      return;
    case DISK_READ:
      disk_read(value);
      return;
    case DISK_WRITE:
      disk_write(value);
      if(g_disk_status == -1)
	fprintf(stderr, "\r\nwrite error: drive:%c  sector: %d\r\n",
		g_disk_drive+'A', g_disk_sector);
      return;
    case DISK_FLUSH:
      disk_flush();
      return;
    case CPM_EXIT:
      fprintf(stderr, "CP/M-68K terminating normally\r\n");
      termination_handler(0);
      return;
    default:
      break;
    }

  WRITE_LONG(g_ram, address, value);
}

/* Called when the CPU pulses the RESET line */
void cpu_pulse_reset(void)
{
  nmi_device_reset();
  MC6850_reset();
}

/* Called when the CPU changes the function code pins */
void cpu_set_fc(unsigned int fc)
{
  g_fc = fc;
}

/* Called when the CPU acknowledges an interrupt */
int cpu_irq_ack(int level)
{
  switch(level)
    {
    case IRQ_NMI_DEVICE:
      return nmi_device_ack();
    case IRQ_MC6850:
      return MC6850_device_ack();
    }
  return M68K_INT_ACK_SPURIOUS;
}




/* Implementation for the NMI device */
void nmi_device_reset(void)
{
  g_nmi = 0;
}

void nmi_device_update(void)
{
  if(g_nmi)
    {
      g_nmi = 0;
      int_controller_set(IRQ_NMI_DEVICE);
    }
}

int nmi_device_ack(void)
{
  printf("\nNMI\n");fflush(stdout);
  int_controller_clear(IRQ_NMI_DEVICE);
  return M68K_INT_ACK_AUTOVECTOR;
}



/* Implementation for the interrupt controller */
void int_controller_set(unsigned int value)
{
  unsigned int old_pending = g_int_controller_pending;

  g_int_controller_pending |= (1<<value);

  if(old_pending != g_int_controller_pending && value > g_int_controller_highest_int)
    {
      g_int_controller_highest_int = value;
      m68k_set_irq(g_int_controller_highest_int);
    }
}

void int_controller_clear(unsigned int value)
{
  g_int_controller_pending &= ~(1<<value);

  for(g_int_controller_highest_int = 7;g_int_controller_highest_int > 0;g_int_controller_highest_int--)
    if(g_int_controller_pending & (1<<g_int_controller_highest_int))
      break;

  m68k_set_irq(g_int_controller_highest_int);
}

unsigned int m68k_read_disassembler_16(unsigned int address)
{
  return cpu_read_word(address);
}

unsigned int m68k_read_disassembler_32(unsigned int address)
{
  return cpu_read_long(address);
}
    
/*
  Print some information on the instruction and state.
 */
void trace(int pc)
{
  char buf[256];

  m68k_disassemble(buf, pc, M68K_CPU_TYPE_68000); 

  fprintf(stderr, "%06x:%s   A0:%06x A1:%06x A2:%06x A3:%06x\r\n", pc, buf,
	  m68k_get_reg(NULL, M68K_REG_A0),
	  m68k_get_reg(NULL, M68K_REG_A1),
	  m68k_get_reg(NULL, M68K_REG_A2),
	  m68k_get_reg(NULL, M68K_REG_A3));

  if(pc > 0x8000)
    termination_handler(0);
}

/*
  Load binary files including cpm400.bin produced from the s-records
  in cpm400.sr.
 */
void load_srecords(void)
{
  int i, fd;


  if((fd = open(CPM_IMAGE, O_RDONLY)) == -1)
    exit_error("Unable to open %s", CPM_IMAGE);

  if((i = read(fd, g_ram, MAX_RAM+1)) == -1)
    exit_error("Error reading %s", CPM_IMAGE);

  fprintf(stderr, "Read %d bytes of CP/M-68K BDOS image.\n", i);

  close(fd);

  if((fd = open(BIOS_IMAGE, O_RDONLY)) == -1)
    exit_error("Unable to open %s", BIOS_IMAGE);

  if(read(fd, g_ram, 8) == -1)         // read initial SP and PC
    exit_error("Error reading %s", BIOS_IMAGE);

  lseek(fd, 0x6000, SEEK_SET);         // skip to BIOS at 0x6000

  if((i = read(fd, &g_ram[0x6000], MAX_RAM+1)) == -1)
    exit_error("Error reading %s", BIOS_IMAGE);

  fprintf(stderr, "Read %d bytes of CP/M-68K BIOS image.\n", i);
}

void load_boot_track(void)
{
  int i;

  if(g_disk_fds[2] == 0)
    {
      fprintf(stderr, "No boot drive available!\n");
      exit(1);
    }
  lseek(g_disk_fds[2], 0, SEEK_SET);
  i =read(g_disk_fds[2], &g_ram[0x400], 32*1024);
  fprintf(stderr, "Read %d bytes from boot track\n", i);


  // Now put in values for the stack and PC vectors
  WRITE_LONG(g_ram, 0, 0xfe0000);   // SP
  WRITE_LONG(g_ram, 4, 0x400);      // PC
}

/* The main loop */
int main(int argc, char* argv[])
{
  int c;
  struct termios newattr;

  init_disks();

  while((c = getopt(argc, argv, "sta:b:")) != -1)
    {
      switch(c)
	{
	case 'a':
	  open_disk(0, optarg, O_RDWR);
	  break;
	case 'b':
	  open_disk(1, optarg, O_RDWR);
	  break;
	case 's':
	  srecord = 1;
	  break;
	case 't':
	  g_trace = 1;
	  break;
	case '?':
	  if(optopt == 'a' || optopt == 'b')
	    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
	  else if (isprint (optopt))
	    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
	  else
	    fprintf (stderr,
		     "Unknown option character `\\x%x'.\n",
		     optopt);
	default:
	  exit(1);
	}
    }
  open_disk(2, DISKC_FILENAME, O_RDWR);
  if(optind != argc)
    exit_error("Usage: cpmsim -a diskimage -s");

  if(srecord)
    load_srecords();
  else
    load_boot_track();


  /*
    Install a handler for various termination events so that we have the
    opportunity to write the simulated file systems back. Plus clean up
    anything else required.
   */
  if (signal (SIGINT, termination_handler) == SIG_IGN)
    signal (SIGINT, SIG_IGN);
  if (signal (SIGHUP, termination_handler) == SIG_IGN)
    signal (SIGHUP, SIG_IGN);
  if (signal (SIGTERM, termination_handler) == SIG_IGN)
    signal (SIGTERM, SIG_IGN);

  /*
    Set the terminal to raw mode (no echo and not cooked) so that it looks
    like a dumb serial port.
   */
  tcgetattr(STDIN_FILENO, &oldattr);
  newattr = oldattr;
  cfmakeraw(&newattr);

  if(g_trace)
    newattr.c_lflag |= ISIG;    // uncomment to process ^C

  newattr.c_cc[VMIN] = 1;       // block until at least one char available
  newattr.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &newattr);


  m68k_pulse_reset();
  MC6850_reset();
  nmi_device_reset();

  while(1)
    {
      if(g_trace)
	trace( m68k_get_reg(NULL, M68K_REG_PC));

      m68k_execute(g_trace ? 1 : 10000); // execute 10,000 MC68000 instructions

      output_device_update();
      input_device_update();
      nmi_device_update();
    }
  return 0;
}
