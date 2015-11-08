
#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Macromedia/Flash-SWF/cmfswftags.h"

// begin namespace
namespace MacromediaFlashSWF {
///////////////////////////////

BitsNBytes::BitsNBytes()
{
	next = fence = NULL;
	shift = shiftLeft = 0;
}

BitsNBytes::BitsNBytes(unsigned char *buffer,int len)
{
	next = buffer;
	begin = buffer;
	fence = buffer + len;
	shift = shiftLeft = 0;
}

void BitsNBytes::Assign(unsigned char *buffer,int len)
{
	next = buffer;
	begin = buffer;
	fence = buffer + len;
	shift = shiftLeft = 0;
}

void BitsNBytes::Seek(unsigned char *ptr)
{
	if (ptr < begin) ptr = begin;
	if (ptr > fence) ptr = fence;
	next = ptr;
}

void BitsNBytes::ByteAlign()
{
	shiftLeft = 0;
}

unsigned char BitsNBytes::UI8()
{
	ByteAlign();
	if (next == NULL || next >= fence) return 0;
	return *next++;
}

unsigned short BitsNBytes::UI16()
{
	unsigned short x;
	ByteAlign();
	if (next == NULL || next >= fence) return 0;
	x  =  (unsigned short)(*next++);
	if (next == NULL || next >= fence) return x;
	x |= ((unsigned short)(*next++)) << 8;
	return x;
}

signed short BitsNBytes::SI16()
{
	unsigned short x;
	ByteAlign();
	if (next == NULL || next >= fence) return 0;
	x  =  (unsigned short)(*next++);
	if (next == NULL || next >= fence) return x;
	x |= ((unsigned short)(*next++)) << 8;
	return (signed short)x;
}

unsigned long BitsNBytes::UI32()
{
	unsigned long x;
	ByteAlign();
	if (next == NULL || next >= fence) return 0;
	x  =  (unsigned short)(*next++);
	if (next == NULL || next >= fence) return x;
	x |= ((unsigned short)(*next++)) << 8;
	if (next == NULL || next >= fence) return x;
	x |= ((unsigned short)(*next++)) << 16;
	if (next == NULL || next >= fence) return x;
	x |= ((unsigned short)(*next++)) << 24;
	return x;
}

unsigned long BitsNBytes::UB(int N)
{
	unsigned long v=0;
	int b;

	b=N-1;
	while (N-- > 0) {
		if (!shiftLeft) {
			if (next != NULL && next < fence) {
				shift = *next++;
				shiftLeft = 8;
			}
			else {
				shift = 0;
				shiftLeft = 0;
				N = 0;
			}
		}

		v |= ((shift >> --shiftLeft) & 1) << b--;
	}

	return v;
}

int BitsNBytes::oodata()
{
	if (!next) return 1;
	if (next >= fence && shiftLeft == 0) return 1;
	return 0;
}

unsigned long BitsNBytes::SB(int N)
{
	unsigned long v=0,lmask;
	int b,Nmax;

	b=Nmax=N-1;
	lmask=0xFFFFFFFF<<N;
	while (N-- > 0) {
		if (!shiftLeft) {
			if (next != NULL && next < fence) {
				shift = *next++;
				shiftLeft = 8;
			}
			else {
				shift = 0;
				shiftLeft = 0;
				N = 0;
			}
		}

		if (N == Nmax) {
			unsigned long p = ((shift >> --shiftLeft) & 1);
			v |= p << b--;
			if (p) v |= lmask;
		}
		else {
			v |= ((shift >> --shiftLeft) & 1) << b--;
		}
	}

	return v;
}

// code to read common SWF structures
int ReadRect(BitsNBytes *bb,Rect *r)
{
	r->Nbits =		bb->UB(5);
	r->Xmin =		bb->SB(r->Nbits);
	r->Xmax =		bb->SB(r->Nbits);
	r->Ymin =		bb->SB(r->Nbits);
	r->Ymax =		bb->SB(r->Nbits);
	return 0;
}

// -1 if not a valid header
//  0 if success
//  1 if SWF needs to be decompressed first
//
// only the first 4 bytes of buf[] are used here and only
// "version" is touched in the FileHeader structure. Another
// function fills in the rest.
int ReadHeader(unsigned char *buf,int buflen,FileHeader *fh)
{
	if (buflen < 4)
		return -1;
	if (buf[1] != 'W' || buf[2] != 'S')
		return -1;

/* currently v7 is the latest, there is no such version as v0 */
	if (buf[3] > 7 || buf[0] == 0)
		return -1;

/* compressed SWF is not directly supported here. it is up
   to the caller to translate this to uncompressed and then
   to decompress the SWF before running it through this code. */
	if (buf[0] == 'C')
		return 1;

	if (buf[0] != 'F')
		return -1;

	fh->version = buf[3];
	return 0;
}

/* assuming that ReadHeader() succeeded, fill in the rest of the structure. */
/* buf[] must contain at least 1+4+4+2+2 = 1+8+4 = 1+12 = 13 bytes because
   of the other structure members. */
int ReadFileHeader(unsigned char *buf,int buflen,FileHeader *fh)
{
	if (buflen < 13) return -1;

	BitsNBytes bb(buf+4,30-4);

	fh->FileLength = bb.UI32();
	ReadRect(&bb,&fh->FrameSize);
	fh->FrameRate =	bb.UI16();
	fh->FrameCount = bb.UI16();
	if (bb.oodata()) return -1;
	fh->FirstTagOffset = (int)(bb.next - buf);
	return 0;
}

// tag reading class
TagReader::TagReader(BitsNBytes* newbb)
{
	bb = newbb;
}

int TagReader::ReadTag()
{
	unsigned short Tag;

	if (bb->oodata()) return -1;

	TagHeader = bb->next;
	Tag = bb->UI16();
	Length = Tag & 0x3F;
	Code = Tag >> 6;

	if (Length == 0x3F) {
		Length = bb->UI32();
		IsLongTag = 1;
	}
	else {
		IsLongTag = 0;
	}

	TagHeaderLength = IsLongTag ? 6 : 2;
	TotalLength = TagHeaderLength + Length;
	TagData = bb->next;
	TagEnd = TagData + Length;
	return 0;
}

int ReadDefineSound_Header(BitsNBytes *bb,DefineSound_Header *h)
{
	h->SoundId =			bb->UI16();
	h->SoundFormat =		bb->UB(4);
	h->SoundRate =			44100 >> (3 - bb->UB(2));
	h->SoundSize =			bb->UB(1);
	h->SoundType =			bb->UB(1);
	h->SoundSampleCount =	bb->UI32();
	return 0;
}

int ReadSoundStreamHead_Header(BitsNBytes *bb,SoundStreamHead_Header *h)
{
								bb->ByteAlign();
	h->Reserved0 =				bb->UB(4);
	h->PlaybackSoundRate =		44100 >> (3 - bb->UB(2));
	h->PlaybackSoundSize =		bb->UB(1);
	h->PlaybackSoundType =		bb->UB(1);
	h->StreamSoundCompression =	bb->UB(4);
	h->StreamSoundRate =		44100 >> (3 - bb->UB(2));
	h->StreamSoundSize =		bb->UB(1);
	h->StreamSoundType =		bb->UB(1);
	h->StreamSampleCount =		bb->UI16();
	if (h->StreamSoundCompression == 2)
		h->LatencySeek =		bb->SI16();
	else
		h->LatencySeek =		0;

	return 0;
}

///////////////////////////////
};
// end namespace
