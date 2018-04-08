/*      @(#)a.out.h 1.1 86/09/24 SMI; from UCB 4.1 83/05/03       */

#include "exec.h"

/*
 * memory management parameters
 */

#ifdef sun
#define	PAGSIZ		0x2000
#define	SEGSIZ		0x20000
#define	OLD_PAGSIZ	0x800	/* THIS DISAPPEARS IN RELEASE 4.0 */
#define	OLD_SEGSIZ	0x8000	/* THIS DISAPPEARS IN RELEASE 4.0 */
#endif /* sun */

#ifdef vax
#define	PAGSIZ		1024
#define	SEGSIZ		PAGSIZ
#endif /* vax */

/*
 * returns 1 if an object file type is invalid, i.e., if the other macros
 * defined below will not yield the correct offsets.  Note that a file may
 * have N_BADMAG(x) = 0 and may be fully linked, but still may not be
 * executable.
 */

#define	N_BADMAG(x) \
	(S((x).a_magic)!=OMAGIC && S((x).a_magic)!=NMAGIC && S((x).a_magic)!=ZMAGIC)

/*
 * relocation parameters. These are architecture-dependent
 * and can be deduced from the machine type.  They are used
 * to calculate offsets of segments within the object file;
 * See N_TXTOFF(x), etc. below.
 */

#ifdef sun
#define	N_PAGSIZ(x) \
	(S((x).a_machtype) == M_OLDSUN2? OLD_PAGSIZ : PAGSIZ)
#define	N_SEGSIZ(x) \
	(S((x).a_machtype) == M_OLDSUN2? OLD_SEGSIZ : SEGSIZ)
#endif /* sun */

#ifdef vax
#define	N_PAGSIZ(x) PAGSIZ
#define	N_SEGSIZ(x) SEGSIZ
#endif /* vax */

/*
 * offsets of various sections of an object file.
 */

#ifdef sun
#define	N_TXTOFF(x) \
	/* text segment */ \
	( S((x).a_machtype) == M_OLDSUN2			     \
	  ? (S((x).a_magic)==ZMAGIC ? N_PAGSIZ(x) : sizeof (struct exec)) \
	  : (S((x).a_magic)==ZMAGIC ? 0 : sizeof (struct exec)) )
#endif /* sun */

#ifdef vax
#define	N_TXTOFF(x) \
	(S((x).a_magic)==ZMAGIC ? PAGSIZ : sizeof (struct exec))
#endif /* vax */


#define	N_SYMOFF(x) \
	/* symbol table */ \
	(N_TXTOFF(x)+L((x).a_text)+L((x).a_data)+L((x).a_trsize)+L((x).a_drsize))

#define	N_STROFF(x) \
	/* string table */ \
	(N_SYMOFF(x) + L((x).a_syms))

/*
 * Macros which take exec structures as arguments and tell where the
 * various pieces will be loaded.
 */

#ifdef sun
#define N_TXTADDR(x) \
	(S((x).a_machtype) == M_OLDSUN2? N_SEGSIZ(x) : N_PAGSIZ(x))
#endif /* sun */

#ifdef vax
#define	TXTRELOC	0
#define N_TXTADDR(x) TXTRELOC
#endif /* vax */

#define N_DATADDR(x) \
	((S((x).a_magic)==OMAGIC)? (N_TXTADDR(x)+L((x).a_text))		\
	 : (N_SEGSIZ(x)+((N_TXTADDR(x)+L((x).a_text)-1) & ~(N_SEGSIZ(x)-1))))

#define N_BSSADDR(x)  (N_DATADDR(x)+L((x).a_data))

/*
 * Format of a relocation datum.
 */

struct relocation_info {
	long	r_address;	/* address which is relocated */
unsigned int	r_symbolnum:24,	/* local symbol ordinal */
		r_pcrel:1, 	/* was relocated pc relative already */
		r_length:2,	/* 0=byte, 1=word, 2=long */
		r_extern:1,	/* does not include value of sym referenced */
		:4;		/* nothing, yet */
};

/*
 * Format of a symbol table entry
 */
struct	nlist {
	union {
		char	*n_name;	/* for use when in-core */
		long	n_strx;		/* index into file string table */
	} n_un;
	unsigned char	n_type;		/* type flag (N_TEXT,..)  */
	char	n_other;		/* unused */
	short	n_desc;			/* see <stab.h> */
	unsigned long	n_value;	/* value of symbol (or sdb offset) */
};

/*
 * Simple values for n_type.
 */
#define	N_UNDF	0x0		/* undefined */
#define	N_ABS	0x2		/* absolute */
#define	N_TEXT	0x4		/* text */
#define	N_DATA	0x6		/* data */
#define	N_BSS	0x8		/* bss */
#define	N_COMM	0x12		/* common (internal to ld) */
#define	N_FN	0x1f		/* file name symbol */

#define	N_EXT	01		/* external bit, or'ed in */
#define	N_TYPE	0x1e		/* mask for all the type bits */

/*
 * Dbx entries have some of the N_STAB bits set.
 * These are given in <stab.h>
 */
#define	N_STAB	0xe0		/* if any of these bits set, a dbx symbol */

/*
 * Format for namelist values.
 */
#define	N_FORMAT	"%08x"
