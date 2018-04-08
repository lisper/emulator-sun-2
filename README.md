# emulator-sun-2
Emulator for Sun-2 workstation

This is a emulator for the Sun Sun-2 Workstation.  It's designed to
boot SunOS and emulate the display and keyboard found on an original
Sun-2 worksation.

I was frustrated by the original Sun emulator - tme.  It's an amazing
piece of software but really hard to follow internally.  It seems to
be an extreme example of abstraction.  Perhaps too abstract, at least
for me.

So I wrote this to learn about the MMU and, well SCSI.  My goal is to
eventually create an FPGA version of the Sun-2.

Right now it boots the ROM, and loads the kernel.  Things are happy
until it tries to use the SCSI disk.  The SCSI disk emulation is not
complete and needs help.  If you know SCSI-1 or SCSI-2 please take a look.
