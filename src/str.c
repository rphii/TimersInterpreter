/////////////
// HEADERS //
/////////////

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>

/* inclusion and configuration of vector */
#include "str.h"

#define VEC_SETTINGS_DEFAULT_SIZE STR_DEFAULT_SIZE
#define VEC_SETTINGS_KEEP_ZERO_END 1
#define VEC_SETTINGS_STRUCT_ITEMS s
#define VEC_SETTINGS_STRUCT_LAST l
#define VEC_SETTINGS_STRUCT_CAP c

VEC_IMPLEMENT(Str, str, char, BY_VAL, 0);


#include "trace.h"

//////////////////////////////////////
// PRIVATE FUNCTION IMPLEMENTATIONS //
//////////////////////////////////////

/////////////////////////////////////
// PUBLIC FUNCTION IMPLEMENTATIONS //
/////////////////////////////////////

Str *str_new_p(void)
{
    Str *result = malloc(sizeof(*result));
    if(!result) THROW("failed to allocate memory for str");
    memset(result, 0, sizeof(*result));
    return result;
error:
    return 0;
}

#if 1
int str_app(Str *str, char *format, ...)
{
    if(!str) THROW(ERR_STR_POINTER);
    if(!format) THROW(ERR_CSTR_POINTER);
    // calculate length of append string
    va_list argp;
    va_start(argp, format);
    size_t len_app = (size_t)vsnprintf(0, 0, format, argp);
    if((int)len_app < 0) {
        THROW("output error");
    }
    va_end(argp);
    // calculate required memory
    size_t len_new = str->l + len_app + 1;
    size_t required = str->cap ? str->cap : STR_DEFAULT_BLOCKSIZE;
    while(required < len_new) required = required << 1;
    // make sure to have enough memory
    if(required > str->cap)
    {
        char *temp = realloc(str->s, required);
        // safety check
        // apply address and set new allocd
        if(!temp) THROW("failed to realloc");
        str->s = temp;
        str->cap = required;
    }
    // actual append
    va_start(argp, format);
    int len_chng = vsnprintf(&(str->s)[str->l], len_app + 1, format, argp);
    va_end(argp);
    // check for success
    if(len_chng >= 0 && (size_t)len_chng <= len_app) {
        str->l += (size_t)len_chng; // successful, change length
    } else {
        THROW("encoding error or string hasn't been fully written");
    }
    return 0;
error:
    return -1;
}
#else
int str_fmt(Str *str, char *format, ...)
{
    if(!str) return -1;
    if(!format) return -1;
    // calculate length of append string
    va_list argp;
    va_start(argp, format);
    size_t len_app = (size_t)vsnprintf(0, 0, format, argp);
    if((int)len_app < 0) {
        return -1;
    }
    va_end(argp);
    // calculate required memory
    size_t len_new = str->l + len_app;
    if(str_reserve(str, len_new)) {
        return -1;
    }
    // actual append
    va_start(argp, format);
    int len_chng = vsnprintf(&(str->s)[str->l], len_app + 1, format, argp);
    va_end(argp);
    // check for success
    if(len_chng >= 0 && (size_t)len_chng <= len_app) {
        str->l += (size_t)len_chng; // successful, change length
    } else {
        return -1;
    }
    return 0;
}
#endif

#include <assert.h>
#include "trace.h"
char str_at(Str *str, size_t index)
{
    if(!str) THROW(ERR_STR_POINTER);
    if(index >= str->l) {
        trace_print();
        ABORT("accessing string index (%zu) out of bounds (%zu)\n", index, str->l);
    }
    //assert(index < str->l && "string");
    //assert(index < str->l && "accessing string index (%zu) out of bounds (%zu)", index, str->l);
    char ch = str->s[index];
    return ch;
error:
    exit(-1);
}

int str_cap_ensure(Str *str, size_t cap)
{
    if(!str) THROW(ERR_STR_POINTER);
    if(cap) {
        /* calculate required memory */
        size_t required = str->cap ? str->cap : STR_DEFAULT_BLOCKSIZE;
        while(required < cap) required *= 2;
        INFO("required %zu < cap %zu", required, cap);

        /* only increase capacity */
        if(required > str->cap) {
            void *temp = realloc(str->s, sizeof(*str->s) * cap);
            if(!temp) THROW("could not increase capacity of string");
            str->s = temp;
            str->cap = required;
        }
    }
    return 0;
error:
    return -1;
}

int str_to_u8_point(char *in, U8Point *point)
{
    if(!in) THROW(ERR_CSTR_POINTER);
    if(!point) THROW(ERR_UTF8_POINTER);
    U8Point tinker = {0};
    // figure out how many bytes we need
    if((*in & 0x80) == 0) point->bytes = 1;
    else if((*in & 0xE0) == 0xC0) point->bytes = 2;
    else if((*in & 0xF0) == 0xE0) point->bytes = 3;
    else if((*in & 0xF8) == 0xF0) point->bytes = 4;
    else THROW("unknown utf-8 pattern");
    // magical mask shifting
    int shift = (point->bytes - 1) * 6;
    int mask = 0x7F;
    if(point->bytes > 1) mask >>= point->bytes;
    // extract info from bytes
    for(int i = 0; i < point->bytes; i++) {
        // add number to point
        if(!in[i]) THROW(ERR_CSTR_INVALID);
        tinker.val |= (uint32_t)((in[i] & mask) << shift);
        if(mask == 0x3F) {
            if((unsigned char)(in[i] & ~mask) != 0x80) {
                THROW("encountered invalid bytes in utf-8 sequence");
                point->bytes = 0;
                break;
            }
        }
        // adjust shift amount
        shift -= 6;
        // update mask
        mask = 0x3F;
    }
    // one final check, unicode doesn't go that far, wth unicode, TODO check ?!
    if(tinker.val > 0x10FFFF || !point->bytes) {
        point->val = (unsigned char)*in;
        point->bytes = 1;
    } else {
        point->val = tinker.val;
    }
    return 0;
error:
    return -1;
}

ErrDecl str_from_u8_point(char out[6], U8Point *point)
{
    if(!out) THROW(ERR_CSTR_POINTER);
    if(!point) THROW(ERR_UTF8_POINTER);
    int bytes = 0;
    int shift = 0;  // shift in bits
    uint32_t in = point->val;
    // figure out how many bytes we need
    if(in < 0x0080) bytes = 1;
    else if(in < 0x0800) bytes = 2;
    else if(in < 0x10000) bytes = 3;
    else if(in < 0x200000) bytes = 4;
    else if(in < 0x4000000) bytes = 5;
    else if(in < 0x80000000) bytes = 6;
    shift = (bytes - 1) * 6;
    uint32_t mask = 0x7F;
    if(bytes > 1) mask >>= bytes;
    // create bytes
    for(int i = 0; i < bytes; i++)
    {
        // add actual character coding
        out[i] = (char)((in >> shift) & mask);
        // add first byte code
        if(!i && bytes > 1) {
            out[i] |= (char)(((uint32_t)~0 << (8 - bytes)) & 0xFF);
        }
        // add any other code
        if(i) {
            out[i] |= (char)0x80;
        }
        // adjust shift and reset mask
        shift -= 6;
        mask = 0x3F;
    }
    point->bytes = bytes;
    return 0;
error:
    return -1;
}

void str_free_single(Str *str)
{
    if(!str) return;
    free(str->s);
    str->s = 0;
    str->cap = 0;
    str->l = 0;
}

void str_recycle(Str *str)
{
    if(!str) return;
    str->l = 0;
}

ErrDecl str_get_str(Str *str)
{
    int err = 0;
    if(!str) THROW(ERR_STR_POINTER);
    int c = 0;
    str_recycle(str);
    fflush(stdin);
    while((c = getchar()) != '\n' && c != EOF) {
        TRY(str_app(str, "%c", (char)c), ERR_STR_APP);  /* append string */
    }
    if(!str->l && (!c || c == EOF || c == '\n')) {
        //THROW("an error"); /* TODO describe this error */
    }
clean:
    fflush(stdin);
    return err;
error: ERR_CLEAN;
}

ErrDecl str_get_int(Val *val)
{
    if(!val) THROW(ERR_VAL_POINTER);
    return 0;
error:
    return -1;
}

int str_n_contains(char *s1, size_t s1_len, char *s2)
{
    if(!s1 || !s2) return 0;
    for(size_t i = 0; i < s1_len; i++) {
        char c = s1[i];
        if(!c) return 0;
        if(strchr(s2, c)) return 1;
    }
    return 0;
}

