/*
 * sun-2 emulator
 *
 * 10/2014  Brad Parker <brad@heeltoe.com>
 */

#ifndef SIM__HEADER
#define SIM__HEADER

unsigned int cpu_read_byte(unsigned int address);
unsigned int cpu_read_word(unsigned int address);
unsigned int cpu_read_long(unsigned int address);
void cpu_write_byte(unsigned int address, unsigned int value);
void cpu_write_word(unsigned int address, unsigned int value);
void cpu_write_long(unsigned int address, unsigned int value);
void cpu_pulse_reset(void);
void cpu_set_fc(unsigned int fc);
int  cpu_irq_ack(int level);

unsigned int cpu_read(int size, unsigned int address);
void cpu_write(int size, unsigned int address, unsigned int value);

#endif /* SIM__HEADER */
