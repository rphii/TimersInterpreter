#ifndef ALGEBRA_H

#include <stddef.h>

#include "err.h"
#include "attr.h"
#include "val.h"

int algebra_size_pow(Val base, Val exp, Val *result);
int algebra_sequence_power(VecVal *n, Val iters, Val offset, Val *sum);

#define ALGEBRA_H
#endif

