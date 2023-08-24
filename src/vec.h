/* MIT License

Copyright (c) 2023 rphii

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#ifndef VEC_H

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include <stdio.h>

#include "err.h"
#include "attr.h"
#include "trace.h"

#define ERR_VEC_POINTER   "expected pointer to vector"
#define ERR_VEC_PUSH_BACK "failed vec push back"
#define ERR_VEC_POP_FRONT "failed vec pop front"

/*#include "trace.h"*/

#ifndef vec_malloc
#define vec_malloc  malloc
#endif
#ifndef vec_realloc
#define vec_realloc realloc
#endif
#ifndef vec_memset
#define vec_memset  memset
#endif
#ifndef vec_memcpy
#define vec_memcpy  memcpy
#endif
#ifndef vec_memmove
#define vec_memmove memmove
#endif

#define VEC_DEBUG   1

/*
 * N = name - used for the type name
 * A = abbreviation - used for the functions
 * T = type - the actual type to base the vector off of
 * F = free - function to use if your structure itself has a custom free routine
 * M = mode - either BY_VAL or BY_REF
 */

#define VEC_DEFAULT_SIZE    4
#define VEC_TYPE_FREE(F)    (void (*)(void *))(F)

typedef enum
{
    VEC_ERROR_NONE,
    /* errors below */
    VEC_ERROR_MALLOC,
    VEC_ERROR_REALLOC,
    /* errors above */
    VEC_ERROR__COUNT,
} VecErrorList;

#define VEC_ITEM_BY_VAL(T)  T
#define VEC_ITEM_BY_REF(T)  T *
#define VEC_ITEM(T, M)  VEC_ITEM_##M(T)

#define VEC_INCLUDE(N, A, T, M) \
    typedef struct N { \
        VEC_ITEM(T, M)*items; \
        size_t len; \
        size_t cap; \
        size_t first; \
    } N; \
    \
    /* common implementation */ \
    void A##_clear(N *vec); \
    size_t A##_length(N *vec) ATTR_PURE; \
    size_t A##_capacity(N *vec) ATTR_PURE; \
    int A##_empty(N *vec) ATTR_PURE; \
    ErrDecl A##_resize(N *vec, size_t cap); \
    ErrDecl A##_shrink(N *vec); \
    /* split implementation */ \
    void A##_free(N *vec); \
    void A##_zero(N *vec); \
    size_t A##_reserved(N *vec) ATTR_PURE; \
    ErrDecl A##_reserve(N *vec, size_t cap); \
    void A##_set_at(N *vec, size_t index, VEC_ITEM(T, M) val); \
    ErrDecl A##_insert_at(N *vec, size_t index, VEC_ITEM(T, M) val); \
    ErrDecl A##_push_front(N *vec, VEC_ITEM(T, M) val); \
    ErrDecl A##_push_back(N *vec, VEC_ITEM(T, M) val); \
    void A##_pop_front(N *vec, T *val); \
    void A##_pop_back(N *vec, T *val); \
    VEC_ITEM(T, M) A##_get_at(N *vec, size_t index) ATTR_PURE; \
    VEC_ITEM(T, M) A##_get_front(N *vec) ATTR_PURE; \
    VEC_ITEM(T, M) A##_get_back(N *vec) ATTR_PURE; \
    VEC_ITEM(T, M)*A##_iter_begin(N *vec); \
    VEC_ITEM(T, M)*A##_iter_end(N *vec); \
    ErrDecl A##_copy(N *dst, N *src); \

/**********************************************************/
/* IMPLEMENTATION *****************************************/
/**********************************************************/

#define VEC_IMPLEMENT(N, A, T, M, F) \
    /* private */ \
    VEC_IMPLEMENT_COMMON_STATIC_F(N, A, T, F);              \
    VEC_IMPLEMENT_COMMON_STATIC_ZERO(N, A, T, F);           \
    VEC_IMPLEMENT_##M##_STATIC_GET(N, A, T, F);             \
    VEC_IMPLEMENT_##M##_STATIC_SHRINK_BACK(N, A, T, F);     \
    VEC_IMPLEMENT_##M##_STATIC_SHRINK_FRONT(N, A, T, F);    \
    /* public */ \
    VEC_IMPLEMENT_COMMON_CLEAR(N, A, T, F);         \
    VEC_IMPLEMENT_COMMON_LENGTH(N, A, T, F);        \
    VEC_IMPLEMENT_COMMON_CAPACITY(N, A, T, F);      \
    VEC_IMPLEMENT_COMMON_EMPTY(N, A, T, F);         \
    VEC_IMPLEMENT_COMMON_RESIZE(N, A, T, F);        \
    VEC_IMPLEMENT_COMMON_SHRINK(N, A, T, F);        \
    VEC_IMPLEMENT_COMMON_ITER_BEGIN(N, A, T, F, M); \
    VEC_IMPLEMENT_COMMON_ITER_END(N, A, T, F, M);   \
    VEC_IMPLEMENT_##M##_FREE(N, A, T, F);           \
    VEC_IMPLEMENT_##M##_ZERO(N, A, T, F);           \
    VEC_IMPLEMENT_##M##_RESERVED(N, A, T, F);       \
    VEC_IMPLEMENT_##M##_RESERVE(N, A, T, F);        \
    VEC_IMPLEMENT_##M##_SET_AT(N, A, T, F);         \
    VEC_IMPLEMENT_##M##_INSERT_AT(N, A, T, F);      \
    VEC_IMPLEMENT_##M##_PUSH_FRONT(N, A, T, F);     \
    VEC_IMPLEMENT_##M##_PUSH_BACK(N, A, T, F);      \
    VEC_IMPLEMENT_##M##_POP_FRONT(N, A, T, F);      \
    VEC_IMPLEMENT_##M##_POP_BACK(N, A, T, F);       \
    VEC_IMPLEMENT_##M##_GET_AT(N, A, T, F);         \
    VEC_IMPLEMENT_##M##_GET_FRONT(N, A, T, F);      \
    VEC_IMPLEMENT_##M##_GET_BACK(N, A, T, F);       \
    VEC_IMPLEMENT_##M##_COPY(N, A, T, F);           \

/**********************************************************/
/* PRIVATE FUNCTION IMPLEMENTATIONS ***********************/
/**********************************************************/

/* implementation for both */
#define VEC_IMPLEMENT_COMMON_STATIC_F(N, A, T, F) \
    static void (*A##_static_f)(void *) = F != 0 ? VEC_TYPE_FREE(F) : 0; \


/* implementation by val */
#define VEC_IMPLEMENT_BY_VAL_STATIC_GET(N, A, T, F) \
    static inline T *A##_static_get(N *vec, size_t index) \
    { \
        if(VEC_DEBUG) { \
            if(!((index < vec->len) && (index >= vec->first))) { \
                printf("\n>> accessing: index %zu, len %zu, first %zu\n", index, vec->len, vec->first); \
                /*trace_print();*/ \
            } \
        } \
        assert(vec); \
        assert(index < vec->len); \
        assert(index >= vec->first); \
        return &vec->items[index]; \
    }

#define VEC_IMPLEMENT_BY_VAL_STATIC_SHRINK_BACK(N, A, T, F) \
    static int A##_static_shrink_back(N *vec) \
    { \
        assert(vec); \
        size_t cap = vec->cap; \
        size_t len = vec->len; \
        size_t required = vec->cap ? vec->cap : VEC_DEFAULT_SIZE;\
        while(required > len) required /= 2; \
        required *= 2; \
        if(required  < vec->cap) { \
            if(F != 0) { \
                for(size_t i = required; i < cap; i++) { \
                    A##_static_f(VEC_TYPE_FREE(&vec->items[i])); \
                } \
            } \
            void *temp = vec_realloc(vec->items, sizeof(*vec->items) * required); \
            if(!temp) return VEC_ERROR_REALLOC; \
            vec->items = temp; \
            vec->cap = required; \
        } \
        return VEC_ERROR_NONE; \
    }

#define VEC_IMPLEMENT_BY_VAL_STATIC_SHRINK_FRONT(N, A, T, F) \
    static void A##_static_shrink_front(N *vec) \
    { \
        assert(vec); \
        size_t first = vec->first; \
        if(first) { \
            vec->first = 0; \
            T *item = A##_static_get(vec, 0); \
            if(F != 0) { \
                for(size_t i = 0; i < first; i++) { \
                    A##_static_f(VEC_TYPE_FREE(&vec->items[i])); \
                } \
            } \
            vec_memmove(item, item + first, sizeof(T) * (vec->len - first)); \
            /* TODO: is that really needed? */ vec_memset(item + vec->len - first, 0, sizeof(T) * (first)); \
            vec->len -= first; \
        } \
    }

/* implementation by ref */
#define VEC_IMPLEMENT_BY_REF_STATIC_GET(N, A, T, F) \
    static inline T **A##_static_get(N *vec, size_t index) \
    { \
        assert(vec); \
        if(VEC_DEBUG) { \
            if(!((index < vec->len) && (index >= vec->first))) { \
                printf("\n>> accessing: index %zu, len %zu, first %zu\n", index, vec->len, vec->first); \
                /*trace_print();*/ \
            } \
        } \
        assert(index < vec->len); \
        assert(index >= vec->first); \
        return &vec->items[index]; \
    }

#define VEC_IMPLEMENT_BY_REF_STATIC_SHRINK_BACK(N, A, T, F) \
    static inline int A##_static_shrink_back(N *vec) \
    { \
        assert(vec); \
        size_t cap = vec->cap; \
        size_t len = vec->len; \
        size_t required = vec->cap ? vec->cap : VEC_DEFAULT_SIZE;\
        while(required > len) required /= 2; \
        required *= 2; \
        if(required < vec->cap) { \
            for(size_t i = required; i < cap; i++) { \
                if(F != 0) { \
                    A##_static_f(VEC_TYPE_FREE(vec->items[i])); \
                } \
                free(vec->items[i]); \
            } \
            void *temp = vec_realloc(vec->items, sizeof(*vec->items) * required); \
            if(!temp) return VEC_ERROR_REALLOC; \
            vec->items = temp; \
            vec->cap = required; \
        } \
        return VEC_ERROR_NONE; \
    }

#define VEC_IMPLEMENT_BY_REF_STATIC_SHRINK_FRONT(N, A, T, F) \
    static inline void A##_static_shrink_front(N *vec) \
    { \
        assert(vec); \
        size_t first = vec->first; \
        if(first) { \
            vec->first = 0; \
            T **item = A##_static_get(vec, 0); \
            if(F != 0) { \
                for(size_t i = 0; i < first; i++) { \
                    A##_static_f(VEC_TYPE_FREE(vec->items[i])); \
                } \
            } \
            T **residuals = malloc(sizeof(T *) * first); \
            vec_memcpy(residuals, item, sizeof(T *) * (first)); \
            vec_memmove(item, item + first, sizeof(T *) * (vec->len - first)); \
            vec_memcpy(item + vec->len - first, residuals, sizeof(T *) * (first)); \
            free(residuals); \
            vec->len -= first; \
        } \
    }


/**********************************************************/
/* PUBLIC FUNCTION IMPLEMENTATIONS ************************/
/**********************************************************/

/* implementation for both */
#define VEC_IMPLEMENT_COMMON_STATIC_ZERO(N, A, T, F) \
    static void A##_static_zero(N *vec) \
    { \
        assert(vec); \
        vec_memset(vec, 0, sizeof(*vec)); \
    }

#define VEC_IMPLEMENT_COMMON_CLEAR(N, A, T, F) \
    inline void A##_clear(N *vec) \
    { \
        assert(vec); \
        vec->first = 0; \
        vec->len = 0; \
    }

#define VEC_IMPLEMENT_COMMON_LENGTH(N, A, T, F) \
    inline size_t A##_length(N *vec) \
    { \
        assert(vec); \
        return vec->len - vec->first; \
    }

#define VEC_IMPLEMENT_COMMON_CAPACITY(N, A, T, F) \
    inline size_t A##_capacity(N *vec) \
    { \
        assert(vec); \
        return vec->cap; \
    }

#define VEC_IMPLEMENT_COMMON_EMPTY(N, A, T, F) \
    inline int A##_empty(N *vec) \
    { \
        assert(vec); \
        return (vec->first == vec->len); \
    }
#define VEC_IMPLEMENT_COMMON_RESIZE(N, A, T, F) \
    inline int A##_resize(N *vec, size_t cap) \
    { \
        assert(vec); \
        int result = VEC_ERROR_NONE; \
        A##_static_shrink_front(vec); \
        result |= result ?: A##_reserve(vec, cap); \
        result |= result ?: A##_static_shrink_back(vec); \
        vec->len = cap; \
        return result; \
    }

#define VEC_IMPLEMENT_COMMON_SHRINK(N, A, T, F) \
    inline int A##_shrink(N *vec) \
    { \
        assert(vec); \
        A##_static_shrink_front(vec); \
        int result = A##_static_shrink_back(vec); \
        return result; \
    }

#define VEC_IMPLEMENT_COMMON_ITER_BEGIN(N, A, T, F, M) \
    inline VEC_ITEM(T, M)*A##_iter_begin(N *vec) \
    { \
        assert(vec); \
        assert(vec->first <= vec->len); \
        return vec->items + vec->first; \
    }

#define VEC_IMPLEMENT_COMMON_ITER_END(N, A, T, F, M) \
    inline VEC_ITEM(T, M)*A##_iter_end(N *vec) \
    { \
        assert(vec); \
        assert(vec->first <= vec->len); \
        return vec->items + vec->len; \
    }

/* implementation by value */
#define VEC_IMPLEMENT_BY_VAL_FREE(N, A, T, F) \
    inline void A##_free(N *vec) \
    { \
        assert(vec); \
        if(F != 0) { \
            for(size_t i = 0; i < vec->cap; i++) { \
                A##_static_f(VEC_TYPE_FREE(&vec->items[i])); \
            } \
        } \
        free(vec->items); \
        A##_static_zero(vec); \
    }

#define VEC_IMPLEMENT_BY_VAL_ZERO(N, A, T, F) \
    inline void A##_zero(N *vec) \
    { \
        assert(vec); \
        vec_memset(vec->items, 0, sizeof(*vec->items) * vec->cap); \
    }

#define VEC_IMPLEMENT_BY_VAL_RESERVED(N, A, T, F) \
    inline size_t A##_reserved(N *vec) \
    { \
        assert(vec); \
        size_t result = 0; \
        result += (sizeof(*vec->items) * vec->cap); \
        return result; \
    }

#define VEC_IMPLEMENT_BY_VAL_RESERVE(N, A, T, F) \
    inline int A##_reserve(N *vec, size_t cap) \
    { \
        assert(vec); \
        size_t len = vec->cap; \
        if(cap > len) { \
            size_t required = vec->cap ? vec->cap : VEC_DEFAULT_SIZE;\
            while(required < cap) required *= 2; \
            if(required > vec->cap) { \
                void *temp = vec_realloc(vec->items, sizeof(*vec->items) * required); \
                if(!temp) return VEC_ERROR_REALLOC; \
                vec->items = temp; \
                vec_memset(&vec->items[len], 0, sizeof(*vec->items) * (required - len)); \
                vec->cap = required; \
            } \
        } \
        return VEC_ERROR_NONE; \
    }

#define VEC_IMPLEMENT_BY_VAL_SET_AT(N, A, T, F) \
    void A##_set_at(N *vec, size_t index, T val) \
    { \
        assert(vec); \
        T *item = A##_static_get(vec, index + vec->first); \
        assert(item); \
        if(F != 0) A##_static_f(item); \
        vec_memcpy(item, &val, sizeof(T)); \
    }

#define VEC_IMPLEMENT_BY_VAL_INSERT_AT(N, A, T, F) \
    int A##_insert_at(N *vec, size_t index, T val) \
    { \
        assert(vec); \
        int result = A##_reserve(vec, index + 1); \
        if(result) return result; \
        vec->len++; \
        T *item = A##_static_get(vec, index + vec->first); \
        vec_memmove(item + 1, item, sizeof(T) * (++vec->len - index)); \
        vec_memcpy(item, &val, sizeof(T)); \
        return VEC_ERROR_NONE; \
    }

#define VEC_IMPLEMENT_BY_VAL_PUSH_FRONT(N, A, T, F) \
    inline int A##_push_front(N *vec, T val) \
    { \
        assert(vec); \
        int result = A##_reserve(vec, vec->len + 1); \
        if(result) return result; \
        size_t len = vec->len++ + vec->first; \
        T *item = A##_static_get(vec, vec->first); \
        vec_memmove(item + 1, item, sizeof(T) * len); \
        vec_memcpy(item, &val, sizeof(T)); \
        return VEC_ERROR_NONE; \
    }

#define VEC_IMPLEMENT_BY_VAL_PUSH_BACK(N, A, T, F) \
    inline int A##_push_back(N *vec, T val) \
    { \
        assert(vec); \
        int result = A##_reserve(vec, vec->len + 1); \
        if(result) return result; \
        size_t index = vec->len++; \
        T *item = A##_static_get(vec, index); \
        vec_memcpy(item, &val, sizeof(T)); \
        return VEC_ERROR_NONE; \
    }

#define VEC_IMPLEMENT_BY_VAL_POP_FRONT(N, A, T, F) \
    inline void A##_pop_front(N *vec, T *val) \
    { \
        assert(vec); \
        assert(vec->len > vec->first); \
        if(val) { \
            size_t index = vec->first; \
            T *item = A##_static_get(vec, index); \
            vec_memcpy(val, item, sizeof(T)); \
        } \
        vec->first++; \
        /*return VEC_ERROR_NONE;*/ \
    }

#define VEC_IMPLEMENT_BY_VAL_POP_BACK(N, A, T, F) \
    inline void A##_pop_back(N *vec, T *val) \
    { \
        assert(vec); \
        if(!(vec->len > vec->first)) trace_print(); \
        assert(vec->len > vec->first); \
        if(val) { \
            size_t index = vec->len - 1; \
            T *item = A##_static_get(vec, index); \
            vec_memcpy(val, item, sizeof(T)); \
        } \
        vec->len--; \
        /*return A##_shrink(vec);*/ \
    }

#define VEC_IMPLEMENT_BY_VAL_GET_AT(N, A, T, F) \
    inline T A##_get_at(N *vec, size_t index) \
    { \
        assert(vec); \
        return *A##_static_get(vec, index + vec->first); \
    }

#define VEC_IMPLEMENT_BY_VAL_GET_FRONT(N, A, T, F) \
    inline T A##_get_front(N *vec) \
    { \
        assert(vec); \
        return *A##_static_get(vec, vec->first); \
    }

#define VEC_IMPLEMENT_BY_VAL_GET_BACK(N, A, T, F) \
    inline T A##_get_back(N *vec) \
    { \
        assert(vec); \
        return *A##_static_get(vec, vec->len - 1); \
    }

#define VEC_IMPLEMENT_BY_VAL_COPY(N, A, T, F) \
    inline int A##_copy(N *dst, N *src) \
    { \
        assert(dst); \
        assert(src); \
        assert(dst != src); \
        A##_clear(dst); \
        int result = A##_reserve(dst, A##_length(src)); \
        if(result) return result; \
        vec_memcpy(dst->items, src->items, sizeof(*dst->items) * A##_length(src)); \
        dst->len = A##_length(src); \
        return 0; \
    }

/* implementaiton by reference */
#define VEC_IMPLEMENT_BY_REF_FREE(N, A, T, F) \
    inline void A##_free(N *vec) \
    { \
        assert(vec); \
        for(size_t i = 0; i < vec->cap; i++) { \
            if(F != 0) { \
                A##_static_f(VEC_TYPE_FREE(vec->items[i])); \
            } \
            free(vec->items[i]); \
        } \
        free(vec->items); \
        A##_static_zero(vec); \
    }

#define VEC_IMPLEMENT_BY_REF_ZERO(N, A, T, F) \
    inline void A##_zero(N *vec) \
    { \
        assert(vec); \
        for(size_t i = 0; i < vec->cap; i++) { \
            vec_memset(&vec->items[i], 0, sizeof(*vec->items)); \
        } \
    }

#define VEC_IMPLEMENT_BY_REF_RESERVED(N, A, T, F) \
    inline size_t A##_reserved(N *vec) \
    { \
        assert(vec); \
        size_t result = 0; \
        result += (sizeof(**vec->items) * vec->cap); \
        result += (sizeof(*vec->items) * vec->cap); \
        return result; \
    }

#define VEC_IMPLEMENT_BY_REF_RESERVE(N, A, T, F) \
    inline int A##_reserve(N *vec, size_t cap) \
    { \
        assert(vec); \
        size_t len = vec->cap; \
        size_t required = vec->cap ? vec->cap : VEC_DEFAULT_SIZE;\
        while(required < cap) required *= 2; \
        if(required > vec->cap) { \
            void *temp = vec_realloc(vec->items, sizeof(*vec->items) * required); \
            if(!temp) return VEC_ERROR_REALLOC; \
            vec->items = temp; \
            vec_memset(&vec->items[len], 0, sizeof(*vec->items) * (required - len)); \
            for(size_t i = len; i < required; i++) { \
                vec->items[i] = vec_malloc(sizeof(**vec->items)); \
                if(!vec->items[i]) return VEC_ERROR_MALLOC; \
                vec_memset(vec->items[i], 0, sizeof(**vec->items)); \
            } \
            vec->cap = required; \
        } \
        return VEC_ERROR_NONE; \
    }

#define VEC_IMPLEMENT_BY_REF_SET_AT(N, A, T, F) \
    void A##_set_at(N *vec, size_t index, T *val) \
    { \
        assert(vec); \
        assert(val); \
        T *item = *A##_static_get(vec, index + vec->first); \
        if(F != 0) A##_static_f(item); \
        vec_memcpy(item, val, sizeof(T)); \
    }

#define VEC_IMPLEMENT_BY_REF_INSERT_AT(N, A, T, F) \
    int A##_insert_at(N *vec, size_t index, T *val) \
    { \
        assert(vec); \
        assert(val); \
        int result = A##_reserve(vec, index + 1); \
        if(result) return result; \
        vec->len++; \
        T **item = A##_static_get(vec, index + vec->first); \
        vec_memmove(item + 1, item, sizeof(T *) * (++vec->len - index)); \
        vec_memcpy(*item, val, sizeof(T)); \
        return VEC_ERROR_NONE; \
    }

#define VEC_IMPLEMENT_BY_REF_PUSH_FRONT(N, A, T, F) \
    inline int A##_push_front(N *vec, T *val) \
    { \
        assert(vec); \
        assert(val); \
        int result = A##_reserve(vec, vec->len + 1); \
        if(result) return result; \
        size_t len = vec->len++ - vec->first; \
        T **item = A##_static_get(vec, vec->first); \
        vec_memmove(item + 1, item, sizeof(T *) * len); \
        vec_memcpy(*item, val, sizeof(T)); \
        return VEC_ERROR_NONE; \
    }

#define VEC_IMPLEMENT_BY_REF_PUSH_BACK(N, A, T, F) \
    inline int A##_push_back(N *vec, T *val) \
    { \
        assert(vec); \
        assert(val); \
        int result = A##_reserve(vec, vec->len + 1); \
        if(result) return result; \
        size_t index = vec->len++; \
        T *item = *A##_static_get(vec, index); \
        vec_memcpy(item, val, sizeof(T)); \
        return VEC_ERROR_NONE; \
    }

#define VEC_IMPLEMENT_BY_REF_POP_FRONT(N, A, T, F) \
    inline void A##_pop_front(N *vec, T *val) \
    { \
        assert(vec); \
        assert(vec->len > vec->first); \
        if(val) { \
            size_t index = vec->first; \
            T *item = *A##_static_get(vec, index); \
            vec_memcpy(val, item, sizeof(T)); \
        } \
        vec->first++; \
        /*return VEC_ERROR_NONE;*/ \
    }

#define VEC_IMPLEMENT_BY_REF_POP_BACK(N, A, T, F) \
    inline void A##_pop_back(N *vec, T *val) \
    { \
        assert(vec); \
        assert(vec->len > vec->first); \
        if(val) { \
            size_t index = vec->len - 1; \
            T *item = *A##_static_get(vec, index); \
            vec_memcpy(val, item, sizeof(T)); \
        } \
        vec->len--; \
        /*return A##_shrink(vec);*/ \
    }

#define VEC_IMPLEMENT_BY_REF_GET_AT(N, A, T, F) \
    inline T *A##_get_at(N *vec, size_t index) \
    { \
        assert(vec); \
        return *A##_static_get(vec, index + vec->first); \
    }

#define VEC_IMPLEMENT_BY_REF_GET_FRONT(N, A, T, F) \
    inline T *A##_get_front(N *vec) \
    { \
        assert(vec); \
        return *A##_static_get(vec, vec->first); \
    }

#define VEC_IMPLEMENT_BY_REF_GET_BACK(N, A, T, F) \
    inline T *A##_get_back(N *vec) \
    { \
        assert(vec); \
        return *A##_static_get(vec, vec->len - 1); \
    }

#define VEC_IMPLEMENT_BY_REF_COPY(N, A, T, F) \
    inline int A##_copy(N *dst, N *src) \
    { \
        assert(dst); \
        assert(src); \
        assert(dst != src); \
        A##_clear(dst); \
        int result = A##_reserve(dst, A##_length(src)); \
        if(result) return result; \
        for(size_t i = 0; i < A##_length(src); i++) { \
            result |= result ?: A##_push_back(dst, A##_get_at(src, i)); \
        } \
        return result; \
    }


VEC_INCLUDE(VecSize, vec_size, size_t, BY_VAL);

#define VEC_H
#endif

