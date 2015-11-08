/* BlockDeviceFile.cpp
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * A block device object that uses a file object
 * as the source of data.
 */

#include <stdio.h>
#include "Apple/common/Apple-Macintosh-Common.h"
#include "common/BlockDeviceFile.h"

int BlockDeviceFile::ReadDisk(uint64 sector,unsigned char *buf,int N)
{
	uint64 ofs = ((uint64)sector) * ((uint64)blocksize) + ((uint64)base);
	if (!floppy) return 0;
	if (sector >= max) return 0;
	if (floppy->Seek(ofs) != ofs) return 0;
	N = floppy->Read(buf,N * blocksize) / blocksize;
	if (N < 0) N = 0;
	return N;
}

int BlockDeviceFile::WriteDisk(uint64 sector,unsigned char *buf,int N)
{
	uint64 ofs = ((uint64)sector) * ((uint64)blocksize) + ((uint64)base);
	if (!floppy) return 0;
	if (sector >= max) return 0;
	if (floppy->Seek(ofs) != ofs) return 0;
	N = floppy->Write(buf,N * blocksize) / blocksize;
	if (N < 0) N = 0;
	return N;
}

uint64 BlockDeviceFile::SizeOfDisk()
{
	return max;
}

int BlockDeviceFile::GetBlockSize()
{
	return blocksize;
}

int BlockDeviceFile::SetBlockSize(unsigned int bs)
{
	if (!floppy) return -1;
	if (!bs)	blocksize = 512;
	else		blocksize = bs;
	max=floppy->GetSize() / blocksize;
	return 0;
}

BlockDeviceFile::BlockDeviceFile()
{
	blocksize=512;
	floppy=NULL;
	base=0;
	max=0;
}

int BlockDeviceFile::Assign(SomeFile *file)
{
	floppy = file;
	max = file->GetSize() / blocksize;
	return 0;
}

uint64 BlockDeviceFile::SetBase(uint64 o)
{
	base=o;
	max=floppy->GetSize();
	if (base > max) base = max;
	max /= (uint64)blocksize;
	return base;
}
