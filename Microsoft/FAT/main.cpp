// NOTE: there is additional code that allows the unicode LFNs to
//       be preserved on Windows systems.

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include "cdosfat.h"

#include "common/BlockDeviceFile.h"
#include "common/SomeFileStdio.h"

class vfatreader : public CDOSFat::reader
{
public:
	vfatreader(char *fn);
	~vfatreader();
public:
	virtual int				blocksize();
	virtual int				blockread(unsigned int sector,int N,unsigned char *buffer);
	virtual uint64			disksize();
	virtual void			comment(int level,char *str,...);
	int						ok();
public:
	SomeFileStdio*			file;
	BlockDeviceFile*		blkf;
};

void vfatreader::comment(int level,char *str,...)
{
	va_list va;

	if (level < 0)		level = 0;
	else if (level > 6)	level = 6;
	printf("VFATREADER[%s]: ",CDOSFat::levs[level]);
	va_start(va,str);
	vprintf(str,va);
	va_end(va);
	printf("\n");
}

int vfatreader::ok()
{
	if (file && blkf) return 1;
	return 0;
}

int vfatreader::blocksize()
{
	return 512;
}

int vfatreader::blockread(unsigned int sector,int N,unsigned char *buffer)
{
	return blkf->ReadDisk(sector,buffer,N);
}

uint64 vfatreader::disksize()
{
	return blkf->SizeOfDisk();
}

vfatreader::vfatreader(char *fn)
{
	file=new SomeFileStdio;
	blkf=new BlockDeviceFile;
	file->Open(fn);
	blkf->Assign(file);
	if (blkf->SetBlockSize(512) < 0) {
		delete blkf;
		delete file;
	}
}

vfatreader::~vfatreader()
{
	delete blkf;
	delete file;
}

void wcscat_from_ansi(unsigned short *str,char *c)
{
	while (*str != 0) str++;
	while (*c != 0) *str++ = (unsigned short)(*c++);
	*str++ = 0;
}

void StripOne(unsigned short *x)
{
	unsigned short *f=x;

	while (*x != 0) x++;
	if (*x == 0) x--;
	if (*x == '\\') *x-- = 0;
	while (x >= f && *x != '\\') *x-- = 0;
}

int SacredDotDot(char *x)
{
	if (x[0] == '.') {
		if (x[1] != '.') return 0;
		return 1;
	}

	return 0;
}

void CreateDirs(unsigned short *x)
{
	unsigned short path[1000],*t;

	wcscpy(path,L"\\\\?\\");
	GetCurrentDirectoryW(1000,path+4);
	t = wcslen(path) + path + -1;
	if (*t != '\\') wcscat(path,L"\\");
	wcscat(path,L"__RIPPED__");
	CreateDirectoryW(path,NULL);
	wcscat(path,L"\\");

	t = path + wcslen(path);
	while (*x != 0) {
		while (!(*x == 0 || *x == '\\')) *t++ = *x++;
		*t = 0;

		CreateDirectoryW(path,NULL);
		if (*x == '\\') *t++ = *x++;
	}
}

void RipFile(unsigned short *x,unsigned short *nameu,char *namea,CDOSFat::reader::entity *read,CDOSFat::reader::entity::FATDIRENTRY *fat)
{
	unsigned short path[1000],*t;
	HANDLE han;

	wcscpy(path,L"\\\\?\\");
	GetCurrentDirectoryW(1000,path+4);
	t = wcslen(path) + path + -1;
	if (*t != '\\') wcscat(path,L"\\");
	wcscat(path,L"__RIPPED__");
	wcscat(path,L"\\");
	wcscat(path,x);
	if (nameu)	wcscat(path,nameu);
	else		wcscat_from_ansi(path,namea);

	read->seek(0);
	han = CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (han != INVALID_HANDLE_VALUE) {
		unsigned char buffer[4096];
		uint64 counter=0;
		DWORD dw;
		int l;

		while ((l=read->read(buffer,4096)) > 0) {
			counter += (uint64)l;
			WriteFile(han,buffer,l,&dw,NULL);
		}

		if (counter < fat->size)
			printf("Warning: Less was ripped than announced! Got %u, expected %u!\n",(unsigned long)counter,fat->size);
		else if (counter > fat->size)
			printf("Warning: More was ripped than announced! Got %u, expected %u!\n",(unsigned long)counter,fat->size);

		CloseHandle(han);
	}
	else {
		printf("Failed extraction!\n");
	}
}

int main(int argc,char **argv)
{
	CDOSFat::reader::entity* dirz[128],*s;
	CDOSFat::reader::entity::FATDIRENTRY f;
	unsigned short cwd_path[1000];
	char spc[129];
	vfatreader* r;
	int i,dirzp;

	if (argc < 2) {
		printf("%s <disk image>\n");
		return 1;
	}

	r = new vfatreader(argv[1]);
	if (!r->ok()) return 1;
	if (!r->init()) return 1;
	if (!r->mount()) return 1;

	dirzp = 0;
	dirz[dirzp] = r->rootdirectory();
	if (!dirz[dirzp]) return 1;
	spc[dirzp] = 0;

	cwd_path[0] = 0;
	CreateDirs(cwd_path);
	i = dirz[dirzp]->dir_findfirst(&f);
	while (i) {
		if (f.attr & CDOSFat::reader::entity::ATTR_VOLUMELABEL)
			printf("%s[VOLUME] %s, size %u starting cluster %u\n",spc,f.fullname,f.size,f.cluster);
		else if (f.attr & CDOSFat::reader::entity::ATTR_DIRECTORY)
			printf("%s[ DIR  ] %s, size %u starting cluster %u\n",spc,f.fullname,f.size,f.cluster);
		else
			printf("%s[ FILE ] %s, size %u starting cluster %u\n",spc,f.fullname,f.size,f.cluster);

		if (!(f.attr & CDOSFat::reader::entity::ATTR_VOLUMELABEL)) {
			CDOSFat::reader::entity::LFN* l;
			unsigned short *wcname=NULL;
			char *acname=NULL;

			acname = (char*)f.fullname;
			l = dirz[dirzp]->long_filename(&f);
			if (l) {
				wcname = l->name_unicode;
				if (l->unicode2ansi()) printf("%s   LFN-> %s\n",spc,l->name_ansi);
			}

			if (!(f.attr & CDOSFat::reader::entity::ATTR_DIRECTORY)) {
				if (CDOSFat::reader::entity* fil = dirz[dirzp]->getdirent(&f)) {
					RipFile(cwd_path,wcname,acname,fil,&f);
					delete fil;
				}
				else {
					printf("Failed to get object!\n");
				}
			}
			else if (!SacredDotDot(acname)) {
				// add to the path
				if (wcname)		wcscat(cwd_path,wcname);
				else			wcscat_from_ansi(cwd_path,acname);
				wcscat(cwd_path,L"\\");

				// create the folder
				CreateDirs(cwd_path);
			}

			if (l) delete l;
		}

		if (f.attr & CDOSFat::reader::entity::ATTR_DIRECTORY && dirzp < 126) {
			if (!SacredDotDot((char*)f.name)) {
				s = dirz[dirzp]->getdirent(&f);
				if (s) {
					spc[dirzp] = ' ';
					dirz[++dirzp] = s;
					spc[dirzp] = 0;
					i = dirz[dirzp]->dir_findfirst(&f);
				}
			}
		}

		i = dirz[dirzp]->dir_findnext(&f);
		while (!i && dirzp >= 0) {
			StripOne(cwd_path);
			spc[dirzp] = 0;
			delete dirz[dirzp--];
			spc[dirzp] = 0;
			if (dirzp >= 0) i = dirz[dirzp]->dir_findnext(&f);
		}
	}

	r->umount();
	r->free();
	delete r;
	return 0;
}
