/* SomeFile.h
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * Abstract classes for faked or real "files"
 */

#ifndef __SOMEFILE_H
#define __SOMEFILE_H

#include "common/Types.h"

// abstract class for reading faked or real "files"
class SomeFile {
public:
	virtual int Read(unsigned char *buf,int N) = 0;
	virtual int Write(unsigned char *buf,int N) = 0;
	virtual uint64 Seek(uint64 ofs) = 0;
	virtual uint64 Tell() = 0;
	virtual uint64 GetSize() = 0;
	virtual char* GetName() = 0;
};

#endif //__SOMEFILE_H
