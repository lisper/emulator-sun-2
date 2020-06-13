/* An alpha version of an 3c400 ethernet adapter for the sun2-emulator 
   2019 - Sigurbjorn B. Larusson (sibbi@dot1q.org)

   I built this drivver from scratch from the 3C400 reference manual, mostly as an interesting project, hope you find some use for it
  
   It works, you need BPF to use it.  Broadcast and BPF aren't always friends, you probably have to add a static arp entry for your host
   to the emulated sun.  You can still compile without BPF support, obviously you can't read or write to the network, but you can enable
   tracing and see what the driver is doing in the background.

   It should be trivial to port this to use some other mechanism than bpf, the bpf code itself is a very minor part of this


   To do that, add an entry into /etc/hosts with your hostname and ip address and then use
   arp -s hostname aa:bb:cc:dd:ee:ff

   You might also want to do the same on the machine you're running on but pointing to the sun machine mac-address, i.e.
   arp -s ipaddress.of.emulated.sun 08:00:20:01:06:e0  (this assumes you didn't change the default mac-address)	

   Your milage with using BPF to reach ouside of the emulated machine might vary, I've been successful in using ftp and telnet from the sun
   to the host running the emulator and telnet in the other direction 

   Since the I/O read in the emulator is basically an endless loop this driver only runs the scan for new packets every 5000 iterations of that
   loop, you can set it with a define right below here.  If you set it too low, the emulator will slow to a crawl since the packet checking code
   takes a good while to load (BPF read function uses 90% of that time).  If you set it to high, the network will be slow with lots of retransmissions

   TODO: It would be far better to use software interrupts here, BPF on the mac, at least upto version 10.14, does not support them
         but other BSD OS's do, so I'd like to test this and find out which ones it works well on, that is much better solution than 
	 skipping 4999 iterations of the io_update code.

   TODO: This code really should be a seperate thread 

   TODO: This code could probably be optimized quite a bit
  
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__unix__)|| defined(__MACH__)
#include <net/bpf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#endif

#include "sim68k.h"
#include "m68k.h"
#include "sim.h"

// This is not a good place but here is the configuration for the mac-address and the BPF device and network device you wish to use
// TODO: There probably should be a configuration file to set this
// Default is 08:00:20:01:06:e0, same as the emulator default
#define MAC1 0x08;
#define MAC2 0x00;
#define MAC3 0x20;
#define MAC4 0x01;
#define MAC5 0x06;
#define MAC6 0xe0;

// BPF interface
#define BPFINTERFACE "vmnet8"


#if defined(__unix__)|| defined(__MACH__)
// This is the filter that we use for BPF, if PA is >2 then this filter is used
struct bpf_insn bpf_ours[] = {
	// Load first 4 from dest addr
	BPF_STMT (BPF_LD + BPF_W + BPF_ABS, 0),
	// Compare to first 4 octets of our mac
	BPF_JUMP (BPF_JMP + BPF_JEQ + BPF_K, 0x08002001, 0, 3 ),
	// Load last 2 from dest addr
	BPF_STMT (BPF_LD + BPF_H + BPF_ABS, 4),
	// Compare to last 2 octets of our mac
	BPF_JUMP (BPF_JMP + BPF_JEQ + BPF_K, 0x06e0, 5, 0 ),
	// Load first 4 from dest addr
	BPF_STMT (BPF_LD + BPF_W + BPF_ABS, 0),
	// Compare to first 4 of broadcast-addrress
	BPF_JUMP (BPF_JMP + BPF_JEQ + BPF_K, 0xFFFFFFFF, 0, 2),
	// Load last 2 from dest addr
	BPF_STMT (BPF_LD + BPF_H + BPF_ABS, 4),
	// Compare to last of broadcast-mac
	BPF_JUMP (BPF_JMP + BPF_JEQ + BPF_K, 0xFFFF, 1, 0),
	// If there's no match, reject the packet
	BPF_STMT (BPF_RET + BPF_K, 0),
	// Otherwise receive the packet
	BPF_STMT (BPF_RET + BPF_K, -1)
};
#endif

// Try to make sure struct is correctly represented
#pragma pack(1)

// ADDRESSING:
// 	MEBASE starts 0xe000, followed by:
//  	Status Register, two bytes, various bits for status and control	
//  	Station ROM address, offset 400, stores the ROM mac-address
//	Station RAM address, offset 600, stores the RAM mac-address (set by computer, used by adapter)
//	Transmit buffer, offset 800, stores header and packet to be transmitted
//	Receive buffer A, offset 1000, stores header and packet received in buffer A
//	Receive buffer B, offset 1800, stores header and packet received in buffer B



//  Control and status register bitfield
struct control_and_status_register {
	// The first two bytes called the control and status register represent control bits used by the controller and user
        // PA controls which packets the workstation is interested in recieving
	// Errors = Runts/Oversize, FCS (Frame Check Sequence errors), Frame (Alignment Errors)
	// Multicast is here Multicast and Broadcast
	// 0 = all packets (promiscious), 1 = all packets-errors, 2 = all packets - fcs - frame
	// 3 = mypackets + multi, 4 = mypackets + multi - errors 5 = mypackets + multi - fcs - frame
	// 6 = mypackets + broadcast, 7 = mypackets + broadcast - errors 8 = mypackets + broadcast - fcs - frame
	uint8_t pa : 4; 
	// Next up is the JINTEN bit, or JAM interrupt enable, if this is set then an interrupt occurs when the JAM bit is set (a collision occured)
	uint8_t jinten : 1;
	// It's brother the TINTEN bit, or Transmit interrupt enabled, interrupts when the transmit buffer has been emptied (TBSW becomes 0) 
	uint8_t tinten : 1;
	// Their sibling is the AINTEN bit, the AINTEN bit enables an interrupt when ABSW becomes zero (a packet has been received in the A buffer)
	uint8_t ainten : 1;
	// AINTEN's twin is the BINTEN, it enables an interrupt when BBSW becomes zero (a packet has been received in the B buffer)
	uint8_t binten : 1;
	// RESET, always reads as zero, when written to, will perform a reset which sets ABSW, BBSW and TBSW to zero, this is the only way to recover from a failure of >15 transmission errors
	uint8_t reset : 1;
	// Bit number 9, is not used
	uint8_t unused : 1;
	// RBBA holds onto whether the A or B receive buffer has been used more recently, when a packet arrives in B RBBA is set to zero
	// If a packet arrives in A, RBBA is set to 1, if both buffers are receive enabled, the buffer with the older packet will receive the new one  
	// If RBBA is 0, then the packet goes to the A buffer, if RBBA is 1, then the packet goes into the B buffer
	uint8_t rbba : 1;
	// Next up is the address memory switch, it is set to one, by software, when the mac-address of this controller has been written into the address ram
	// Once set, the driver will use that address to determine which packets are "mypackets"
	uint8_t amsw : 1;
	//  JAM bit, set to 1 if transmission error occurs, software must write a delay multplier into MEBACK and then 1 into JAM to clear JAM (retry transmission)
	uint8_t jam : 1;
	// TBSW, set to 1 when you wish to transmit a packet, MEXHDR must first be set to the offset of the transmit buffer to the start of the packet
	// Next the packet must be written to that offset and until the end of the buffer, and then finally TBSW set to 1, when the packet has been transmitted
	// TBSW will return to zero, if tinten is enabled, an interrupt will also occur 
	uint8_t tbsw : 1;
	// ABSW, set to 1 when you wish to receive a packet in receive buffer A, 
	uint8_t absw : 1;
	// BBSW, set to 1 when you wish to receive a packet in receive buffer B, note that if both AB and BB are set, then RBBA sequence describer earlier, is used to
	// select which buffer gets the data
	uint8_t bbsw : 1;
};

struct transmit_header {
	// This is the start of the transmit buffer, @MEBASE+800
	uint16_t firstbyte : 11;
	// Next five bits are unused
	uint8_t unused : 5;
};

struct receive_header {
	// First free byte, 11 bits, essentially an offset into the first free byte of the buffer (MEBASE + 1000 + MEAHDR + packet)
	uint16_t firstfree : 11;
	//  Framing error bit, set to 1 if there's a frame error
	uint8_t framingerror : 1;
	// Address match bit, set to 0, if it's destined for our address, 1 otherwise
	uint8_t addressmatch : 1;
	//  Range error bit, set to 1 if there's a range error
	uint8_t rangeerror : 1;
	// Broadcast bit, set to 0 if this is a broadcast, 1, if it is not
	uint8_t broadcast : 1;
	// FCS error bit, set to 1 if there's an FCS error
	uint8_t fcserror : 1;
};

struct mac_addr {
	uint8_t o1;
	uint8_t o2;
	uint8_t o3;
	uint8_t o4;
	uint8_t o5;
	uint8_t o6;
	uint8_t o7;
	uint8_t o8;
};

// The point of the unions is to enable us to return whole bitfields, while still being able to manipulate members within them without using binary operators

union csr {
	struct control_and_status_register csr;
	uint16_t csrword;
};

union macaddr {
	struct mac_addr addr;
};

union txhdr {
	struct transmit_header hdr;
	uint16_t header;
};

union rxhdr {
	struct receive_header hdr;
	uint16_t header;
};

// Variable for the status register
union csr *mecsr;

// Retransmission backoff counter
uint16_t meback;

// This is the start of the 3c400 rom, @MEBASE+400
union macaddr *romaddr;

// This is the station address ram, @MEBASE+600, it holds the station's mac address, used by the adapter to determine which packets are "ours"
union macaddr *ramaddr;

// This is the adapter's mac-address once set
unsigned char macaddr[7];


// Transmit header
union txhdr *mexhdr;
// Transmit buffer
unsigned char *mexbuffer;

// Receive headers A and B
union rxhdr *meahdr;
union rxhdr *mebhdr;

// Receive buffers
unsigned char *meabuffer;
unsigned char *mebbuffer;

//  Trace positive means outputting lots of debug information on the lowlevel processing of the emulation, best enabled by using the e3c400_enable_trace function...
int trace_3c400 = 0;

#if defined(__unix__)|| defined(__MACH__)
// File handle for the BPF connection
int bpf = 0;
// Counter to hold onto how many frames are waiting in the buffer 
int bpf_buflen = 0;
// Struct for BPF header and received packet
struct bpf_hdr* bpf_buf;
struct bpf_hdr* bpf_packet;
// Slowdown counter which prevents the BPF code from running on every I/O iteration since it slows down the emulator signficantly
uint32_t bpfscan = 0;
//  Some time measurement tools
clock_t bpfbegin = 0;
clock_t bpfend = 0;
double bpfelapsed;
#endif

// Polynomial used
uint32_t poly = 0xedb88320;

// This is slow but functional using bit math
unsigned crc32(unsigned char *data, int length) {
	// Init is fixed here
	uint32_t crc = 0xffffffff;

	while(--length >= 0) {
		unsigned char current = *data++;
		int bit;

		for (bit = 8; --bit >= 0; current >>= 1) {
			if ((crc ^ current) & 1) {
				crc >>= 1;
				crc ^= poly;
			} else
				crc >>= 1;
		}
	}
	return ~crc;
}

// Function to enable and disable tracing
void e3c400_enable_trace(int level) {
	trace_3c400=level;
}

// Ack interrupt request
int32_t e3c400_device_ack() {
	if(trace_3c400)
		printf("3C400 IRQ Device ACK:\n");

	// Clear interrupt and return
	int_controller_clear(IRQ_SW_INT3);
	return M68K_INT_ACK_AUTOVECTOR;
}

// Functions to interact with the network
// At the moment only BPF is supported but even if not present, the functions will "work" but obviously no packets can be received or sent
//
// If you want to use BPF, you should put yourself in the group that owns /dev/bpf* and chmod it to 0660 (if it isn't already)
// The alternative is to run the emulator as root, which is A VERY BAD IDEA(TM)
// You should also set the bpf device and network device (near the top of this file)

// Check whether this packet is desirable, depending on the adapter mode, and PA settings
uint8_t is_mypacket(unsigned char buffer) {
	// Check which buffer we're checking
	if(buffer == 'A') {
		//  Check if packet is broadcast packet
		//  Note that this is reversed, bit is set, if packet is not a broadcast
		if(meabuffer[0] == 0xff && meabuffer[1] == 0xff && meabuffer[2] == 0xff && meabuffer[3] == 0xff && meabuffer[4] == 0xff && meabuffer[5] == 0xff) {
			meahdr->hdr.broadcast = 0;
			if(trace_3c400)
				printf("Incoming packet is destined to buffer A and is a broadcast packet\n");
		} else {
			meahdr->hdr.broadcast = 1;
			if(trace_3c400)
				printf("Incoming packet is destined to buffer A and is a regular packet\n");
		}
		// Check if destined to us, this again is reversed, bit is set if packet is not addressed to us
		if(meabuffer[0] == macaddr[0] && meabuffer[1] == macaddr[1] && meabuffer[2] == macaddr[2] && 
	           meabuffer[3] == macaddr[3] && meabuffer[4] == macaddr[4] && meabuffer[5] == macaddr[5]) {
			meahdr->hdr.addressmatch = 0;
			if(trace_3c400)
				printf("Incoming packet is destined to buffer A and is destined to us\n");
		} else {
			meahdr->hdr.addressmatch = 1;
			if(trace_3c400)
				printf("Incoming packet was destined to buffer A but does not belong to us\n");
		}
		//  Any setting of all packets means every packet will be delivered, error packets won't make it to us anyway
		if(mecsr->csr.pa == 0 || mecsr->csr.pa == 1 || mecsr->csr.pa == 2) {
			if(trace_3c400)
				printf("Promiscious mode, all packets should be delivered to CPU\n");
			return 1;
		// Any other setting means delivering all packets destined to us, and all broadcast packets, the multicast settings are ignored..
		} else {
			// If addressed to us, or broadcast
			if(meahdr->hdr.addressmatch == 0 || meahdr->hdr.broadcast == 0) {
				if(trace_3c400)
					printf("Packet should be delivered to CPU\n");
				return 1;
			}
			
		} 
	} else if(buffer == 'B') {
		//  Check if packet is broadcast packet
		//  Note that this is reversed, bit is set, if packet is not a broadcast
		if(mebbuffer[0] == 0xff && mebbuffer[1] == 0xff && mebbuffer[2] == 0xff && mebbuffer[3] == 0xff && mebbuffer[4] == 0xff && mebbuffer[5] == 0xff) {
			mebhdr->hdr.broadcast = 0;
			if(trace_3c400)
				printf("Incoming packet is destined to buffer B and is a broadcast packet\n");
		} else {
			mebhdr->hdr.broadcast = 1;
			if(trace_3c400)
				printf("Incoming packet is destined to buffer B and is a regular packet\n");
		}
		// Check if destined to us, this again is reversed, bit is set if packet is not addressed to us
		if(mebbuffer[0] == macaddr[0] && mebbuffer[1] == macaddr[1] && mebbuffer[2] == macaddr[2] && 
	           mebbuffer[3] == macaddr[3] && mebbuffer[4] == macaddr[4] && mebbuffer[5] == macaddr[5]) {
			mebhdr->hdr.addressmatch = 0;
			if(trace_3c400)
				printf("Incoming packet is destined to buffer B and is destined to us\n");
		} else {
			mebhdr->hdr.addressmatch = 1;
			if(trace_3c400)
				printf("Incoming packet was destined to buffer B but does not belong to us\n");
		}
		//  Any setting of all packets means every packet will be delivered, error packets won't make it to us anyway
		if(mecsr->csr.pa == 0 || mecsr->csr.pa == 1 || mecsr->csr.pa == 2) {
			if(trace_3c400)
				printf("Promiscious mode, all packets should be delivered to CPU\n");
			return 1;
		// Any other setting means delivering all packets destined to us, and all broadcast packets, the multicast settings are ignored..
		} else {
			// If addressed to us, or broadcast
			if(mebhdr->hdr.addressmatch == 0 || mebhdr->hdr.broadcast == 0) {
				if(trace_3c400)
					printf("Packet should be delivered to CPU\n");
				return 1;
			}
			
		} 
	}
	// we never reach here
	return 0;
}

// Function to find which buffer is free
// Returns A, B, or Z (if no buffer is available)
unsigned char find_buffer() {
	// Both ABSW and BBSW are set?
	if(mecsr->csr.absw && mecsr->csr.bbsw) {
		// Check value of RBBA
		if(mecsr->csr.rbba) {
			// RBBA is set, B buffer should get packet
			return 'B';
		} else {
			return 'A';
		}
	// ABSW is set
	} else if(mecsr->csr.absw) {
		return 'A';
	// BBSW
	} else if(mecsr->csr.bbsw) {
		return 'B';
	} else {
		return 'Z';
	}
}

// Function to handle packets that are outbound
void handle_outgoing_packets() {
	// Integer to store first byte of packet to transmit
	uint16_t firstbyte;
	uint32_t crc;
	// Loop variable
	int p=0;
	/// Check if we have an outgoing paccket
	if(mecsr->csr.tbsw) {
		// Read packet header first_byte value
		firstbyte = mexhdr->hdr.firstbyte;
		if(trace_3c400)
			printf("3C400 Write packet, offset %u, size %u\n",firstbyte,2048-firstbyte);
		// We calculate the CRC32 in case we need to use it later
		// The CRC includes the entire frame, minus the 4 bytes of CRC 32, and the 12 bytes of spacing set at the end
		crc=crc32(&mexbuffer[firstbyte],2048-16-firstbyte);
		// Print it all out if we're tracing
		for(p=firstbyte;p<2048;p++) {
			if(trace_3c400) 
				printf("%02x ",mexbuffer[p]);
		}
		if(trace_3c400)
			printf("\n");
		int byteswritten;
		// Send packet if we have BPF
		#if defined(__unix__) || defined(__MACH__)
		if(bpf) {		
			byteswritten=write(bpf,&mexbuffer[firstbyte],2048-firstbyte);
			if(trace_3c400)
				printf("Bytes written to BPF: %d\n",byteswritten);
		}
		#endif
		// Set tbsw to zero so that other packets can be sent
		mecsr->csr.tbsw = 0;
		// Check whether interrupt on transmit sent is set
		if(mecsr->csr.tinten)
			// interrupt the cpu to tell it we "sent" the packet
			int_controller_set(IRQ_SW_INT3);
	}
}

// Function to handle packets that are inbound
void handle_incoming_packet() {
	// Temp variable
	int p=0;
	// Both ABSW and BBSW are set?
	if(mecsr->csr.absw && mecsr->csr.bbsw) {
		// Check value of RBBA
		if(mecsr->csr.rbba) {
			// Are we interested in this packet?
			if(is_mypacket('B')) {
				if(trace_3c400) {
					printf("Packet delivered to buffer B, trace follows\n");
					// Print it all out if we're tracing
					for(p=0;p<mebhdr->hdr.firstfree;p++) {
						if(trace_3c400) 
							printf("%02x ",mebbuffer[p]);
					}
				}
				// Increase firstfree by 6 since the OS skips the last 6 bytes of the packet in the buffer
				mebhdr->hdr.firstfree += 6;
				// Set BBSW to zero to indicate we have a packet
				mecsr->csr.bbsw = 0;
				// Clear RBBA bit to indicate the most recent packet is in B
				mecsr->csr.rbba = 0;
				// We have a packet, check if we should interrupt
				if(mecsr->csr.binten) 
					// Interrupt CPU to tell it about the packet
					int_controller_set(IRQ_SW_INT3);
			} 
		} else {
			// Are we interested in this packet?
			if(is_mypacket('A')) {
				if(trace_3c400) {
					printf("Packet delivered to buffer A, trace follows\n");
					// Print it all out if we're tracing
					for(p=0;p<meahdr->hdr.firstfree;p++) {
						if(trace_3c400) 
							printf("%02x ",meabuffer[p]);
					}
					printf("\nMeaHDR firstfree: %u\n",meahdr->hdr.firstfree);
				}
				// Increase firstfree by 6 since the OS skips the last 6 bytes of the packet in the buffer
				meahdr->hdr.firstfree += 6;
				// Set ABSW to zero to indicate we have a packet
				mecsr->csr.absw = 0;
				// Set RBBA bit to indicate that most recent packet is in A
				mecsr->csr.rbba = 1;
				// We have a packet, check if we should interrupt
				if(mecsr->csr.ainten) 
					// Interrupt CPU to tell it about the packet
					int_controller_set(IRQ_SW_INT3);
			}
		}
	// Is ABSW set?
	} else if(mecsr->csr.absw) {
		// Are we interested in this packet?
		if(is_mypacket('A')) {
			if(trace_3c400) {
				printf("Packet delivered to buffer A, trace follows\n");
				// Print it all out if we're tracing
				for(p=0;p<meahdr->hdr.firstfree;p++) {
					if(trace_3c400) 
						printf("%02x ",meabuffer[p]);
				}
			}
			// Increase firstfree by 6 since the OS skips the last 6 bytes of the packet in the buffer
			meahdr->hdr.firstfree += 6;
			// Set ABSW to zero to indicate we have a packet
			mecsr->csr.absw = 0;
			// Set RBBA to 1 to indicate most recent packet is in A
			mecsr->csr.rbba = 1;
			// Check if we should interrupt the CPU
			if(mecsr->csr.ainten) 
				// Interrupt CPU to tell it about the packet
				int_controller_set(IRQ_SW_INT3);
		}
	// Is BBSW set?
	} else if(mecsr->csr.bbsw) {
		// Are we interested in this packet?
		if(is_mypacket('B')) {
			if(trace_3c400) {
				printf("Packet delivered to buffer A, trace follows\n");
				// Print it all out if we're tracing
				for(p=0;p<meahdr->hdr.firstfree;p++) {
					if(trace_3c400) 
						printf("%02x ",meabuffer[p]);
				}
			}
			// Increase firstfree by 6 since the OS skips the last 6 bytes of the packet in the buffer
			mebhdr->hdr.firstfree += 6;
			// Set BBSW to zero to indicate we have a packet
			mecsr->csr.bbsw = 0;
			// Clear RBBA bit to indicate the most recent packet is in B
			mecsr->csr.rbba = 0;
			// We have a packet, check if we should interrupt
			if(mecsr->csr.binten)
				// Interrupt CPU to tell it about the packet
				int_controller_set(IRQ_SW_INT3);
		}
	}
}

// Init the controller, malloc, set reasonable defaults for status registers, this is run by the emulator on startup
void e3c400_init(void) {
	// Local loop variables
	uint32_t p=0; uint32_t j=0;

	// Retransmit counter set to 0
	meback = 0;

	// Set mac-addr
	macaddr[0] = 0; macaddr[1] = 0; macaddr[2] = 0; macaddr[3] = 0; macaddr[4] = 0; macaddr[5] = 0; macaddr[6] = '\0'; 

	// Reset loop variables
	p=0; j=0;

	// Enable trace if you want to debug something
	e3c400_enable_trace(0);
		
	// Allocate memory 
	// Variable for the status register
	mecsr = malloc(sizeof(union csr));

	// This is the start of the 3c400 rom, @MEBASE+400
	romaddr = malloc(sizeof(union macaddr));

	// This is the station address ram, @MEBASE+600, it holds the station's mac address, used by the adapter to determine which packets are "ours"
	ramaddr = malloc(sizeof(union macaddr));

	// Transmit header
	mexhdr = malloc(sizeof(union txhdr));
	// Transmit buffer
	mexbuffer = malloc(2048*sizeof(char *));

	// Receive headers A and B
	meahdr = malloc(sizeof(union rxhdr));
	mebhdr = malloc(sizeof(union rxhdr));

	// Receive buffers
	meabuffer = malloc(2048*sizeof(char *));
	mebbuffer = malloc(2048*sizeof(char *));

	// Set reasonable defaults, receive all our packets - error, interrupt on jam, transmit, and receive buffers a&b
	// Set rest to 0
	mecsr->csr.pa = 7;
	mecsr->csr.jinten = 0;
	mecsr->csr.tinten = 0;
	mecsr->csr.ainten = 1;
	mecsr->csr.binten = 1;
	mecsr->csr.reset = 0;
	mecsr->csr.unused = 0;
	mecsr->csr.amsw = 0;
	mecsr->csr.jam = 0;
	mecsr->csr.tbsw = 0;
	mecsr->csr.absw = 0;
	mecsr->csr.tbsw = 0;

	//  Rom address set to MAC addresses previously defined in this file
	romaddr->addr.o1 = MAC1; romaddr->addr.o2 = MAC2; romaddr->addr.o3 = MAC3; romaddr->addr.o4 = MAC4;
	romaddr->addr.o5 = MAC5; romaddr->addr.o6 = MAC6; romaddr->addr.o7 = 0x00; romaddr->addr.o8 = 0x00;

	// Ram address set to all-zeroes, software will overwrite it
	ramaddr->addr.o1 = 0x00; ramaddr->addr.o2 = 0x00; ramaddr->addr.o3 = 0x00; ramaddr->addr.o4 = 0x00; 
	ramaddr->addr.o5 = 0x00; ramaddr->addr.o6 = 0x00; ramaddr->addr.o7 = 0x00; ramaddr->addr.o8 = 0x00;

	// Transmit buffer first byte set to zero
	mexhdr->hdr.firstbyte = 0;
	mexhdr->hdr.unused = 0;
	// Fill in the transmit buffer itself with zeros
	for(p=0;p<2046;p++)	
		mexbuffer[p] = 0;

	// Receive buffer A hdr
	meahdr->hdr.firstfree=16;
	meahdr->hdr.framingerror=0;
	meahdr->hdr.addressmatch=1;
	meahdr->hdr.rangeerror=0;
	meahdr->hdr.broadcast=1;
	meahdr->hdr.broadcast=1;
	meahdr->hdr.fcserror=0;
	//  Fill in the receive buffer itself with zeros
	for(p=0;p<2046;p++)	
		meabuffer[p] = 0;

	
	// Receive buffer B hdr
	mebhdr->hdr.firstfree=16;
	mebhdr->hdr.framingerror=0;
	mebhdr->hdr.addressmatch=1;
	mebhdr->hdr.rangeerror=0;
	mebhdr->hdr.broadcast=1;
	mebhdr->hdr.broadcast=1;
	mebhdr->hdr.fcserror=0;
	//  Fill in the receive buffer itself with zeros
	for(p=0;p<2046;p++)	
		mebbuffer[p] = 0;

	// Open BPF and bind to the interface
	#if defined(__unix__) || defined(__MACH__)
	char buf[11] = { 0 };
	// This should open the next available BPF device
	for(p=0;p<99;p++) {
		snprintf(buf,sizeof(buf),"/dev/bpf%i",p);
		bpf = open(buf, O_RDWR);
		// If we managed to open the device exit the loop
		if(bpf != -1)
			break;
	}	
	// Check whether BPF is still -1
	if(bpf == -1)
		// Warn user
		printf("Can't open BPF interface, check permissions of /dev/bpf*\n");
	// Associate the BPF with our requested network device (set near the top of this file)
	const char* interface = BPFINTERFACE;
	struct ifreq bindif;
	strcpy(bindif.ifr_name, interface);
	// Check whether we were able to bind
	if(ioctl(bpf, BIOCSETIF, &bindif ) < 0) {
		// Set BPF to 0 so we don't use it
		bpf=0;
		// Warn user
		printf("Unable to bind to requested network device %s\n",interface);
	}

	// Initial buffer length set to 1
	int bpg_buflen = 1;
	// Set immediate mode
	if(ioctl(bpf, BIOCIMMEDIATE, &bpf_buflen) == -1) {
		// Set BPF to 0 so we don't use it
		bpf=0;
		// Warn user
		printf("Unable to set BPF immediate mode, BPF disabled\n");
	}
	int fionbio = 1;
	
	// Set non-blocking I/O
	if(ioctl(bpf,FIONBIO, &fionbio) == -1) {
		// Set BPF to 0 so we don't use it
		bpf=0;
		// Warn user
		printf("Unable to enable BPF non-blocking I/O, BPF disabled\n");
	}
	
	// Set the BPF packet filter, we're not really interested in overwhelming the 68010 processor with packets not destined to the SUN ethernet addr
	// or broadcast packets.  If the PA value in the control register isn't explicitely set to promiscious mode we'll drop all frames not destined to us or broadcast     

	// Set up filter struct from our pre-defined packet filter
	struct bpf_program filter;
	filter.bf_len = sizeof(bpf_ours) / sizeof(struct bpf_insn);
	filter.bf_insns = &bpf_ours[0];
	// Init filter if PA >2
	if(mecsr->csr.pa > 2) {
		printf("Driver mode is promiscious, no filter set\n");
		if(ioctl(bpf, BIOCSETF, &filter) < 0) {
			//  Set BPF to 0 so we don't use it
			bpf=0;
			// Warn user
			printf("Can't enable BPF packet filtering, BPF disabled\n");
		} 
	} 
	// Set promiscious mode
	int promisc=1;
	if(ioctl(bpf,BIOCPROMISC,&promisc) <0) {
		// Warn user
		printf("Failed to set promiscious mode, your success with broadcast packet will vary\n");
	}
	// Stop BPF from overwriting the mac address in the header, it makes life much easier if we can identify our own fake mac-address on the wire
	int noautofill=1;
	if(ioctl(bpf,BIOCSHDRCMPLT,&noautofill) < 0) {
		//  Set BPF to 0 so we don't usee it
		bpf=0;
		//  Warn user
		printf("Can't disable BPF autofill of src mac-address, BPF disabled\n");
	}
	#endif
}


// Perform our daily tasks
void e3c400_update(void) {
	// Temporary loop variable
	int p=0;
	// BPF buffer
	char *buffer = NULL;
	char *pointer = NULL;
	// Get which buffer to use
	unsigned char whichbuffer;
	// Buffer length
	size_t blen=0;
	ssize_t readbytes = 0;
	// BPF header
	struct bpf_hdr *bh = NULL;
	// Check the reset bit
	if(mecsr->csr.reset) {
		// Reset the Transmit and Receive enable bits
		mecsr->csr.tbsw=0;
		mecsr->csr.absw=0;
		mecsr->csr.bbsw=0;
		if(trace_3c400)
			printf("3C400 Performing board reset\n");
		// Disable reset bit
		mecsr->csr.reset=0;
		// Set PA to 7, my packets, and broadcast packets, but no errors
		mecsr->csr.pa = 7;
	}
	// Check JAM bit and interrupt CPU if needed
	if(mecsr->csr.jam) {
		printf("3C400: JAM bit was set\n");
		// Is JINTEN set?  Interrupt CPU
		if(mecsr->csr.jinten) {
			if(trace_3c400)
				printf("3C400: JAM interrupt sent to CPU\n");
			// Interrupt the CPU to tell it about it
			int_controller_set(IRQ_SW_INT3);
		}
	}
	// Check the AMSW switch
	if(mecsr->csr.amsw) {
		// RAM address of the controller should now be in the ram addr, let's read it into the mac-addr array
		memcpy(&macaddr[0],&ramaddr->addr,6);
		if(trace_3c400) {
			printf("3C400: Mac-address set to");
			for(p=0;p<6;p++)
				printf(":%02x",macaddr[p]);
			printf("\n");
		}
		// Set the amsw back down
		mecsr->csr.amsw=0;
			
	}
	//  Check for outgoing packets and send them
	handle_outgoing_packets();
	// Increment the BGP scan counter by 1 for each update loop
	bpfscan++;
	// If BPF is active and set, there are are packets waiting and the adapter is ready to receive them, and we're on an update round where we take care of the bpf
	// Despite only running this every 5k runs it still manages to perform at around 10Mbits per second on a modern processor
	// If you find that the network is very slow, you might consider lowering this number from 5000, maybe by 1000 at a time until you find a compromise
	// It would be better to have BPF signal the process when there is a packet waiting, that is however unsupported on at least Mac OS X
	if(bpf && (mecsr->csr.absw || mecsr->csr.bbsw) && (bpfscan % 5000 == 0)) {
		// Get buffer length
		if(ioctl(bpf, BIOCGBLEN, &blen) <0) {
			printf("Can't get BPF buffer length\n");
			bpf=0;
			return;
		}
		// Malloc buffer
		if( ( buffer = malloc(blen)) == NULL) {
			printf("Can't allocate memory for BPF buffer\n");
			bpf=0;
			return;
		}
		// Zero out readbytes
		readbytes=0;

		// Loop if there are packets
		while(mecsr->csr.absw || mecsr->csr.bbsw) {
			// Set null terminator at start of buffer
			(void)memset(buffer,'\0',blen);

			// Read from BPF
			readbytes=read(bpf,buffer,blen);
	
			// If no bytes are available, exit
			if(readbytes <= 0)
				break;

			if(trace_3c400 && readbytes >=0)
				printf("3C400: %ld bytes available to process in receive buffer\n",readbytes);

			// Set up pointer to work with packet(s)
			pointer=buffer;	

			// Process packets if the OS is ready to accept packets and we have data
			while(readbytes >0 && pointer < (buffer + readbytes)) {
				// Get the BPF header
				bh = (struct bpf_hdr *)pointer;		

				// Find which buffer to use
				whichbuffer = find_buffer();
				if(whichbuffer == 'A') {
					// Verify that caplen isn't longer than our buffer size
					if(bh->bh_caplen < 2046) {
						// Copy from after the BPF header, and for as long as we had packets
						// Packet is always placed until the end of the meabuffer
						memcpy(&meabuffer[0],(pointer + bh->bh_hdrlen),bh->bh_caplen);
						// Set the first free bytes of the meahdr, to tell the OS where the packet starts
						// We have to take into account the 2 byte header of meahdr and the FCS
						meahdr->hdr.firstfree=bh->bh_datalen;
						// Process the packet
						handle_incoming_packet();		
					} else {
						// Notify user
						printf("Packet length exceeds 3C400 ethernet buffer\n");
					}
				} else if(whichbuffer == 'B') {
					// Verify that caplen isn't longer than our buffer size
					if(bh->bh_caplen < 2046) {
						// Copy from after the BPF header, and for as long as we had packets
						// Packet is always placed until the end of the meabuffer
						memcpy(&mebbuffer[0],(pointer + bh->bh_hdrlen),bh->bh_caplen);
						// Set the first free bytes of the mebhdr, to tell the OS where the packet starts
						// We have to take into account the 2 byte header of meahdr and the FCS
						mebhdr->hdr.firstfree=bh->bh_caplen;
						// Process the packet
						handle_incoming_packet();		
					} else {
						// Notify user
						printf("Packet length exceeds 3C400 ethernet buffer\n");
					}
				} else {
					// No buffers available, exit loop, we'll come back later...
					printf("No buffers available for received ethernet packet, discarding\n");
					// Return
					return;
				}
				pointer += BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);
			}
		} // Loop while buffers are ready
	}
	if(bpf && buffer)
		free(buffer);
	
}
	
// Handle reading bytes from the emulated e3c400
uint32_t e3c400_read(uint32_t addr, int32_t size) {
	if(trace_3c400)
		printf("3C400 Read addr: %x, size: %d\n",addr,size);

	// Temporary variables
	int p=0; int offset=0; uint32_t retvalue; uint32_t retarray[8];

	// Reading and writing from the status register, the status word, and the MEBACK retransmission counter are repeated throughout that space
	if(addr >= 0xe0000 && addr <0xe0400 && size==2) {
		if(trace_3c400)
			printf("3C400 Status Register read, value: %u\n",mecsr->csrword);
		return mecsr->csrword;
	// Receive buffer A read
	} else if(addr >= 0xe1002 & addr <0xe1800 && size <=4) {
		// Initial offset calculated
		offset= addr - 0xe1002;
		switch(size) {
			case 4:
				retvalue =  meabuffer[offset] <<24;
				retvalue += meabuffer[offset+1] <<16;
				retvalue += meabuffer[offset+2] <<8;
				retvalue += meabuffer[offset+3];
				break;
			 case 2:
				retvalue = meabuffer[offset] << 8;
				retvalue += meabuffer[offset+1];
				break;
			 case 1:
				retvalue = meabuffer[offset];
				break;
			default:
				printf("3C400: rx buffer A, unmatched size %d at addr: %x \n",size,addr);
				retvalue=0;
				break;
		}
		if(trace_3c400) {
			if(size == 4)
				printf("Inside receive buffer A total length: %u, read addr: %x offset: %u value: %08x\n",meahdr->hdr.firstfree,addr,offset,retvalue);
			if(size == 2)
				printf("Inside receive buffer A total length: %u, read addr: %x offset: %u value: %04x\n",meahdr->hdr.firstfree,addr,offset,retvalue);
			if(size == 1)
				printf("Inside receive buffer A total length: %u, read addr: %x offset: %u value: %02x\n",meahdr->hdr.firstfree,addr,offset,retvalue);
		}
		// Return the value
		return retvalue;
	// Receive buffer B read
	} else if(addr >= 0xe1802 & addr <0xe2000 && size <2046) {
		// Initial offset calculated
		offset= addr - 0xe1802;

		switch(size) {
			case 4:
				retvalue = mebbuffer[offset] << 24;
				retvalue += mebbuffer[offset+1] << 16;
				retvalue += mebbuffer[offset+2] << 8;
				retvalue += mebbuffer[offset+3];
				break;
			 case 2:
				retvalue = mebbuffer[offset] << 8;
				retvalue += mebbuffer[offset+1];
				break;
			 case 1:
				retvalue = mebbuffer[offset];
				break;
			default:
				printf("3C400: rx buffer B, unmatched size %d at addr: %x \n",size,addr);
				retvalue=0;
				break;
		}
		if(trace_3c400) {
			if(size == 4)
				printf("Inside receive buffer B read, addr: %x offset: %u value: %08x\n",addr,offset,retvalue);
			if(size == 2)
				printf("Inside receive buffer B read, addr: %x offset: %u value: %04x\n",addr,offset,retvalue);
			if(size == 1)
				printf("Inside receive buffer B read, addr: %x offset: %u value: %02x\n",addr,offset,retvalue);
		}
		// Return the value
		return retvalue;
 	// Reading the rom address, 8 bytes, 6 of which represent the mac-address, the final two 6adding
	// This then repeats through the rom-address space from 0xe0400 and until 0xe0600
	} else if(addr >= 0xe0400 && addr < 0xe0600) {
		// Reading two bytes?
		if(size==2) {
			// Start
			if(addr % 8 == 0) {
				retarray[0] = romaddr->addr.o1;
				retarray[1] = romaddr->addr.o2;
			// Second
			} else if(addr % 2 == 0) {
				retarray[0] = romaddr->addr.o3;
				retarray[1] = romaddr->addr.o4;
			// Third
			} else if(addr % 4 == 0) {
				retarray[0] = romaddr->addr.o5;
				retarray[1] = romaddr->addr.o6;
			// These are present but always zero...
			} else if(addr % 6 == 0) {
				retarray[0] = romaddr->addr.o7;
				retarray[1] = romaddr->addr.o8;
			}
		// Reading four bytes?
		} else if(size==4) {
			// Start-3
			if(addr % 8 == 0) {
				retarray[0] = romaddr->addr.o1;
				retarray[1] = romaddr->addr.o2;
				retarray[2] = romaddr->addr.o3;
				retarray[3] = romaddr->addr.o4;
			// 2-6
			} else if(addr % 2 == 0) {
				retarray[0] = romaddr->addr.o3;
				retarray[1] = romaddr->addr.o4;
				retarray[2] = romaddr->addr.o5;
				retarray[3] = romaddr->addr.o6;
			// 4-8
			} else if(addr % 4 == 0) {
				retarray[0] = romaddr->addr.o5;
				retarray[1] = romaddr->addr.o6;
				retarray[2] = romaddr->addr.o7;
				retarray[3] = romaddr->addr.o8;
			}
		} else {
			printf("3C400: Uncaught Fetching of %u bytes from rom ethernet address\n",size);
			return 0;
		}
 	// Reading the ram address, 8 bytes, 6 of which represent the mac-address, the final two padding
	// This then repeats through the rom-address space from 0xe0600 and untili 0xe0800
	} else if(addr >= 0xe0600 && addr < 0xe0800) {
		// Reading two bytes?
		if(size==2) {
			// Start
			if(addr % 8 == 0) {
				retarray[0] = ramaddr->addr.o1;
				retarray[1] = ramaddr->addr.o2;
			// Second
			} else if(addr % 2 == 0) {
				retarray[0] = ramaddr->addr.o3;
				retarray[1] = ramaddr->addr.o4;
			// Third
			} else if(addr % 4 == 0) {
				retarray[0] = ramaddr->addr.o5;
				retarray[1] = ramaddr->addr.o6;
			// These are present but always zero...
			} else if(addr % 6 == 0) {
				retarray[0] = ramaddr->addr.o7;
				retarray[1] = ramaddr->addr.o8;
			}
		// Reading four bytes?
		} else if(size==4) {
			// Start-3
			if(addr % 8 == 0) {
				retarray[0] = ramaddr->addr.o1;
				retarray[1] = ramaddr->addr.o2;
				retarray[2] = ramaddr->addr.o3;
				retarray[3] = ramaddr->addr.o4;
			// 2-6
			} else if(addr % 2 == 0) {
				retarray[0] = ramaddr->addr.o3;
				retarray[1] = ramaddr->addr.o4;
				retarray[2] = ramaddr->addr.o5;
				retarray[3] = ramaddr->addr.o6;
			// 4-8
			} else if(addr % 4 == 0) {
				retarray[0] = ramaddr->addr.o5;
				retarray[1] = ramaddr->addr.o6;
				retarray[2] = ramaddr->addr.o7;
				retarray[3] = ramaddr->addr.o8;
			}
		} else {
			printf("3C400: Uncaught Fetching of %u bytes from ram ethernet address\n",size);
			return 0;
		}
	} else {
		// Transmit and receive buffer headers
		switch(addr) {
			// Transmit buffer header
			case 0xe0800:
				// Reading the entire header
				if(size==2)
					return mexhdr->header;
				else
					printf("3C400: Uncaught Fetching of %u bytes from transmit header\n",size);
				return -1;
				break;
			// Receive buffer header A
			case 0xe1000:
				// Reading the entire header
				if(size==2)
					return meahdr->header;
				else
					printf("3C400: Uncaught Fetching of %u bytes from receive header A\n",size);
				return -1;
				break;
			// Receive buffer header B
			case 0xe1800:
				// Reading the entire header
				if(size==2)
					return mebhdr->header;
				else
					printf("3C400: Uncaught Fetching of %u bytes from receive header B\n",size);
				return -1;
				break;
			default:
				printf("3C400: Reaching Uncaught read instruction for address %x, size: %d\n",addr,size);
				return -1;
				break;
		}
	} // End address range if
  // This is unreachable
  return -1;
}

// Handle writing bytes to the emulated 3c400
void e3c400_write(uint32_t addr, uint32_t size, uint32_t value) {
	// Offset is here
	int offset=0; 
  	// Write values to the e3c400, this is limited to the control and status register as well as the transport buffeer
	if(trace_3c400)
		printf("3C400 Write addr: %x, value: %u, size: %d\n",addr,value,size);

	if(addr == 0xe0000 && size==2) {
		// The PA is the first 4 bits
		mecsr->csr.pa = (value & 0xf)<<4;
		if(trace_3c400)
			printf("3C400 Setting PA to %u\n",(value&0xf)<<4);
		// Interrupt settings for jam, a/b receive buffers, transmit bufffer
		mecsr->csr.jinten = (value & (1 << 4))>>4;
		if(trace_3c400)
			printf("3C400 Setting JINTEN to %u\n",(value & (1 << 4))>>4);
		mecsr->csr.tinten = (value & (1 << 5))>>5;
		if(trace_3c400)
			printf("3C400 Setting TINTEN to %u\n",(value & (1 << 5))>>5);
		mecsr->csr.ainten = (value & (1 << 6))>>6;
		if(trace_3c400)
			printf("3C400 Setting AINTEN to %u\n",(value & (1 << 6))>>6);
		mecsr->csr.binten = (value & (1 << 7))>>7;
		if(trace_3c400)
			printf("3C400 Setting BINTEN to %u\n",(value & (1 << 7))>>7);
		// Reset flag, this resets the controller
		mecsr->csr.reset = (value & (1 << 8))>>8;
		if(trace_3c400)
			printf("3C400 Setting RESET to %u\n",(value & (1 << 8))>>8);
		// AMSW, this is set to 1 when our mac-address has been written into the 3c400 RAM
		mecsr->csr.amsw = (value & (1 << 11))>>11;
		if(trace_3c400)
			printf("3C400 Setting AMSW to %u\n",(value & (1 << 11))>>11);
		// JAM bit
		mecsr->csr.jam = (value & (1 << 12))>>12;
		if(trace_3c400)
			printf("3C400 Setting JAM to %u\n",(value & (1 << 12))>>12);
		// Transmit buffer, set to 1 when packet is in tx buffer and ready to be transmitted
		// Not possible to set to 0 after it's set to one, it's set to 0 when the packet has been transmitted
		if(mecsr->csr.tbsw == 0) {
			mecsr->csr.tbsw = (value & (1 << 13))>>13;
			if(trace_3c400)
				printf("3C400 Setting TBSW to %u\n",(value & (1 << 13))>>13);
		} else {
			printf("3C400: Setting transmit buffer bit with active transfer in progress\n");
		}
		// Receive buffer, set to 1 when you want packet to be delivered to this buffer
		// It's not possible to set absw when it's 1, it will be cleared if a packet arrives in the A buffer
		if(mecsr->csr.absw == 0) {
			mecsr->csr.absw = (value & (1 << 14))>>14;
			if(trace_3c400)
				printf("3C400 Setting ABSW to %u\n",(value & (1 << 14)>>14));
		} 
		// It's not possible to set bbsw when it's 1, it will be cleared if a packet arrives in the B buffer
		if(mecsr->csr.bbsw == 0) {
			mecsr->csr.bbsw = (value & (1 << 15))>>15;
			if(trace_3c400)
				printf("3C400 Setting BBSW to %u\n",(value & (1 << 15)>>15));
		} 
	} else if(addr == 0xe0600 && size ==4) {
		// Setting the RAM address,first two bytes
		ramaddr->addr.o1 = (value & 0xFFFFFFFF)>>24;;
		ramaddr->addr.o2 = (value & 0xFFFFFF)>>16;
		ramaddr->addr.o3 = (value & 0xFFFF)>>8;
		ramaddr->addr.o4 = value & 0xFF;
		if(trace_3c400)
			printf("3C400 setting RAM mac-address to %02x:%02x:%02x:%02x:%02x:%02x\n",ramaddr->addr.o1,ramaddr->addr.o2,ramaddr->addr.o3,ramaddr->addr.o4,ramaddr->addr.o5,ramaddr->addr.o6);
	} else if(addr == 0xe0604 && size ==2) {
		// Setting the RAM address,last two bytes
		ramaddr->addr.o5 = (value & 0xFFFFFFFF)>>8;
		ramaddr->addr.o6 = value & 0xFf;
	} else if(addr == 0xe0800 && size ==2) {
		// Mex HDR write
		mexhdr->hdr.firstbyte = value;
	} else if(addr >= 0xe0802 & addr <0xe1000 && size <=4) {
		// Initial offset calculated
		offset= addr - 0xe0800;
	
		switch(size) {
			case 4:
				mexbuffer[offset]   = (value & 0xFFFFFFFF)>>24;;
				mexbuffer[offset+1] = (value & 0xFFFFFF)>>16;
				mexbuffer[offset+2] = (value & 0xFFFF)>>8;
				mexbuffer[offset+3] = value & 0xFF;
				break;
			 case 2:
				mexbuffer[offset] = (value & 0xFFFF)>>8;
				mexbuffer[offset+1] = value & 0xFF;
				break;
			 case 1:
				mexbuffer[offset] = value & 0xFF;
				break;
			default:
				printf("3C400: tx buffer, unmatched size %d at addr: %x \n",size,addr);
				break;
		}
	}  else {
		printf("3C400: Uncaught write to addr %x, size %u\n",addr,size);
	}
}
