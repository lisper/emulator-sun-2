#
# Makefile for Sun ROM Monitor, August 1981
#	Jeffrey Mogul @ Stanford
#
# Philip Almquist  November 5, 1985
#	- Update descriptive text
#	- Fix typo in make clean
#
# This makefile is constructed from a machine description:
#	cd /usr/sun/src/sunmon/conf
#	sunconfig machine
# Most changes should be made in the machine description
#	/usr/sun/src/sunmon/conf/``machineid''
# after which you should do
#	cd /usr/sun/src/sunmon/conf
#	 sunconfig machineid
# Generic makefile changes should be made in
#	/usr/sun/src/sunmon/conf/makefile
# after which sunconfig should be rerun for all machines.
#
CC=cc68
AS=as68
LD=ld68

# define LFLAG as -L to get assembly listings
LFLAG=

COPTS= ${IDENT}
CFLAGS= -O ${COPTS} -DMC68000 $(LFLAG)

RELOC= 200000

%OBJS

%CFILES

%LOAD

%SYMBOLS

clean:
	rm -f *.dl *.b *.s *.list touch errs linterrs *.ls

lint: /tmp
	@lint -hbxn -I. - ${COPTS} ${CFILES} | \
	    grep -v 'struct/union .* never defined' | \
	    grep -v 'possible pointer alignment problem'
touch: ../conf/touch.c
	cc ../conf/touch.c -o touch

depend:
	@echo 'making dependencies ...'
	@grep '^#include' ${CFILES} | grep -v '<' | \
	      sed 's/:[^"]*"\([^"]*\)".*/: \1/' | \
	      sed 's/\.c/.b/' | sed 's,../[0-9a-z]*/,,' | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
	@echo '$$r makedep' >>eddep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w' >>eddep
	@cp makefile makefile.bak
	@ed - makefile < eddep
	@rm eddep makedep
	@echo '... done'

tags:
	/usr/ucb/ctags ${CFILES}

print:
	yapp $(CFILES)
	yapp ../h/*.h

%RULES

# DO NOT DELETE THIS LINE -- make depend uses it

