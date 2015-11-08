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

class BlockDeviceFile : public BlockDevice {
public:
	virtual int ReadDisk(uint64 sector,unsigned char *buf,int N);
	virtual int WriteDisk(uint64 sector,unsigned char *buf,int N);
	virtual uint64 SizeOfDisk();
	virtual int GetBlockSize();
	virtual int SetBlockSize(unsigned int bs);
	virtual uint64 SetBase(uint64 o);
public:
	BlockDeviceFile();
	int					Assign(SomeFile *file);
private:
	SomeFile*			floppy;
	unsigned int		blocksize;
	uint64				base;
	uint64				max;
};
