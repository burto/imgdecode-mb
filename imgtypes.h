#ifndef __IMGTYPES__H
#define __IMGTYPES__H

#include "config.h"
#include <sys/types.h>
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <string>

using namespace std;

typedef unsigned char byte_t;
typedef int16_t word_t;
typedef int32_t dword_t;
#ifdef HAVE_U_INT16_T
typedef u_int16_t uword_t;
typedef u_int32_t udword_t;
#else
typedef uint16_t uword_t;
typedef uint32_t udword_t;
#endif

struct coord_struct {
	dword_t	ulat;
	dword_t ulong;
};
typedef struct coord_struct coord_t;

#endif

