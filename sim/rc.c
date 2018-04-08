/*
 * rc.c
 * record compare - used to compare trace records to find cpu deviations
 * (i.e. compares cpu flows against golden model)
 */
#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int fd1, fd2;

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

int rbsize[2];
unsigned int *rbptr[2];
char rbuff[2][64*1024];

unsigned int get_int32(int which)
{
    int w = which - 1;
    unsigned int v;
    int fd, r;

    //printf("w %d, rbsize[w] %d\n",w, rbsize[w]);
    if (rbsize[w] < sizeof(unsigned int)) {
	if (rbsize[w] > 0) {
	    memcpy(rbuff[w], (char *)rbptr[w], rbsize[w]);
	}

	fd = w ? fd2 : fd1;
	r = read(fd, rbuff[w] + rbsize[w], 32*1024);
	//printf("read; r %d\n", r);
	if (r < 0) {
	    rbsize[w] = -1;
	    return 0;
	}
	rbsize[w] += r;
	rbptr[w] = (unsigned int *)&rbuff[w];
    }

    v = *rbptr[w];
    //printf("rbuff[w] %p, rbptr[w] %p, v %08x\n", rbuff[w], rbptr[w], v);
    rbptr[w]++;
    rbsize[w] -= sizeof(unsigned int);
    return v;
}

unsigned int get_eof(int which)
{
    int w = which - 1;
    if (rbsize[w] < 0)
	return -1;
    return 0;
}

int get_next(int which, unsigned int *record, int *pwhat)
{
    int w, ww, wi, ws, i;

    if (get_eof(which))
	return -1;

    ww = get_int32(which);
    wi = ww >> 8;
    ws = ww & 0xff;

    *pwhat = wi;
    //printf("w %x wi %d ws %d\n", ww, wi, ws);

    record[0] = ww;
    for (i = 0; i < ws-1; i++)  {
 	record[i+1] = get_int32(which);
    }

#if 0
   switch (ws) {
    case 8:
	record[0] = ww;
	record[1] = get_int32(which);
	record[2] = get_int32(which);
	record[3] = get_int32(which);
	record[4] = get_int32(which);
	record[5] = get_int32(which);
	record[6] = get_int32(which);
	record[7] = get_int32(which);
	break;

    default:
    case 20:
	record[0] = ww;
	record[1] = get_int32(which);
	record[2] = get_int32(which);
	record[3] = get_int32(which);
	record[4] = get_int32(which);
	record[5] = get_int32(which);
	record[6] = get_int32(which);
	record[7] = get_int32(which);
	record[8] = get_int32(which);
	record[9] = get_int32(which);
	record[10] = get_int32(which);
	record[11] = get_int32(which);
	record[12] = get_int32(which);
	record[13] = get_int32(which);
	record[14] = get_int32(which);
	record[15] = get_int32(which);
	record[16] = get_int32(which);
	record[17] = get_int32(which);
	record[18] = get_int32(which);
	record[19] = get_int32(which);
	//printf("record %x %x %x\n", record[0], record[1], record[2]);
	break;
    }
#endif
}

void compare(void)
{
    unsigned int record1[20];
    unsigned int record2[20];
    int ok;
    unsigned long cycles;
    unsigned int last1, last2;
    int mismatches;

    last1 = last2 = 0;
    mismatches = 0;
    cycles = 0;
    while (1) {
	int rs, r1, r2, what;

	ok = 1;

	while (1) {
	    r1 = get_next(1, record1, &what);
	    if (r1 < 0) { printf("eof on file 1\n"); break; }
	    if (what == 1 || what == 2)
		break;
	}

	while (1) {
	    r2 = get_next(2, record2, &what);
	    if (r2 < 0) { printf("eof on file 2\n"); break; }
	    if (what == 1 || what == 2)
		break;
	}

	if (record1[1] != record2[1]) { printf("pc! "); ok = 0; }
	if (record1[2] != record2[2]) {
	    if (cycles > 10) {
		printf("sr! %x %x ", record1[2], record2[2]);
		ok = 0;
	    }
	}
	if (record1[3] != record2[3]) { printf("A0! %x %x ", record1[3], record2[3]); ok = 0; }
	if (record1[4] != record2[4]) { printf("A1! %x %x ", record1[4], record2[4]); ok = 0; }
	if (record1[5] != record2[5]) { printf("A2! %x %x ", record1[5], record2[5]); ok = 0; }
//	if (record1[6] != record2[6]) { printf("A3! %x %x ", record1[6], record2[6]); ok = 0; }
//	if (record1[7] != record2[7]) { printf("A4! %x %x ", record1[7], record2[7]); ok = 0; }
	if (record1[8] != record2[8]) { printf("A5! %x %x ", record1[7], record2[8]); ok = 0; }
	if (record1[9] != record2[9]) { printf("A6! %x %x ", record1[8], record2[9]); ok = 0; }
	if (record1[10] != record2[10]) { printf("A7! %x %x ", record1[10], record2[10]); ok = 0; }

	if (record1[11] != record2[11]) { printf("D0! %x %x ", record1[11], record2[11]); ok = 0; }
	if (record1[12] != record2[12]) { printf("D1! %x %x ", record1[12], record2[12]); ok = 0; }
	if (record1[13] != record2[13]) { printf("D2! %x %x ", record1[13], record2[13]); ok = 0; }
	if (record1[14] != record2[14]) { printf("D3! %x %x ", record1[14], record2[14]); ok = 0; }
	if (record1[15] != record2[15]) { printf("D4! %x %x ", record1[15], record2[15]); ok = 0; }
	if (record1[16] != record2[16]) { printf("D5! %x %x ", record1[16], record2[16]); ok = 0; }
	if (record1[17] != record2[17]) { printf("D6! %x %x ", record1[17], record2[17]); ok = 0; }
	if (record1[18] != record2[18]) { printf("D7! %x %x ", record1[18], record2[18]); ok = 0; }
	if (!ok) {
	    printf("; pc1 %06x pc2 %06x cycles %lu\n", record1[1], record2[1], cycles);

	    printf(" D0:%08x D1:%08x D2:%08x D3:%08x D4:%08x D5:%08x D6:%08x D7:%08x\n",
		   record1[11], record1[12], record1[13], record1[14], 
		   record1[15], record1[16], record1[17], record1[18]);
	    printf(" A0:%08x A1:%08x A2:%08x A3:%08x A4:%08x A5:%08x A6:%08x A7:%08x\n",
		   record1[3], record1[4], record1[5], record1[6], 
		   record1[7], record1[8], record1[9], record1[10]);
	    printf("\n");
	    printf(" D0:%08x D1:%08x D2:%08x D3:%08x D4:%08x D5:%08x D6:%08x D7:%08x\n",
		   record2[11], record2[12], record2[13], record2[14], 
		   record2[15], record2[16], record2[17], record2[18]);
	    printf(" A0:%08x A1:%08x A2:%08x A3:%08x A4:%08x A5:%08x A6:%08x A7:%08x\n",
		   record2[3], record2[4], record2[5], record2[6], 
		   record2[7], record2[8], record2[9], record2[10]);

	    printf("last pc1 %06x, last pc2 %06x\n", last1, last2);

	    mismatches++;
	    if (mismatches > 10)
		break;
	}

	last1 = record1[1];
	last2 = record2[1];

	cycles++;
	if ((cycles % 1000000) == 0) {
	    printf("--- %lu ---\n", cycles);
	}
    }
    printf("\n");
}

void dump_one(int which, unsigned long cycles, int what, unsigned int *record)
{
    int size;

    switch (what) {
    case 1:
    case 2:
	printf("\n");
	printf("%lu: pc %06x sr %04x %s\n",
	       cycles, record[1], record[2], what == 1 ? "slow" : "fast");

	printf(" D0:%08x D1:%08x D2:%08x D3:%08x D4:%08x D5:%08x D6:%08x D7:%08x\n",
	       record[11], record[12], record[13], record[14], 
	       record[15], record[16], record[17], record[18]);
	printf(" A0:%08x A1:%08x A2:%08x A3:%08x A4:%08x A5:%08x A6:%08x A7:%08x\n",
	       record[3], record[4], record[5], record[6], 
	       record[7], record[8], record[9], record[10]);
	break;
    case 11:
	printf(" tlb fill; ");
	printf(" pc %06x address %08x va %08x bus_type %d boot %d pte %08x\n",
	       record[1], record[2], record[3], record[4], record[5], record[6]);
	break;
    case 12:
	printf(" pte set; ");
	printf(" pc %06x address %08x pa %08x bus_type %d context_user %d pte %08x\n",
	       record[1], record[2], record[3], record[4], record[5], record[6]);
    	break;
    case 21:
	printf(" mem write; ");

	size = record[2] & 0xffff;
	printf(" pc %06x size %d fc %d ea %08x value %08x\n",
	       record[1], size, record[3], record[4], record[5]);
	if (which == 1) {
	    int mtype, fault;
	    mtype = (record[2] >> 16) & 0xff;
	    fault = (record[2] >> 24) & 0xff;
	    printf(" pa %x mtype %d fault %d pte %08x\n", record[6], mtype, fault, record[7]);
	}
	break;
    case 22:
	printf(" mem read; ");

	size = record[2] & 0xffff;
	printf(" pc %06x size %d fc %d ea %08x value %08x\n",
	       record[1], size, record[3], record[4], record[5]);
	if (which == 1) {
	    int mtype, fault;
	    mtype = (record[2] >> 16) & 0xff;
	    fault = (record[2] >> 24) & 0xff;
	    printf(" pa %x mtype %d fault %d pte %08x\n", record[6], mtype, fault, record[7]);
	}
	break;
    }
}

int find_pc(int which, unsigned int start_pc)
{
    unsigned int record[20];
    unsigned long cycles;
    int what, r, show;

    cycles = 0;
    while (1) {
	r = get_next(which, record, &what);
	if (r < 0) {
	    printf("eof on file %d\n", which);
	    break;
	}

	if (what == 1 || what == 2) {
	    if (record[1] == start_pc)
		break;
	    cycles++;
	}
    }

    if (r < 0)
	return -1;

    return 0;
}

void find_pc_and_dump(int which, unsigned int start_pc)
{
    unsigned int record[20];
    unsigned long cycles;
    int what, r, show;

    cycles = 0;
    while (1) {
	r = get_next(which, record, &what);
	if (r < 0) {
	    printf("eof on file %d\n", which);
	    break;
	}

	if (what == 1 || what == 2) {
	    if (record[1] == start_pc)
		break;
	    cycles++;
	}
    }

    if (r < 0)
	return;

    while (1) {
	dump_one(which, cycles, what, record);

	if (what == 1 || what == 2) {
	    cycles++;
	}

	r = get_next(which, record, &what);
	if (r < 0) {
	    printf("eof\n");
	    break;
	}
    }

}

void dump(int which, unsigned int start_isn)
{
    unsigned int record[20];
    unsigned long cycles;
    int what, r;

    cycles = 0;
    while (1) {
	r = get_next(which, record, &what);
	if (r < 0) {
	    printf("eof\n");
	    break;
	}

	if (cycles > start_isn) {
	    dump_one(which, cycles, what, record);
	}

	if (what == 1 || what == 2) {
	    cycles++;
	}
    }

}

extern char *optarg;

main(int argc, char *argv[])
{
    int c;
    int do_dump, do_compare, do_find;
    int which, start_isn;
    char *fname1, *fname2;
    unsigned int start_pc;

    do_dump = 0;
    do_compare = 0;
    do_find = 0;
    which = 1;

    fname1 = "trace.bin";
    fname2 = "/net/mini/disk/files1/sun2/brad/record.bin";

    while ((c = getopt(argc, argv, "cd:12f:g:p:")) != -1) {
	switch (c) {
	case 'c':
	    do_compare++;
	    break;
	case 'd':
	    do_dump++;
	    start_isn = atoi(optarg);
	    break;
	case '1':
	    which = 1;
	    break;
	case '2':
	    which = 2;
	    break;
	case 'f':
	    fname1 = strdup(optarg);
	    break;
	case 'g':
	    fname2 = strdup(optarg);
	    break;
	case 'p':
	    do_find = 1;
	    start_pc = strtol(optarg, NULL, 0);
	    break;
	}
	
    }

    fd1 = open(fname1, O_RDONLY | O_LARGEFILE);

    if (do_find) {
	if (!do_compare) {
	    size_file(fd1);
	    find_pc_and_dump(1, start_pc);
	    dump(1, start_isn);
	} else {
	    fd2 = open(fname2, O_RDONLY | O_LARGEFILE);

find_pc(1, start_pc);
find_pc(1, start_pc);
	    if (find_pc(1, start_pc) < 0)
		exit(1);
	    printf("found pc for file1\n");
	    if (find_pc(2, start_pc) < 0)
		exit(1);
	    printf("found pc for file2\n");
	    compare();
	}
    }

    if (do_dump) {
	size_file(fd1);
	dump(1, start_isn);
    }

    if (do_compare) {
	fd1 = open(fname1, O_RDONLY | O_LARGEFILE);
	fd2 = open(fname2, O_RDONLY | O_LARGEFILE);

	//size_file(fd1);
	//size_file(fd2);
	compare();
    }
    exit(0);
}


/* Local Variables:  */
/* mode: c           */
/* c-basic-offset: 4 */
/* End:              */
