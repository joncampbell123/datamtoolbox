/* Types.cpp
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * code to handle typedefs */

#include "common/Types.h"

uint16 llei16(void *x)
{
	unsigned char *p = (unsigned char*)x;
	uint16 r;

	r  = ((uint16)p[1])<<8;
	r |=  (uint16)p[0];
	return r;
}

uint32 llei32(void *x)
{
	unsigned char *p = (unsigned char*)x;
	uint32 r;

	r  = ((uint32)p[3])<<24;
	r |= ((uint32)p[2])<<16;
	r |= ((uint32)p[1])<<8;
	r |=  (uint32)p[0];
	return r;
}

uint64 llei64(void *x)
{
	unsigned char *p = (unsigned char*)x;
	uint64 r;

	r  = ((uint64)p[7])<<((uint64)56);
	r |= ((uint64)p[6])<<((uint64)48);
	r |= ((uint64)p[5])<<((uint64)40);
	r |= ((uint64)p[4])<<((uint64)32);
	r |= ((uint64)p[3])<<((uint64)24);
	r |= ((uint64)p[2])<<((uint64)16);
	r |= ((uint64)p[1])<<((uint64)8);
	r |=  (uint64)p[0];
	return r;
}

uint16 lbei16(void *x)
{
	unsigned char *p = (unsigned char*)x;
	uint16 r;

	r  = ((uint16)p[0])<<8;
	r |=  (uint16)p[1];
	return r;
}

uint32 lbei32(void *x)
{
	unsigned char *p = (unsigned char*)x;
	uint32 r;

	r  = ((uint32)p[0])<<24;
	r |= ((uint32)p[1])<<16;
	r |= ((uint32)p[2])<<8;
	r |=  (uint32)p[3];
	return r;
}

void slei16(void *x,uint16 w)
{
	unsigned char *p = (unsigned char*)x;

	p[0] = w;
	p[1] = w >> 8;
}

void slei32(void *x,uint32 w)
{
	unsigned char *p = (unsigned char*)x;

	p[0] = w;
	p[1] = w >> 8;
	p[2] = w >> 16;
	p[3] = w >> 24;
}

void sbei16(void *x,uint16 w)
{
	unsigned char *p = (unsigned char*)x;

	p[0] = w >> 8;
	p[1] = w;
}

void sbei32(void *x,uint32 w)
{
	unsigned char *p = (unsigned char*)x;

	p[0] = w >> 24;
	p[1] = w >> 16;
	p[2] = w >> 8;
	p[3] = w;
}
