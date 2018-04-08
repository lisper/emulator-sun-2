#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int debug = 1;
unsigned char f[1024*512];

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
	if (t == 0) return 'U';
	if (t == 2) return 'a';
	if (t == 4) return 't';
	if (t == 6) return 'd';
	if (t == 8) return 'b';
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

main()
{
	int r;
	struct exec *eh;
	unsigned int n_txtoff, n_datoff, n_treloff, n_dreloff, n_symoff, n_stroff;
	struct nlist *nl;
	int i, nls;
	char *s;

	r = read(0, f, sizeof(f));
	eh = (struct exec *)f;

	printf("file bytes %d\n", r);
	printf("magic %o\n", ntohs(eh->a_magic));

	n_txtoff  = 0x800/*0*/;
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
		add_sym(htonl(nl->n_value), s, nl->n_type, htons(nl->n_desc));

		nl++;
		nls -= 12;
	}

	if (debug) printf("---------\n");

	sort_syms();
	print_syms();

	exit(0);
}
