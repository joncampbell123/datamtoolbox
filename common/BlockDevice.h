/* BlockDevice.h
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * Abstract classes for faked or real "block devices"
 */

#ifndef __COMMON_BLOCKDEVICE_H
#define __COMMON_BLOCKDEVICE_H

#include "common/Types.h"

// abstract class for reading faked or real "block devices"
class BlockDevice {
public:
	virtual int ReadDisk(uint64 sector,unsigned char *buf,int N) = 0;
	virtual int WriteDisk(uint64 sector,unsigned char *buf,int N) = 0;
	virtual uint64 SizeOfDisk() = 0;
	virtual int GetBlockSize() = 0;
	virtual int SetBlockSize(unsigned int bs) = 0;
	virtual uint64 SetBase(uint64 bs) = 0;
};

#endif //__COMMON_BLOCKDEVICE_H
