#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "../../m68k/m68k.h"

struct exec {
	unsigned char a_vers;
	unsigned char a_machtype;
	unsigned short a_magic;
	unsigned int a_text;
	unsigned int a_data;
	unsigned int a_bss;
	unsigned int a_syms;
	unsigned int a_entry;
	unsigned int a_trsize;
	unsigned int a_drsize;
};

struct nlist {
	int n_strx;
	unsigned char n_type;
	char n_other;
	short n_desc;
	unsigned n_value;
};

int debug = 0;
int disasm = 0;
unsigned char f[1024*1024];

void dumpbuffer(char *buf, int len)
{
    unsigned char *b = (unsigned char *)buf;
    int offset = 0;
    int i;
    char lb[17];

    while (len > 0) {
        printf("%03x: ", offset);
        for (i = 0; i < 8; i++) {
            if (i < len) {
                printf("%02x ", b[i]);
                lb[i] = (b[i] >= ' ' && b[i] <= '~') ? b[i] : '.';
            } else {
                printf("xx ");
                lb[i] = 'x';
            }
        }
        lb[8] = 0;
        
        printf(" %s\n", lb);
        len -= 8;
        b += 8;
        offset += 8;
    }
}

#define MAX_SYMS 10000
struct sym_s {
	char *sym;
	unsigned int v;
	unsigned int t, d;
} syms[MAX_SYMS];
int sym_count;

void add_sym(unsigned int v, char *s, unsigned int t, unsigned int d)
{
	syms[sym_count].sym = strdup(s);
	syms[sym_count].v   = v;
	syms[sym_count].t   = t;
	syms[sym_count].d   = d;
	sym_count++;
}

void sort_syms(void)
{
	int i, j, swapped;
	for (i = 0; i < sym_count-1; i++) {
		swapped = 0;
		for (j = 0; j < sym_count-1; j++) {
			if (syms[j].v > syms[j+1].v) {
				struct sym_s s;
				swapped++;
				s = syms[j];
				syms[j] = syms[j+1];
				syms[j+1] = s;
			}
		}
		if (!swapped)
			break;
	}
}

char map_type(int t)
{
	switch (t) {
	case 0: return 'U';
	case 2: return 'a';
	case 3: return 'A';
	case 4: return 't';
	case 5: return 'T';
	case 6: return 'd';
	case 7: return 'D';
	case 8: return 'b';
	case 9: return 'B';
	}
	return '?';
}

void print_syms(void)
{
	int i;
	for (i = 0; i < sym_count; i++) {
		if (debug)
			printf("%08x %x %s\n", syms[i].v, syms[i].t, syms[i].sym);
		else
			printf("%08x %c %s\n", 	syms[i].v, map_type(syms[i].t), syms[i].sym);
	}
}

unsigned int n_txtoff, n_datoff, n_treloff, n_dreloff, n_symoff, n_stroff;
struct nlist *nl;

int read_aout(const char *fname)
{
	int r, fd;
	struct exec *eh;
	int i, nls;
	char *s;

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		perror(fname);
		exit(2);
	}

	r = read(fd, f, sizeof(f));
	close(fd);

	if (r < sizeof(struct exec)) {
		fprintf(stderr, "short read?");
		exit(3);
	}

	eh = (struct exec *)f;

	if (debug) {
		printf("file bytes %d\n", r);
		printf("magic    %o\n", ntohs(eh->a_magic));
		printf("machtype %o\n", ntohs(eh->a_machtype));
	}

	n_txtoff  = eh->a_machtype == 0 ? 0x800 : 0x20;
	n_datoff  = n_txtoff  + ntohl(eh->a_text);
	n_treloff = n_datoff  + ntohl(eh->a_data);
	n_dreloff = n_treloff + ntohl(eh->a_trsize);
	n_symoff  = n_dreloff + ntohl(eh->a_drsize);
	n_stroff  = n_symoff  + ntohl(eh->a_syms);

 	nl = (struct nlist *)&f[n_symoff];
	nls = ntohl(eh->a_syms);

	if (debug) {
		printf("a_text %d, a_data %d, a_bss %d, a_syms %d, a_entry %d, a_trsize %d, a_drsize %d\n",
		       ntohl(eh->a_text), ntohl(eh->a_data), ntohl(eh->a_bss),
		       ntohl(eh->a_syms), ntohl(eh->a_entry), ntohl(eh->a_trsize), ntohl(eh->a_drsize));

		printf("n_txtoff %d, n_datoff %d, n_treloff %d, n_dreloff %d, n_symoff %d, n_stroff %d\n",
		       n_txtoff, n_datoff, n_treloff, n_dreloff, n_symoff, n_stroff);

		printf("\n");
		printf("sizeof(struct nlist) %d\n", sizeof(struct nlist));
	}

	if (debug) {
		printf("nlist:\n");
		dumpbuffer((char *)nl, 64);
		printf("strings:\n");
		dumpbuffer((char *)&f[ n_stroff], 128);
	}

	while (nls > 0) {
		int stroff;

		if (debug) {
			printf("n_strx %d, n_type 0x%x, n_desc 0x%x, n_value %08x\n",
			       htonl(nl->n_strx), nl->n_type, htons(nl->n_desc), htonl(nl->n_value));
		}

		stroff = htonl(nl->n_strx);
		s = &f[ n_stroff + stroff ];
		if (debug) printf("%08x x %s\n", htonl(nl->n_value), s);
		if (nl->n_type & 0xe0)
			;
		else
			add_sym(htonl(nl->n_value), s, nl->n_type, htons(nl->n_desc));

		nl++;
		nls -= 12;
	}

	if (debug) printf("---------\n");
}

unsigned int m68k_read_disassembler_16(unsigned int address)
{
	int offset = address - 0x4000;
	unsigned char *s = &f[n_txtoff];

	return (s[offset] << 8) | s[offset+1];
}

unsigned did_read32;

unsigned int m68k_read_disassembler_32(unsigned int address)
{
	did_read32 = address;
	return (m68k_read_disassembler_16(address) << 16) |
		m68k_read_disassembler_16(address+2);
}

int lookup_sym(unsigned int v, char **psym, int *poffset)
{
	int offset, i;

	offset = 0;
	for (i = 0; i < sym_count; i++) {
		if ((syms[i].t & 1) == 0)
			continue;
		if (v == syms[i].v)
			break;
		if (syms[i].v > v) {
			i--;
			offset = v - syms[i].v;
			break;
		}
	}

	if (i >= sym_count)
		return 0;

	*psym = syms[i].sym;
	*poffset = offset;
	return 1;
}

void disassemble(void)
{
	unsigned char *s, *e;
	unsigned pc;
	s = &f[n_txtoff];
	e = &f[n_datoff];

	pc = 0x4000;
	while (s < e) {
		unsigned short w, last_w;
		int i, offset;
		char *sym;

		w = (s[0] << 8) | s[1];

		if (lookup_sym(pc, &sym, &offset)) {
			if (offset == 0) {
				printf("%s:\n", sym);
			}
		}

		if (0) {
			printf(" %08x: %04x\n", pc, w);
			s += 2;
			pc += 2;
		} else {
			char buf[256];
			int b;

			if (last_w == 0x4e75 && w == 0) {
				printf(" %08x: %04x\n", pc, w);
				s += 2;
				pc += 2;
			} else {
				char comment[128];
				comment[0] = 0;

#if 0
2a79
267c

4ab8
4ab9

5ab8
b0b8
dfb8

2038
2039
2078
2f38
2f39

0cab
04ab
#endif
				if (w == 0x4eb9) {
					unsigned l = m68k_read_disassembler_32(pc+2);
					if (lookup_sym(l, &sym, &offset))
						;
					if (offset == 0)
						sprintf(comment, "\t; %s", sym);
					else
						sprintf(comment, "\t; %s+0x%x", sym, offset);
				}

				did_read32 = 0;
				b = m68k_disassemble(buf, pc, M68K_CPU_TYPE_68010); 
				if (did_read32) {
					unsigned l = m68k_read_disassembler_32(pc+2);
					if (l > 0x2000 && lookup_sym(l, &sym, &offset)) {
						if (offset == 0)
							sprintf(comment, "\t; %s", sym);
						else
							sprintf(comment, "\t; %s+0x%x", sym, offset);
					}
				}
				printf(" %08x: %04x %s%s\n", pc, w, buf, comment);
				s += b;
				pc += b;
			}
			last_w = w;
		}
	}
}

void usage(void)
{
	fprintf(stderr, "\n");
	exit(1);
}

main(int argc, char *argv[])
{
	int i;
	char *fname = NULL;

	if (argc < 2) 
		usage();

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'd': debug++; break;
			case 'D': disasm++; break;
			}
		} else
			fname = argv[i];
	}

	read_aout(fname);
	sort_syms();

	if (disasm)
		disassemble();
	else
		print_syms();

	exit(0);
}
