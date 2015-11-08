
#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Other/MCO/cmcofs.h"
#include <string.h>
#include <stdarg.h>

#ifdef LINUX
unsigned long long lseek64(int fd,unsigned long long off,int whence);
#endif

class mcoreader : public CMCOFS::reader
{
public:
	virtual int					blocksize();
	virtual int					blockread(unsigned int sector,int N,unsigned char *buffer);
	virtual unsigned long		disksize();
	virtual void				comment(int level,char *str,...);
	int							ok();
public:
	unsigned long				bsize;
	int							fd;
	SomeFile*					sf;
};

static char *levs[] = {
	"PANIC   ",	"CRITICAL",	"ERROR   ",	"WARNING ",
	"INFO    ",	"DEBUG   ",	"        ",
};
void mcoreader::comment(int level,char *str,...)
{
	va_list va;

	if (level < 0)		level = 0;
	else if (level > 6)	level = 6;
	printf("mcoreader[%s]: ",levs[level]);
	va_start(va,str);
	vprintf(str,va);
	va_end(va);
	printf("\n");
}

int mcoreader::ok()
{
	if (fd >= 0) return 1;
	return 0;
}

int mcoreader::blocksize()
{
	return 512;
}

int mcoreader::blockread(unsigned int sector,int N,unsigned char *buffer)
{
	return 0;
}

unsigned long mcoreader::disksize()
{
	return bsize;
}

int main(int argc,char **argv)
{
	mcoreader::entity::DIRENTRY f;
	mcoreader::entity* root;
	SomeFileStdio sfs;
	mcoreader r;
	int x;

	if (argc < 2) {
		printf("%s <disk image>\n");
		return 1;
	}

	if (sfs.Open(argv[1]) < 0) {
		printf("Cannot open %s\n",argv[1]);
		return 1;
	}

	r.sf = &sfs;
	if (!r.init()) return 1;
	if (!r.mount()) return 1;

	root = r.rootdirectory();
	if (!root) return 1;

	x = root->dir_findfirst(&f);
	while (x) {
		if (f.attr & CMCOFS::reader::entity::ATTR_SUBDIRECTORY)
			printf("[DIR ] %s, offset %lu, size %lu\n",f.name,f.pointer,f.length);
		else
			printf("[FILE] %s, offset %lu, size %lu\n",f.name,f.pointer,f.length);

		x = root->dir_findnext(&f);
	}

	r.umount();
	r.free();
	return 0;
}
