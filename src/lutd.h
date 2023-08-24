#ifndef LUTD_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

typedef enum {
    LUTD_ERROR_NONE,
    /* errors below */
    LUTD_ERROR_MALLOC,
    LUTD_ERROR_REALLOC,
    LUTD_ERROR_EXPECT_NULL,
} LUTDErrorList;


#define LUTD_DEFAULT_SIZE    4
#define LUTD_TYPE_FREE(F)    (void (*)(void *))(F)
#define LUTD_TYPE_CMP(T, M, C)  (int (*)(LUTD_ITEM(T, M), LUTD_ITEM(T, M)))C

/* N = name
 * A = abbreviation
 * T = type
 * W = width - 2 ^ W = bits
 * M = mode - either BY_VAL or BY_REF
 * H = hash function
 * C = cmp function
 * F = free function (optional)
 * D = dump function (optional, required if T is a struct ???)
 */

#define LUTD_ITEM_BY_VAL(T)  T
#define LUTD_ITEM_BY_REF(T)  T *
#define LUTD_ITEM(T, M)  LUTD_ITEM_##M(T)

#define LUTD_INCLUDE(N, A, T, M) \
    typedef struct N##Bucket { \
        size_t cap; \
        size_t len; \
        size_t *count; \
        LUTD_ITEM(T, M)*items; \
    } N##Bucket; \
    typedef struct { \
        N##Bucket *buckets; \
        size_t width; \
    } N; /* if free() is too long: add an array that holds each item index that actually needs to be freed */ \
    \
    int A##_init(N *l, size_t width); \
    int A##_join(N *l, N *arr, size_t n); \
    int A##_add(N *l, LUTD_ITEM(T, M) v); \
    int A##_add_count(N *l, LUTD_ITEM(T, M) v, size_t count); \
    int A##_del(N *l, T v); \
    bool A##_has(N *l, LUTD_ITEM(T, M) v); \
    int A##_find(N *l, LUTD_ITEM(T, M) v, size_t *i, size_t *j); \
    int A##_print(N *l); \
    int A##_free(N *l); \
    int A##_recycle(N *l); \
    int A##_dump_sorted(N *l, T **arr, size_t **counts, size_t *len); /* TODO differentiate when by ref/by val... (namely when it's a struct) */ \
    int A##_dump(N *l, T **arr, size_t **counts, size_t *len);

#define LUTD_IMPLEMENT(N, A, T, M, H, C, F) \
    _Static_assert(H != 0, "missing hash function"); \
    /*LUTD_IMPLEMENT_COMMON_STATIC_F(N, A, T, F);*/ \
    LUTD_IMPLEMENT_COMMON_STATIC_CMP(N, A, T, M, C, F); \
    /*LUTD_IMPLEMENT_COMMON_STATIC_THREAD_JOIN(N, A, T, F);*/ \
    /*LUTD_IMPLEMENT_COMMON_JOIN(N, A, T, F);*/ \
    LUTD_IMPLEMENT_##M(N, A, T, H, F);

#define LUTD_IMPLEMENT_BY_VAL(N, A, T, H, F) \
    LUTD_IMPLEMENT_BY_VAL_INIT(N, A, T, F); \
    LUTD_IMPLEMENT_BY_VAL_ADD(N, A, T, H, F); \
    LUTD_IMPLEMENT_BY_VAL_ADD_COUNT(N, A, T, H, F); \
    LUTD_IMPLEMENT_BY_VAL_DEL(N, A, T, H, F); \
    LUTD_IMPLEMENT_BY_VAL_HAS(N, A, T, H, F); \
    LUTD_IMPLEMENT_BY_VAL_FREE(N, A, T, F); \
    LUTD_IMPLEMENT_BY_VAL_RECYCLE(N, A, T, F); \
    LUTD_IMPLEMENT_BY_VAL_DUMP(N, A, T, F); /* TODO this will be a challenge */ \
    LUTD_IMPLEMENT_BY_VAL_FIND(N, A, T, H, C, F); \

    /*LUTD_IMPLEMENT_BY_VAL_DUMP_SORTED(N, A, T, F); *//* TODO this will be a challenge */ \

#define ERR_LUTD_ADD            "failed to add value to lut"
#define ERR_LUTD_DEL            "failed to delete value from lut"
#define ERR_LUTD_INIT           "failed to init lut"
#define ERR_LUTD_DUMP_SORTED    "failed to dump sorted lut"
#define ERR_LUTD_RECYCLE        "failed to recycle lut"
#define ERR_LUTD_DUMP           "failed to dump lut"

#define LUTD_IMPLEMENT_BY_REF(N, A, T, H, F) \
    LUTD_IMPLEMENT_BY_REF_INIT(N, A, T, F); \
    LUTD_IMPLEMENT_BY_REF_ADD(N, A, T, H, F); \
    LUTD_IMPLEMENT_BY_REF_ADD_COUNT(N, A, T, H, F); \
    LUTD_IMPLEMENT_BY_REF_HAS(N, A, T, H, F); \
    LUTD_IMPLEMENT_BY_REF_FREE(N, A, T, F); \
    LUTD_IMPLEMENT_BY_REF_RECYCLE(N, A, T, F); \
    LUTD_IMPLEMENT_BY_REF_FIND(N, A, T, H, C, F); \

    //LUTD_IMPLEMENT_BY_REF_DEL(N, A, T, F); 
    //LUTD_IMPLEMENT_BY_REF_HAS(N, A, T, F); 
    //LUTD_IMPLEMENT_BY_REF_DUMP(N, A, T, F); /* TODO this will be a challenge */ 

/* TODO make sure to free when:
 * - free()
 * - recycle() ?
 * - del()
 */
/*
#define LUTD_IMPLEMENT_COMMON_STATIC_F(N, A, T, F) \
    static void (*A##_static_f)(void *) = F != 0 ? LUTD_TYPE_FREE(F) : 0; \
    */

#define LUTD_IMPLEMENT_COMMON_STATIC_CMP(N, A, T, M, C, F) \
    static int (*A##_static_cmp)(LUTD_ITEM(T, M), LUTD_ITEM(T, M)) = C != 0 ? LUTD_TYPE_CMP(T, M, C) : 0;

#define LUTD_IMPLEMENT_STATIC_SHELLSORT(N, A, T, F) \
    static void A##_static_shellsort(T *arr, size_t *counts, size_t n) \
    { \
        if(!arr) return; \
        for(size_t interval = n / 2; interval > 0; interval /= 2) { \
            for(size_t i = interval; i < n; i++) { \
                T temp2 = arr[i]; \
                size_t temp = counts[i]; \
                size_t j = 0; \
                for(j = i; j >= interval && arr[j - interval] > temp2; j -= interval) { \
                    arr[j] = arr[j - interval]; \
                    counts[j] = counts[j - interval]; \
                } \
                arr[j] = temp2; \
                counts[j] = temp; \
            } \
        } \
    } /* \
    static void A##_static_shellsort(T *arr, size_t *counts, size_t n) \
    { \
        if(!arr) return; \
        for(size_t interval = n / 2; interval > 0; interval /= 2) { \
            for(size_t i = interval; i < n; i++) { \
                T temp = counts[i]; \
                T temp2 = arr[i]; \
                size_t j = 0; \
                for(j = i; j >= interval && counts[j - interval] < temp; j -= interval) { \
                    arr[j] = arr[j - interval]; \
                    counts[j] = counts[j - interval]; \
                } \
                counts[j] = temp; \
                arr[j] = temp2; \
            } \
        } \
    }*/

#define LUTD_IMPLEMENT_BY_VAL_INIT(N, A, T, F) \
    int A##_init(N *l, size_t width) \
    { \
        assert(l); \
        assert(width < 8 * sizeof(size_t)); \
        /*A##_recycle(l);*/ \
        void *temp = realloc(l->buckets, sizeof(*l->buckets) * (1UL << width)); \
        if(!temp) return -1; \
        l->buckets = temp; \
        l->width = width; \
        for(size_t i = 0; i < 1UL << width; i++) { \
            memset(&l->buckets[i], 0, sizeof(l->buckets[i])); \
        } \
        return 0; \
    }

#define LUTD_IMPLEMENT_BY_REF_INIT(N, A, T, F) \
        LUTD_IMPLEMENT_BY_VAL_INIT(N, A, T, F);

#if 0
#include <pthread.h>
#define N_THREADS   16 /* TODO make this variable */

#define LUTD_IMPLEMENT_COMMON_STATIC_THREAD_JOIN(N, A, T, F) \
    typedef struct { \
        N *src; \
        N *dst; \
        size_t i_bucket; \
        pthread_mutex_t *mutex; \
    } Thread##A##Join; \
    void *A##_static_thread_join(void *args) { \
        assert(args); \
        Thread##A##Join *tj = args; \
        int result = 0; \
        if(tj->i_bucket >= 1UL << tj->src->width) { \
            return 0; /* TODO return proper code!? */ \
        } \
        for(size_t j = 0; j < tj->src->buckets[tj->i_bucket].len; j++) { \
            T item = tj->src->buckets[tj->i_bucket].items[j]; \
            size_t count = tj->src->buckets[tj->i_bucket].count[j]; \
            pthread_mutex_lock(tj->mutex); \
            result |= result ?: A##_add_count(tj->dst, item, count); \
            pthread_mutex_unlock(tj->mutex); \
            if(result) break; \
        } \
        /* TODO fix this shit: return (void *)(uintptr_t)(size_t)result;*/ \
        return 0; \
    }

#define LUTD_IMPLEMENT_COMMON_JOIN(N, A, T, F) \
    int A##_join(N *l, N *arr, size_t n) \
    { \
        assert(l); \
        assert(arr); \
        assert(l->width); \
        pthread_t thread_ids[N_THREADS] = {0}; \
        pthread_attr_t thread_attr; \
        pthread_attr_init(&thread_attr); \
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE); \
        pthread_mutex_t thread_mutex; \
        pthread_mutex_init(&thread_mutex, 0); \
        Thread##A##Join tj[N_THREADS] = {0}; \
        for(size_t k = 0; k < n; k++) { \
            assert(arr[k].width == l->width); \
            for(size_t i = 0; i < 1ULL << l->width; i += N_THREADS) { \
                for(size_t j = 0; j < N_THREADS; j++) { \
                    tj[j].src = &arr[k]; \
                    tj[j].dst = l; \
                    tj[j].i_bucket = i; \
                    tj[j].mutex = &thread_mutex; \
                    pthread_create(&thread_ids[j], 0, A##_static_thread_join, &tj[j]); \
                } \
                for(size_t j = 0; j < N_THREADS; j++) { \
                    pthread_join(thread_ids[j], 0); \
                } \
                /* MULTITHREAD THIS
                for(size_t j = 0; j < arr[k].buckets[i].len; j++) { \
                    T item = arr[k].buckets[i].items[j]; \
                    size_t count = arr[k].buckets[i].count[j]; \
                    result |= result ?: A##_add_count(l, item, count); \
                } */ \
            } \
        } \
        /* TODO fix this shit: return (void *)(uintptr_t)(size_t)result;*/ \
        pthread_mutex_destroy(&thread_mutex); \
        return 0; \
    }

#else
/* TODO provide the blow NO THREAD VERSION */
#define LUTD_IMPLEMENT_COMMON_JOIN(N, A, T, F) \
    int A##_join(N *l, N *arr, size_t n) \
    { \
        assert(l); \
        assert(arr); \
        assert(l->width); \
        int result = 0; \
        for(size_t k = 0; k < n; k++) { \
            assert(arr[k].width == l->width); \
            for(size_t i = 0; i < 1ULL << l->width; i++) { \
                for(size_t j = 0; j < arr[k].buckets[i].len; j++) { \
                    T item = arr[k].buckets[i].items[j]; \
                    size_t count = arr[k].buckets[i].count[j]; \
                    result |= result ?: A##_add_count(l, item, count); \
                } \
            } \
        } \
        return 0; \
    }
#endif

#define LUTD_IMPLEMENT_BY_VAL_ADD(N, A, T, H, F) \
    int A##_add(N *l, T v) \
    { \
        assert(l); \
        int result = A##_add_count(l, v, 1); \
        return result; \
    }

#define LUTD_IMPLEMENT_BY_VAL_ADD_COUNT(N, A, T, H, F) \
    int A##_add_count(N *l, T v, size_t count) \
    { \
        assert(l); \
        bool exists = false; \
        size_t hash = H(v) % (1UL << l->width); /* TODO this is stupid. */ \
        size_t exist_index = 0; \
        for(exist_index = 0; exist_index < l->buckets[hash].len; exist_index++) { \
            if(memcmp(&l->buckets[hash].items[exist_index], &v, sizeof(v))) continue; \
            /*if(l->buckets[hash].items[exist_index] != v) continue;*/ \
            exists = true; \
            break; \
        } \
        if(!exists) { \
            size_t len = l->buckets[hash].cap; \
            size_t required = len ? len : LUTD_DEFAULT_SIZE;\
            size_t cap = exist_index + 1; \
            while(required < cap) required *= 2; \
            if(required > len) { \
                void *temp = realloc(l->buckets[hash].items, sizeof(T) * required); \
                if(!temp) return LUTD_ERROR_REALLOC; \
                l->buckets[hash].items = temp; \
                memset(&l->buckets[hash].items[exist_index], 0, sizeof(T) * (required - len)); \
                temp = realloc(l->buckets[hash].count, sizeof(size_t) * required); \
                if(!temp) return LUTD_ERROR_REALLOC; \
                l->buckets[hash].count = temp; \
                memset(&l->buckets[hash].count[exist_index], 0, sizeof(size_t) * (required - len)); \
                /* finish up */ \
                l->buckets[hash].cap = required; \
            } \
            l->buckets[hash].items[exist_index] = v; \
            l->buckets[hash].len++; \
        } \
        l->buckets[hash].count[exist_index] += count; \
        return 0;   \
    }

#define LUTD_IMPLEMENT_BY_REF_ADD(N, A, T, H, F) \
    int A##_add(N *l, T *v) \
    { \
        assert(l); \
        int result = A##_add_count(l, v, 1); \
        return result; \
    }

#define LUTD_IMPLEMENT_BY_REF_ADD_COUNT(N, A, T, H, F) \
    int A##_add_count(N *l, T *v, size_t count) { \
        assert(l); \
        assert(v); \
        bool exists = false; \
        size_t hash = H(v) % (1UL << l->width); /* TODO this is stupid. */ \
        size_t exist_index = 0; \
        for(exist_index = 0; exist_index < l->buckets[hash].len; exist_index++) { \
            if(memcmp(l->buckets[hash].items[exist_index], v, sizeof(*v))) continue; \
            exists = true; \
            break; \
        } \
        if(!exists) { \
            size_t len = l->buckets[hash].cap; \
            size_t required = len ? len : LUTD_DEFAULT_SIZE;\
            size_t cap = exist_index + 1; \
            while(required < cap) required *= 2; \
            if(required > len) { \
                void *temp = realloc(l->buckets[hash].items, sizeof(T) * required); \
                if(!temp) return LUTD_ERROR_REALLOC; \
                l->buckets[hash].items = temp; \
                /*memset(&l->buckets[hash].items[exist_index], 0, sizeof(T) * (required - len));*/ \
                temp = realloc(l->buckets[hash].count, sizeof(*l->buckets[hash].count) * required); \
                if(!temp) return LUTD_ERROR_REALLOC; \
                l->buckets[hash].count = temp; \
                l->buckets[hash].cap = required; \
            } \
            l->buckets[hash].items[exist_index] = v; \
            l->buckets[hash].len++; \
        } \
        l->buckets[hash].count[exist_index] += count; \
        return 0; \
    }

#define LUTD_IMPLEMENT_BY_VAL_DEL(N, A, T, H, F) \
    int A##_del(N *l, T v) \
    { \
        assert(l); \
        bool exists = false; \
        size_t hash = H(v) % (1UL << l->width); \
        size_t exist_index = 0; \
        for(exist_index = 0; exist_index < l->buckets[hash].len; exist_index++) { \
            if(memcmp(&l->buckets[hash].items[exist_index], &v, sizeof(v))) continue; \
            /*if(l->buckets[hash].items[exist_index] != v) continue;*/ \
            exists = true; \
            break; \
        } \
        if(exists) { \
            size_t len = l->buckets[hash].len; \
            memmove(&l->buckets[hash].items[exist_index], &l->buckets[hash].items[1 + exist_index], (len - exist_index - 1) * sizeof(T)); \
            l->buckets[hash].len--; \
        } \
        return 0;   \
    }

#define LUTD_IMPLEMENT_BY_VAL_HAS(N, A, T, H, F) \
    bool A##_has(N *l, T v) \
    { \
        assert(l); \
        bool exists = false; \
        size_t hash = H(v) % (1UL << l->width); /* TODO this is stupid. */ \
        size_t exist_index = 0; \
        for(exist_index = 0; exist_index < l->buckets[hash].len; exist_index++) { \
            if(memcmp(&l->buckets[hash].items[exist_index], &v, sizeof(v))) continue; \
            /*if(l->buckets[hash].items[exist_index] != v) continue;*/ \
            exists = true; \
            break; \
        } \
        return exists; \
    }

#define LUTD_IMPLEMENT_BY_REF_HAS(N, A, T, H, F) \
    bool A##_has(N *l, T *v) \
    { \
        assert(l); \
        bool exists = false; \
        size_t hash = H(v) % (1UL << l->width); /* TODO this is stupid. */ \
        size_t exist_index = 0; \
        for(exist_index = 0; exist_index < l->buckets[hash].len; exist_index++) { \
            if(A##_static_cmp(l->buckets[hash].items[exist_index], v)) continue; \
            /*if(memcmp(&l->buckets[hash].items[exist_index], &v, sizeof(v))) continue;*/ \
            /*if(l->buckets[hash].items[exist_index] != v) continue;*/ \
            exists = true; \
            break; \
        } \
        return exists; \
    }

#define LUTD_IMPLEMENT_BY_VAL_FIND(N, A, T, H, C, F) \
    int A##_find(N *l, T v, size_t *i, size_t *j) \
    { \
        assert(l); \
        assert(i); \
        assert(j); \
        size_t hash = H(v) % (1UL << l->width); /* TODO this is stupid. */ \
        size_t exist_index = 0; \
        for(exist_index = 0; exist_index < l->buckets[hash].len; exist_index++) { \
            /*if(memcmp(&l->buckets[hash].items[exist_index], &v, sizeof(v))) continue;*/ \
            if(A##_static_cmp(l->buckets[hash].items[exist_index], v)) continue; \
            /*if(l->buckets[hash].items[exist_index] != v) continue;*/ \
            *i = hash; \
            *j = exist_index; \
            return 0; \
        } \
        return -1; \
    } \
    
#define LUTD_IMPLEMENT_BY_REF_FIND(N, A, T, H, C, F) \
    int A##_find(N *l, T *v, size_t *i, size_t *j) \
    { \
        assert(l); \
        assert(i); \
        assert(j); \
        size_t hash = H(v) % (1UL << l->width); /* TODO this is stupid. */ \
        size_t exist_index = 0; \
        for(exist_index = 0; exist_index < l->buckets[hash].len; exist_index++) { \
            /*if(memcmp(&l->buckets[hash].items[exist_index], &v, sizeof(v))) continue;*/ \
            if(A##_static_cmp(l->buckets[hash].items[exist_index], v)) continue; \
            /*if(l->buckets[hash].items[exist_index] != v) continue;*/ \
            *i = hash; \
            *j = exist_index; \
            return 0; \
        } \
        return -1; \
    } \

#define LUTD_IMPLEMENT_BY_VAL_FREE(N, A, T, F) \
    int A##_free(N *l) \
    { \
        assert(l); \
        for(size_t i = 0; i < 1UL << l->width; i++) { \
            /* TODO this'll need to be in BY_REF
            for(size_t j = 0; j < 1UL << l->width; j++) { \
                if(F != 0) A##_static_f(&l->buckets[i].items[j]); \
            } */ \
            free(l->buckets[i].items); \
            free(l->buckets[i].count); \
        } \
        memset(l->buckets, 0, sizeof(*l->buckets) * (1UL << l->width)); \
        free(l->buckets); \
        memset(l, 0, sizeof(*l)); \
        return 0;   \
    }

#define LUTD_IMPLEMENT_BY_REF_FREE(N, A, T, F) \
    LUTD_IMPLEMENT_BY_VAL_FREE(N, A, T, F)

#define LUTD_IMPLEMENT_BY_VAL_RECYCLE(N, A, T, F) \
    int A##_recycle(N *l) \
    { \
        assert(l); \
        if(!l->width) return 0; \
        for(size_t i = 0; i < 1UL << l->width; i++) { \
            l->buckets[i].cap = 0; \
            l->buckets[i].len = 0; \
            /* TODO this'll need to be in BY_REF
            for(size_t j = 0; j < 1UL << l->width; j++) { \
                if(F != 0) A##_static_f(&l->buckets[i].items[j]); \
            } */ \
        } \
        return 0;   \
    }

#define LUTD_IMPLEMENT_BY_REF_RECYCLE(N, A, T, F) \
    int A##_recycle(N *l) \
    { \
        assert(l); \
        if(!l->width) return 0; \
        for(size_t i = 0; i < 1UL << l->width; i++) { \
            l->buckets[i].cap = 0; \
            l->buckets[i].len = 0; \
            /* TODO this'll need to be in BY_REF
            for(size_t j = 0; j < 1UL << l->width; j++) { \
                if(F != 0) A##_static_f(&l->buckets[i].items[j]); \
            } */ \
        } \
        return 0;   \
    }

#define LUTD_IMPLEMENT_BY_VAL_DUMP_SORTED(N, A, T, F) \
    LUTD_IMPLEMENT_STATIC_SHELLSORT(N, A, T, F); /* TODO this'll also be annoying */ \
    int A##_dump_sorted(N *l, T **arr, size_t **counts, size_t *len) \
    { \
        assert(l); \
        assert(arr); \
        assert(counts); \
        assert(len); \
        size_t used_total = 0; \
        for(size_t i = 0; i < 1UL << l->width; i++) { \
            used_total += l->buckets[i].len; \
        } \
        if(*arr) return LUTD_ERROR_EXPECT_NULL; \
        if(*counts) return LUTD_ERROR_EXPECT_NULL; \
        *arr = malloc(sizeof(T) * used_total); \
        if(!*arr) return LUTD_ERROR_MALLOC; \
        *counts = malloc(sizeof(size_t) * used_total); \
        if(!*counts) return LUTD_ERROR_MALLOC; \
        *len = used_total; \
        size_t used_index = 0; \
        for(size_t i = 0; i < 1UL << l->width; i++) { \
            for(size_t j = 0; j < l->buckets[i].len; j++) { \
                (*arr)[used_index] = l->buckets[i].items[j]; \
                (*counts)[used_index] = l->buckets[i].count[j]; \
                used_index++; \
            } \
        } \
        /* shellsort */ \
        A##_static_shellsort(*arr, *counts, used_total); \
        /*A##_static_shellsort2(*arr, *counts, used_total);*/ \
        return 0;   \
    }

#define LUTD_IMPLEMENT_BY_VAL_DUMP(N, A, T, F) \
    int A##_dump(N *l, T **arr, size_t **counts, size_t *len) \
    { \
        assert(l); \
        assert(arr); \
        assert(len); \
        size_t used_total = 0; \
        for(size_t i = 0; i < 1UL << l->width; i++) { \
            used_total += l->buckets[i].len; \
        } \
        if(*arr) return LUTD_ERROR_EXPECT_NULL; \
        if(counts) { \
            if(*counts) return LUTD_ERROR_EXPECT_NULL; \
        } \
        *arr = malloc(sizeof(T) * used_total); \
        if(!*arr) return LUTD_ERROR_MALLOC; \
        if(counts) { \
            *counts = malloc(sizeof(size_t) * used_total); \
            if(!*counts) return LUTD_ERROR_MALLOC; \
        } \
        *len = used_total; \
        size_t used_index = 0; \
        for(size_t i = 0; i < 1UL << l->width; i++) { \
            for(size_t j = 0; j < l->buckets[i].len; j++) { \
                (*arr)[used_index] = l->buckets[i].items[j]; \
                if(counts) { \
                    (*counts)[used_index] = l->buckets[i].count[j]; \
                } \
                used_index++; \
            } \
        } \
        return 0;   \
    }


/******************************************************************************/
/* FUNCTION IMPLEMENTATIONS ***************************************************/
/******************************************************************************/


#define LUTD_H
#endif


