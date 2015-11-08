
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "common/zlib/zlib.h"

class MacromediaFlashSWFunzip {
public:
	MacromediaFlashSWFunzip();
	~MacromediaFlashSWFunzip();
public:
	virtual void comment(char *fmt,...);
	int CheckHeader(unsigned char *buf);		// give this function the first 8 bytes of the SWF file.
	int ConvertHeader(unsigned char *buf);		// convert CWS to FWS
	int Begin();
	int End();
	int Decompress(unsigned char *inbuf,int *insz,unsigned char *outbuf,int *outsz,int force);
public:
	unsigned char*		in;
	int					insz;
	unsigned char*		out;
	int					outsz;
	z_stream			zlibs;
	char				zlib_init;
};
