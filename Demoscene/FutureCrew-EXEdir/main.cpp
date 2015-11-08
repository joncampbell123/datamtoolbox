
/* (C) 1998, 2005 Jonathan Campbell */

#include <stdio.h>
#include <string.h>
#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Demoscene/FutureCrew-EXEdir/fcxdir.h"

// see s3m_correct.cpp
int S3M_CorrectSign(char *name);

// finds a file within the demo, and extracts it to a REAL file of the same name
int FindAndRip(FutureCrewEXEDirReader *fr,char *name,char *filename)
{
	FutureCrewEXEDirReader::ITEM item;
	unsigned char buf[512];
	int x;

	if (fr->FindItem(&item,name) >= 0) {
		FutureCrewEXEDirReader::file *f;
		FILE *fp;

		f = fr->GetFile(&item);
		if (f) {
			fp=fopen(filename,"wb");
			f->Seek(0);
			while ((x=f->Read(buf,sizeof(buf))) > 0) fwrite(buf,1,x,fp);
			fclose(fp);
			delete f;
			return 1;
		}
	}

	return -1;
}

int main(int argc,char **argv)
{
	FutureCrewEXEDirReader::ITEM item;
	FutureCrewEXEDirReader fr;
	SomeFileStdio sfs;
	int i;

	if (sfs.Open(argv[1]) < 0) {
		printf("Cannot open file %s\n",argv[1]);
		return 1;
	}

	fr.Assign(&sfs);
	if (fr.IsEXE() < 0) {
		printf("Error. This does not look like a valid .EXE.\n");
		printf("The FC demos this code is programmed for are all valid MS-DOS .EXE files\n");
		return 1;
	}
	if (fr.IsFCProduct() < 0) {
		printf("Error. This does not look like a Future Crew product that this program.\n");
		printf("was written to target (Unreal, Theparty, Panic, Fishtro).\n");
		return 1;
	}
	if (fr.Check() < 0) {
		printf("Error. Invalid directory table.\n");
		return 1;
	}

	for (i=0;i < fr.numFiles;i++) {
		if (fr.ReadItem(i,&item) >= 0) {
			printf("%-16s offset=%-8u size=%-8u\n",item.name,item.offset,item.size);
		}
	}

// identify the demos based on certain files that exist within them
	if (FindAndRip(&fr,"water.s3m","fishtro.s3m") >= 0) {
		printf("Looks like: Fishtro\n");
		// HACK: Samples in WATER.S3M need to be converted from signed PCM to unsigned PCM
		//       or they will sound loud, distorted and scratchy in some modern S3M players
		//       (like Nullsoft WinAMP). A very unpleasant surprise to the unaware who have
		//       good amplifiers, though it also gives the song an interesting grunge effect!
		S3M_CorrectSign("fishtro.s3m");
	}
	else if (FindAndRip(&fr,"p.p","panic.s3m") >= 0)
		printf("Looks like: Panic\n");
	else if (FindAndRip(&fr,"music.s3m","theparty.s3m") >= 0)
		printf("Looks like: TheParty\n");
// Unreal has multiple S3M files
	else if (FindAndRip(&fr,"chaos.s3m","unreal1.s3m") >= 0) {
		printf("Looks like: Unreal\n");
		FindAndRip(&fr,"lave2.s3m","unreal2.s3m");
		FindAndRip(&fr,"tmp.s3m","unreal3.s3m");
		FindAndRip(&fr,"tiger.s3m","unreal4.s3m");
	}
	else {
		printf("WARNING: I don't recognize this demo\n");
	}

	sfs.Close();
	return 0;
}
