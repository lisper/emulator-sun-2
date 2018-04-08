#
# Makefile for Sun ROM Monitor, August 1981
#	Jeffrey Mogul @ Stanford
#
# This makefile is constructed from a machine description:
#	config machine
# Most changes should be made in the machine description
#	/usr/sun/src/monitor/conf2/``machineid''
# after which you should do
#	 config machineid
# Generic makefile changes should be made in
#	/usr/sun/src/monitor/conf2/makefile
# after which config should be rerun for all machines.
#
CC=cc68
AS=as68
LD=ld68

INCLUDES= -I/usr/sun/include/pup/

# define LFLAG as -L to get assembly listings
LFLAG=

COPTS= ${IDENT}
CFLAGS= -O ${COPTS} -DMC68000 $(LFLAG) $(INCLUDES)

RELOC= 400000

%OBJS

%CFILES

%LOAD

%SYMBOLS

clean:
	rm -f *.dl *.b *.s.list touch errs linterrs *.ls

lint: /tmp
	@lint -hbxn -I. - ${COPTS} ${INCLUDES} -I/usr/sun/include/ ${CFILES} | \
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
# already done in ../conf/makefile:	yapp ../h/*.h

%RULES

# DO NOT DELETE THIS LINE -- make depend uses it

