
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Macromedia/Flash-SWF/cmfswfunzip.h"

int main(int argc,char **argv)
{
	MacromediaFlashSWFunzip mswf;
	SomeFileStdio sfs,dfs;
	unsigned char buf[8];
	int r,rs,io,force,doh,err,do_continue=1;

	if (argc < 3) {
		printf("Takes Flash compressed SWF files and produces an uncompressed version.\n");
		printf("Usage: SWFUNZIP [compressed SWF] [destination uncompressed SWF]\n");
		return 1;
	}

	if (sfs.Open(argv[1]) < 0) {
		printf("Cannot open %s\n",argv[1]);
		return 1;
	}
	if (dfs.Create(argv[2]) < 0) {
		printf("Cannot create %s\n",argv[2]);
		return 1;
	}

	if (sfs.Read(buf,8) < 8) {
		printf("Unable to read first 8 bytes\n");
		return 1;
	}

	r=mswf.CheckHeader(buf);
	if (r < 0) {
		printf("Not a Flash SWF movie\n");
		return 1;
	}
	else if (r == 0) {
		printf("Not compressed\n");
		return 1;
	}

	if (mswf.ConvertHeader(buf) < 0) {
		printf("Failed to convert header\n");
		return 1;
	}

	if (dfs.Write(buf,8) < 8) {
		printf("Failure to write\n");
		return 1;
	}

	if (mswf.Begin() < 0) {
		printf("Failure to begin decompressing\n");
		return 1;
	}

	io = 0;
	force = 0;
	while (do_continue) {
		rs = mswf.insz - io;
		if (rs > 0) {
			r = sfs.Read(mswf.in + io,rs);
			if (r > 0) {
				force = 0;
				io += r;
			}
			else {
				if (force == 0) {
					force = 1;
				}
				else if (force++ >= 1000) {
					printf("zlib decompression stalled. exiting\n");
					break;
				}
			}
		}
		else {
			if (force == 0) {
				force = 1;
			}
			else if (force++ >= 1000) {
				printf("zlib decompression stalled. exiting\n");
				break;
			}
		}

		doh = mswf.outsz;
		rs  = io;
		err = mswf.Decompress(mswf.in,&rs,mswf.out,&doh,force);
		if (err < 0) {
			printf("zlib decompression error\n");
			do_continue=0;
		}
		else if (err == 1) {
			if (io > rs) printf("zlib decompression ended early! extra junk at the end!\n");
			do_continue=0;
		}

		if (rs > 0) {
			if (rs < io) {
				memmove(mswf.in,mswf.in+rs,io-rs);
				io -= rs;
			}
			else {
				io = 0;
			}
		}

		if (doh > 0) dfs.Write(mswf.out,doh);
	}

	mswf.End();
}
