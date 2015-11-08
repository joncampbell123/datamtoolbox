
#include <stdio.h>
#include <string.h>
#include "common/Types.h"
#include "common/SomeFile.h"

class FutureCrewEXEDirReader {
public:
	typedef struct ITEM {
		unsigned char		name[16+1];
		unsigned long		offset,size;
	};
public:
	class file : public SomeFile {
		public:
			int Read(unsigned char *buf,int N);
			int Write(unsigned char *buf,int N);
			uint64 Seek(uint64 ofs);
			uint64 Tell();
			uint64 GetSize();
			char* GetName();
		public:
			FutureCrewEXEDirReader* parent;
			uint64 offset;
			ITEM item;
	};
public:
	FutureCrewEXEDirReader();
	void Assign(SomeFile *f);
	int Check();
	int IsEXE();
	int IsFCProduct();
	int ReadItem(int x,ITEM *i);
	int FindItem(ITEM *i,char *name);
	file *GetFile(ITEM *i);
public:
	unsigned int	numFiles;
	uint32			dirOffset;
public:
	SomeFile*		sf;
	unsigned long	entry_pt_ofs;
	unsigned short	entry_pt_cs,entry_pt_ip;
	unsigned long	entry_pt_stackofs;
	unsigned short	entry_pt_ss,entry_pt_sp;
	unsigned long	resmem_length,fsize,hdr_size;
	unsigned short	magic_entry_ip;
	unsigned long	magic_entry_ofs;
	unsigned short	magic_entry_ip2;
	unsigned long	magic_entry_ofs2;
	unsigned short	magic_entry_ip3;
	unsigned long	magic_entry_ofs3;
	unsigned short	magic_entry_ip4;
	unsigned long	magic_entry_ofs4;
};
