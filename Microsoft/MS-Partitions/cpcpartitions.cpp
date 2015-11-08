
/* a C++ class for reading the IBM DOS/PC partition table
 * (C) 2005 Jonathan Campbell
 *-------------------------------------------
 *
 * extra features supported:
 *
 *   NEC MS-DOS 3.30 8-partition extension
 *   (see see http://www.geocities.com/thestarman3/asm/mbr/DOS33MBR.htm for details)
 *
 */

#define CPCPARTITIONS_H__INSIDER

#include "cpcpartitions.h"
#include <string.h>
#include <stdio.h>
#include "common/Types.h"

// BEGIN CPcPartition namespace
namespace CPcPartition {
////////////////////////////////////////////////

int reader::init()
{
	if (blocksize() != 512) return 0;
	tmpsect = new unsigned char[512];
	return 1;
}

int reader::free()
{
	umount();
	if (tmpsect) delete tmpsect;
	return 1;
}

int reader::mount()
{
	int i,ofs;

	if (!tmpsect) return 0;
	if (!blockread(0,1,tmpsect)) return 0;
	if (llei16(tmpsect+510) != 0xAA55) return 0;

	/* NEC MS-DOS 3.30 8-partition support */
	/* see http://www.geocities.com/thestarman3/asm/mbr/DOS33MBR.htm for details */
	if (llei16(tmpsect+0x17C) == 0xA55A) {
		entries = 8;
		ofs = 0x17E;
	}
	else {
		entries = 4;
		ofs = 0x1BE;
	}

	for (i=0;i < entries && i < MAX_ENTRIES;i++) {
		entry[i].flags =			 tmpsect[ofs+0x0];
		entry[i].start_head =			 tmpsect[ofs+0x1];
		entry[i].start_sector =			 tmpsect[ofs+0x2] & 0x3F;
		entry[i].start_cylinder =		 tmpsect[ofs+0x3] | ((tmpsect[ofs+0x2] & 0xC0) << 2);
		entry[i].system_id =			 tmpsect[ofs+0x4];
		entry[i].end_head =			 tmpsect[ofs+0x5];
		entry[i].end_sector =			 tmpsect[ofs+0x6] & 0x3F;
		entry[i].end_cylinder =			 tmpsect[ofs+0x7] | ((tmpsect[ofs+0x6] & 0xC0) << 2);
		entry[i].start =			 llei32(tmpsect+ofs+0x8);
		entry[i].size =				 llei32(tmpsect+ofs+0xC);
		entry[i].parent_ent =			-1;
		entry[i].partition_sector =		 0;
		entry[i].partition_sector_offset =	 ofs;
		ofs += 16;
	}

	/* look for extended DOS partitions */
	for (i=0;i < entries && i < MAX_ENTRIES;i++) {
		// TODO
	}

	return 1;
}

int reader::umount()
{
	entries=0;
	return 1;
}

reader::ENTRY* reader::info(int index)
{
	if (index < 0 || index >= entries) return NULL;
	return &entry[index];
}

reader::partition* reader::get(int index)
{
	partition* x;

	if (index < 0 || index >= entries) return NULL;
	x = new partition(this);
	if (!x) return NULL;
	x->size = entry[index].size;
	x->offset = entry[index].start;
	return x;
}

reader::partition::partition(reader *parent)
{
	mom=parent;
}

reader::partition::~partition()
{
}

int reader::partition::blocksize()
{
	return mom->sectsize;
}

int reader::partition::blockread(unsigned int sector,int N,unsigned char *buffer)
{
	if (sector >= size) return 0;
	return mom->blockread(sector+offset,N,buffer);
}

////////////////////////////////////////////////
};
// END CPcPartition namespace
