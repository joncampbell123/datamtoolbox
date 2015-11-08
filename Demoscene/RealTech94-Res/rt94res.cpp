
/* (C) 1999, 2005 Jonathan Campbell */

/* Designed to read resource items in DIM.RES for RealTech 94 demo.
   It can also handle MEGAMIX.RES from the Megamix 94 demo. */

#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Demoscene/RealTech94-Res/rt94res.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>

RealTech94Resources::RealTech94Resources()
{
	offsets = NULL;
	f = NULL;
}

RealTech94Resources::~RealTech94Resources()
{
	if (offsets) delete offsets;
}

int RealTech94Resources::GetItem(int i,ITEM *item)
{
	unsigned char buf[17];
	unsigned long o;

	if (!f) return -1;
	if (!item) return -1;
	if (i < 0 || i >= numItems) return -1;
	o = (i + 1) * 17;
	if (f->Seek(o) != o) return -1;
	if (f->Read(buf,17) < 17) return -1;

	memcpy(item->name,buf,13);
	item->size = llei32(buf+13);
	item->offset = offsets[i];
	return 0;
}

void RealTech94Resources::Assign(SomeFile *x)
{
	f = x;
}

static char *MegaMix94res_sig = "REALTECH94=F";
static char *RealTech94res_sig = "REALTECH94=Z";
void RealTech94Resources::comment(char *fmt,...)
{
	va_list va;

	va_start(va,fmt);
	printf("[RealTech94Resources]: ");
	vprintf(fmt,va);
	printf("\n");
	va_end(va);
}

int RealTech94Resources::ValidName(char *x)
{
	while (*x != 0) {
		if (!isalpha(*x) && !isdigit(*x) && *x != '.' && *x != '_')
			return 0;

		x++;
	}

	return 1;
}

int RealTech94Resources::Check()
{
	unsigned char buf[17];
	unsigned long offset,fsize;
	ITEM item;
	int i,adjust;

/* the DIM.RES file format is very simple.

   at the beginning of the file there is an array of 17-byte long entries.
   13 bytes are the file name (NULL terminated) and 4 bytes are a DWORD
   specifying the size of the file. you find the offset of a file by
   starting from 0 and adding the sizes of the previous files.

   the first entry is always REALTECH94=Z and it specifies the size of
   the list.
	
   For MegaMix, the first entry is REALTECH94=F and the size field is one
   too large. */

	if (!f) return -1;
	if (f->GetSize() >= 0x30000000) return -1;
	fsize=(unsigned long)f->GetSize();
	if (f->Seek(0) != 0) return -1;
	if (f->Read(buf,17) < 17) return -1;

	if (!memcmp(buf,RealTech94res_sig,12))
		adjust = 0;
	else if (!memcmp(buf,MegaMix94res_sig,12))
		adjust = -1;
	else
		return -1;

	if (buf[12] != 0) return -1;
	offset = first_offset = llei32(buf+13) + adjust;
	if (first_offset < 17) return -1;
	if (first_offset >= fsize) return -1;

	numItems = (first_offset - 17) / 17;
	if ((first_offset % 17) != 0)
		comment("Initial offset value is unusual. %u %% 17 != 0",first_offset);

	if (offsets) delete offsets;
	offsets = new unsigned long[numItems];

	for (i=0;i < numItems;i++) {
		if (GetItem(i,&item) < 0) {
			numItems = i;
			comment("ERROR failed to get item %u",i);
			return -1;
		}

		if (item.name[12] != 0) {
			numItems = i;
			comment("Missing ASCIIZ terminator for item %u, stopping search",i);
		}

		if (!ValidName(item.name)) {
			numItems = i;
			comment("Name for item %u has bad characters, stopping search",i);
		}

		offsets[i] = offset;
		offset += item.size;
		if (offset > fsize) {
			numItems = i;
			comment("item %u has size that extends beyond end of file by %u bytes, stopping search",i,offset - fsize);
		}
	}

	if (offset < fsize)
		comment("there are %u extra bytes at the end of this file",fsize-offset);

	return 0;
}

RealTech94Resources::file* RealTech94Resources::GetFile(ITEM *item)
{
	RealTech94Resources::file* f;

	f = new RealTech94Resources::file;
	f->parent = this;
	f->offset = 0;
	memcpy(&f->item,item,sizeof(ITEM));
	return f;
}

int RealTech94Resources::file::Read(unsigned char *buf,int N)
{
	uint64 ofs;
	int sz,r;

	if (!parent->f) return 0;
	if (offset >= item.size) return 0;
	sz = item.size - ((int)offset);
	if (N > sz) N = sz;
	ofs = offset + item.offset;
	if (parent->f->Seek(ofs) != ofs) return 0;
	r = parent->f->Read(buf,N);
	offset += r;
	return r;
}

int RealTech94Resources::file::Write(unsigned char *buf,int N)
{
	return 0;
}

uint64 RealTech94Resources::file::Seek(uint64 ofs)
{
	if (ofs > item.size) ofs = item.size;
	offset = ofs;
	return ofs;
}

uint64 RealTech94Resources::file::Tell()
{
	return offset;
}

uint64 RealTech94Resources::file::GetSize()
{
	return item.size;
}

char* RealTech94Resources::file::GetName()
{
	return NULL;
}

int RealTech94Resources::FindItem(ITEM *item,char *name)
{
	int x;

	for (x=0;x < numItems;x++) {
		if (GetItem(x,item) >= 0) {
			if (!strcmpi(name,(char*)item->name)) return 0;
		}
	}

	return -1;
}
