/* BlockDeviceStdioFile.h
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * A BlockDevice class that uses a file as a block device.
 * No fancy translations are done for you, if you want that
 * write your own derived class.
 */

#include "common/BlockDevice.h"
#include "common/SomeFile.h"

class BlockDeviceDiskCopyImage : public BlockDevice {
public:
	virtual int ReadDisk(uint64 sector,unsigned char *buf,int N);
	virtual int WriteDisk(uint64 sector,unsigned char *buf,int N);
	virtual uint64 SizeOfDisk();
	virtual int GetBlockSize();
	virtual int SetBlockSize(unsigned int bs);
	virtual uint64 SetBase(uint64 o);
public:
	BlockDeviceDiskCopyImage();
	int					Assign(SomeFile *file);
public:
	virtual	void		commentary(char *fmt,...);
public:
	/* you can use this to read the 12 bytes of tag data in DiskCopy images */
	int					ReadTagData(uint64 sector,unsigned char *buf,int N);
	int					WriteTagData(uint64 sector,unsigned char *buf,int N);
private:
	SomeFile*			floppy;
	uint64				base;
	uint64				max;
	uint64				datasize;
	uint64				tagsize;
public:
	enum {
		diskFormat_400k=0,
		diskFormat_800k=1,
		diskFormat_720k=2,
		diskFormat_1440k=3
	};
	enum {
		formatByte_400kold=0x02,	// this occurs on DiskCopy images I have of System 3.0/earlier
		formatByte_400k=0x12,
		formatByte_400kmore=0x22,
		formatByte_800k_appleII=0x24,
	};
};
