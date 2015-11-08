
#include "common/Types.h"

class MSDOS_WinEXEStub {
public:
	int Ident(unsigned char *buf,int buflen);
	int Ident_Type1(unsigned char *buf,int buflen);
public:
	virtual void comment(char isErr,char *fmt,...);
	virtual void onFoundMessage(char *message);
};
