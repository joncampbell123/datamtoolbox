
#ifndef __SOMEFILESTDIO_H
#define __SOMEFILESTDIO_H

/* SomeFileStdio.h
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * Derived class of SomeFile for use with
 * fopen/fclose/fseek/fread/etc....
 */

#include <stdio.h>
#include "common/SomeFile.h"

// abstract class for reading faked or real "files"
class SomeFileStdio : public SomeFile {
public:
	virtual int Read(unsigned char *buf,int N);
	virtual int Write(unsigned char *buf,int N);
	virtual uint64 Seek(uint64 ofs);
	virtual uint64 Tell();
	virtual uint64 GetSize();
	virtual char* GetName();
public:
	SomeFileStdio();
	virtual ~SomeFileStdio();
public:
	int Open(char *name,int readonly=1);
	int Create(char *name);
	int Close();
private:
	char *myname;
	FILE *fp;
	char ro;
};

#endif //__SOMEFILESTDIO_H
