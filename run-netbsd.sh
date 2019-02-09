#!/bin/sh

#sim/sim --prom=media/rom/sun2-multi-rev-R.bin --disk=media/disk/netbsd.img --tape=/files/netbsd/netbsd-1.6.1-bin/make-netbsd-1.6.1
sim/sim --prom=media/rom/sun2-multi-rev-R.bin --disk=media/disk/netbsd.img --tape=/files/netbsd/netbsd-1.6.1-new

# initially type "b st()"

#
# after the failed boot prompt, type "b vmunix"
#

