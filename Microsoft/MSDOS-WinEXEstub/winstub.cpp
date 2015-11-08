
#include "common/Types.h"
#include "common/SomeFileStdio.h"
#include "Microsoft/MSDOS-EXE/msdosexe.h"
#include "Microsoft/MSDOS-WinEXEstub/winstub.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

/* these represent Type 1 stubs, in which all the executable code is stored first followed
   by the string it prints out. */

// WARNING: DO NOT CHANGE THIS! THERE IS CODE THAT ASSUMES THIS IS 0xE BYTES LARGE!
static unsigned char StubType1_0000[] = {
/*1010:0000*/  0x0E,                         // PUSH CS
/*1010:0001*/  0x1F,                         // POP DS
/*1010:0002*/  0xBA,0x0E,0x00,               // MOV DX,000Eh	(WORD @ cs:0x3 = offset to string)
/*1010:0005*/  0xB4,0x09,                    // MOV AH,09h
/*1010:0007*/  0xCD,0x21,                    // INT 21h
/*1010:0009*/  0xB8,0x01,0x4C,               // MOV AX,4C01h
/*1010:000C*/  0xCD,0x21,                    // INT 21h
// string usually follows here
};

// WARNING: DO NOT CHANGE THIS! THERE IS CODE THAT ASSUMES THIS IS 0xE BYTES LARGE!
static unsigned char StubType1_0001[] = {
/*1010:0000*/  0xBA,0x10,0x00,               // MOV DX,0010h	(WORD @ cs:0x1 = offset to string)
/*1010:0003*/  0x0E,                         // PUSH CS
/*1010:0004*/  0x1F,                         // POP DS
/*1010:0005*/  0xB4,0x09,                    // MOV AH,09h
/*1010:0007*/  0xCD,0x21,                    // INT 21h
/*1010:0009*/  0xB8,0x01,0x4C,               // MOV AX,4C01h
/*1010:000C*/  0xCD,0x21,                    // INT 21h
// these are ignored by the matching code since they can be anything
/*1010:000E*/  0x90,                         // NOP
/*1010:000F*/  0x90,                         // NOP
// string usually follows here
};

/* these represent Type 2 stubs, where the first byte is a CALL to the executable code and
   the message string is sandwiched between the CALL and the rest of the code */
static unsigned char StubType2_0000[] = {
#if REFERENCE_ONLY
/*1010:0000*/  0xE8,0x53,0x00,               // CALL 0056
// string usually follows here
#endif
/*1010:0056*/  0x5A,                         // POP DX			(DX becomes the return address...)
/*1010:0057*/  0x0E,                         // PUSH CS			(...which is also the address of the string! ick!)
/*1010:0058*/  0x1F,                         // POP DS
/*1010:0059*/  0xB4,0x09,                    // MOV AH,09h
/*1010:005B*/  0xCD,0x21,                    // INT 21h
/*1010:005D*/  0xB8,0x01,0x4C,               // MOV AX,4C01h
/*1010:0060*/  0xCD,0x21,                    // INT 21h
};

// common stub messages
static char *StubMsg_Win95Era1_Eng = "This program cannot be run in DOS mode.\x0D\x0D\x0A";
static char *StubMsg_Win32Era1_Eng = "This program must be run under Win32\x0D\x0A";
static char *StubMsg_Win31Era1_Eng = "This program requires Microsoft Windows.\x0D\x0A";

int MSDOS_WinEXEStub::Ident_Type1(unsigned char *buf,int buflen)
{
	char string[512],terminator;
	unsigned char *p1,*pfence;
	int ofs=-1,stringo,ret=-1;
	bool unusual_offset=false;

	if (!memcmp(buf       ,StubType1_0000       ,0x0003) &&
		!memcmp(buf+0x0005,StubType1_0000+0x0005,0x0009)) {

		ret = 0;
		terminator = '$';
		comment(0,"Type 1, Windows 95+ era stub with executable code first followed by string which is MS-DOS $-terminated");

		// read the offset of the string
		ofs = llei16(buf+0x0003);

		// read the string, or complain about odd values
		if (ofs < 0xE) {
			comment(1,"The offset of the string is invalid (0x%04X, which points to the executable code itself)",ofs);
			ofs = -1;
		}
		else if (ofs >= buflen) {
			comment(1,"The offset of the string is invalid (0x%04X, which points beyond the resident image)",ofs);
			ofs = -1;
		}
		else {
			if (ofs != 0xE)
				unusual_offset=true;
		}
	}

	if (!memcmp(buf       ,StubType1_0001       ,0x0001) &&
		!memcmp(buf+0x0003,StubType1_0001+0x0003,0x000B)) {

		ret = 0;
		terminator = '$';
		comment(0,"Type 1, Win32 (pre-Windows 95) era stub with executable code first followed by string which is MS-DOS $-terminated");

		// read the offset of the string
		ofs = llei16(buf+0x0001);

		// read the string, or complain about odd values
		if (ofs < 0xE) {
			comment(1,"The offset of the string is invalid (0x%04X, which points to the executable code itself)",ofs);
			ofs = -1;
		}
		else if (ofs >= buflen) {
			comment(1,"The offset of the string is invalid (0x%04X, which points beyond the resident image)",ofs);
			ofs = -1;
		}
		else {
			if (ofs != 0x10)
				unusual_offset=true;
		}

		// usually this stub has two NOPs at 0xE and 0xF
		if (buf[0xE] != 0x90 || buf[0xF] != 0x90)
			comment(0,"Strange, this stub usually has two NO-OPs at byte offsets 0xE and 0xF. Instead they are 0x%02X and 0x%02X",buf[0xE],buf[0xF]);
	}

	// is the first instruction a CALL 0x0056 or some value like that?
	if (buf[0] == 0xE8) {
		int callofs = llei16(buf+1)+3;
		bool junk1=false;
		int ii;

		if (callofs != 0x56 && callofs != 0x2E)
			unusual_offset=true;

		if (callofs >= 0 && callofs <= (buflen-12)) {
			if (!memcmp(buf+callofs,StubType2_0000,12)) {
				ret = 0;
				terminator = '$';
				comment(0,"Type 1, Windows 3.x era stub with executable code first followed by string which is MS-DOS $-terminated");
				// the offset is the return address from the CALL, which happens to be the string
				ofs = 3;

				// scan backwards to find the '$' in the string.
				// while we're at it, take note if anything funny shows up between the end of string and the code.

				ii = callofs - 1;
				while (ii >= 3 && buf[ii] != terminator) {
					if (buf[ii] != 0x20) junk1=true;
					ii--;
				}

				if (junk1)
					comment(0,"It looks like there is extra data in the stub, between the end of the string and the executable");
			}
		}
	}

	if (ofs >= 0) {
		/* copy down the string and show it */
		pfence = buf + buflen;
		p1 = buf + ofs;
		stringo=0;
		while (p1 < pfence && *p1 != terminator) {
			if (stringo < 511)
				string[stringo++] = *p1;

			p1++;
		}
		string[stringo] = 0;

		/* complain about odd things we find */
		if (*p1 != terminator) {
			// we can assume here that p1 >= pfence
			comment(0,"The string is missing the terminator char '%c', program will likely fill the screen with garbage when run",terminator);
		}

		if (stringo == 0) {
			comment(0,"The string is null-length. Nothing will show when you run this program");
		}
		else {
			onFoundMessage(string);

			/* identify the message */
			if (!strcmp(string,StubMsg_Win95Era1_Eng))
				comment(0,"Windows 95 era DOS-mode message, English, CR CR LF version");
			else if (!strcmp(string,StubMsg_Win32Era1_Eng))
				comment(0,"Win32 pre-Windows 95 era message, English, CR LF version");
			else if (!strcmp(string,StubMsg_Win31Era1_Eng))
				comment(0,"Windows 3.x era message, English, CR LF version");
			else
				comment(0,"Unrecognized message");
		}

		if (unusual_offset)
			comment(0,"Strange, offset is unusual implying extra data between stub and string (0x%04X)",ofs);
	}

	return ret;
}

int MSDOS_WinEXEStub::Ident(unsigned char *buf,int buflen)
{
	if (Ident_Type1(buf,buflen) >= 0)
		return 0;

	return -1;
}

void MSDOS_WinEXEStub::onFoundMessage(char *message)
{
	printf("\nMSDOSEXE::onFoundMessage: Stub message is:\n---------------------------------------------------------------\n");
	printf("%s",message);
	printf("\n---------------------------------------------------------------\n");
}

void MSDOS_WinEXEStub::comment(char isErr,char *fmt,...)
{
	va_list va;

	va_start(va,fmt);
	printf("%sMSDOSEXE::Ident> ",isErr ? "ERROR: " : "INFO: ");
	vprintf(fmt,va);
	printf("\n");
	va_end(va);
}
