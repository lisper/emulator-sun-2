#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "dklabel.h"

unsigned char disk[100*1024*1024];
unsigned char part[5*1024*1024];
unsigned int offsets[8];

main(int argc, char *argv[])
{
	int dsize, psize, i;
	int dfd, pfd;
	char *dname, *pname;
	struct dk_label *l;

	if (sizeof(struct dk_label) != 512) {
		printf("struct dk_label %d\n", (int)sizeof(struct dk_label));
		exit(1);
	}

	if (argc < 2)
		exit(1);

	dname = argv[1];
	if (argc == 3)
		pname = argv[2];
	else
		pname = NULL;

	dfd = open(dname, O_RDONLY);
	if (dfd < 0) {
		perror(dname);
		exit(1);
	}
	dsize = read(dfd, disk, sizeof(disk));
	close(dfd);

	if (pname) {
		pfd = open(pname, O_RDONLY);
		if (pfd < 0) {
			perror(pname);
			exit(1);
		}
		psize = read(pfd, part, sizeof(part));
		close(pfd);
	}

	l = (struct dk_label *)disk;
	printf("label: %s\n", l->dkl_asciilabel);
	if (ntohs(l->dkl_magic) != DKL_MAGIC) {
		printf("bad magic number %x %x %x\n",
		       ntohs(l->dkl_magic), l->dkl_magic, DKL_MAGIC);
		return -1;
	}

	for (i = 0; i < NDKMAP; i++) {
		int cylno, nblk, boff;
		unsigned long coff;
		cylno = ntohl(l->dkl_map[i].dkl_cylno);
		nblk = ntohl(l->dkl_map[i].dkl_nblk);
		boff = cylno * ntohs(l->dkl_nhead) * ntohs(l->dkl_nsect);
		coff = boff * 512;
		printf("part %d: cyl %5d, blocks %6d; block offset %6d, byte offset %lu (0x%lx)\n", i, cylno, nblk, boff, coff, coff);
		offsets[i] = coff;
	}

	if (pname) {
		printf("checking partition 1\n");
		for (i = 0; i < psize; i++) {
			unsigned char pb, db;
			pb = part[i];
			db = disk[offsets[1] + i];
			if (pb != db) {
				printf("partition mismatch want %02x, found %02x @ %d bytes\n", pb, db, i);
//			exit(1);
			}
		}
	}

	exit(0);
}
