
#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Microsoft/MSDOS-EXE/msdosexe.h"
#include "Microsoft/MSDOS-WinEXEstub/winstub.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

int main(int argc,char **argv)
{
	unsigned char resident[4096];
	MSDOS_WinEXEStub wstub;
	SomeFileStdio src;
	unsigned long a;
	int residentlen;
	MSDOSEXE exe;

	if (argc < 2) {
		printf("stublook <EXE file>\n");
		return 1;
	}

	if (src.Open(argv[1]) < 0) {
		printf("Failed to open %s\n",argv[1]);
		return 1;
	}

	exe.Assign(&src);
	if (exe.Check() < 0) {
		printf("%s not an EXE file\n",argv[1]);
		return 1;
	}

	if ((a=exe.GetWindowsEXEHeader()) != 0)
		printf("This EXE has a Windows Header at offset %u\n",a);

	if ((a=exe.GetSPoffset()) != -1)
		printf("Initial stack pointer SS:SP can be found at offset %u\n",a);
	else
		printf("Cannot locate initial stack pointer.\n");

	if ((a=exe.GetIPoffset()) != -1) {
		printf("Entry point CS:IP can be found at offset %u\n",a);
	}
	else {
		printf("Cannot locate entry point.\n");
		return 1;
	}

/* so a = initial instruction pointer. load what is there into simulated resident memory and check it. */
	residentlen = exe.GetResidentSize() + exe.GetResidentOffset();
	if (residentlen < a) {
		printf("BUG: last byte of resident data < CS:IP\n");
		return 1;
	}

	residentlen -= a;
	if (residentlen > 4096) {
		printf("WARNING: Resident area %u is > 4096 bytes, which is very unusual for a stub\n",residentlen);
		residentlen = 4096;
	}

	memset(resident,0,sizeof(resident));
	if (src.Seek(a) != a || src.Read(resident,residentlen) < residentlen) {
		printf("ERROR: problem loading resident area\n");
		return 1;
	}

	if (wstub.Ident(resident,residentlen) < 0)
		printf("Not a Windows MS-DOS stub\n");

	return 0;
}
