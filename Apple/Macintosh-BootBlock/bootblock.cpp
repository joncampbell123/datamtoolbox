/* bootblock.cpp
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * Provides a simple API for reading the Boot Block Header that
 * exists in the first two sectors of a Macintosh floppy disk or
 * hard drive.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "common/Types.h"
#include "Apple/common/Apple-Macintosh-Common.h"
#include "Apple/Macintosh-BootBlock/bootblock.h"

/*********************************************************************
 * boot block handling code. This isn't really all that relevant to  *
 * MFS or HFS but it can be useful for other interesting projects.   *
 *********************************************************************/

int BootBlockReader::CheckBootBlock(unsigned char buf[1024],MacintoshBootBlkHdr *BootHdr)
{
	if ((BootHdr->bbID = lbei16(buf)) != 0x4C4B)
		return 0;

	BootHdr->bbEntry =				lbei32(	buf+0x02);
	BootHdr->bbVersion =			lbei16(	buf+0x06);
	BootHdr->bbPageFlags =			lbei16(	buf+0x08);
	aStr15cpy(&BootHdr->bbSysName,			buf+0x0A);
	aStr15cpy(&BootHdr->bbShellName,		buf+0x1A);
	aStr15cpy(&BootHdr->bbDdbg1Name,		buf+0x2A);
	aStr15cpy(&BootHdr->bbDdbg2Name,		buf+0x3A);
	aStr15cpy(&BootHdr->bbScreenName,		buf+0x4A);
	aStr15cpy(&BootHdr->bbHelloName,		buf+0x5A);
	aStr15cpy(&BootHdr->bbScrapName,		buf+0x6A);
	BootHdr->bbCntFCBs =			lbei16(	buf+0x7A);
	BootHdr->bbCntEvts =			lbei16(	buf+0x7C);
	BootHdr->bb128KSHeap =			lbei32(	buf+0x7E);
	BootHdr->bb256KSHeap =			lbei32(	buf+0x82);
	BootHdr->bbSysHeapSize =		lbei32(	buf+0x86);
	BootHdr->filler =				lbei16(	buf+0x8A);		/* on System 1.1 disks this is where the boot code starts */
	BootHdr->bbSysHeapExtra =		lbei32(	buf+0x8C);
	BootHdr->bbSysHeapFract =		lbei32(	buf+0x90);

	/* look for "BRA.S $someoffset" and determine boot code entry from there */
	if ((BootHdr->bbEntry>>24) == 0x60) {
		int offset = (BootHdr->bbEntry&0xFFFFFF)+6;
		if (offset >= 0x86 && offset < 0x400) {
			unsigned char match;
			int ii;

			bootcode = buf+offset;
			ii = 1023;

			/* try to find the real end of the code vs the sector end where repeating filler bytes are */
			match=buf[ii];
			if (match == 0xDA || match == 0x00) {
				while (ii >= 0 && buf[ii] == match) ii--;
			}

			ii++;
			bootcode_sz = ii-offset;
		}
		else {
			commentary("WARNING: Invalid pointer to boot code in bbEntry field\n");
			bootcode = NULL;
			bootcode_sz = 0;
		}

		commentary("*Offset, size in sector to boot code:              %u ($%X), %u ($%X)\n",offset,offset,bootcode_sz,bootcode_sz);
	}
	else {
		commentary("WARNING: Unusual boot code in bbEntry field\n");
	}

	return 1;
}

void BootBlockReader::commentary(char *fmt,...)
{
	va_list val;

	va_start(val,fmt);
	vprintf(fmt,val);
	va_end(val);
}
