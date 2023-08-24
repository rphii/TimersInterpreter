#ifndef VAL_H

#include <stddef.h>
#include <sys/types.h>
#include <limits.h>

#if 0
typedef ssize_t Val;
#define FMT_VAL     "%zu"
#define VAL_MAX     SSIZE_MAX
#else
typedef size_t Val;
#define FMT_VAL     "%zu"
#define VAL_MAX     SIZE_MAX
#endif

#define ERR_VAL_POINTER "expected pointer to Val"

#if 0
typedef struct {
    size_t v;
} Val;
#define FMT_VAL     "%zi"
#define VAL_P(x)    ((x).v)
#define VAL_MAX     (Val){.v = SSIZE_MAX}

#define VAL(x)              (Val){.v = x}
#define VAL_NZERO(x)        !((x).v)
#define VAL_GZERO(x)        ((x).v > 0)
#define VAL_OBOUNDS(x)      ((x).v >= SSIZE_MAX)
#define VAL_OP(x, op, y)    (Val){.v = ((x).v op (y).v)}
#define VAL_EVAL(x, ev, y)  ((x).v ev (y).v)
#define VAL_INC(x)          (x).v++
#define VAL_DEC(x)          (x).v--
#define VAL_PREC_DIV(x, y)  (Val){.v = ((double)(x).v / (double)(y).v)}
#endif

#if 0
typedef unsigned char Val;
#define FMT_VAL     "%i"
#define VAL_MAX     255
#endif

#if 0
typedef struct {
    size_t v;
} Val;
#define FMT_VAL     "%zu"
#define VAL_MAX     SIZE_MAX
#endif

#if 0
typedef double Val;
#define FMT_VAL     "%f"
#define VAL_MAX     1.0
#endif

#if 0
typedef __int128 Val;
#define FMT_VAL     "%llx"
#define VAL_MAX     340282366920938463463374607431768211455LLU
#endif



#include "vec.h"
VEC_INCLUDE(VecVal, vec_val, Val, BY_VAL);

#define VAL_H
#endif

