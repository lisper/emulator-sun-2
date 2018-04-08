/*
 * sun-2 emulator
 *
 * 10/2014  Brad Parker <brad@heeltoe.com>
 */

/* IRQ connections */
#define IRQ_9513_TIMER1	7
#define IRQ_9513_TIMER2 5
#define IRQ_SC 2

/* ROM and RAM sizes */
#define MAX_ROM 32768       // 32k ROM
#define MAX_RAM 0xffffff    // 16MB of RAM

void abortf(const char *fmt, ...);
void enable_trace(int);
