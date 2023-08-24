#ifndef STR_H

/////////////
// HEADERS //
/////////////

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"
#include "utils.h"
#include "utf8.h"
#include "val.h"

/////////////
// DEFINES //
/////////////

#define STR_DEFAULT_BLOCKSIZE   32
#define STR_F                   "%.*s"
#define STR_UNP(str)            (int)(str).l, (str).s
#define STR(x)                  (Str){.s = x, .l = sizeof(x)-1}
#define STR_L(x)                (Str){.s = x, .l = strlen(x)}

#define str_new(str)  Str str = {0}
#define str_free(...)   do { \
    for(size_t i = 0; i < SIZE_ARRAY(((Str []){__VA_ARGS__})); i++) \
        str_free_single(&((Str []){__VA_ARGS__})[i]); \
    } while(0)
#define str_free_p(...) do { \
    for(size_t i = 0; i < SIZE_ARRAY(((Str []){__VA_ARGS__})); i++) \
        str_free_single(((Str []){__VA_ARGS__})[i]); \
        free(((Str []){__VA_ARGS__})[i]); \
        ((Str []){__VA_ARGS__})[i] = 0; \
    } while(0);

///////////
// ENUMS //
///////////

/////////////
// STRUCTS //
/////////////

typedef struct Str {
    char *s;    /* string */
    size_t l;   /* length */
    size_t c;   /* capacity */
} Str;

////////////////////////////////
// PUBLIC FUNCTION PROTOTYPES //
////////////////////////////////

#define ERR_STR_POINTER     "expected pointer to str"
#define ERR_STR_APP         "failed appending string"
#define ERR_STR_TO_U8_POINT "failed conversion of string to u8 point"
#define ERR_STR_FROM_U8_POINT "failed conversion of string from u8 point"


Str *str_new_p(void);
ErrDecl str_app(Str *str, char *format, ...) ATTR_FORMAT(2, 3);
ErrDecl str_cap_ensure(Str *str, size_t cap);
char str_at(Str *str, size_t index) ATTR_PURE;
ErrDecl str_to_u8_point(char *in, U8Point *point);
ErrDecl str_from_u8_point(char out[6], U8Point *point);
void str_free_single(Str *str);
void str_recycle(Str *str);
int str_n_contains(char *s1, size_t s1_len, char *s2);

#define ERR_STR_GET_STR "failed getting string"
ErrDecl str_get_str(Str *str);

#define ERR_STR_GET_INT "failed getting integer"
ErrDecl str_get_int(Val *val);

#define STR_H
#endif
