/* bootblock.h
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * Provides a simple API for reading the Boot Block Header that
 * exists in the first two sectors of a Macintosh floppy disk or
 * hard drive.
 */

#include "Apple/common/Apple-Macintosh-Common.h"


/* BOOT BLOCK HEADER. Every Macintosh disk has a 1024-byte (2-sector) area at the
   very beginning that is reserved for executable code necessary to boot the machine.
   If the disk is not bootable this area contains zeros or 0xFC. */
typedef struct {
	aInteger		bbID;			// should contain 0x4C4B
	aLongInt		bbEntry;		// entry point to the boot code [1]
	aInteger		bbVersion;		// version in low byte unless bit 15 set, then meaning changes
	aInteger		bbPageFlags;	// ????
	aStr15			bbSysName;		// Name of the system file, Typically "System"
	aStr15			bbShellName;	// Name of the shell file, Typically "Finder"
	aStr15			bbDdbg1Name;	// Name of the first debugger, Typically "MacsBug"
	aStr15			bbDdbg2Name;	// Name of the 2nd debugger, Typically "Disassembler"
	aStr15			bbScreenName;	// Name of the file with startup screen, Typically "StartUpScreen"
	aStr15			bbHelloName;	// Name of the startup program, Typically "Finder"
	aStr15			bbScrapName;	// Name of the system scrap file, Typically "Clipboard"
	aInteger		bbCntFCBs;		// Number of FCBs to put in the FCB buffer
	aInteger		bbCntEvts;		// Number of event queue elements to allocate.
	aLongInt		bb128KSHeap;	// Size of System Heap on 128K Macintoshes
	aLongInt		bb256KSHeap;	// Reserved
	aLongInt		bbSysHeapSize;	// Size of System Heap on 512K or larger Macintoshes
	aInteger		filler;			// Reserved
	aLongInt		bbSysHeapExtra;	// Additional system heap required
	aLongInt		bbSysHeapFract;	// Fraction of available RAM to be used for System heap
} MacintoshBootBlkHdr;
// [1] Not a pointer. This field contains the 68k instruction BRA.S which
//     branches to a location following this structure.

/*********************************************************************
 * boot block handling code. This isn't really all that relevant to  *
 * MFS or HFS but it can be useful for other interesting projects.   *
 *********************************************************************/
class BootBlockReader {
public:
	int CheckBootBlock(unsigned char buffer[1024],MacintoshBootBlkHdr *BootHdr);
public:
	// variables set by CheckBootBlock()
	unsigned char *bootcode;
	int bootcode_sz;
public:
	// a method for this class to make comments with.
	// by default it is routed to printf().
	virtual void commentary(char *fmt,...);
};
