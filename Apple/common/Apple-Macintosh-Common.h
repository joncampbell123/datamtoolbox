/* Apple-Macintosh-Common.h
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * provides defines for C/C++ that allows the use of typedefs
 * used in Apple Developer documentation. Also provides
 * Big Endian to host integer conversion, and Pascal to C-string
 * conversion.
 */

#ifndef __APPLE_MACINTOSH_COMMON_H
#define __APPLE_MACINTOSH_COMMON_H

/*********************************************************************
 * typedefs, so that types used in this code match the types shown   *
 * in Apple's developer documentation regarding HFS.                 *
 *********************************************************************/
typedef unsigned short int		aInteger;		// 16-bit unsigned int
typedef unsigned short int		aUInt16;		// 16-bit unsigned int
typedef signed short int		aSInt16;		// 16-bit signed int
typedef unsigned char			aSignedByte;	// we cast as unsigned char anyway
typedef unsigned long int		aLongInt;		// 32-bit unsigned int
typedef unsigned char			aOSType[4];		// OS type

typedef struct {
	aSInt16	v,h;
} aPoint;

typedef struct {
	unsigned char			len;
	unsigned char			str[15];
} aStr15;

typedef struct {
	unsigned char			len;
	unsigned char			str[27];
} aStr27;

typedef struct {
	unsigned char			len;
	unsigned char			str[31];
} aStr31;

typedef struct {
	unsigned char			len;
	unsigned char			str[255];
} aStr255;

typedef aStr255 PascalString; // generic concept for Pascal-style strings

/*********************************************************************
 * enumerations and constants                                        *
 *********************************************************************/

/* Macintosh Finder flags */
enum {
   kIsOnDesk =		0x0001,
   kColor =			0x000E,
   kIsShared =		0x0040,
   kHasNoINITs =	0x0080,
   kHasBeenInited =	0x0100,
   kHasCustomIcon =	0x0400,
   kIsStationery =	0x0800,
   kNameLocked =	0x1000,
   kHasBundle =		0x2000,
   kIsInvisible =	0x4000,
   kIsAlias =		0x8000
};

/*********************************************************************
 * functions to convert integers and strings from Apple/Big Endian   *
 * to host byte order & C-strings.                                   *
 *********************************************************************/
aInteger lei16(void *x);
aLongInt lei32(void *x);
void aStr15cpy(aStr15 *d,void *_s);
void aStr27cpy(aStr27 *d,void *_s);
void aStr31cpy(aStr31 *d,void *_s);
void aStr255cpy(aStr255 *d,void *_s);
void PascalToCString(char *dst,PascalString *src,int max);
int  PascalCmp(PascalString *s1,PascalString *s2);
void MacFnFilter(char *str);

#endif //__APPLE_MACINTOSH_COMMON_H
