
/* (C) 2005 Jonathan Campbell */
/* some S3M files apparently have signed PCM samples that need to be converted to unsigned */
// WARNING: Does minimal error checking

#include <stdio.h>
#include <string.h>
#include "common/Types.h"

static void S3M_SampleCorrect(FILE *f,unsigned long ofs)
{
	unsigned long Length,MemPtr,sofs;
	unsigned char buf[4096];
	int i,s,x;

	fseek(f,ofs,SEEK_SET);
	if (fread(buf,1,0x50,f) < 0x50) return;

/* this is a sample type NOT an adlib type? */
	if (buf[0] != 1) return;
/* another check */
	if (memcmp(buf+0x4C,"SCRS",4)) return;

	Length = llei32(buf+0x10);
	MemPtr = llei16(buf+0x0E) * 16;

/* locate the sample data */
	for (i=0;i < Length;i += 4096) {
		s = Length - i;
		if (s > 4096) s = 4096;
		sofs = MemPtr + i;

		/* read */
		fseek(f,sofs,SEEK_SET);
		fread(buf,1,s,f);
		/* convert */
		for (x=0;x < s;x++) buf[x] ^= 0x80;
		/* write */
		fseek(f,sofs,SEEK_SET);
		fwrite(buf,1,s,f);
	}
}

int S3M_CorrectSign(char *name)
{
	unsigned char buf[0x60];
	unsigned short OrdNum,InsNum,ParaPtr;
	int Ins;
	FILE *f;

	f=fopen(name,"rb+");
	if (!f) return -1;
	if (fread(buf,1,0x60,f) < 0x60)
		goto fail;

/* make sure there is a SCRM in there */
	if (memcmp(buf+0x2C,"SCRM",4))
		goto fail;

	OrdNum = llei16(buf + 0x20);
	InsNum = llei16(buf + 0x22);

/* enumerate the instruments */
	for (Ins=0;Ins < InsNum;Ins++) {
		fseek(f,0x60 + OrdNum + (Ins * 2),SEEK_SET);
		memset(buf,0,2); fread(buf,1,2,f);
		ParaPtr = llei16(buf);
		S3M_SampleCorrect(f,((unsigned long)ParaPtr)*16L);
	}

	fclose(f);
	return 0;
fail:
	fclose(f);
	return -1;
}
