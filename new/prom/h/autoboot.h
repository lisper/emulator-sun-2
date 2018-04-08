struct BootTableEntry {
	char *filename;		/* name of file to be booted */
	int   devtype;		/* boot device to boot from */
				/* zero means "invalid entry" */
};
