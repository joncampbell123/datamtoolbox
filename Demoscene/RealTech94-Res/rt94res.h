
#ifndef ___RT94_RES_H
#define ___RT94_RES_H

#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>

class RealTech94Resources {
public:
	typedef struct ITEM {
		char			name[13];
		unsigned long	size;
		unsigned long	offset;
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
			RealTech94Resources* parent;
			uint64 offset;
			ITEM item;
	};
public:
						RealTech94Resources();
						~RealTech94Resources();
	void				Assign(SomeFile *x);
	int					FindItem(ITEM *item,char *name);
	int					GetItem(int i,ITEM *item);
	int					ValidName(char *x);
	file*				GetFile(ITEM *item);
	int					Check();
public:
	virtual void		comment(char *fmt,...);
public:
	int					numItems;
public:
	unsigned long		first_offset;
	unsigned long*		offsets;
	SomeFile*			f;
};

#endif
