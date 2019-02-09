/*
 * tdump.c
 * trace dump
 */
#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "../m68k/m68k.h"

int fd;
int dumping = 0;
int skip_eeprom = 0;

typedef struct record_s {
    unsigned int what;
    unsigned int data[20];
    unsigned pc, sr;
} record_t;

#define MAX_KSYM 5000
struct {
    unsigned pc;
    char t;
    char *sym;
} ksym[MAX_KSYM];
int ksym_count;

void size_file(int fd)
{
    off64_t cp, lsize;
    unsigned long ic;
    cp = lseek64(fd, (size_t)0, SEEK_CUR);
    lsize = lseek64(fd, (size_t)0, SEEK_END);
    lseek64(fd, cp, SEEK_SET);   // seek back to the beginning of file
    ic = (unsigned long)(lsize / (sizeof(unsigned int)*18));
    printf("file size %llu, instructions %lu (%uM)\n",
	   lsize, ic, (unsigned int)(ic/(1024*1024)));
}

typedef struct {
    unsigned address;
    unsigned value;
    int bits;
} mc_t;

#define MEM_CACHE_SIZE 32
#define MEM_CACHE_MASK 0x1f

mc_t mc[MEM_CACHE_SIZE];
int mc_wptr;

#define MAX_MEMORY_BYTES 8*1024*1024
unsigned char memory[MAX_MEMORY_BYTES];
unsigned int memory_marks[(MAX_MEMORY_BYTES)/32];

void memory_mark(unsigned int addr, int bytes);

void memory_cache_set(unsigned address, unsigned data, int size)
{
    int i;

    if (0) printf("memory_cache_set(address=%08x, data=%08x, size=%d)\n", address, data, size);

    for (i = 0; i < MEM_CACHE_SIZE; i++) {
	if (mc[i].address == address) {
	    mc[i].address = address;
	    mc[i].value = data;
	    mc[i].bits = size * 8;
	    return;
	}
    }

    mc[mc_wptr].address = address;
    mc[mc_wptr].value = data;
    mc[mc_wptr].bits = size * 8;
    mc_wptr = (mc_wptr + 1) & MEM_CACHE_MASK;

    // emulate the actual memory
    if (address < 0x800000) {
	switch (size) {
	case 1:
	    memory[address] = data;
	    memory_mark(address, 1);
	    break;
	case 2:
	    memory[address+0] = data >> 8;
	    memory[address+1] = data & 0xff;
	    memory_mark(address, 2);
	    break;
	case 4:
	    memory[address+0] = data >> 24;
	    memory[address+1] = data >> 16;
	    memory[address+2] = data >> 8;
	    memory[address+3] = data >> 0;
	    memory_mark(address, 4);
	    break;
	}
    }
}

unsigned memory_cache_read(int bits, unsigned address)
{
    int i;

    for (i = 0; i < MEM_CACHE_SIZE; i++) {
	switch (bits) {
	case 16:
	    switch (mc[i].bits) {
	    case 16:
		if (address == mc[i].address)
		    return mc[i].value;
		break;
	    case 32:
		if (address == mc[i].address)
		    return mc[i].value >> 16;
		if (address == mc[i].address + 2)
		    return mc[i].value & 0xffff;
		break;
	    }
	    break;
	case 32:
	    switch (mc[i].bits) {
	    case 16:
		break;
	    case 32:
		if (address == mc[i].address)
		    return mc[i].value;
		if (address == mc[i].address+2) {
		    unsigned h, l;
		    int n;
		    h = ((mc[i].value & 0xffff)<<16);
		    n = (i+1) & MEM_CACHE_MASK;
		    if (address+2 == mc[n].address) {
			l = mc[n].value >> 16;
			return h | l;
		    }
		}
		break;
	    }
	    break;
	}
    }

#if 0
    printf("\n\n------\n");
    printf("cache miss; address %x bits %d\n", address, bits);
    for (i = 0; i < MEM_CACHE_SIZE; i++) {
	printf("%d: %x %x %d\n",
	       i, mc[i].address, mc[i].value, mc[i].bits);
    }
    exit(1);
#else
    if (address < 0x800000) {
	unsigned int data;
	// emulate the actual memory
	switch (bits) {
	case 8:
	    data = memory[address];
	    break;
	case 16:
	    data = (memory[address+0] << 8) | memory[address+1];
	    break;
	case 32:
	    data =
		(memory[address+0] << 24) |
		(memory[address+1] << 16) |
		(memory[address+2] <<  8) |
		(memory[address+3] <<  0);
	    break;
	}
	if (0) printf("cache miss; address %x bits %d; memory -> %08x\n", address, bits, data);
	return data;
    }
#endif
}

void memory_mark(unsigned int addr, int bytes)
{
    int i;

    for (i = 0; i < bytes; i++) {
	int index = addr / 32;
	int offset = addr % 32;
	memory_marks[index] |= 1 << offset;
	addr++;
    }
}

void memory_save(char *fname)
{
    int i, fd, r;

    for (i = ((MAX_MEMORY_BYTES)/32)-1; i >= 0; i--) {
	if (memory_marks[i])
	    break;
    }
    i++;

    fd = open(fname, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd < 0) {
	perror(fname);
	exit(2);
    }

    r = write(fd, memory, i*4);
    close(fd);
}

void memory_read(char *fname)
{
    int i, fd, r;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
	perror(fname);
	exit(2);
    }

    r = read(fd, memory, MAX_MEMORY_BYTES);
    close(fd);

    for (i = 0; i < r/32; i++) {
	memory_marks[i] = 0xffffffff;
    }
}


int rbsize;
unsigned int *rbptr;
char rbuff[64*1024];

unsigned int get_int32(void)
{
    unsigned int v;
    int r;

    if (0) printf("rbsize %d\n",rbsize);
    if (rbsize < sizeof(unsigned int)) {
	if (rbsize > 0) {
	    memcpy(rbuff, (char *)rbptr, rbsize);
	}

	r = read(fd, rbuff + rbsize, 32*1024);
	if (0) printf("read; r %d\n", r);
	if (r < 0) {
	    rbsize = -1;
	    return 0;
	}
	rbsize += r;
	rbptr = (unsigned int *)&rbuff;
    }

    v = *rbptr;
    rbptr++;
    rbsize -= sizeof(unsigned int);
    return v;
}

unsigned int get_eof(void)
{
    if (rbsize < 0)
	return -1;
    return 0;
}

int get_next_record(record_t *r)
{
    int w, ww, wi, ws, i;

    if (get_eof())
	return -1;

    ww = get_int32();
    wi = ww >> 8;
    ws = ww & 0xff;

    r->what = wi;
    if (0) printf("w %x wi %d ws %d\n", ww, wi, ws);

    r->data[0] = ww;
    for (i = 0; i < ws-1; i++)  {
 	r->data[i+1] = get_int32();
    }

    r->pc = r->data[0];
    return 0;
}

#define CACHE_SIZE 16
#define CACHE_MASK 0xf
record_t cache[CACHE_SIZE];
int cache_count;
int cache_wptr;
int cache_rptr;

int get_next(record_t *rp)
{
    int r, eof = 0;

    while (cache_count < CACHE_SIZE) {
	r = get_next_record(&cache[cache_wptr]);

	{
	    unsigned size;
	    record_t *r = &cache[cache_wptr];
	    if (r->what == 22) {
		size = r->data[2] & 0xffff;
		memory_cache_set(r->data[4], r->data[5], size);
	    }
	}

	cache_wptr = (cache_wptr + 1) & CACHE_MASK;
	cache_count++;
	if (r < 0) eof++;
    }

    *rp = cache[cache_rptr];
    cache_rptr = (cache_rptr + 1) & CACHE_MASK;
    cache_count--;

    return eof ? -1 : 0;
}

unsigned int m68k_read_disassembler_16(unsigned int address)
{
    //printf("m68k_read_disassembler_16(%x)\n", address);
    return memory_cache_read(16, address);
}

unsigned int m68k_read_disassembler_32(unsigned int address)
{
    //printf("m68k_read_disassembler_32(%x)\n", address);
    return memory_cache_read(32, address);
}

void dump_one(unsigned long cycles, record_t *r)
{
    int size;
    unsigned pc, sr;

    switch (r->what) {
    case 1:
    case 2:
	printf("\n");
	pc = r->data[1];
	sr = r->data[2];

	if (r->data[2] >= 0x2000 && pc < 0xf00000) {
	    int i, offset = 0;
	    char *ks;
	    for (i = 0; i < ksym_count; i++) {
		if (pc > ksym[i].pc)
		    continue;
		if (pc == ksym[i].pc)
		    break;
		if (pc < ksym[i].pc) {
		    i--;
		    offset = pc - ksym[i].pc;
		    break;
		}
	    }

	    //printf("pc %x, i %d, offset %d %s:%x\n", pc, i, offset, ksym[i].sym, ksym[i].pc);
	    if (i >= ksym_count)
		ks = "";
	    else
		ks = ksym[i].sym;

	    if (offset > 0)
		printf("%s+0x%x ", ks, offset);
	    else
		printf("%s ", ks);
	}

	printf("%lu: pc %06x sr %04x ", cycles, pc, sr);

	{
	    char buf[256];
	    m68k_disassemble(buf, pc, M68K_CPU_TYPE_68010); 
	    printf("%s\n", buf);
	}

#if 1
	printf(" D0:%08x D1:%08x D2:%08x D3:%08x D4:%08x D5:%08x D6:%08x D7:%08x\n",
	       r->data[11], r->data[12], r->data[13], r->data[14], 
	       r->data[15], r->data[16], r->data[17], r->data[18]);
	printf(" A0:%08x A1:%08x A2:%08x A3:%08x A4:%08x A5:%08x A6:%08x A7:%08x\n",
	       r->data[3], r->data[4], r->data[5], r->data[6], 
	       r->data[7], r->data[8], r->data[9], r->data[10]);
#endif
	break;
    case 11:
	printf(" tlb fill; ");
	printf(" pc %06x address %08x va %08x bus_type %d boot %d pte %08x\n",
	       r->data[1], r->data[2], r->data[3], r->data[4], r->data[5], r->data[6]);
	break;
    case 12:
	printf(" pte set; ");
	printf(" pc %06x address %08x pa %08x bus_type %d context_user %d pte %08x\n",
	       r->data[1], r->data[2], r->data[3], r->data[4], r->data[5], r->data[6]);
    	break;
    case 21:
	size = r->data[2] & 0xffff;
	printf(" mem write; ");
	printf(" pc %06x size %d fc %d ea %08x value %08x\n",
	       r->data[1], size, r->data[3], r->data[4], r->data[5]);

	{
	    int mtype, fault;
	    mtype = (r->data[2] >> 16) & 0xff;
	    fault = (r->data[2] >> 24) & 0xff;
	    printf(" pa %x mtype %d fault %d pte %08x\n", r->data[6], mtype, fault, r->data[7]);
	}
	break;
    case 22:
	size = r->data[2] & 0xffff;
	printf(" mem read; ");
	printf(" pc %06x size %d fc %d ea %08x value %08x\n",
	       r->data[1], size, r->data[3], r->data[4], r->data[5]);
	{
	    int mtype, fault;
	    mtype = (r->data[2] >> 16) & 0xff;
	    fault = (r->data[2] >> 24) & 0xff;
	    printf(" pa %x mtype %d fault %d pte %08x\n", r->data[6], mtype, fault, r->data[7]);
	}
	break;
    }
}

void dump(unsigned int start_isn, unsigned int max_cycles)
{
    record_t record;
    unsigned long cycles;
    int r;
    unsigned old_pc, old_sr;
    unsigned new_pc, new_sr;

    cycles = 0;
    while (1) {
	if (0) printf("get %lu\n", cycles);
	r = get_next(&record);
	if (r < 0) {
	    printf("eof\n");
	    break;
	}

	//printf("xxx %ld: %d %x %x\n", cycles, record.what, record.data[1], record.data[2]);

	//if (record.data[2] < 0x2000 && record.what == 2)
	//printf("xxx %ld: %d %x %x\n", cycles, record.what, record.data[1], record.data[2]);

#if 1
	if (record.what == 1 || record.what == 2) {
	    new_pc = record.data[1];
	    new_sr = record.data[2];

	    if (start_isn > 0 && cycles > start_isn)
		dumping = 1;

	    if (new_sr < 0x2000) dumping = 1;

	    if (skip_eeprom && (new_pc & 0x00ff0000) == 0xef0000)
		dumping = 0;
	}
#endif

	if (dumping) {
	    dump_one(cycles, &record);
	    if ((record.what == 1 || record.what == 2) && record.data[2] >= 0x2000)
		dumping = 0;
	} else {
#if 0
		if (record.data[2] < 0x2000) {
		    dumping = 1;
		}
		//printf("%lu: pc %x sr %x\n", cycles, record[1], record[2]);
#endif

#if 0
	    if (record.what == 1 || record.what == 2) {
		new_pc = record.data[1];
		new_sr = record.data[2];

		if (old_sr >= 0x2000 && new_sr < 0x2000) {
		    printf("enter userland\n");
		    dumping = 1;
		}
		if (old_sr < 0x2000 && new_sr >= 0x2000) {
		    printf("exit userland\n");
		    dumping = 1;
		}

		old_pc = new_pc;
		old_sr = new_sr;
	    }
#endif

	}

	cycles++;
	if (max_cycles > 0 && cycles >= max_cycles)
	    break;
    }

    printf("%lu cycles\n", cycles);
}

void read_kmap(char *fname)
{
    FILE *f;
    char line[1024];

    f = fopen(fname, "r");
    if (f == NULL) {
	perror(fname);
	exit(1);
    }

    while (fgets(line, sizeof(line), f)) {
	unsigned int _pc;
	char _t;
	char _sym[256];
	sscanf(line, "%08x %c %s", &_pc, &_t, _sym);
	if (0) printf("%08x %c %s\n", _pc, _t, _sym);
	ksym[ksym_count].pc = _pc;
	ksym[ksym_count].t = _t;
	ksym[ksym_count].sym = strdup(_sym);
	ksym_count++;
    }

    ksym[ksym_count].sym = NULL;

    printf("%d kernel symbols\n", ksym_count);
    fclose(f);
}

void usage(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "tdump { -d <start-isn> } { -f <fname> } { -k <kmap-name> } { -m <max-cycles> }\n");
    exit(1);
}

extern char *optarg;

main(int argc, char *argv[])
{
    int c;
    int do_dump;
    char *fname, *kmapname, *save_mem_fname, *read_mem_fname;
    unsigned int start_isn, max_cycles;

    do_dump = 0;

    fname = "trace.bin";
    save_mem_fname = NULL;
    read_mem_fname = NULL;
    kmapname = (char *)0;
    start_isn = 1;
    max_cycles = 10;
    skip_eeprom = 1;

    if (argc == 0)
	usage();

    while ((c = getopt(argc, argv, "d:f:m:k:s:r:")) != -1) {
	switch (c) {
	case 'd':
	    do_dump++;
	    start_isn = atoi(optarg);
	    break;
	case 'f':
	    fname = strdup(optarg);
	    break;
	case 'k':
	    kmapname = strdup(optarg);
	    break;
	case 'm':
	    max_cycles = atoi(optarg);
	    break;
	case 's':
	    save_mem_fname = strdup(optarg);
	    break;
	case 'r':
	    read_mem_fname = strdup(optarg);
	    break;
	}
	
    }

    if (read_mem_fname) {
	memory_read(read_mem_fname);
    }

    fd = open(fname, O_RDONLY | O_LARGEFILE);

    if (kmapname) {
	read_kmap(kmapname);
    }

    if (do_dump) {
	size_file(fd);
	dump(start_isn, max_cycles);
    }

    if (save_mem_fname) {
	memory_save(save_mem_fname);
    }

    exit(0);
}


/* Local Variables:  */
/* mode: c           */
/* c-basic-offset: 4 */
/* End:              */

