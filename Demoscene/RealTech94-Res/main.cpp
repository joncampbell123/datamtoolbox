
#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Demoscene/RealTech94-Res/rt94res.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>

// finds a file within the demo, and extracts it to a REAL file of the same name
int FindAndRip(RealTech94Resources *fr,char *name,char *filename)
{
	RealTech94Resources::ITEM item;
	unsigned char buf[512];
	int x;

	if (fr->FindItem(&item,name) >= 0) {
		RealTech94Resources::file *f;
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
	RealTech94Resources::ITEM item;
	RealTech94Resources rt94;
	SomeFileStdio sfs;
	char *name;
	int i;

	if (argc < 2)	name = "DIM.RES";
	else			name = argv[1];

	if (sfs.Open(name) < 0)
		if (argc < 2)
			name = "MEGAMIX.RES";

	if (sfs.Open(name) < 0) {
		printf("Cannot open %s\n",name);
		return 1;
	}

	rt94.Assign(&sfs);
	if (rt94.Check() < 0) {
		printf("Not a RealTech 94 resource file\n");
		return 1;
	}

	for (i=0;i < rt94.numItems;i++) {
		if (rt94.GetItem(i,&item) >= 0) {
			printf("%-12s offset=%-9u size=%-9u\n",item.name,item.offset,item.size);
		}
	}

	/* demonstrate our capabilities by ripping the music files & images */
	/* AMF is a music format apparently, that is supported by Nullsoft WinAMP */
	if (FindAndRip(&rt94,"part0.amf","music0.amf") > 0) {
		printf("This is the '94 Dimension demo\n");
		FindAndRip(&rt94,"part1.amf","music1.amf");
		FindAndRip(&rt94,"part3.amf","music2.amf");
		FindAndRip(&rt94,"dimlogo.gif","dimlogo.gif");		// <-- 3D raytraced image
		FindAndRip(&rt94,"execom.gif","execom.gif");		// <-- who is this guy?
		FindAndRip(&rt94,"intel.gif","intel.gif");			// <-- "Intel Outside" LOL
		FindAndRip(&rt94,"presents.gif","presents.gif");	// <-- as seen at the beginning of the demo
		FindAndRip(&rt94,"scfin33.gif","scfin33.gif");
	}
	else if (FindAndRip(&rt94,"credit.amf","music0.amf") > 0) {
		printf("This is the '94 Megamix demo\n");
		FindAndRip(&rt94,"demo1.amf","music1.amf");
		FindAndRip(&rt94,"demo2.amf","music2.amf");
		FindAndRip(&rt94,"demo3.amf","music3.amf");
		FindAndRip(&rt94,"pub.amf","music4.amf");
		FindAndRip(&rt94,"talkgod.amf","music5.amf");
	}
}
