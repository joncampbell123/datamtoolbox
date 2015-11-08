// note: uses structure names and type defs from Macromedia's official SWF File Format Specification

#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Macromedia/Flash-SWF/cmfswftags.h"

int main(int argc,char **argv)
{
	MacromediaFlashSWF::FileHeader swfhdr;
	unsigned long TagOfs,TagWas;
	unsigned char buf[512];
	SomeFileStdio sfs;
	int r;

	if (sfs.Open(argv[1]) < 0) {
		printf("Cannot open %s\n",argv[1]);
		return 1;
	}

	if (sfs.Seek(0) != 0 || sfs.Read(buf,4) < 4) {
		printf("file problem\n");
		return 1;
	}

	if ((r=MacromediaFlashSWF::ReadHeader(buf,4,&swfhdr)) < 0) {
		printf("Not a SWF file\n");
		return 1;
	}
	if (r == 1) {
		printf("The SWF file is compressed. Use swfunzip to decompress it.\n");
		return 1;
	}

	if ((r=sfs.Read(buf+4,26)) < 9) {
		printf("file problem\n");
		return 1;
	}

	if (MacromediaFlashSWF::ReadFileHeader(buf,r+4,&swfhdr) < 0) {
		printf("Problem reading the file header\n");
		return 1;
	}

	printf("Macromedia Flash version %u\n",swfhdr.version);
	printf("  Frame bounds are (%u,%u)-(%u,%u) twips\n",
		swfhdr.FrameSize.Xmin,swfhdr.FrameSize.Ymin,
		swfhdr.FrameSize.Xmax,swfhdr.FrameSize.Ymax);
	printf("  ...aka (%u,%u)-(%u,%u) pixels\n",
		swfhdr.FrameSize.Xmin/20,swfhdr.FrameSize.Ymin/20,
		swfhdr.FrameSize.Xmax/20,swfhdr.FrameSize.Ymax/20);
// usually the upper left corner is (0,0).
// SoThink is the only Flash authoring tool I know of that sets the
// upper-left coordinates to anything but (0,0) for whatever reason.
	printf("  Frame size is %u x %u twips\n",
		swfhdr.FrameSize.Xmax-swfhdr.FrameSize.Xmin,
		swfhdr.FrameSize.Ymax-swfhdr.FrameSize.Ymin);
	printf("  ...aka %u x %u pixels\n",
		(swfhdr.FrameSize.Xmax-swfhdr.FrameSize.Xmin)/20,
		(swfhdr.FrameSize.Ymax-swfhdr.FrameSize.Ymin)/20);
// to my knowledge Flash ignores the fractional part of the frame rate
	printf("  Frame rate is %u.%03u frames/sec\n",
		swfhdr.FrameRate >> 8,
		((swfhdr.FrameRate & 0xFF) * 1000) >> 8);
	printf("  Frame count is %u frames\n",swfhdr.FrameCount);

	TagOfs = swfhdr.FirstTagOffset;
	while (sfs.Seek(TagOfs) == TagOfs && (r=sfs.Read(buf,6)) >= 2) {
		MacromediaFlashSWF::BitsNBytes swfbnb(buf,r);
		MacromediaFlashSWF::TagReader swftag(&swfbnb);

		if (swftag.ReadTag() < 0)
			break;

		TagWas = TagOfs;
		TagOfs += swftag.TotalLength;
		if (TagOfs > sfs.GetSize())
			break;

		printf("Tag code=%u length=%u at offset=%u\n",
			swftag.Code,swftag.Length,TagWas);

		if (swftag.Code == MacromediaFlashSWF::Tag::DefineSprite) {
			unsigned long subTagOfs = TagWas + swftag.TagHeaderLength;
			unsigned long subTagEnd = subTagOfs + swftag.Length;
			unsigned long subTagWas;
			unsigned short int SpriteID;
			unsigned short int FrameCount;

			if (sfs.Seek(subTagOfs) == subTagOfs && (r=sfs.Read(buf,4)) == 4) {
				swfbnb.Assign(buf,4);
				SpriteID = swfbnb.UI16();
				FrameCount = swfbnb.UI16();
				subTagOfs += 4;
				printf(" DefineSprite ID %u FrameCount %u\n",SpriteID,FrameCount);
				while (subTagOfs <= (subTagEnd-2) && sfs.Seek(subTagOfs) == subTagOfs && (r=sfs.Read(buf,6)) >= 2) {
					MacromediaFlashSWF::BitsNBytes subswfbnb(buf,r);
					MacromediaFlashSWF::TagReader subswftag(&subswfbnb);

					if (subswftag.ReadTag() < 0)
						break;

					subTagWas = subTagOfs;
					subTagOfs += subswftag.TotalLength;
					if (subTagOfs > sfs.GetSize())
						break;

					printf("  Tag code=%u length=%u at offset=%u\n",
						subswftag.Code,subswftag.Length,subTagWas);
				}

				SpriteID++;
			}
		}
	}
}
