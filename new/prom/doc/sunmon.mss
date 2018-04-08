@make(manual)
@style(singlesided)
@device(dover)
@modify[example, font smallbodyfont]
@pageheading( center "@c[SUN ROM Monitor Internals]", left "@c[@Value(page)]")
@pagefooting( center "@c[@Value(Date)]")
@begin(titlePage)
@Blankspace(-3inches)
@blankspace(1 inch)
@majorheading(SUN
ROM Monitor
Internals)
@blankspace(1 inch)
@value(Date)
Editor: @b[Jeffrey Mogul]
(updated by @b[Ross Finlayson], April 1984)
@blankspace(1 inch)
@CopyRightNotice(Stanford University)
@end(titlepage)

@chapter(Introduction)
This document describes the internal structure of the ROM
monitor used on the SUN processor board, and how to configure
and install versions of this monitor.  The monitor code is
still under active development, so most of this document
should be taken with a grain of salt.

It is assumed that the reader has a working understanding of the
following topics (references are given for further reading):
@begin(enumeration)
MC68000 processor architecture@\see the MC68000 User's Manual

SUN processor board@\see the SUN User's Manual

C Programming Language@\see the book by Kernighan and Ritchie

Unix@\see the Unix Programmer's Manual

User-visible features of the Monitor@\see the SUN User's Manual

EPROM programming@\see the PROM programmer manual
@end(enumeration)

@chapter(Structure of the Monitor)

The monitor is broken into several semi-distinct sections:
@Begin(enumeration)
Kernel@\this section provides the necessary hardware support
functions, such as initialization, memory refresh, and trap/interrupt
vectors.

Console Monitor@\this section manages interaction with the user,
including memory examine/deposit, ``help'' functions, breakpoint
trapping, etc.

Console I/O@\this section manages I/O using the on-board UART
and the SUN Framebuffer if present.

Emulator@\this section implements a few simple ``monitor calls''
that can be invoked by programs wishing to make use of monitor
services

Bootstrap loader(s)@\this section provides one or more methods
of bootstrap loading files into processor memory.  Currently
implemented are Ethernet loaders (using PUP EFTP and IP TFTP) and
serial-line loaders.
@end(enumeration)

Only the first section is strictly necessary.  However, in practice
the first three sections are always part of the monitor, although
perhaps only partially there.

@chapter(Source code organization)

The monitor sources normally live in subdirectories
under @t[/usr/sun/src/sunmon]
on Diablo (and other Stanford Vaxes).  These subdirectories are organized
roughly
on rational grounds.  (In this section, subdirectories will be
referred to using Unix relative pathnames; e.g., @t[../h] is normally
@t[/usr/sun/src/sunmon/h].)

@section(Header files)

Header files (files with suffix @t[.h]) are found in the @t[../h]
subdirectory.  These define constants and macros used by the
rest of the monitor.   Other, more generally useful header files
are found in @t[/usr/sun/include] and @t[/usr/include], and will
not be described here.

@subsection(asmdef.h)
@index(asmdef.h)
This file defines virtually all of the @t[asm]s used to extend the
C language for the low-level parts of the monitor.  Macros are defined
here for saving and restoring registers and sets of registers,
trap information, etc.  Also defined here are symbolic names for
several MC68000 opcodes, for use when absolutely necessary (e.g.,
setting a breakpoint trap instruction.)

@subsection(autoboot.h)
@Index[autoboot.h]
This file defines the data structure used in the table that
specifies what programs may be automatically boot-loaded.

@subsection(buserr.h)
@index(buserr.h)
This file describes the hardware-defined data structure pushed on
the processor stack after a Bus Error or an Address Error.

@subsection(confreg.h)
@Index[confreg.h]
This file defines the layout of the ``configuration register'';
@Index[configuration register]
i.e., the way the monitor interprets the 16-bit parallel input port.

@subsection(globram.h)
@index(globram.h)
This file defines the monitor's global data structure, which lives
in the low segment of writeable memory.  All of the monitor's static
data lives in this structure, including the console input buffer,
monitor state and option information, and static data used by the
frame buffer support code.

@subsection(m68vectors.h)
@index(m68vectors.h)
This file defines symbolically all of the processor-specific
trap and interrupt vector addresses.

@subsection(prom2.h)
@index(prom2.h)
This file defines the entry points into the second pair of PROMs,
which are used to hold sections of the monitor that do not fit
into the first pair of PROMs.

@subsection(screen.h)
@index(screen.h)
This file defines the parameters of the screen (frame buffer)
support, such as character spacing, screen width, etc.

@subsection(sunbug.h)
@index(sunbug.h)
This file defines the codes to be used in an as yet unimplemented
remote debugging system for the SUN processor.

@subsection(sunemt.h)
@index(sunemt.h)
This file defines the trap codes for the emulator trap facility,
and indicates which of them are reserved to programs running
in supervisor mode.

@subsection(sunmon.h)
@index(sunmon.h)
This is the main header file for the monitor; it defines
various constants (such as the monitor version), the layout
of monitor data structures, and constants used by several
different modules.

@subsection(sunuart.h)
@index(sunuart.h)
This defines some macros and constants which make interaction
with the on-board dual UART less painful.  The aim of this file,
not totally achieved, is to mask all hardware-specific features
of the particular UART used.

@subsection(suntimer.h)
@index(timer.h)
This file defines macros and constants which make interaction
with the on-board quintuple timer less painful.

@section(Kernel files)

The source files in the @t[../kernel] subdirectory implement
the Kernel, Console Monitor,
and the Emulator facilities.  Some of
these facilities are spread out across a few sources files,
due to the somewhat fuzzy nature of the distinctions made.

@subsection(commands.c)
@index[commands.c]
The routines in @t[commands.c] implement the bulk of the console
user interaction features.  The routines in @t[commands.c] are
@begin(description)
@index[TakeBreak()]
TakeBreak(pcp)@\Given @i[pcp], a pointer to the saved program
counter after a breakpoint trap is taken, this routine does
most of the interaction necessary to make this useful.  Unfortunately,
there is a fair amount of code spread throughout the @i[dobreak()]
and @i[monitor()] routines that fiddles with monitor state related
to breakpoints; these regions are usually surrounded by an
@t[#ifdef BREAKTRAP], but are rather obscure and must be understood
as a whole.

@index[dobreak()]
dobreak()@\Somewhat mistitled, this routine interacts with the user
to set or clear a breakpoint trap.

@index[doload()]
doload()@\This puts the monitor into a state where it can accept
S-records coming from the host computer, and thus load a program
via the serial line.  An implicit @t[I B] command is done; the
down-loader on the host must end its command sequence with an
explicit @t[I A].

@index[getrecord()]
char getrecord(pcadx)@\Accepts (or rejects, depending) an S-record
from the controlling UART, and stores it into memory.
Returns, by reference via @t[pcadx], the value of the program start
address if the S-record contained it.

@index[monitor()]
@begin(multiple)
monitor(@i[<registers>])@\Called with a stack frame representing
the saved processor registers after a trap, this is the main routine of the
console interaction section.  It takes action appropriate to the
sort of trap that occurred, and then enters a loop where it reads
a command line and dispatches according to the command name.  Many
commands are implemented entirely within this routine, in order
to save the code-space overhead of a procedure call.

@index(autoboot)
@index(bootload)
Several configuration options are interpreted here, including
the ``auto-bootload'' and ``error-restart'' options.

@end(multiple)

@index[openitem()]
openitem(num,adrs,digs,dsize)@\This is used to ``open'' a memory
address or map register for inspection and modification by the user.
The parameters describe what to read/write and how to display it.

@index[openreg()]
openreg(r,radx)@\This is used to ``open'' an address or data register.
The parameters describe the register number to open, and the
address of the saved register area.

@index[setmode()]
setmode(c)@\Sets the UART mode to one of @t[A], @t[B], @t[T], or @t[S].
@end(description)

@subsection(transparent.c)
This module implements the ``transparent mode'' command.
@index[transparent()]
@begin(description)
transparent()@\Implements ``transparent mode'' by connecting
the two UARTs together in software to simulate a bidirectional
wire.
@end(description)

@subsection(emulate.c)
@index[emulate.c]
The single routine in this module is called after an emulator
trap is taken to service the request.  It checks the access mode
of the trapping program, and validates the emulator trap code
before dispatching.  Values returned are placed in the caller's
A0 register by the @i[monitor()] routine in @t[commands.c].
@begin(description)
@index[Emulate()]
Emulate(pcadr,usrsr,usrsp)@\@t[pcadr] and @t[usrsp] are
reference parameters pointing to, respectively, the program
counter and stack pointer at the trap.  @t[usrsr] is the
contents of the status register at the trap.
@end(description)
@subsection(givehelp.c)
@index[givehelp.c]
@index[givehelp()]
@begin(multiple)
This module defines one routine, @i[givehelp()], which
either prints a short help message or bootloads a help
program, depending on whether bootloading is implemented.
If it attempts to bootload a help program and fails, it
falls back on the short help message.

The name of the help program bootloaded is in the source
of this module; it is normally ``sunmonhelp''.
@end(multiple)

@subsection(mapmem.c)
@index[mapmem.c]
@index[mapmem()]
This file contains the single routine @i[mapmem()] which
sizes memory and
sets up the page and segment maps to follow the standard
defined for the monitor.  This means an identity mapping
for on-board memory and a standard mapping for Multibus
memory and I/O spaces.

@subsection(tables.c)
@index[tables.c]
This file defines a number of read-only data tables used for
setting various parameters.  Currently, it contains the initialization
sequence for the UARTs and a table giving timer parameters for
several possible UART speeds.

@subsection(proxies.c)
@index[proxies.c]
@index[prom2()]
This module contains routines that simply redispatch to the second
PROM, by calling the @i[prom2()] routine at the very first location in the
second PROM (see section @ref(PRM2ENTRY)), provided that the second PROM exists.
@Begin(description)
@index[bootload()]
bootload(fname)@\(Effectively) call the @i[bootload()] routine in the second
PROM
(see section @ref(PRM2BOOTLOAD)), returning what it returns.
If no bootload PROM exists, print a message to that
effect, and return 0.

@index[boottype()]
boottype(bt)@\
@Begin(multiple)
If the bootload PROM exists, (effectively) calls the @i[boottype()] routine in
 the
second PROM (see section @ref(PRM2BOOTTYPE).)  Otherwise, print an appropriate
error message.

As a special case (i.e., kludge),
if @t[bt] is greater than or equal to 0xFF, call the @i[listboots()]
routine (in @t[autoboot.c].), instead of @i[boottype()].  This is done
regardless of the existence of the second PROM.
@End(multiple)

@index[writechar()]
writechar(c)@\If the second PROM exists, (effectively) calls the @i[writechar()]
routine in the second PROM (see section @ref(PRM2WRITECHAR).)  Otherwise,
call @i[busyouta()].  In either case, the specified character
@t[c] is passed as an argument.  This routine allows the use of
the Frame Buffer as a console output device, since the support
code for this is in  the second PROM.
@End(description)

@subsection(autoboot.c)
This module implements the ``automatic'' bootstrap facility of
the monitor (which is invoked by an appropriate code in the
relevant field of the configuration register.)
@index[autoboot.c]
@Begin(description)
@index[autoboot()]
autoboot(code)@\
@begin(multiple)
Uses @t[code] as an index into @t[BootTable] (see section @ref(BOOTTABLE).)
(If the code is out of range, the routine exits.)  Using the table entry
located by this index, it sets the current boot device type, and then calls
@i[bootload()] with the filename from the table entry.  If this all
succeeds, the program is started; otherwise, an error message is printed.

If the pre-processor symbol AUTOBOOTNAME is defined, @t[code] is ignored.
Instead, the value of AUTOBOOTNAME is used as the name of the file to
bootload (the boot device type is not set; i.e., the default is used.)
@end(multiple)

@index[listboots()]
listboots()@\
@Begin(multiple)
Lists (on the console) the auto-boot table (@t[BootTable].)

If AUTOBOOTNAME is defined, this instead indicates what file will
be booted.
@End(multiple)
@End(description)

@subsection(boottable.c)
@Label(BOOTTABLE)
This module defines the table (@t[BootTable]) that associates
bootfile numbers with (@i[filename], @i[boot-device]) pairs.
It may be edited to change the set of possible bootfiles.
(This is not entirely satisfactory; it is hoped that the
number of candidates for automatic bootloading is fairly low.)
@index[boottable.c]

@subsection(bedecode.c)
@index[bedecode.c]
The routine in this module is used to deduce what actually
caused a Bus Error.  It is fairly complex, and specific to
the processor version.
@Begin(description)
@Index[bedecode()]
char *bedecode(bei, r_sr)@\Deduces what caused a bus error.
The @t[bei] parameter is a @t[struct BusErrInfo*] (defined
in @t[<buserr.h>].)  @t[r_sr] is a copy of the saved status
register.  This routine uses this information, along with
the state of the segment and page maps, to decide what caused
the Bus Error.  It then returns a string, suitable for use
in an error message.
@End(description)

@subsection(version.c)
@index[version.c]
This file defines a character string used by the monitor
as a name and version banner after a reset.  If you want
a different banner, this can be edited.  However, the insertion
of the version number into the identifier string uses a somewhat
obscure feature of the pre-processor; it may have to be
altered by hand to actually insert the version number, if your
pre-processor does not support this.

@subsection(sunmon.c)
@index[sunmon.c]
This module implements the lowest-level interface to
the MC68000 hardware, include interrupt vectors, memory
refresh, and system initialization.  Because it is quite
complex, it is described here in some detail.

@index[main()]
The only ``routine'' defined in this module is @i[main()],
but in fact this is not really an entry point at all.  Because
all of the entry points here are called from assembly code,
they are defined using the @i[label()] macro which creates
a label visible to assembly code.  These entry points are
described here:
@begin(description)
@index[REFRsrv]
REFRsrv@\This is the service routine for the level-7 auto-vectored
interrupt.  The MC68000 defines interrupt level 7 as the highest
priority, @i[non-maskable] interrupt.  It is triggered by one of
the on-board timers every 2 milliseconds.  During each invocation
of this service routine, the following events occur:
@begin(enumeration)
register save@\Since this is an interrupt routine, register @t[D0]
is explicitly saved on the stack.

watchdog timer re-trigger@\The watchdog timer is reloaded; this
is to keep it from going off (ever!), since it will reset the processor
if it does.  If the refresh process ever dies, this reload will not
be done, and the processor will reset.

timer reset@\The refresh timer is reset, to clear the interrupt
and start a new refresh timing cycle.

@index[memory refresh]
memory refresh@\The refresh subroutine is called.

@index[abort]
abort checking@\The status register of UART A is checked to see
if a ``break'' character has been seen; if it has, then the
@i[monitor()] routine in @t[commands.c] is called with a code indicating
that an abort has occurred.

register restore@\The saved register is popped from the stack and
restored, and the interrupt service routine is exited.
@end(enumeration)

@index[HardReset]
HardReset@\This is a ``secondary'' entry point into the monitor,
called from the command level when a ``@t[K 1]'' is given.  It simply
sets a flag indicating that memory should not be cleared, and then
jumps into the sequence started by @t[startmon].

@index[startmon]
startmon@\This is the initial entry point into the ROM monitor,
called after a processor reset.  It must initialize all of the
on-board hardware components, including timers, UARTs, mapping,
and memory.  Since these are all in undefined states after a reset,
the monitor must execute without memory references until mapping
is set up; therefore, all data is stored in registers and no
procedure calls are done during the initial phase of the process.

The proper sequencing of events is fairly critical.  In order,
they are:
@begin(enumeration)
@index[reset]
reset occurs@\This is done by the hardware.  The SUN processor
board enters ``boot state'', during which
@begin(enumeration)
read access to memory is re-mapped so that a read access to location
0XXXXX returns data from 2XXXXX (i.e., PROM address space),
and interrupts (including normally non-maskable ones) are disabled.
Boot state is exited by attempting to write to the PROM address space.

The processor fetches its initial stack pointer and program counter
from what appears to be locations 0 and 4, but are actually the first
two longwords in the PROM.  The initial program counter value stored
here is @i[startmon()].
@end(enumeration)

@index[configuration register]
read configuration@\The configuration register is read, and stored
in a register for later use.

@index[timer]
timer reset@\The first thing the monitor does is to reset the
on-board timers, to avoid being interrupted (since there will
be no interrupt vectors for a while.)

@index[watchdog timer]
initial watchdog timer@\The watchdog timer is started, with a long
(ca. 10 seconds) initial period.  This should last us until the
refresh process is running.

map initialization@\The segment and page maps are initialized
in a simplistic way, using an identity mapping that assumes
that the entire address space is mapped onto on-board memory.

exit boot state@\This allows subroutine calls to take place (since
during a subroutine return the stack is popped.)

@i[mapmem()] called@\This routine re-initializes the maps to
correspond to reality.  It returns the size of on-board
memory in bytes.  The memory-sizing algorithm in @i[mapmem()]
can lead to spurious Bus Errors, so the Bus Error vector is
temporarily initialized to call a routine which simply ignored
Bus Errors.

parity initialization@\The parity bits for the on-board memory
are still random.  Every memory location must be written before
being read to avoid spurious parity errors (note that up to here,
we have been careful to follow this rule.)  All of memory is
filled with a opcode that will cause a trap if executed, thus
providing some security against runaway code.  (This step
is not done on a ``Hard Reset'' from the console, since we assume
that it has already been done.)

global data initialization@\The address of the global data
structure is cached in a register, and the memory size is stored
there.

@index[frame buffer]
frame buffer location@\The monitor checks for the presence of
a frame buffer by accessing a location where it should respond
and noting if a Bus Error occurs.

@index[second PROM]
second PROM location@\The monitor checks for the presence of
a second pair of PROMs by reading the first location in the
second PROM address space and checking the value returned against
what the location should contain.

@index[UART]
UART initialization@\The UARTs are initialized, using the speeds specified
by the configuration register.

@index[vectors]
vector initialization@\The trap and interrupt vectors are initialized
so that all traps and interrupts are caught by the appropriate entry
into the monitor.  Note that a lot of them are vectored through
what is effectively a dispatch table, described later.

refresh routine creation@\The memory refresh routine must access
a certain number of sequential memory locations.  Since this
executes quite often, the routine lives in memory rather than
in PROM (which has a slower read cycle.)  The routine consists
of a lot of @t[NOP]s followed by an @t[RTS] opcode.

@index[memory refresh]
refresh initialization@\The refresh timer is started.  

@index[watchdog timer]
watchdog timer re-initialization@\The watchdog timer is set up
to run on a 3 millisecond square wave.  This requires that the
refresh process be running, to reload the watchdog timer every
2 milliseconds.

global data base pointer initialization@\By convention,
the longword at location 0 contains a pointer to the monitor
global data structure.

additional global data initialization@\Various mundane
initializations

second PROM version check@\A call is made to the second PROM
to determine if its version matches that of the main PROM.  If
it doesn't, a warning is given, but nothing further is done about
the problem.

Execution then falls through to the label @i[SoftReset].
@end(enumeration)

@index[SoftReset]
SoftReset@\The @i[monitor()] routine is normally entered
after an exception (i.e., trap or interrupt) is caught.
This entry point creates a ``fake'' exception stack and
then falls through to @i[ENTERmon].

@index[ENTERmon]
ENTERmon@\In preparation for calling the console monitor
routine @i[monitor()], the processor registers are saved
and the interrupt level is set to 7 (which inhibits all
but the memory refresh interrupt during console interaction.)
The @i[monitor()] routine is then called; if it returns, the
saved registers are restored and an @t[RTS] is done, since
this point is usually called after a trap or interrupt.
@end(description)

The other entry points in this module form a sort of
dispatch table for traps and interrupts.  When one is
reached after an exception, it pushes either a small
integer or a two-byte ``human-readable'' code onto the
stack, and then branches to @i[ENTERmon].  The @i[monitor()]
routine interprets this integer or code to determine what
action to take and to advise the user what exception
has occurred.

@section(Prom2)
@index(prom2)
The subdirectory @t[../prom2] contains the routines that normally
live in the second EPROM.  They, in turn, reference routines
from other subdirectories.  Thus, these routines must also be
included in the second prom.

@subsection(prom2.c)
Rather than build into the code of the first EPROM a number of
absolute address pointers into the address space of the second EPROM,
there is only one externally known entry point into the second EPROM.
This is at the very first location in the address space (i.e.,
0x400000), and is known in @t[../h/prom2.h].  This entry point
is that of a dispatch routine, @i[prom2()], which takes a type code and calls
the appropriate routine.
@index(prom2.c)

@begin(description)
@Label(PRM2ENTRY)
@index[prom2()]
prom2(code, arg)@\Calls the routine specified by @t[code] (codes
are defined in @t[../h/prom2.h]) with one argument @t[arg].
@end(description)

@subsection(boot.c)
@index(boot.c)
This module is the top-level implementation of the bootstrap
loading commands.  Other routines (in other subdirectories, see
sections @ref(EFTPENETLOADER) and @ref(TFTPENETLOADER)), which implement the
actual loaders, are called
from this module.
@Label(PRM2BOOTLOAD)
@Begin(description)
@Index[bootload()]
bootload(fname)@\looks up the entry point of the current bootstrap
loader in a table (@t[DevTab], also in this module), and calls
that routine with the filename specified by @t[fname] as an argument.
It returns the value passed back by the loader routine.

@Index[boottype()]
@Label(PRM2BOOTTYPE)
boottype(code)@\This routine sets the ``current bootstrap loader'',
which is an index (stored in the monitor globals) into the @t[DevTab]
table.  If @t[code] is within the range of index values allowed (based
at 1, not 0), the monitor global is set to @t[code].  Otherwise, the
entire table is listed on the console, showing the possible
code number/device associations.  In either case, the name of the
current bootstrap loader is printed, after the rest of the command
is executed.
@End(description)

@subsection(writechar.c)
@Index[writechar.c]
@Label(PRM2WRITECHAR)
This file defines a routine which
is used in the second prom as a level of indirection
when writing characters to the console.  It decides
whether to use the console UART, or the frame buffer,
depending on the state of the appropriate monitor global.
@Begin(description)
@Index[writechar]
writechar(c)@\write character @t[c] on the console UART or
on the framebuffer.
@End(description)


@section(Common Subroutines)
These modules, in @t[../subrs], implement routines that are used in both
sets of proms.  Most are related to console I/O and formatting.

@subsection(busyio.c)
@index[busyio.c]
This file implements busy-wait I/O to and from the console UARTs.
If frame buffer support is included, it special-cases output to
UART A by passing the work to the frame buffer routines.

@begin(description)
@index[busyouta()]
busyouta(x)@\Prints character @t[x] on UART A or the frame buffer,
depending.

@index[busyoutb()]
busyoutb(x)@\Prints character @t[x] on UART B.

@index[getchar()]
char getchar()@\Returns the next character available from the
controlling UART.

@index[putchar()]
putchar(x)@\Prints character @t[x] on the controlling UART (by
calling one of @i[busyouta()] or @i[busyoutb()].)
@end(description)

@subsection(gethexbyte.c)
@Index[gethexbyte.c]
@index[gethexbyte()]
@begin(description)
gethexbyte()@\Retrieves one hex-coded byte from the input
buffer; e.g. the character sequence @t[1F] yields the value
31 (decimal).
@end(description)

@subsection(getline.c)
@Index[getline.c]
@index[getline()]
@begin(description)
getline()@\This is called to prime the input buffer (which
holds one complete line) by reading characters from the controlling
UART.  This routine does character-delete and line-delete processing.

@index[getone()]
char getone()@\Returns the next character in the input buffer.

@index[peekchar()]
char peekchar()@\Returns the next character in the input buffer
without advancing the read pointer; all other routines that
access the input buffer advance the read pointer past the last
character used.
@end(description)

@subsection(getnum.c)
@Index[getnum.c]
@index[getnum()]
@begin(description)
longword getnum()@\Returns the value of the hexadecimal number
pending in the input buffer.  Any non-hexadecimal character terminates
the routine.
@end(description)

@subsection(ishex.c)
@Index[ishex.c]
@index[ishex()]
@begin(description)
int ishex(ch)@\If the character @t[ch] is
a legal hexadecimal digit, return the corresponding numeric value, otherwise -1.
@end(description)

@subsection(message.c)
@Index[message.c]
@index[message()]
@begin(description)
message(mess)@\Prints the character string whose address is @t[mess]
on the console.
@end(description)

@subsection(printhex.c)
@Index[printhex.c]
@index[printhex()]
@begin(description)
printhex(val,digs)@\Prints @t[digs] hexadecimal digits of @t[val].
@end(description)

@subsection(probe.c)
@Index[probe.c]
This module is used in I/O-related tasks to determine if an
interface is present (i.e., if it responds to its address.)
@index[ProbeAddress()]
@Begin(description)
ProbeAddress(address)@\Returns true (non-zero) @i[iff] the
specified address responds to a 16-bit read without causing
a Bus Error.  (Note: this may incur an Address Error.)
@End(description)

@subsection(queryval.c)
@Index[queryval.c]
@index[queryval()]
@begin(description)
queryval(m,v,l)@\Used as a prompting input routine, this prints
a message whose address is @t[m], prints @t[l] hexadecimal digits
of @t[v], and reads a new line into the input buffer.
@end(description)

@section(Bootstrap loaders - General)
@Index[Bootstrap loaders]

By convention, bootstrap loader code resides in the second PROM.
(An exception is the ``serial-line downloader'', which is actually
quite simple and thus lives in the low PROM.)  Bootstrap loaders
are called via the @i[bootload()] routine (see section @ref(PRM2BOOTLOAD)),
and have a very simple interface:
@Begin(enumerate)
the top-level routine of the loader is called with one argument, which
is a string pointer to the remainder of the command line (after the
leading @t[F] or @t[N] and any leading spaces.)

It returns the entry point of the loaded program, or 0 to indicate
error.
@End(enumerate)
These top-level routines are inserted into the @t[DevTab] table in
@t[../prom2/boot.c] (section @ref(PRM2BOOTLOAD)), and are thus easily
integrated into the monitor.

In order to provide information for interactive debugging, a convention
exists that specifies the layout of a program in memory once it has
been loaded.  The Ethernet loader adheres to this convention, as does
the program which generates files to be loaded by the serial loader.  This
layout is based upon the ``b.out'' format (described by
@t[/usr/sun/include/b.out.h]), and (using the terminology of that format)
it contains the following ``segments'':
@Begin(enumerate)
``Text'' (code) segment.

Initialized Data segment.

``BSS'' (Uninitialized Data) segment.

Symbol Table Length.

Symbol Table.
@End(enumerate)
The fourth ``segment'' is actually a single longword which gives the
length (in bytes) of the symbol table.  It can be addressed
as the longword at the address given by the value of
the symbol @t[_end]; the @t[crtsun.b] routine normally linked as the
entry point of every program contains the address of this location.
All segments are assumed to
follow one another contiguously.

@section(3Mbit Ethernet bootstrap loader)
@Label(EFTPENETLOADER)
The files in the @t[../eftpnetboot] subdirectory implement a
PUP-based bootstrap loader for the 3Mbit experimental Ethernet.
The protocol used is a modification
of the ``Alto Bootstrap Protocol'' and is described in
@t[/usr/local/doc/SunBoot.Press] on Shasta.

Basically, the requesting host sends a Pup packet to the server
host (or broadcasts it to all servers) on the Miscellaneous Services
socket, with a Pup type indicating that it is a ``Sun Boot Request''.
The name of the bootfile is sent in the PupData section of the Pup.
However, for compatability with the Alto protocol, and for use
by extremely simple loaders, if the PupData section is empty,
the PupID field is taken as the identifying number of a bootfile
stored in a standard directory.

If a server can respond with the requested file, it then sends
it via the EFTP protocol to the socket specified by the requestor.
Since the requesting host is not expected to be very capable,
the normal EFTP error mechanisms are not in effect for bootstrap
loading.

The ROM-resident loader is a stripped-down version of the EFTP
package in the standard Pup library, with some additional code
to convert the @t[b.out] format files into memory images of programs.
Although the code has been heavily optimized for space, it is
still written with an eye towards readability; however, you
should read the original sources first (in @t[/usr/local/src/lib/pup])
if you really want to understand the stripped-down version.

@index(bfread.c)
@subsection(bfread.c)
This module is used to read a bootfile using the EFTP protocol,
once the file has been requested by @i[getbootfile()].
@begin(description)

@index[bfread()]
bfread(EfChan,buffer,buflen,rbuflen)@\Reads a buffer (i.e.,
one Pup packet) from the open EFTP channel @t[EfChan]
into the buffer at @t[buffer] with a maximum
size of @t[buflen].  The actual buffer size read is returned via
the reference parameter @t[rbuflen].
@end(description)

@index(enbootload.c)
@subsection(enbootload.c)
This is the top-level routine used for bootloading a file.
Since the console monitor code does not distinguish between
different bootstrap loaders, each possible loader (e.g.,
Ethernet, disk, tape, etc.) provides a top-level routine
called @i[enbootload()].
@begin(description)

@index[enbootload()]
enbootload(bfname)@\Attempts to bootload the routine named by
the character string at @t[bfname], and returns either the
starting address or, upon failure, 0.
@end(description)

@index(getbootfile.c)
@subsection(getbootfile.c)
This routine requests and the reads an entire bootfile over
the network.  It parses the bootfile name and, if an explicit
host name is present, extracts it and looks up its Pup address.
It also calls @t[mpuprt()] to determine the local Pup network number
and to determine a route.
It then sets up an ``EftpChan'' data structure to
store the EFTP protocol state, sends a ``Sun Boot Request''
to the specified host (or else to all hosts), and then uses
@i[bfread()] to read the bootfile via the net.
@begin(description)

@index[getbootfile()]
getbootfile(bfn,bfname,storage,inhib)@\Requests the bootfile
named by the string @t[bfname] (or identified by the bootfile
number @t[bfn] if @t[bfname] is null), and places it in
the buffer starting at @t[storage], returning either the
bootfile length or -1 on failure.   If the parameter @t[inhib]
is true, then a request is not made, and the routine assumes
that some third party has prodded a server into sending a
bootfile.
@end(description)

@index(mefrecpckt.c)
@subsection(mefrecpckt.c)
This modules is used to read the next packet of EFTP data from
the network.
@begin(description)

@index[EfAck()]
EfAck(efc,seqnum)@\Sends an acknowledegment of packet #@t[seqnum]
over the EFTP Channel @t[efc].

@index[mefrecpckt()]
mefrecpckt(Efchan, buf, blen)@\Receives a packet over @t[Efchan]
and stores it into a buffer beginning at @t[buf], returning the
buffer length in the reference parameter @t[blen].  Returns
an EFTP internal status code.
@end(description)

@index(mlkup.c)
@subsection(mlkup.c)
A very minimal name lookup routine that converts a name into
a Pup host address by sending a request to a Miscellaneous
Services server.  In addition to returning the Pup host and
net numbers of the specified host, it returns the net number
of the responding server.  This is assumed to be the net
number of the local host, and may be used in constructing an internet route.
@begin(description)

@index[mlkup()]
mlkup(name,namelen,sh,sn,on)@\Looks up the network address of the
string starting at @t[name] whose length is @t[namelen],
and returns the host-number part (or 0 on failure).  It also
returns (by reference) in @t[sh], @t[sn], and @t[on] the
host-number and net-number of the specified host, and ``our''
net-number, respectively.
@end(description)

@index(mpuprt.c)
@subsection(mpuprt.c)
A simple routine to determine Pup routes.  It sends a broadcast
``routing table request'', and from this tries to find a route
to a given destination.
@begin(description)

@index[mpuprt()]
mpuprt(destnet, desthost, routenet, routehost)@\
Given the destination specified by (@i[destnet], @i[desthost]),
returns in @i[routehost] the first-hop destination along a
route to this host.  It also returns the local network number
(i.e, the net that we are on) in @i[routenet].
@end(description)

@index(msetfilt.c)
@subsection(msetfilt.c)
Creates a Pup packet ``filter'' to be used by @i[puprd()] to
ignore unwanted packets.
@begin(description)

@index[msetfilt()]
msetfilt(Pchan,priority)@\Sets up the filter data structure
for Pup Channel @t[Pchan]; @t[priority] is ignored.
@end(description)

@index(pupopn.c)
@subsection(pupopn.c)
Opens a Pup channel, initializing the data structure and the
ethernet interface.
@begin(description)

@index[pupopn()]
pupopn(Pchan,srcsock,Dst)@\Opens the Pup Channel @t[Pchan],
with a source socket of @t[srcsock] and a destination
address of @t[Dst].
Routing is not done at this level, so
the network-number part of @t[Dst] is ignored.  The caller
of @i[pupopn()] should modify the @t[ImmHost] and @t[SrcPort.net]
fields of the Pup Channel to implement routing.
@end(description)

@index(puprd.c)
@subsection(puprd.c)
Reads a Pup packet from the ethernet.
@begin(description)

@index[puprd()]
puprd(Pchan,buf,buflen, Ptype, ID, SrcPort)@\Reads a Pup packet
from @t[Pchan] into @t[buffer], returning the buffer length, PupType,
PupID, and Source Port into, respectively, @t[buflen], @t[Ptype], @t[ID],
and @t[SrcPort].
@end(description)

@index(pupwrt.c)
@subsection(pupwrt.c)
Writes a Pup packet to the ethernet.
@begin(description)

@index[pupwrt()]
pupwrt(Pchan, Ptype, ID, buf,buflen)@\Writes a Pup packet via
@t[Pchan] using @t[Ptype], @t[ID], @t[buf], and @t[buflen], as,
respectively, the PupType, PupID, PupData, and PupData length.
@end(description)

@index(snb.c)
@subsection(snb.c)
This appears to be a simple test program that Jeff Mogul wrote to test the
above code.


@section(10Mbit Ethernet bootstrap loader)
@Label(TFTPENETLOADER)
The files in the @t[../tftpnetboot] subdirectory implement an
IP-based TFTP bootstrap loader for 10Mbit Ethernets. (It could also be
configured with the 3Mbit driver.) 
The code is well documented, and so will not be described in great detail here.
As with the EFTP 3Mbit bootstrap loader
(described in section @ref(EFTPENETLOADER)), the top-level routine is
@index[enbootload()]
@i[enbootload()], in
@index[enbootload.c]
@t[enbootload.c].

The TFTP implementation consists of several layers:
@begin[itemize]
@index[tftp.c]
@t[tftp.c]
contains the TFTP layer. TFTP @i[writing] is not implemented, since we
wish to
read files only.

TFTP is implemented on top of UDP.
@index[udp.c]
@t[udp.c]
contains the code for the
UDP layer. Routines for sending and receiving UDP packets are
provided. (UDP 'sends' are used for TFTP read requests, and acknowledgements.)
To save code space, we do not compute a checksum on UDP packets.

UDP is implemented on to of IP.
@index[ip.c]
@t[ip.c]
contains the code for the
IP layer, and provides routines for sending and receiving IP
packets. To save code space, the IP read routine assumes that incoming IPs
have not been fragmented.

The IP layer makes use of the Ethernet routines in
@index[enet.c]
@t[enet.c]. These in turn call the routines that make up the (10Mbit or 3Mbit)
Ethernet
driver.
@end[itemize]

In addition to TFTP (and completely independent of it), both ``Address
Resolution'' (ARP)
@index[ARP]
 and ``Reverse Address Resolution'' (RARP)
@index[RARP]
 are used.
ARP is implemented in
@index[arp.c]
@t[arp.c]. It is used to find the Ethernet address to which to send the initial
TFTP read request. In principle this is not necessary, since the request could
be sent as an Ethernet broadcast packet. In practice, however, the sending of
an ARP request packet is useful, since it ensures that the mapping from the 
workstation's internet address to its Ethernet address is stored in the
receiving
host's ARP table. If  this did not happen, then when the receiving host then
tried to send the first (UDP) data packet, it would be forced to @i[itself]
 send  out an ARP request, which the workstation would not handle. (Note
that the ARP
code in @t[arp.c] only sends out a request packet, and interprets the reply;
it does not listen to anyone else's ARP requests.)

'Reverse' ARP, or RARP,  is used to find the workstation's internet address,
given its 
Ethernet
address (which, of course, is known to the workstation).
(This is done prior to sending
the ARP request mentioned above.) This code is implemented in
@index[revarp.c]
@t[revarp.c].
Alternatively, the user can enter the
workstation's internet address as part of the boot command typed at the
console, in which case RARP is not used.
At the time of writing, RARP has not been accepted as a standard outside of
Stanford, although we hope that this will happen eventually.

Associated with each of the @t[@i(x).c] files mentioned  above, there is a
corresponding
header file @t[@i(x).h].


@section(Miscellaneous routines for bootloading)
The @t[../miscnetboot] subdirectory contains routines that are common
to both the
EFTP loader and the TFTP loader.


@index(setup.c)
@subsection(setup.c)
@begin(description)
This routine takes a bootfile in @t[b.out] format, as read into
memory by @i[enbootload()], and converts it into a runnable
memory image.  It moves the ``text'' and ``initialized data''
segments to the proper locations, zeros the ``bss'' segment,
and moves the symbol table to start at the second longword
after the bss segment (not necessarily in that order).  The
symbol table size is stored into the first longword after the
bss segment.

@index[setup()]
setup(p,l)@\``Sets up'' the bootfile buffer starting at @t[p]
and extending for @t[l] bytes.  Returns the entry point of
the bootfile.
@end(description)

@section[3Mbit Ethernet driver]
@label[ENET3MDRIVER]
The @t[../enet3] subdirectory contains a simple driver for our 3Mbit Ethernet
boards. The EFTP bootloader uses this driver. The TFTP bootloader could also be
configured to use this driver, to boot off a 3MBit Ethernet. In practice,
however,
it uses the 10Mbit Ethernet driver described in section @ref[ENET10MDRIVER], to
boot off a 10MB Ethernet.

@index(uether.c)
@subsection(uether.c)
This is a ``micro'' implementation of a 3Mbit ethernet driver (hence
the @i[u]ether), which uses busy-wait I/O to avoid confronting
all sorts of difficult issues.@begin(description)

@index[engethost()]
unsigned char engethost(fid)@\Returns the host number on
the local ethernet board; @t[fid] is ignored.

@index[enetaddress()]
enetaddress(enetAddr)@\Another version of the above routine, which returns the
local
Ethernet address in the reference parameter @t[enetAddr].

@index[enopen()]
enopen()@\Initializes the ethernet interface.

@index[enread()]
enread(fid, packet, filter, timeout)@\Reads from the ethernet
into the buffer @t[packet], using the packet filter @t[filter]
to reject unwanted packets.  If nothing happens
in roughly (@i[very] roughly) @t[timeout] seconds, a failure
code is returned; otherwise, the packet length is returned.

@index[enwrite()]
enwrite(DstHost, Ptype, fid, packet, len)@\Write @t[len]
bytes starting at @t[packet] onto the ethernet, directed to
@t[DstHost] with an Ethernet Packet Type of @t[Ptype].

@index[mc68keninit()]
mc68keninit()@\Initializes the ethernet interface.

@index[mc68kenread()]
mc68kenread(buffer,timeout,maxShorts)@\Reads a packet into @t[buffer],
returning the length, or returns a failure code after roughly
@t[timeout] seconds. At most @t[maxShorts] 16-bit words are read.

@index[mc68kenwrite()]
mc68kenwrite(buffer,count)@\Writes @t[count] @i[words] starting
at @t[buffer] onto the ethernet.
@end(description)


@section[10Mbit Ethernet driver]
@label[ENET10MDRIVER]
The @t[../enet10] subdirectory contains a driver for the Excelan EXOS/101
10Mbit Ethernet
board. The TFTP bootloader has been configured to use this driver. This 
subdirectory contains a single source code file, @t[excelan.c], and two 
header files,
@index[excelan.h]
@t[excelan.h], and
@index[constants.h]
@t[constants.h].

@index(excelan.c)
@subsection(excelan.c)
This is a ``bare bones'' ethernet driver for the Excelan board. Nevertheless,
because of the complexity of the Excelan board, it is still quite substantial.
The exported routines are
@index[enetaddress()]
@index[enopen()]
@index[mc68kenread()]
@index[mc68kenwrite()]
@i[enetaddress()],
@i[enopen()],
@i[mc68kenread()] and
@i[mc68kenwrite()]. These are equivalent to the corresponding routines in the
3Mbit driver (see section @ref[ENET3MDRIVER]).


@section(Screen Support)
@index(frame buffer)
The monitor in its most basic configuration communicates with the
user via the console UART (channel A).  However, a workstation
with a Sun Frame Buffer may not have a terminal attached, although
it probably will still have a keyboard.  Therefore, it is possible
to configure the monitor to use the frame buffer as the output
device for the console.  The code for this is stored in the
@t[../screen] subdirectory.

Since program and data space in the ROM is quite limited, the
terminal emulated is fairly simple.  Only a few bytes of static
storage are used, so scrolling is essentially impossible.  (It
might be possible to do scrolling by copying the entire bitmap
to the new location, but since the UARTs are not interrupt-driven,
in transparent mode at 9600 baud there is not enough
time available for updating the screen.) Therefore, the terminal
``rolls'' instead of scrolling (like an MIT ITS terminal.)

The character set is stored in ROM, and is a fixed width font
(to minimize the difficulty of displaying it.)  The screen width
and height (in units of characters) are compile-time parameters,
but values of 80 and 24, respectively, are reasonable.  (A higher
screen would require more time to update when using a screen
editor.)

The functions emulated by the terminal, besides character display,
are: addressable cursor motion, up-line, down-line, erase screen,
and erase-to-end-of-line.  These vaguely follow VT52 syntax.  An
entry suitable for insertion into @t[/etc/termcap] (stored in
@t[tcap]) is
@Begin(example)
# Sun ROM-resident emulator
sr|sunrom|SunRom|Sun Rom:\
	am:ns:pt:bs:co#80:li#24:ce=10\EK:up=\EA:cl=120\f:nd=\EC:\
	nl=\ED:cm=\EY%+ %+ :
@End(example)

@index(font.c)
@subsection(font.c)
This module defines a two-dimensional array which is used as a
bitmap font.  The major dimension is indexed by the character
to be displayed (offset so that the non-printing characters do
not take up space.)  The minor dimension is indexed by the scan-line
of the character.  Elements are bytes (for character sets up to 8
pixels wide) or shorts (for character sets from 9 to 16 pixels wide.)

@index(screen.c)
@subsection(screen.c)
This module is a (nominally) hardware-independent terminal emulator.
It takes a stream of characters and either displays them, or interprets
them as commands and alters its internal state accordingly.

@begin(description)
@index[writechar()]
writechar(c)@\Displays the character @t[c] on the screen, or uses it as
part of a command sequence depending.
@end(description)

@index(sunscreen.c)
@subsection(sunscreen.c)
This module implements the low-level operations on the frame buffer
needed to support @t[screen.c].  For reasons of efficiency, there
are a few @t[asm]s embedded in this module.

@begin(description)
@index[clearEOL()]
clearEOL(x,y)@\Clears from the given (@t[x],@t[y]) coordinate to
the end of that line.

@index[initscreen()]
initscreen()@\Clears the screen and initializes the frame buffer hardware.

@index[putat()]
putat(x,y,c)@\Displays character @t[c] at the point (@t[x],@t[y]).

@index[putcursor()]
putcursor(x,y,set)@\If @t[set] is true, places a cursor at
the point (@t[x],@t[y]); otherwise, erases the cursor at that point.

@end(description)

The following files are used to generate a font description (i.e.,
they produce the file @t[font.c] when properly invoked.)

@index(cmr.c)
@subsection(cmr.c)
A fixed-width version of Computer Modern Roman generated by Metafont
and converted to a two-dimensional array by a program written by
Bill Nowicki.  This represents the largest possible font usable in
the monitor -- 128 characters, each up to 16 by 16 pixels.

@index(convert.c)
@subsection(convert.c)
This program uses the information in @t[../h/screen.h] to produce
a properly configured font description (to be used as @t[font.c].)

@index(basefont.c)
@subsection(basefont.c)
An auxilliary file for the @i[convert] program.

@section(SunBug)
@i[(This has not yet been implemented.)]

@chapter(Configuring and Compiling a Monitor)

The monitor has numerous options and many source files, and thus
compiling a PROM-image of the monitor can be a confusing task.
However, for most purposes the task has been mechanised using
a modified version of the @i[config] program (which was written
to allow easier configuration of Unix systems.)  You should read
the entry for @i[config] in chapter 8 of the 4.1BSD ``Unix Programmer's
Manual'' before continuing.

@index(sunconfig)
@index(configuration)
The @i[sunconfig] program is a modification of @i[config] that
knows about the SUN ROM monitor.   This program is normally
run in the @t[../conf] subdirectory (or the @t[../conf2]
subdirectory when configuring the code for the second PROM.)
It produces a @i[makefile] in a separate subdirectory for
each configuration you define; the name of the subdirectory
and the name of the ``configuration description file'' match.
The configuration description files are kept in the @t[../conf]
subdirectory.

For example, assume that the configuration is called ``TEST''.
Then, the configuration description file is called
@t[../conf/TEST] and it places its output in the
subdirectory called @t[../TEST] (which you must @i[mkdir]
before using!)
By convention, the configuration for the second PROM to match
TEST would be named ``TEST2''.

@section(Configuration files for basic PROM)
The files of interest (on @t[../conf]) are:
@subsection(configuration specification file)
@index(configuration)
These files are the only ones that should be changed in
normal use.  They follow the format described in the
Unix manual entry for @i[config] fairly closely.  For example,
a ``generic'' configuration file (one suitable for a simple
environment) is
@begin(example)
#
# GENERIC configuration file for Sun PROM Monitor
#	Low PROMS
#
# NOTE: Do NOT edit this file; make a copy and edit that!
#
# (lines beginning with "#" are comments.)
#
# The "cpu" line identifies the hardware version used
cpu		"SUNV1PCREVD"
# The "ident" line defines the configuration name
ident		GENERIC
#
# The "options" line defines the options in effect.
# Possible compile-time options are:
#
#	ABORTSWITCH	Abort switch is connected to SYNCA' on UART.
#	BOOTHELP	Attempt to load help program by bootstrapping.
#	BREAKKEY	<Break> on UART A causes abort.
#	BREAKTRAP	Breakpoint trap command available.
#	EMULATE		Emulator traps available.
#	FRAMEBUF	Frame buffer supported as console output device.
#	WATCHDOG	Watchdog timer enabled.
#
#	The following options take arguments:
#
#	AUTOBOOTNAME=<filename>		e.g., AUTOBOOTNAME=memtest
#			This is used to override the autoboot table;
#			if the configuration register specifies autoboot,
#			use <filename> regardless.
#
#	This Generic configuration includes all but
#	    ABORTSWITCH, BOOTHELP, WATCHDOG, AUTOBOOTNAME
#
options		BREAKKEY, BREAKTRAP, EMULATE, FRAMEBUF

# This line names the output file; the "dummy" part is only syntactically
# required for now, and the final PROM image will be called "generic.dl"
config	dummy generic

@end(example)
The format of this may change slightly as things become more
elaborate.  I expect, for example, to use the field in the
``config'' line now filled with @i[dummy] to specify a primary
bootstrap device.

@subsection(files)
@index(files)
The file @t[files] names all of the source files used in the
monitor and describes whether they are standard or optional.
This follows the practice used with @i[config].

@subsection(makefile)
This is a skeleton ``makefile'' which is read by the
@i[sunconfig] program and used to build a real makefile
in the target directory.

@section(Configuration files for second PROM)
The configuration files for the second PROM are stored in
@index(conf2)
@t[../conf2].  Basically, the files go by the same names,
except that, as mentioned earlier, the name of the
specification file for a given configuration has a ``2'' appended.
An example of a ``generic'' file is
@begin(example)
#
# GENERIC2 configuration file for Sun PROM Monitor
#	High PROMS
#
# NOTE: Do NOT edit this file; make a copy and edit that!
#
cpu		"SUNV1PCREVD"
ident		GENERIC
#
# Possible compile-time options are:
#
#	ENETBOOT	Allow ethernet bootstrap loader.  If this
#			option is given, you must specify 
#			("pseudo-device eftpnetboot"
#			or "pseudo-device tftpnetboot")
#			and ("pseudo-device enet3" or "pseudo-device enet10")
#			as well.
#
#	ENET10		(If ENETBOOT is specified) indicates a 10Mb Ethernet,
#			as opposed to an experimental 3Mb Ethernet.
#
#	DISKBOOT	Allow local disk bootstrap loader.  If this
#			option is given, you must specify
#			"pseudo-device diskboot" as well.
#
#	The following options take arguments:
#
#	MAXBOOTTRIES=<number>		e.g., MAXBOOTTRIES=100
#			Overrides default retry count for bootloading.
#
#	This Generic configuration includes all but
#	    MAXBOOTTRIES, ENETBOOT, DISKBOOT

options		NONE
#	(if no options are given, "options NONE" must be specified)

config	dummy generic2

pseudo-device screen
# 
# Specify "pseudo-device eftpnetboot" and "pseudo-device enet3"
# (or tftpnetboot and/or enet10 - see above) iff you specify "options ENETBOOT".
# pseudo-device eftpnetboot
# pseudo-device enet3
#
# Specify "pseudo-device diskboot" iff you specify "options DISKBOOT".
# pseudo-device diskboot
@end(example)

@section(Running @i[sunconfig])
@index(sunconfig)
To configure a monitor, the @i[sunconfig] program is used.
Assume that the name of the configuration is ``TEST'', that
the directories @t[../TEST] and @t[../TEST2] exist, and that
the configuration specifications @t[../conf/TEST] and
@t[../conf2/TEST2] have been written.

First, set your working directory to @t[../conf] and
issue the command
@t(sunconfig TEST);
the program should not give any error messages.  It will
create @t[../TEST/makefile], and then do a @t[make depend]
in the @t[../TEST] directory,
which creates the dependency information at the end of
the @t[makefile].  (This is used to determine which files
must be recompiled after something is edited.  If you
edit a source file to change the files that it @t[#include]s,
you should do a @t[make depend] in the appropriate directories.)

If you have a two-prom configuration, you should then
set your working directory to @t[../conf2] and
issue the command @t(sunconfig TEST2).

At this point, @t[makefile]s sufficient to compiler
the monitor exist in the directories @t[../TEST]
and @t[../TEST2].

@section(Compiling a Monitor)
@index(make)
Proceeding using the example configurations produced in the
previous section, one can now compile the monitor, or produce
informational output.

@subsection(Compilation)
@index(compilation)
The compilation process is managed by the @t[make] program,
which uses the information in a file called @t[makefile] in
the current working directory to decide what should be
compiled.  (For more information on @t[make], see the
appropriate Unix manual.)  

The @t[makefile]s produced
by the @i[sunconfig] program use the source files
in the various subdirectories to produce object files
(with @t[.b] suffixes) which are stored in the current
working directory.  Once all the required object files
are compiled (and are more recent than the source files
that they depend upon, the @t[make] program links them
together to form an ``executable'' program, and then
converts this program into a format usable for downloading
over a serial line (@t[.dl] suffix.)  This format is used
to load the PROM programmer to produce the final PROMS.

To have all this happen, simply set your working directory
to @t[../TEST] and issue the command @t[make].  The
appropriate files will be compiled, and after a while,
a @t[.dl] file should be produced.  No error or warning
messages should be produced,
@foot(actually, two modules, @t[probe.c] and @t[sunmon.c]
have what appear to the compiler to be ``unreachable'' lines
and thus give rise to warnings.)
although you may be told
that the output file (e.g., @t[netmon.dl]) is ``up to date''.
The size of the monitor code will be displayed; this is useful
if you are trying to cram a lot into a PROM.

If you have a two-prom configuration, you should set your
working directory to @t[../TEST2] and issue the @t[make]
command there.

If an error message does occur, it could be because
you lack sufficient file access rights (you need write access
to the directory and the files in it), in which case you
should take whatever measure you can to obtain access.

If the error message seems to refer to a programming error,
then you should either attempt to fix the source code, or
find a maintainer of the source code and have it fixed.  If
you do not understand what is going on, don't get in over
your head.

@subsection(Informational output)
The @t[makefile] also specifies how to obtain a variety of
informational output, such as assembly listings of the monitor
code.  These options are:
@begin(enumeration)
clean@\@t[make clean] removes various object files and listing
files from the current directory.  This is useful to force
recompilation, if necessary.

print@\@t[make print] prints (on the Dover) source-code listings
of the monitor code.

symbols@\@t[make symbols] produces a symbol table listing, with
suffix @t[.sym], of the current monitor.

assembly listings@\To get assembly listings of the code, use the
@t[make LFLAG=-L] command.  Note that this will produce listing files
(with suffix @t[.ls]) @i[only] for those files that need to be
recompiled.  To get listings of the entire monitor, you should
first issue a @t[make clean] command.

@end(enumeration)

@section(Burning PROMs)
Now that you have the monitor compiled, you have the
file @t[../TEST/netmon.dl] (and possibly @t[../TEST2/netmontwo.dl]).
These are ready to be loaded into the PROM programmer and burnt
into pairs of EPROMs.

NOTE: The Data I/O PROM programmer in ERL recognizes S19 ``@t[.dl]''
format (16-bit addresses), rather than the default format (S28 - 24-bit
addresses) generated by @t[dl68]. If you wish to use this PROM programmer, then
you will need to change
your
makefile to run @t[dl68] with the ``@t[-s19]'' flag.
