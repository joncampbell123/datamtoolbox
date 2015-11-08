
#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Microsoft/MSDOS-EXE/msdosexe.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

void MSDOSEXE::NAME(const char *s)
{
	__func_name = s;
}

void MSDOSEXE::Assign(SomeFile *f)
{
	source = f;
}

int MSDOSEXE::Check()
{
	NAME("Check");
	unsigned char buf[32];

	if (!source) return -1;
	if (source->Seek(0) != 0) return -1;
	if (source->Read(buf,28) < 28) return -1;
	if (memcmp(buf,"MZ",2)) return -1;

	problems = 0;
	hdr.signature =						llei16(buf +  0);
	hdr.bytes_in_last_block =			llei16(buf +  2);
	hdr.blocks_in_file =				llei16(buf +  4);
	hdr.num_relocs =					llei16(buf +  6);
	hdr.header_paragraphs =				llei16(buf +  8);
	hdr.min_extra_paragraphs =			llei16(buf + 10);
	hdr.max_extra_paragraphs =			llei16(buf + 12);
	hdr.ss =							llei16(buf + 14);
	hdr.sp =							llei16(buf + 16);
	hdr.checksum =						llei16(buf + 18);
	hdr.ip =							llei16(buf + 20);
	hdr.cs =							llei16(buf + 22);
	hdr.reloc_table_offset =			llei16(buf + 24);
	hdr.overlay_number =				llei16(buf + 26);

	if (GetUsableSize() > source->GetSize()) {
		comment(0,"Usable size > file size!");
		problems |= PROBLEM_USABLE_SIZE;
	}
	if (GetResidentOffset() >= source->GetSize()) {
		comment(0,"Header size > file size!");
		problems |= PROBLEM_HEADER_SIZE;
	}

	return 0;
}

MSDOSEXE::EXEhdr* MSDOSEXE::GetHeader()
{
	return &hdr;
}

/* WARNING: Returns the offset to the Windows header, but may return an offset
            to garbage if there really is no Windows header. It is up to the
			caller to then verify that there is a PE, NE, or LE header there! */
unsigned long MSDOSEXE::GetWindowsEXEHeader()
{
	NAME("GetWindowsEXEHeader");
	unsigned char buf[4];
	unsigned long ofs;

	/* file must be big enough to have such a field */
	if (source->GetSize() < 64) {
		comment(1,"File too small to contain Windows EXE header offset");
		return 0L;
	}

	/* MS-DOS header must be at least 64 bytes large to hold DWORD value @ 60 */
	if (hdr.header_paragraphs < 4) {
		comment(1,"MS-DOS EXE header too small to contain Windows EXE header offset");
		return 0L;
	}

	/* make sure that this DWORD does not collide with the relocation table */
	CheckRelocationTable();
	if (hdr.reloc_table_offset < 64) {
		if ((hdr.reloc_table_offset + (hdr.num_relocs * 4)) > 60) {
			comment(1,"Windows EXE header offset collides with relocation table");
			return 0;
		}
	}

	if (source->Seek(60) != 60)
		return 0;
	if (source->Read(buf,4) < 4)
		return 0;

	ofs = llei32(buf);
	return ofs;
}

unsigned long MSDOSEXE::GetResidentOffset()
{
	return ((unsigned long)hdr.header_paragraphs) * 16L;
}

unsigned long MSDOSEXE::GetUsableSize()
{
	NAME("GetUsableSize");
	unsigned long sz;

	if (hdr.blocks_in_file == 0) {
		comment(0,"Blocks in file == 0, which is unusual");
		return 0;
	}

	if (hdr.bytes_in_last_block >= 512) {
		comment(0,"Bytes in last block >= 512, which is unusual");
		return 0;
	}

// FIXME: is this right? taken from DJGPP documentation
	sz = hdr.blocks_in_file * 512;
	if (hdr.bytes_in_last_block != 0) {
		sz -= 512;
		sz += hdr.bytes_in_last_block;
	}

	return sz;
}

unsigned long MSDOSEXE::GetResidentSize()
{
	unsigned long sz=GetUsableSize(),hsz=GetResidentOffset();
	if (sz >= hsz)	sz -= hsz;
	else			sz  = 0;
	return sz;
}

unsigned long MSDOSEXE::GetResidentTotalSize()
{
	unsigned long sz=GetUsableSize(),asz=((unsigned long)hdr.min_extra_paragraphs)*16L;
	return sz+asz;
}

void MSDOSEXE::CheckRelocationTable()
{
	NAME("CheckRelocationTable");
	int max = GetResidentOffset();

	if (hdr.num_relocs != 0 && hdr.reloc_table_offset == 0) {
		comment(0,"Relocation table ignored, num_relocs != 0 but offset == 0");
		hdr.reloc_table_offset = 0;
		hdr.num_relocs = 0;
	}
	else if (hdr.num_relocs != 0 && hdr.reloc_table_offset < 28) {
		comment(0,"Relocation table ignored, num_relocs != 0 but offset invalid");
		hdr.reloc_table_offset = 0;
		hdr.num_relocs = 0;
	}

	if (max >= hdr.reloc_table_offset)	max -= hdr.reloc_table_offset;
	else								max  = 0;

	max >>= 2;
	if (hdr.num_relocs > max) {
		hdr.num_relocs = max;
		comment(0,"Relocation table cut short because it extends past EXE header");
	}
}

unsigned long MSDOSEXE::GetIPoffset()
{
	NAME("GetIPoffset");
	unsigned long o;

	o = ((hdr.cs << 4) + hdr.ip) & 0xFFFFF;
	o += GetResidentOffset();

	/* check to make sure that it's within bounds */
	if (o >= GetResidentSize()) {
		if (o >= GetResidentTotalSize())
			comment(1,"Initial instruction pointer points outside entire image");
		else
			comment(1,"Initial instruction pointer points into uninitialized RAM");

		return -1;
	}

	return o;
}

unsigned long MSDOSEXE::GetSPoffset()
{
	unsigned long o;

	o = ((hdr.ss << 4) + hdr.sp) & 0xFFFFF;
	o += GetResidentOffset();

	/* check to make sure that it's within bounds.
	   NOTE: It is legal to have SS:SP point beyond the image into uninitialized
	         RAM given by min_extra_paragraphs != 0 */
	if (o >= GetResidentSize()) {
		if (o >= GetResidentTotalSize())
			comment(1,"Initial stack pointer points outside entire image");
		else
			// this condition is legal
			comment(0,"Initial stack pointer points into uninitialized RAM");

		return -1;
	}

	return o;
}

void MSDOSEXE::comment(char isErr,char *str,...)
{
	va_list va;

	va_start(va,str);
	printf("%sMSDOSEXE::%s> ",isErr ? "ERROR: " : "WARNING: ",__func_name);
	vprintf(str,va);
	printf("\n");
	va_end(va);
}
