# emulator-sun-2
Emulator for Sun-2 workstation

This is a emulator for the Sun Sun-2 Workstation.  It's designed to
boot SunOS and emulate the display and keyboard found on an original
Sun-2 worksation.

I was frustrated by the original Sun 2 emulator - tme.  It's an amazing
piece of software but really hard to follow internally.  It seems to
be an extreme example of abstraction.  Perhaps too abstract, at least
for me.

So I wrote this to learn about the MMU and, well SCSI.  My goal is to
eventually create an FPGA version of the Sun-2.

It now boots SunOS 2.0, 3.2, and 3.5 cleanly.  You can install from the distribution tapes.
The SCSI emulation code is still shakey for tapes, however.

SunOS 2.0 will boot multiuser and works as you'd expect.
