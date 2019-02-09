/*
 * sun-2 emulator
 *
 * 10/2014  Brad Parker <brad@heeltoe.com>
 */

/* IRQ connections */
#define IRQ_9513_TIMER1	7
#define IRQ_SCC         6
#define IRQ_9513_TIMER2 5
#define IRQ_SW_INT3     3
#define IRQ_SW_INT2     2
#define IRQ_SC          2
#define IRQ_SW_INT1     1

/* ROM and RAM sizes */
#define MAX_ROM 32768       // 32k ROM
#define MAX_RAM 0xffffff    // 16MB of RAM

void abortf(const char *fmt, ...);
void enable_trace(int);

unsigned int scc_read(unsigned int pa, int size);
void scc_write(unsigned int pa, unsigned int value, int size);
int scc_device_ack(int which);
void scc_update(void);
int scc_in_pop(int ch, unsigned int *pv);
void scc_in_push(int ch, int v);

unsigned int am9513_read(unsigned int pa, int size);
void am9513_write(unsigned int pa, unsigned int value, int size);
int am9513_device_ack(int which);
void am9513_update(void);

unsigned mm58167_read(unsigned pa, int size);
void mm58167_write(unsigned pa, unsigned value, int size);
void mm58167_update(void);

unsigned int sc_read(unsigned address, int size);
void sc_write(unsigned int address, int size, unsigned int value);
int sc_device_ack(void);

void sc_set_cmd_reg(unsigned int v);
unsigned int sc_get_data(void);
void sc_reset_odd_len(void);
void sc_dma_read_data(unsigned char *buf, int bufsiz);
void sc_dma_write_data(unsigned char *buf, int bufsiz);


unsigned int sun2_video_read(unsigned int address, int size);
unsigned int sun2_video_write(unsigned int address, int size, unsigned int value);
unsigned int sun2_kbm_read(unsigned int address, int size);
unsigned int sun2_kbm_write(unsigned int address, int size, unsigned int value);
unsigned int sun2_video_ctl_read(unsigned int address, int size);
unsigned int sun2_video_ctl_write(unsigned int address, int size, unsigned int value);
void sun2_kb_write(int value, int size);

void int_controller_set(unsigned int irq);
void int_controller_clear(unsigned int irq);

int sw_int_ack(int intr);
void sdl_poll(void);
void sun2_init(void);

void m68k_mark_buserr(void);
void m68k_set_buserr(unsigned int pc);

unsigned int cpu_map_address(unsigned int address, unsigned int fc, int m, unsigned int *mtype, unsigned int *pfault, unsigned int *ppte);

void abortf(const char *fmt, ...);

unsigned int cpu_read(int size, unsigned int address);
void cpu_write(int size, unsigned int address, unsigned int value);

