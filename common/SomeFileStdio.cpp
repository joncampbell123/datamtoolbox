/* SomeFileStdio.cpp
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * SomeFile derived class for fopen/fclose/fseek/fread/etc...
 */

#include <stdio.h>
#include <string.h>
#include "common/SomeFileStdio.h"

int SomeFileStdio::Read(unsigned char *buf,int N)
{
	if (!fp) return 0;
	return fread(buf,1,N,fp);
}

int SomeFileStdio::Write(unsigned char *buf,int N)
{
	if (!fp || ro) return 0;
	return fwrite(buf,1,N,fp);
}

uint64 SomeFileStdio::Seek(uint64 ofs)
{
	if (!fp) return 0;
	if (ofs >= 0x7FFF0000) ofs = 0x7FFF0000;
	fseek(fp,(unsigned long)ofs,SEEK_SET);
	return (uint64)((unsigned long)ftell(fp));
}

uint64 SomeFileStdio::Tell()
{
	return (uint64)((unsigned long)ftell(fp));
}

uint64 SomeFileStdio::GetSize()
{
	unsigned long ops;
	uint64 sz;

	if (!fp) return 0;
	ops = (unsigned long)ftell(fp);
	fseek(fp,0,SEEK_END);
	sz = ftell(fp);
	fseek(fp,ops,SEEK_SET);
	return sz;
}

char* SomeFileStdio::GetName()
{
	return myname;
}

SomeFileStdio::SomeFileStdio()
{
	myname=NULL;
	fp=NULL;
}

SomeFileStdio::~SomeFileStdio()
{
	Close();
}

int SomeFileStdio::Open(char *name,int readonly)
{
	int l;

	Close();

	l = strlen(name);
	myname = new char[l+1];
	if (!myname) return -1;
	strcpy(myname,name);
	ro = readonly;

	fp=fopen(name,readonly ? "rb" : "rb+");
	if (!fp) {
		if (!readonly) fp=fopen(name,"wb");
		if (!fp) {
			Close();
			return -1;
		}
	}
	fseek(fp,0,SEEK_SET);
	return 0;
}

int SomeFileStdio::Create(char *name)
{
	int l;

	Close();

	l = strlen(name);
	myname = new char[l+1];
	if (!myname) return -1;
	strcpy(myname,name);
	ro = 0;

	fp=fopen(name,"wb");
	if (!fp) {
		Close();
		return -1;
	}
	fseek(fp,0,SEEK_SET);
	return 0;
}

int SomeFileStdio::Close()
{
	if (fp) fclose(fp);
	if (myname) delete myname;
	myname=NULL;
	fp=NULL;
	return 0;
}
