
#include <stdio.h>
#include <direct.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include "cext2.h"

#include "common/BlockDeviceFile.h"
#include "common/SomeFileStdio.h"

class ext2reader : public CEXT2::reader
{
public:
	ext2reader(char *fn);
	~ext2reader();
public:
	virtual int				blocksize();
	virtual int				blockread(uint64 sector,int N,unsigned char *buffer);
	virtual uint64			disksize();
	virtual void			comment(int level,char *str,...);
	int						ok();
public:
	uint64					partition_offset;
	SomeFileStdio*			file;
	BlockDeviceFile*		blkf;
};

void ext2reader::comment(int level,char *str,...)
{
	va_list va;

	if (level < 0)		level = 0;
	else if (level > 6)	level = 6;
	printf("ext2reader[%s]: ",CEXT2::levs[level]);
	va_start(va,str);
	vprintf(str,va);
	va_end(va);
	printf("\n");
}

int ext2reader::ok()
{
	if (file && blkf) return 1;
	return 0;
}

int ext2reader::blocksize()
{
	return 512;
}

int ext2reader::blockread(uint64 sector,int N,unsigned char *buffer)
{
	sector += partition_offset;
	return blkf->ReadDisk(sector,buffer,N);
}

uint64 ext2reader::disksize()
{
	uint64 sz=blkf->SizeOfDisk();
	if (sz > partition_offset)	sz -= partition_offset;
	else						sz  = 0;
	return sz;
}

ext2reader::ext2reader(char *fn)
{
	partition_offset=0;
	file=new SomeFileStdio;
	blkf=new BlockDeviceFile;
	file->Open(fn);
	blkf->Assign(file);
	if (blkf->SetBlockSize(512) < 0) {
		delete blkf;
		delete file;
	}
}

ext2reader::~ext2reader()
{
	delete blkf;
	delete file;
}

static int IsDotDot(char *x)
{
	if (x[0] == '.' && x[1] == 0)
		return 1;
	if (x[0] == '.' && x[1] == '.' && x[2] == 0)
		return 1;

	return 0;
}

static int indent=0;
void List(ext2reader::readdir* root,ext2reader* r)
{
	int i;

	indent++;
	while (root->read()) {
		CEXT2::reader::inode inod;
		uint64 file_size;

		memset(&inod,0,sizeof(inod));
		r->GetInode(root->cur_dirent.inode,&inod);

		for (i=1;i < indent;i++) printf("    ");
		// print "ls" style file modes
		{
			char typ;
			unsigned int mod = inod.i_mode & 0777,ftyp;
			int i;

			file_size = r->GetFileSize(&inod);
			ftyp = inod.i_mode & EXT2__S_IFMT;
			switch (ftyp) {
				case EXT2__S_IFDIR:			typ = 'd'; break;
				case EXT2__S_IFCHR:			typ = 'c'; break;
				case EXT2__S_IFBLK:			typ = 'b'; break;
				case EXT2__S_IFREG:			typ = '-'; break;
				case EXT2__S_IFIFO:			typ = 'f'; break;
				case EXT2__S_IFLNK:			typ = 'l'; break;
				case EXT2__S_IFSOCK:		typ = 's'; break;
				default:					typ = '?'; break;
			};

			printf("%c",typ);
			for (i=0;i < 3;i++) {
				unsigned char smod = (mod >> ((2-i)*3)) & 7;
				printf("%c%c%c",
					(smod & 4) ? 'r' : '-',
					(smod & 2) ? 'w' : '-',
					(smod & 1) ? 'x' : '-');
			}
		}
		printf(" %8u %8I64u ",root->cur_dirent.inode,file_size);
		printf("%s",root->cur_dirent.name);
		printf("\n");

		if ((inod.i_mode & EXT2__S_IFMT) == EXT2__S_IFDIR) {
			if (root->cur_dirent.inode > r->e2fs_first_inode &&
				!IsDotDot(root->cur_dirent.name)) {
				CEXT2::reader::readdir *rd = new CEXT2::reader::readdir(r);
				if (rd) {
					if (rd->get_inode(root->cur_dirent.inode)) {
						printf("-----------------------------------------\n");
						mkdir(root->cur_dirent.name);
						if (chdir(root->cur_dirent.name) < 0) return;
						List(rd,r);
						if (chdir("..") < 0) return;
						printf("\n");
					}

					delete rd;
				}
			}
		}
		else {
			if (root->cur_dirent.inode > r->e2fs_first_inode &&
				!IsDotDot(root->cur_dirent.name)) {
				CEXT2::reader::file *fil = new CEXT2::reader::file(r);
				if (fil) {
					if (fil->get_inode(root->cur_dirent.inode)) {
						FILE *fp = fopen(root->cur_dirent.name,"wb");
						if (fp) {
							unsigned char buffy[4096];
							int sz;

							while ((sz=fil->read(buffy,sizeof(buffy))) > 0)
								fwrite(buffy,sz,1,fp);

							fclose(fp);
						}
					}

					delete fil;
				}
			}
		}
	}
	indent--;
}

int main(int argc,char **argv)
{
	ext2reader* r;
	ext2reader::readdir* root;

	if (argc < 2) {
		printf("%s <disk image>\n");
		return 1;
	}

	r = new ext2reader(argv[1]);
	if (!r->ok()) return 1;
	if (!r->init()) return 1;
	if (!r->mount()) return 1;
	if ((root=r->GetRoot()) == NULL) return 1;

	mkdir("__RIPPED__");
	if (chdir("__RIPPED__") < 0) return 1;

	List(root,r);

	if (chdir("..") < 0) return 1;

	r->umount();
	r->freeMe();
	delete r;
	return 0;
}
