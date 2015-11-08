
/* test program for CMSOLEFS classes */

#ifdef WIN32
#include <windows.h>
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef WIN32
#include <conio.h>
#endif
#include <string.h>
#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Microsoft/MS-OLE/cmsolefs.h"

#ifdef LINUX
unsigned long long lseek64(int fd,unsigned long long off,int whence);
#endif

class msolereader : public CMSOLEFS::reader
{
public:
	virtual int					blocksize();
	virtual int					blockread(unsigned int sector,int N,unsigned char *buffer);
	virtual unsigned long		disksize();
	virtual void				comment(int level,char *str,...);
public:
	unsigned long				bsize;
	SomeFile*					sf;
};

static char *levs[] = {
	"PANIC   ",	"CRITICAL",	"ERROR   ",	"WARNING ",
	"INFO    ",	"DEBUG   ",	"        ",
};

void msolereader::comment(int level,char *str,...)
{
	va_list va;

	if (level < 0)		level = 0;
	else if (level > 6)	level = 6;
	printf("msolereader[%s]: ",levs[level]);
	va_start(va,str);
	vprintf(str,va);
	va_end(va);
	printf("\n");
}

int msolereader::blocksize()
{
	return 512;
}

int msolereader::blockread(unsigned int sector,int N,unsigned char *buffer)
{
	uint64 o;
	int r;

	o = ((uint64)sector) * ((uint64)512);
	if (sf->Seek(o) != o) return 0;
	r = sf->Read(buffer,N*512);
	if (r < 0)	r = 0;
	else		r >>= 9;
	return r;
}

unsigned long msolereader::disksize()
{
	return sf->GetSize() >> 9;
}

int main(int argc,char **argv)
{
	msolereader::entity::DIRENTRY f;
	msolereader::entity* stk[64],*xx;
	SomeFileStdio sfs;
	msolereader r;
	int stksp=0;
	int x;

	if (argc < 2) {
		printf("%s <OLE container file>\n");
		return 1;
	}

	if (sfs.Open(argv[1]) < 0) {
		printf("Cannot open %s\n");
		return 1;
	}

	r.sf = &sfs;
	if (!r.init()) return 1;
	if (!r.mount()) return 1;

	stksp = 0;
	stk[stksp] = r.rootdirectory();
	if (!stk[stksp]) return 1;

	x = stk[stksp]->dir_findfirst(&f);
	while (x) {
		for (x=0;x < stksp;x++) printf(" ");
		if (f.attr & CMSOLEFS::reader::entity::ATTR_SUBDIRECTORY)
			printf("[DIR ] %s, starts at block %u, size %u\n",f.ps_name,f.ps_start,f.ps_size);
		else
			printf("[FILE] %s, starts at block %u, size %u\n",f.ps_name,f.ps_start,f.ps_size);

		x=0;
		if (f.attr & CMSOLEFS::reader::entity::ATTR_SUBDIRECTORY && stksp < 63) {
			xx = stk[stksp]->getdirent(&f);
			if (xx) {
				stk[++stksp] = xx;
				x = stk[stksp]->dir_findfirst(&f);
			}
		}

		if (!x) x = stk[stksp]->dir_findnext(&f);
		while (!x && stksp >= 0) {
			delete stk[stksp];
			stk[stksp]=NULL;
			stksp--;
			if (stksp >= 0) x = stk[stksp]->dir_findnext(&f);
		}

//		Sleep(500);
	}

	r.umount();
	r.free();
	return 0;
}
