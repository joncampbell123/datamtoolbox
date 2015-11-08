/* BlockDeviceDiskCopyImage.cpp
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * A block device object that uses an Apple
 * DiskCopy image as a source of data
 */
// TODO: provide a function that recalculates the CRC!

#include <stdio.h>
#include <stdarg.h>
#include "Apple/common/Apple-Macintosh-Common.h"
#include "Apple/Image-DiskCopy/BlockDeviceDiskCopyImage.h"

int BlockDeviceDiskCopyImage::ReadDisk(uint64 sector,unsigned char *buf,int N)
{
	uint64 ofs = ((uint64)sector) * ((uint64)512) + base;
	if (!floppy) return 0;
	if (sector >= max) return 0;
	if (floppy->Seek(ofs) != ofs) return 0;
	N = floppy->Read(buf,N * 512) / 512;
	if (N < 0) N = 0;
	return N;
}

int BlockDeviceDiskCopyImage::WriteDisk(uint64 sector,unsigned char *buf,int N)
{
	// TODO: reimplement
	return 0;
}

uint64 BlockDeviceDiskCopyImage::SizeOfDisk()
{
	return max;
}

int BlockDeviceDiskCopyImage::GetBlockSize()
{
	return 512;
}

int BlockDeviceDiskCopyImage::SetBlockSize(unsigned int bs)
{
	if (bs == 512) return 0;
	return -1;
}

BlockDeviceDiskCopyImage::BlockDeviceDiskCopyImage()
{
	floppy=NULL;
	max=0;
}

int BlockDeviceDiskCopyImage::Assign(SomeFile *file)
{
	unsigned char header[84];
	unsigned char buffer[512];
	char diskName[65];
	aLongInt dataSize,tagSize,dataChecksum,tagChecksum,checksum,data;
	unsigned char diskFormat,formatByte;
	aInteger privatee;
	int i,j;

	if (file->GetSize() < 84)
		return -1;
	if (file->Seek(0) != 0)
		return -1;
	if (file->Read(header,84) < 84)
		return -1;

	commentary("Apple DiskCopy image analysis by BlockDeviceDiskCopyImage::Assign\n");

	PascalToCString(diskName,(aStr255*)header,sizeof(diskName));
	commentary("....disk name is %s\n",diskName);

	base = 84;

	dataSize = lbei32(header + 64);
	commentary("....user data: %u bytes\n",dataSize);
	tagSize = lbei32(header + 68);
	commentary("....tag data: %u bytes\n",tagSize);
	dataChecksum = lbei32(header + 72);
	tagChecksum = lbei32(header + 76);
	diskFormat = header[80];
	formatByte = header[81];
	privatee = lbei16(header + 82);

	if ((dataSize + tagSize + base) > file->GetSize()) {
		commentary("Disk image may be corrupt! Data size + tag size + header > file size!\n",tagSize);
		return -1;
	}

	if (privatee != 0x0100) {
		commentary("Disk image may be corrupt or wrong version! Private value is 0x%04X not 0x0100\n",privatee);
		return -1;
	}

	commentary("....Disk format is:\n");
	if (diskFormat == diskFormat_400k)			commentary("........400K\n");
	else if (diskFormat == diskFormat_800k)		commentary("........800K\n");
	else if (diskFormat == diskFormat_720k)		commentary("........720K\n");
	else if (diskFormat == diskFormat_1440k)	commentary("........1440K\n");
	else										commentary("........unknown 0x%02X\n",diskFormat);

	commentary("....format byte is:\n");
	if (formatByte == formatByte_400kold)			commentary("........400K (older format)\n");
	else if (formatByte == formatByte_400k)			commentary("........400K\n");
	else if (formatByte == formatByte_400kmore)		commentary("........larger than 400K\n");
	else if (formatByte == formatByte_800k_appleII)	commentary("........800K Apple II\n");
	else											commentary("........unknown 0x%02X\n",formatByte);

	max = dataSize / 512;
	floppy = file;

	/* compute checksum over data */
	/* as documented on the internet (http://web.pdx.edu/~heiss/technotes/ftyp/ftn.e0.0005.html) */
	checksum = 0;
	for (i=0;i < max;i++) {
		if (ReadDisk(i,buffer,1) < 1) {
			commentary("Cannot read sector %u!\n",i);
			floppy = NULL;
			return -1;
		}

		for (j=0;j < 512;j += 2) {
			data = lbei16(buffer + j);
			checksum += data;
			checksum = (checksum >> 1) | (checksum << 31);
		}
	}

	if (checksum != dataChecksum) {
		commentary("Disk image may be corrupt! Got checksum 0x%08X which does not match 0x%08X\n",checksum,dataChecksum);
		return -1;
	}

	datasize = dataSize;
	tagsize = tagSize;
	return 0;
}

uint64 BlockDeviceDiskCopyImage::SetBase(uint64 o)
{
	return -1;
}

void BlockDeviceDiskCopyImage::commentary(char *fmt,...)
{
	va_list val;

	va_start(val,fmt);
	vprintf(fmt,val);
	va_end(val);
}

// untested!
int BlockDeviceDiskCopyImage::ReadTagData(uint64 sector,unsigned char *buf,int N)
{
	uint64 ofs = ((uint64)sector) * ((uint64)512) + base + datasize;
	if (!floppy) return 0;
	if (sector >= (tagsize/12)) return 0;
	if (floppy->Seek(ofs) != ofs) return 0;
	N = floppy->Read(buf,N * 12) / 12;
	if (N < 0) N = 0;
	return N;
}

int BlockDeviceDiskCopyImage::WriteTagData(uint64 sector,unsigned char *buf,int N)
{
	// TODO: reimplement
	return 0;
}
