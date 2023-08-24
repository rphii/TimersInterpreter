#ifndef UTF8_H

#include <stdint.h>

typedef struct U8Point {
    uint32_t val;
    int bytes;
} U8Point;

#define ERR_UTF8_POINTER    "expected pointer to utf-8 point"

#define UTF8_H
#endif

