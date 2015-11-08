/* Apple-Macintosh-Common.cpp
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * Provides Big Endian to host integer conversion, and Pascal to
 * C-string conversion.
 */

#include <string.h>
#include "Apple/common/Apple-Macintosh-Common.h"

void aStr15cpy(aStr15 *d,void *_s)
{
	unsigned char *s = (unsigned char*)_s;
	d->len = s[0];
	memcpy(d->str,s+1,15);
}

void aStr27cpy(aStr27 *d,void *_s)
{
	unsigned char *s = (unsigned char*)_s;
	d->len = s[0];
	memcpy(d->str,s+1,27);
}

void aStr31cpy(aStr31 *d,void *_s)
{
	unsigned char *s = (unsigned char*)_s;
	d->len = s[0];
	memcpy(d->str,s+1,31);
}

void aStr255cpy(aStr255 *d,void *_s)
{
	unsigned char *s = (unsigned char*)_s;
	d->len = s[0];
	memcpy(d->str,s+1,255);
}

void PascalToCString(char *dst,PascalString *src,int max)
{
	int l=src->len;

	max--;	// allow for NULL at the end
	if (l > max) l = max;
	if (l < 0) return;
	if (l > 0) memcpy(dst,src->str,l);
	dst[l]=0;
}

int PascalCmp(PascalString *s1,PascalString *s2)
{
	int x,i;

	x = (s1->len - s2->len);
	if (x) return x;
	i = x = 0;
	while (i < s1->len && (x=(s1->str[i]-s2->str[i])) == 0) i++;
	return x;
}

/*********************************************************************
 * we will be using the file names in the filesystem to create       *
 * actual files on the host system. filter out characters that would *
 * cause problems.                                                   *
 *********************************************************************/
void MacFnFilter(char *str)
{
	while (*str != 0) {
		/* satisfy both signed char assumptions and unsigned char possibilities. */
		/* the point is to remove extended chars */
		if (*str < 0 || *str >= 128)	*str = 'x';
		else if (*str == '/')			*str = '-';
		else if (*str == '\\')			*str = '-';
		else if (*str == '\"')			*str = '-';
		else if (*str == '\'')			*str = '-';
		else if (*str == '>')			*str = '-';
		else if (*str == '<')			*str = '-';
		else if (*str == ':')			*str = '-';
		else if (*str == '?')			*str = '-';
		else if (*str < 32)				*str = ' ';
		str++;
	}
}
