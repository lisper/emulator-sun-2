#!/bin/sh

F=x1
KSYM=ksyms-sunos3.2

F=y1
KSYM=ksym-sun2-s2.0.txt

F=y2
KSYM=ksym-sun2-s2.0.txt

F=y3
KSYM=ksym-sun2-s2.0.txt

F=../log5
#KSYM=ksym-sun2-s2.0.txt
KSYM=ksyms-sunos3.2

awk -- '/^PC / { printf "%s; ",$0; getline; if (substr($1, 0, 2) != "A0") print; else printf "\n"; }' <$F >$F.2
cc ann.c
./a.out $KSYM $F.2 >$F.3
