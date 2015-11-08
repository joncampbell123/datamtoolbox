/* Types.h
 *
 * (C) 2004, 2005 Jonathan Campbell
 *
 * The root of all Typedefs
 */

typedef unsigned short			uint16;
typedef signed short			sint16;
typedef unsigned long			uint32;
typedef signed long				sint32;

#ifdef WIN32
typedef unsigned __int64		uint64;
typedef signed __int64			sint64;
#else
typedef unsigned long long		uint64;
typedef signed long long		sint64;
#endif

uint16 llei16(void *x);
uint32 llei32(void *x);
uint64 llei64(void *x);
uint16 lbei16(void *x);
uint32 lbei32(void *x);
void slei16(void *x,uint16 w);
void slei32(void *x,uint32 w);
void sbei16(void *x,uint16 w);
void sbei32(void *x,uint32 w);
