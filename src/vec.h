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

//#ifndef VEC_H

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

#undef VEC_DEFAULT_SIZE
#undef VEC_STRUCT_ITEMS
#undef VEC_STRUCT_LAST
#undef VEC_STRUCT_CAP
#undef VEC_KEEP_ZERO_END

#ifdef  VEC_SETTINGS_DEFAULT_SIZE
#define VEC_DEFAULT_SIZE VEC_SETTINGS_DEFAULT_SIZE
#else
#define VEC_DEFAULT_SIZE    4
#endif

#ifdef  VEC_SETTINGS_STRUCT_ITEMS
#define VEC_STRUCT_ITEMS VEC_SETTINGS_STRUCT_ITEMS
#else
#define VEC_STRUCT_ITEMS items
#endif

#ifdef  VEC_SETTINGS_STRUCT_LAST
#define VEC_STRUCT_LAST VEC_SETTINGS_STRUCT_LAST
#else
#define VEC_STRUCT_LAST len
#endif

#ifdef  VEC_SETTINGS_STRUCT_CAP
#define VEC_STRUCT_CAP VEC_SETTINGS_STRUCT_CAP
#else
#define VEC_STRUCT_CAP cap
#endif

#ifdef  VEC_SETTINGS_KEEP_ZERO_END
#define VEC_KEEP_ZERO_END VEC_SETTINGS_KEEP_ZERO_END
#else
#define VEC_KEEP_ZERO_END 0
#endif


#ifndef VEC_H

#define VEC_CAST_FREE(X)        ((void *)(X))
#define VEC_TYPE_FREE(F,X,T)    ((void (*)(T *))(F))(VEC_CAST_FREE(X))

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

#define VEC_REF_BY_VAL   &
#define VEC_REF_BY_REF
#define VEC_REF(M)       VEC_REF_##M

#define VEC_ASSERT_BY_REF(x)    assert(x)
#define VEC_ASSERT_BY_VAL(x)
#define VEC_ASSERT(x, M)        VEC_ASSERT_##M(x)

#define VEC_H
#endif

/*
 * === THE VECTOR DESCRIPTION ===
 * N = name - used for the type name
 * A = abbreviation - used for the functions
 * T = type - the actual type to base the vector off of
 * F = free - function to use if your structure itself has a custom free routine
 * M = mode - either BY_VAL or BY_REF
 */

#define VEC_INCLUDE(N, A, T, M) \
    typedef struct N { \
        size_t cap; \
        size_t VEC_STRUCT_LAST; \
        size_t first; \
        VEC_ITEM(T, M) *VEC_STRUCT_ITEMS; \
    } N; \
    \
    /* common implementation */ \
    void A##_clear(N *vec); \
    size_t A##_length(N *vec); \
    size_t A##_capacity(N *vec); \
    int A##_empty(N *vec); \
    int A##_resize(N *vec, size_t cap); \
    int A##_shrink(N *vec); \
    void A##_pop_front(N *vec, T *val); \
    void A##_pop_back(N *vec, T *val); \
    void A##_set_at(N *vec, size_t index, VEC_ITEM(T, M) val); \
    void A##_swap(N *vec, size_t i1, size_t i2); \
    void A##_reverse(N *vec); \
    int A##_insert_at(N *vec, size_t index, VEC_ITEM(T, M) val); \
    int A##_push_front(N *vec, VEC_ITEM(T, M) val); \
    int A##_push_back(N *vec, VEC_ITEM(T, M) val); \
    VEC_ITEM(T, M) A##_get_at(N *vec, size_t index); \
    VEC_ITEM(T, M) A##_get_front(N *vec); \
    VEC_ITEM(T, M) A##_get_back(N *vec); \
    VEC_ITEM(T, M)*A##_iter_begin(N *vec); \
    VEC_ITEM(T, M)*A##_iter_end(N *vec); \
    /* split implementation */ \
    void A##_free(N *vec); \
    void A##_zero(N *vec); \
    size_t A##_reserved(N *vec); \
    int A##_reserve(N *vec, size_t cap); \
    int A##_copy(N *dst, N *src); \

/*
 * int A##_cmp(N *a, N *b) -> compare vec
 * ssize_t A##_find(N *vec, ssize_t index) -> find item
 * ssize_t A##_match(N *vec,  -> find vec
 */

/**********************************************************/
/* IMPLEMENTATION *****************************************/
/**********************************************************/

#define VEC_IMPLEMENT(N, A, T, M, F) \
    /* private */ \
    /*VEC_IMPLEMENT_COMMON_STATIC_F(N, A, T, F);              */\
    VEC_IMPLEMENT_COMMON_STATIC_ZERO(N, A, T, F);           \
    VEC_IMPLEMENT_COMMON_STATIC_GET(N, A, T, F, M);             \
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
    VEC_IMPLEMENT_COMMON_POP_FRONT(N, A, T, F, M);      \
    VEC_IMPLEMENT_COMMON_POP_BACK(N, A, T, F, M);       \
    VEC_IMPLEMENT_COMMON_GET_AT(N, A, T, F, M);         \
    VEC_IMPLEMENT_COMMON_GET_FRONT(N, A, T, F, M);      \
    VEC_IMPLEMENT_COMMON_GET_BACK(N, A, T, F, M);       \
    VEC_IMPLEMENT_COMMON_SET_AT(N, A, T, F, M);         \
    VEC_IMPLEMENT_COMMON_PUSH_FRONT(N, A, T, F, M);     \
    VEC_IMPLEMENT_COMMON_PUSH_BACK(N, A, T, F, M);      \
    VEC_IMPLEMENT_COMMON_INSERT_AT(N, A, T, F, M);      \
    VEC_IMPLEMENT_COMMON_SWAP(N, A, T, F, M);      \
    VEC_IMPLEMENT_COMMON_REVERSE(N, A, T, F);       \
    VEC_IMPLEMENT_##M##_FREE(N, A, T, F);           \
    VEC_IMPLEMENT_##M##_ZERO(N, A, T, F);           \
    VEC_IMPLEMENT_##M##_RESERVED(N, A, T, F);       \
    VEC_IMPLEMENT_##M##_RESERVE(N, A, T, F);        \
    VEC_IMPLEMENT_##M##_COPY(N, A, T, F);           \

/**********************************************************/
/* PRIVATE FUNCTION IMPLEMENTATIONS ***********************/
/**********************************************************/

/* implementation for both */

/**
 * @brief A##_static_f [COMMON] - internal use, sets up the optional freeing function
 */
/*#define VEC_IMPLEMENT_COMMON_STATIC_F(N, A, T, F) \
    static void (*A##_static_f)(void *) = F != 0 ? VEC_TYPE_FREE(F) : 0; \
    */


/* implementation by val */

/**
 * @brief A##_static_get [COMMON] - internal use, get reference to item at index
 * @param vec - the vector
 * @param index - the index
 * @return pointer to item (by reference) at index
 */
#define VEC_IMPLEMENT_COMMON_STATIC_GET(N, A, T, F, M) \
    static inline VEC_ITEM(T, M) *A##_static_get(N *vec, size_t index) \
    { \
        assert(vec); \
        assert(index < vec->VEC_STRUCT_LAST); \
        assert(index >= vec->first); \
        return &vec->VEC_STRUCT_ITEMS[index]; \
    }

/**
 * @brief A##_static_shrink_back [BY_VAL] - internal use, shrink end of vector (resize)
 * @param vec - the vector
 * @return zero if success, non-zero if failure
 */
#define VEC_IMPLEMENT_BY_VAL_STATIC_SHRINK_BACK(N, A, T, F) \
    static int A##_static_shrink_back(N *vec) \
    { \
        assert(vec); \
        size_t cap = vec->cap; \
        size_t len = vec->VEC_STRUCT_LAST; \
        size_t required = vec->cap ? vec->cap : VEC_DEFAULT_SIZE;\
        while(required > len) required /= 2; \
        required *= 2; \
        if(required  < vec->cap) { \
            if(F != 0) { \
                for(size_t i = required; i < cap; i++) { \
                    VEC_TYPE_FREE(F, &vec->VEC_STRUCT_ITEMS[i], T); \
                } \
            } \
            void *temp = vec_realloc(vec->VEC_STRUCT_ITEMS, sizeof(*vec->VEC_STRUCT_ITEMS) * required); \
            if(!temp) return VEC_ERROR_REALLOC; \
            vec->VEC_STRUCT_ITEMS = temp; \
            vec->cap = required; \
        } \
        return VEC_ERROR_NONE; \
    }

/**
 * @brief A##_static_shrink_front [BY_VAL] - internal use, shrink vector at beginning (resize)
 * @param vec - the vector
 * @return zero if success, non-zero if failure
 */
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
                    VEC_TYPE_FREE(F, &vec->VEC_STRUCT_ITEMS[i], T); \
                } \
            } \
            vec_memmove(item, item + first, sizeof(T) * (vec->VEC_STRUCT_LAST - first)); \
            /* TODO: is that really needed? */ vec_memset(item + vec->VEC_STRUCT_LAST - first, 0, sizeof(T) * (first)); \
            vec->VEC_STRUCT_LAST -= first; \
        } \
    }

/* implementation by ref */

/**
 * @brief A##_static_shrink_back [BY_REF] - internal use, shrink end of vector (resize)
 * @param vec - the vector
 * @return zero if success, non-zero if failure
 */
#define VEC_IMPLEMENT_BY_REF_STATIC_SHRINK_BACK(N, A, T, F) \
    static inline int A##_static_shrink_back(N *vec) \
    { \
        assert(vec); \
        size_t cap = vec->cap; \
        size_t len = vec->VEC_STRUCT_LAST; \
        size_t required = vec->cap ? vec->cap : VEC_DEFAULT_SIZE;\
        while(required > len) required /= 2; \
        required *= 2; \
        if(required < vec->cap) { \
            for(size_t i = required; i < cap; i++) { \
                if(F != 0) { \
                    VEC_TYPE_FREE(F, vec->VEC_STRUCT_ITEMS[i], T); \
                } \
                free(vec->VEC_STRUCT_ITEMS[i]); \
            } \
            void *temp = vec_realloc(vec->VEC_STRUCT_ITEMS, sizeof(*vec->VEC_STRUCT_ITEMS) * required); \
            if(!temp) return VEC_ERROR_REALLOC; \
            vec->VEC_STRUCT_ITEMS = temp; \
            vec->cap = required; \
        } \
        return VEC_ERROR_NONE; \
    }

/**
 * @brief A##_static_shrink_front [BY_REF] - internal use, shrink vector at beginning (resize)
 * @param vec - the vector
 * @return zero if success, non-zero if failure
 */
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
                    VEC_TYPE_FREE(F, vec->VEC_STRUCT_ITEMS[i], T); \
                } \
            } \
            T **residuals = malloc(sizeof(T *) * first); \
            vec_memcpy(residuals, item, sizeof(T *) * (first)); \
            vec_memmove(item, item + first, sizeof(T *) * (vec->VEC_STRUCT_LAST - first)); \
            vec_memcpy(item + vec->VEC_STRUCT_LAST - first, residuals, sizeof(T *) * (first)); \
            free(residuals); \
            vec->VEC_STRUCT_LAST -= first; \
        } \
    }


/**********************************************************/
/* PUBLIC FUNCTION IMPLEMENTATIONS ************************/
/**********************************************************/

/* implementation for both */

/**
 * @brief A##_static_zero [COMMON] - set the vector struct without freeing to zero
 * @param vec - the vector
 * @return void
 */
#define VEC_IMPLEMENT_COMMON_STATIC_ZERO(N, A, T, F) \
    static void A##_static_zero(N *vec) \
    { \
        assert(vec); \
        vec_memset(vec, 0, sizeof(*vec)); \
    }

/**
 * @brief A##_clear [COMMON] - set the vector length to zero without leaving it's allocated memory unchanged (aka. recycle)
 * @param vec - the vector
 * @return void
 */
#define VEC_IMPLEMENT_COMMON_CLEAR(N, A, T, F) \
    inline void A##_clear(N *vec) \
    { \
        assert(vec); \
        vec->first = 0; \
        vec->VEC_STRUCT_LAST = 0; \
    }

/**
 * @brief A##_length [COMMON] - get the indexed vector length (in items)
 * @param vec - the vector
 * @return length in items
 */
#define VEC_IMPLEMENT_COMMON_LENGTH(N, A, T, F) \
    inline size_t A##_length(N *vec) \
    { \
        assert(vec); \
        return vec->VEC_STRUCT_LAST - vec->first; \
    }

/**
 * @brief A##_capacity [COMMON] - get number of allocated item spaces (not in bytes)
 * @param vec - the vector
 * @return capacity in item spaces
 */
#define VEC_IMPLEMENT_COMMON_CAPACITY(N, A, T, F) \
    inline size_t A##_capacity(N *vec) \
    { \
        assert(vec); \
        return vec->cap; \
    }

/**
 * @brief A##_empty [COMMON] - check if vector is empty
 * @param vec - the vector
 * @return boolean comparison: true if empty, false if not empty
 */
#define VEC_IMPLEMENT_COMMON_EMPTY(N, A, T, F) \
    inline int A##_empty(N *vec) \
    { \
        assert(vec); \
        return (vec->first == vec->VEC_STRUCT_LAST); \
    }

/**
 * @brief A##_resize [COMMON] - resize vector to be able to hold a certain number of items
 * @param vec - the vector
 * @param cap - certain number of items
 * @return zero if success, non-zero if failure
 */
#define VEC_IMPLEMENT_COMMON_RESIZE(N, A, T, F) \
    inline int A##_resize(N *vec, size_t cap) \
    { \
        assert(vec); \
        int result = VEC_ERROR_NONE; \
        A##_static_shrink_front(vec); \
        result |= result ?: A##_reserve(vec, cap); \
        vec->VEC_STRUCT_LAST = cap; \
        result |= result ?: A##_static_shrink_back(vec); \
        return result; \
    }

/**
 * @brief A##_shrink [COMMON] - shrink memory usage to minimum number of items while still capable of holding the current number of items in use
 * @param vec - the vector
 * @return zero if success, non-zero if failure
 */
#define VEC_IMPLEMENT_COMMON_SHRINK(N, A, T, F) \
    inline int A##_shrink(N *vec) \
    { \
        assert(vec); \
        A##_static_shrink_front(vec); \
        int result = A##_static_shrink_back(vec); \
        return result; \
    }

/**
 * @brief A##_iter_begin [COMMON] - get pointer to first item
 * @param vec - the vector
 * @return pointer to first item
 */
#define VEC_IMPLEMENT_COMMON_ITER_BEGIN(N, A, T, F, M) \
    inline VEC_ITEM(T, M)*A##_iter_begin(N *vec) \
    { \
        assert(vec); \
        assert(vec->first <= vec->VEC_STRUCT_LAST); \
        return vec->VEC_STRUCT_ITEMS + vec->first; \
    }

/**
 * @brief A##_iter_end [COMMON] - get pointer to one past len item
 * @param vec - the vector
 * @return pointer to one past len item
 */
#define VEC_IMPLEMENT_COMMON_ITER_END(N, A, T, F, M) \
    inline VEC_ITEM(T, M)*A##_iter_end(N *vec) \
    { \
        assert(vec); \
        assert(vec->first <= vec->VEC_STRUCT_LAST); \
        return vec->VEC_STRUCT_ITEMS + vec->VEC_STRUCT_LAST; \
    }

/**
 * @brief A##_pop_front [COMMON] - pop one item from the front (and adjust first index)
 * @param vec - the vector
 * @return val - write back for popped value, pass 0 to ignore
 * @return void
 */
#define VEC_IMPLEMENT_COMMON_POP_FRONT(N, A, T, F, M) \
    inline void A##_pop_front(N *vec, T *val) \
    { \
        assert(vec); \
        assert(vec->VEC_STRUCT_LAST > vec->first); \
        if(val) { \
            size_t first = vec->first; \
            T *item = VEC_REF(M) *A##_static_get(vec, first); \
            vec_memcpy(val, item, sizeof(T)); \
        } \
        vec->first++; \
    }

/**
 * @brief A##_pop_back [COMMON] - pop one item from the back (and adjust length)
 * @param vec - the vector
 * @param val - write back for popped value, pass 0 to ignore
 * @return void
 */
#define VEC_IMPLEMENT_COMMON_POP_BACK(N, A, T, F, M) \
    inline void A##_pop_back(N *vec, T *val) \
    { \
        assert(vec); \
        assert(vec->VEC_STRUCT_LAST > vec->first); \
        if(val) { \
            size_t back = vec->VEC_STRUCT_LAST - 1; \
            T *item = VEC_REF(M) *A##_static_get(vec, back); \
            vec_memcpy(val, item, sizeof(T)); \
        } \
        vec->VEC_STRUCT_LAST--; \
    }

/**
 * @brief A##_get_at [COMMON] - get item at certain index
 * @param vec - the vector
 * @param index - the index
 * @return item (by value) at index
 */
#define VEC_IMPLEMENT_COMMON_GET_AT(N, A, T, F, M) \
    inline VEC_ITEM(T, M) A##_get_at(N *vec, size_t index) \
    { \
        assert(vec); \
        return *A##_static_get(vec, index + vec->first); \
    }

/**
 * @brief A##_get_front [COMMON] - get item at front
 * @param vec - the vector
 * @return item (by value) at front
 */
#define VEC_IMPLEMENT_COMMON_GET_FRONT(N, A, T, F, M) \
    inline VEC_ITEM(T, M) A##_get_front(N *vec) \
    { \
        assert(vec); \
        return *A##_static_get(vec, vec->first); \
    }

/**
 * @brief A##_get_back [COMMON] - get item at end
 * @param vec - the vector
 * @return item (by value) at end
 */
#define VEC_IMPLEMENT_COMMON_GET_BACK(N, A, T, F, M) \
    inline VEC_ITEM(T, M) A##_get_back(N *vec) \
    { \
        assert(vec); \
        return *A##_static_get(vec, vec->VEC_STRUCT_LAST - 1); \
    }

/**
 * @brief A##_set_at [COMMON] - overwrite one item at certain index
 * @param vec - the vector
 * @param index - the certain index
 * @param val - the value (by reference) to overwrite said index with
 * @return void
 */
#define VEC_IMPLEMENT_COMMON_SET_AT(N, A, T, F, M) \
    void A##_set_at(N *vec, size_t index, VEC_ITEM(T, M) val) \
    { \
        assert(vec); \
        VEC_ASSERT(val, M); \
        T *item = VEC_REF(M) *A##_static_get(vec, index + vec->first); \
        if(F != 0) VEC_TYPE_FREE(F, item, T); \
        vec_memcpy(item, VEC_REF(M) val, sizeof(T)); \
    }

/**
 * @brief A##_insert_at [COMMON] - add one item at index and move everything back
 * @param vec - the vector
 * @param index - the index
 * @param val - the value (by reference) to be written to said index
 * @return zero if success, non-zero if failure
 */
#define VEC_IMPLEMENT_COMMON_INSERT_AT(N, A, T, F, M) \
    int A##_insert_at(N *vec, size_t index, VEC_ITEM(T, M) val) \
    { \
        assert(vec); \
        VEC_ASSERT(val, M); \
        int result = A##_reserve(vec, vec->VEC_STRUCT_LAST + 1); \
        if(result) return result; \
        vec->VEC_STRUCT_LAST++; \
        VEC_ITEM(T, M) *item = A##_static_get(vec, index + vec->first); \
        vec_memmove(item + 1, item, sizeof(*item) * (vec->VEC_STRUCT_LAST - index - 1)); \
        vec_memcpy(VEC_REF(M) *item, VEC_REF(M) val, sizeof(T)); \
        return VEC_ERROR_NONE; \
    }

/**
 * @brief A##_push_front [COMMON] - push one item from the front and move everything back
 * @param vec - the vector
 * @param val - the value (by reference) to be pushed
 * @return zero if success, non-zero if failure
 */
#define VEC_IMPLEMENT_COMMON_PUSH_FRONT(N, A, T, F, M) \
    inline int A##_push_front(N *vec, VEC_ITEM(T, M) val) \
    { \
        assert(vec); \
        VEC_ASSERT(val, M); \
        int result = A##_reserve(vec, vec->VEC_STRUCT_LAST + 1); \
        if(result) return result; \
        size_t len = vec->VEC_STRUCT_LAST++ - vec->first; \
        VEC_ITEM(T, M) *item = A##_static_get(vec, vec->first); \
        vec_memmove(item + 1, item, sizeof(*item) * len); \
        vec_memcpy(VEC_REF(M) *item, VEC_REF(M) val, sizeof(T)); \
        return VEC_ERROR_NONE; \
    }

/**
 * @brief A##_push_back [COMMON] - push one item do the back
 * @param vec - the vector
 * @param val - the value (by reference) to be pushed
 * @return zero if success, non-zero if failure
 */
#define VEC_IMPLEMENT_COMMON_PUSH_BACK(N, A, T, F, M) \
    inline int A##_push_back(N *vec, VEC_ITEM(T, M) val) \
    { \
        assert(vec); \
        VEC_ASSERT(val, M); \
        int result = A##_reserve(vec, vec->VEC_STRUCT_LAST + 1); \
        if(result) return result; \
        size_t back = vec->VEC_STRUCT_LAST++; \
        T *item = VEC_REF(M) *A##_static_get(vec, back); \
        vec_memcpy(item, VEC_REF(M) val, sizeof(T)); \
        return VEC_ERROR_NONE; \
    }

/**
 * @brief A##_swap [COMMON] - swap two items at index 1 and 2
 * @param vec - the vector
 * @param i1 - index 1
 * @param i2 - index 2
 * @return void
 */
#define VEC_IMPLEMENT_COMMON_SWAP(N, A, T, F, M) \
    void A##_swap(N *vec, size_t i1, size_t i2) \
    { \
        assert(vec); \
        VEC_ITEM(T, M) *v1 = A##_static_get(vec, i1 + vec->first); \
        VEC_ITEM(T, M) *v2 = A##_static_get(vec, i2 + vec->first); \
        VEC_ITEM(T, M) tmp; \
        vec_memcpy(&tmp, v1, sizeof(tmp)); \
        vec_memcpy(v1, v2, sizeof(tmp)); \
        vec_memcpy(v2, &tmp, sizeof(tmp)); \
    }

/**
 * @brief A##_reverse [COMMON] - reverse a vector
 * @param vec - the vector
 * @return void
 */
#define VEC_IMPLEMENT_COMMON_REVERSE(N, A, T, F) \
    void A##_reverse(N *vec) \
    { \
        assert(vec); \
        size_t len = A##_length(vec); \
        size_t len2 = len / 2; \
        for(size_t i = 0; i < len2; i++) { \
            A##_swap(vec, i, len - i - 1); \
        } \
    }

/* implementation by value */

/**
 * @brief A##_free [BY_VAL] - free the vector and all it's items
 * @param vec - the vector
 * @return void
 */
#define VEC_IMPLEMENT_BY_VAL_FREE(N, A, T, F) \
    inline void A##_free(N *vec) \
    { \
        assert(vec); \
        if(F != 0) { \
            for(size_t i = 0; i < vec->cap; i++) { \
                VEC_TYPE_FREE(F, &vec->VEC_STRUCT_ITEMS[i], T); \
            } \
        } \
        free(vec->VEC_STRUCT_ITEMS); \
        A##_static_zero(vec); \
    }

/**
 * @brief A##_zero [BY_VAL] - set the vector struct without freeing to zero
 * @param vec - the vector
 * @return void
 */
#define VEC_IMPLEMENT_BY_VAL_ZERO(N, A, T, F) \
    inline void A##_zero(N *vec) \
    { \
        assert(vec); \
        vec_memset(vec->VEC_STRUCT_ITEMS, 0, sizeof(*vec->VEC_STRUCT_ITEMS) * vec->cap); \
    }

/**
 * @brief A##_reserved [BY_VAL] - get reserved number of bytes
 * @param vec - the vector struct
 * @return allocated size in bytes
 */
#define VEC_IMPLEMENT_BY_VAL_RESERVED(N, A, T, F) \
    inline size_t A##_reserved(N *vec) \
    { \
        assert(vec); \
        size_t result = 0; \
        result += (sizeof(*vec->VEC_STRUCT_ITEMS) * vec->cap); \
        return result; \
    }

/**
 * @brief A##_reserve [BY_VAL] - reserve memory for a certain minimum number of items (only expanding), doesn't change the indexed vector length
 * @param vec - the vector
 * @param cap - the minimum desired item capacity
 * @return zero if success, non-zero if failure
 */
#define VEC_IMPLEMENT_BY_VAL_RESERVE(N, A, T, F) \
    inline int A##_reserve(N *vec, size_t cap) \
    { \
        assert(vec); \
        cap += VEC_KEEP_ZERO_END; \
        size_t cap_is = vec->cap; \
        if(cap > cap_is) { \
            size_t required = vec->cap ? vec->cap : VEC_DEFAULT_SIZE;\
            while(required < cap) required *= 2; \
            if(required > vec->cap) { \
                void *temp = vec_realloc(vec->VEC_STRUCT_ITEMS, sizeof(*vec->VEC_STRUCT_ITEMS) * required); \
                if(!temp) return VEC_ERROR_REALLOC; \
                vec->VEC_STRUCT_ITEMS = temp; \
                vec_memset(&vec->VEC_STRUCT_ITEMS[cap_is], 0, sizeof(*vec->VEC_STRUCT_ITEMS) * (required - cap_is)); \
                vec->cap = required; \
            } \
        } \
        return VEC_ERROR_NONE; \
    }

/**
 * @brief A##_copy [BY_VAL] - copy a vector and its items
 * @param dst - the destination vector
 * @param src - the source vector
 * @return zero if succes, non-zero if failure
 */
#define VEC_IMPLEMENT_BY_VAL_COPY(N, A, T, F) \
    inline int A##_copy(N *dst, N *src) \
    { \
        assert(dst); \
        assert(src); \
        assert(dst != src); \
        A##_clear(dst); \
        int result = A##_reserve(dst, A##_length(src)); \
        if(result) return result; \
        vec_memcpy(dst->VEC_STRUCT_ITEMS, src->VEC_STRUCT_ITEMS, sizeof(*dst->VEC_STRUCT_ITEMS) * A##_length(src)); \
        dst->VEC_STRUCT_LAST = A##_length(src); \
        return VEC_ERROR_NONE; \
    }


/* implementaiton by reference */

/**
 * @brief A##_free [BY_REF] - free the vector and all it's items
 * @param vec - the vector
 * @return void
 */
#define VEC_IMPLEMENT_BY_REF_FREE(N, A, T, F) \
    inline void A##_free(N *vec) \
    { \
        assert(vec); \
        for(size_t i = 0; i < vec->cap; i++) { \
            if(F != 0) { \
                VEC_TYPE_FREE(F, vec->VEC_STRUCT_ITEMS[i], T); \
            } \
            free(vec->VEC_STRUCT_ITEMS[i]); \
        } \
        free(vec->VEC_STRUCT_ITEMS); \
        A##_static_zero(vec); \
    }

/**
 * @brief A##_zero [BY_REF] - set the vector struct items without freeing to zero
 * @param vec - the vector
 * @return void
 */
#define VEC_IMPLEMENT_BY_REF_ZERO(N, A, T, F) \
    inline void A##_zero(N *vec) \
    { \
        assert(vec); \
        for(size_t i = 0; i < vec->cap; i++) { \
            vec_memset(&vec->VEC_STRUCT_ITEMS[i], 0, sizeof(*vec->VEC_STRUCT_ITEMS)); \
        } \
    }

/**
 * @brief A##_reserved [BY_REF] - get reserved number of bytes
 * @param vec - the vector struct
 * @return allocated size in bytes
 */
#define VEC_IMPLEMENT_BY_REF_RESERVED(N, A, T, F) \
    inline size_t A##_reserved(N *vec) \
    { \
        assert(vec); \
        size_t result = 0; \
        result += (sizeof(**vec->VEC_STRUCT_ITEMS) * vec->cap); \
        result += (sizeof(*vec->VEC_STRUCT_ITEMS) * vec->cap); \
        return result; \
    }

/**
 * @brief A##_reserve [BY_REF] - reserve memory for a certain minimum number of items (only expanding), doesn't change the indexed vector length
 * @param vec - the vector
 * @param cap - the minimum desired item capacity
 * @return zero if success, non-zero if failure
 */
#define VEC_IMPLEMENT_BY_REF_RESERVE(N, A, T, F) \
    inline int A##_reserve(N *vec, size_t cap) \
    { \
        assert(vec); \
        cap += VEC_KEEP_ZERO_END; \
        size_t cap_is = vec->cap; \
        size_t required = vec->cap ? vec->cap : VEC_DEFAULT_SIZE;\
        while(required < cap) required *= 2; \
        if(required > vec->cap) { \
            void *temp = vec_realloc(vec->VEC_STRUCT_ITEMS, sizeof(*vec->VEC_STRUCT_ITEMS) * required); \
            if(!temp) return VEC_ERROR_REALLOC; \
            vec->VEC_STRUCT_ITEMS = temp; \
            vec_memset(&vec->VEC_STRUCT_ITEMS[cap_is], 0, sizeof(*vec->VEC_STRUCT_ITEMS) * (required - cap_is)); \
            for(size_t i = cap_is; i < required; i++) { \
                vec->VEC_STRUCT_ITEMS[i] = vec_malloc(sizeof(**vec->VEC_STRUCT_ITEMS)); \
                if(!vec->VEC_STRUCT_ITEMS[i]) return VEC_ERROR_MALLOC; \
                vec_memset(vec->VEC_STRUCT_ITEMS[i], 0, sizeof(**vec->VEC_STRUCT_ITEMS)); \
            } \
            vec->cap = required; \
        } \
        return VEC_ERROR_NONE; \
    }

/**
 * @brief A##_copy [BY_REF] - copy a vector and its items
 * @param dst - the destination vector
 * @param src - the source vector
 * @return zero if succes, non-zero if failure
 */
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

//#define VEC_H
//#endif

