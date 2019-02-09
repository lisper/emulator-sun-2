#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct {
	unsigned pc, t;
	char *s;
} syms[5000];
int nsyms;

int read_syms(const char *fn)
{
	FILE *f;
	char line[1024];
	f = fopen(fn, "r");
	if (f) {
		while (fgets(line, sizeof(line), f)) {
			int n;
			unsigned pc;
			char t;
			char *p;
			char sym[1024];
			n = sscanf(line, "%08x %c %s", &pc, &t, sym);
			if (n != 3) {
				if (n == 2 && t == 't')
					continue;
				printf("bad line: %s\n", line);
				return -1;
			}
			p = malloc(strlen(sym)+1);
			strcpy(p, sym);
			syms[nsyms].pc = pc;
			syms[nsyms].t = t;
			syms[nsyms].s = p;
			nsyms++;
		}

		fclose(f);
	}
	if (0) printf("%d symbols\n", nsyms);
	return 0;
}

char buf[1024];

char *lookup(unsigned pc)
{
	unsigned offset = 0;
	int i;
	for (i = 0; i < nsyms; i++) {
		if (pc > syms[i].pc)
			continue;
		if (pc == syms[i].pc || i == 0) {
			break;
		}
		i--;
		offset = pc - syms[i].pc;
		break;
	}
	if (i == 0) return "";
	if (offset == 0)
		sprintf(buf, "%s", syms[i].s);
	else
		sprintf(buf, "%s+0x%x", syms[i].s, offset);
	return buf;
}

int annotate(const char *fn)
{
	FILE *f;
	char line[1024];
	f = fopen(fn, "r");
	if (f) {
		while (fgets(line, sizeof(line), f)) {
			unsigned pc, sr;
			char *s;
			int l = strlen(line);
			sscanf(line, "PC %x sr %x", &pc, &sr);
			if (0) printf("pc %x\n", pc);
			if (line[l-1] == '\n') line[l-1] = 0;
			if (sr >= 0x2000) {
				s = lookup(pc);
				if (l < 81) strcat(line, "\t");
				strcat(line, "\t");
				strcat(line, s);
			} else {
			}
			printf("%s\n", line);
		}
		fclose(f);
	}
	return 0;
}

main(int argc, char *argv[])
{
	char *ksym_name, *log_name;

	ksym_name = "ksyms-sunos3.2";
	log_name = "x2";

	if (argc > 1) {
		ksym_name = argv[1];
		log_name = argv[2];
	}

	read_syms(ksym_name);
	annotate(log_name);
	exit(0);
}
