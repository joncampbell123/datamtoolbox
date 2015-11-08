
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "Macromedia/Flash-SWF/cmfswfunzip.h"

MacromediaFlashSWFunzip::MacromediaFlashSWFunzip()
{
	zlib_init=0;
	in=out=NULL;
	memset(&zlibs,0,sizeof(zlibs));
}

MacromediaFlashSWFunzip::~MacromediaFlashSWFunzip()
{
	End();
}

// input:
//        inbuf  = ptr to compressed data
//        insz   = ptr to integer with length of compressed data
//        outbuf = ptr to store decompressed data
//        outsz  = ptr to integer with length of output buffer
//
// output:
//        return value: -1 if failure, 0 if success, 1 if EOS
//        insz   = ptr to integer with number of compressed bytes used
//        outsz  = ptr to integer with number of decompressed bytes output
int MacromediaFlashSWFunzip::Decompress(unsigned char *inbuf,int *insz,unsigned char *outbuf,int *outsz,int force)
{
	int err;

	if (!inbuf || !insz || !outbuf || !outsz) return -1;
	if (*insz <= 0 && *outsz <= 0) return 0;

	zlibs.avail_in=*insz;
	zlibs.next_in=inbuf;
	zlibs.avail_out=*outsz;
	zlibs.next_out=outbuf;

	err = inflate(&zlibs,force);

	*insz -= zlibs.avail_in;
	if (*insz < 0) return -1;
	*outsz -= zlibs.avail_out;
	if (*outsz < 0) return -1;

	if (err == Z_STREAM_END) return 1;
	if (err != Z_OK) return -1;
	return 0;
}

int MacromediaFlashSWFunzip::Begin()
{
	if (zlib_init) return 0;

	insz = 4096;
	outsz = 4096;

	in = new unsigned char[insz];
	if (!in) {
		comment("not enough memory for input buffer");
		End();
		return -1;
	}

	out = new unsigned char[outsz];
	if (!out) {
		comment("not enough memory for output buffer");
		End();
		return -1;
	}

	memset(&zlibs,0,sizeof(zlibs));
	zlibs.data_type=Z_BINARY;
	zlibs.avail_in=0;
	zlibs.total_in=0;
	zlibs.avail_out=outsz;
	zlibs.total_out=0;
	zlibs.next_in=in;
	zlibs.next_out=out;
	if (inflateInit(&zlibs) != Z_OK) {
		comment("ZLIB inflateInit() failure");
		End();
		return -1;
	}

	zlib_init=1;
	return 0;
}

int MacromediaFlashSWFunzip::End()
{
	if (zlib_init) inflateEnd(&zlibs);
	zlib_init=0;
	if (in) delete in;
	in=NULL;
	if (out) delete out;
	out=NULL;
	return 0;
}

void MacromediaFlashSWFunzip::comment(char *fmt,...)
{
	va_list va;

	va_start(va,fmt);
	printf("[MacromediaFlashSWFunzip]: ");
	vprintf(fmt,va);
	printf("\n");
	va_end(va);
}

// give this function the first 8 bytes of the SWF file.
// return values are:
//  = -1 this is not a SWF movie
//  =  0 this is a SWF movie, but not compressed
//  =  1 this is a SWF movie, compressed
int MacromediaFlashSWFunzip::CheckHeader(unsigned char *buf)
{
	// must be xWSn where x is 'C' or 'F' and n is >= 0x01
	if (!(buf[1] == 'W' && buf[2] == 'S'))
		return -1;

	// 'F' means uncompressed 
	if (buf[0] == 'F')
		return 0;
	else if (buf[0] != 'C')
		return -1;

	// beware of possible changes in future versions of Flash.
	// version 8 SWF movies don't exist... yet (v7 is the latest at the time)
	if (buf[3] > 7) {
		comment("There is no such thing as Macromedia Flash version %u... yet. If there is, please update this code and recompile",buf[3]);
		return -1;
	}

	// ZLIB compression wasn't introduced until v6
	if (buf[3] < 6)
		comment("CWS signature signifies compression but version %u suggests SWF was made prior to v6 when compression was introduced",buf[3]);

	return 1;
}

// convert the CWS compressed SWF header to FWS
int MacromediaFlashSWFunzip::ConvertHeader(unsigned char *buf)
{
	// ASSUME that CheckHeader() succeeded!
	buf[0] = 'F';		/* change CWS to FWS */
	return 0;
}
