#if 0
#include "lutd.h"

/* for plut.h : also add the following functions, besides the hash function:
 * - copy function from source to origin
 * - comparison of two values for equality
 * - freeing function for one value
 * - (optional) printing function for one value
 */
#define LUTD_IMPL(N, T) \
    static void lutd_##N##_static_shellsort(T *arr, size_t n) \
    { \
        if(!arr) return; \
        for(size_t interval = n / 2; interval > 0; interval /= 2) { \
            for(size_t i = interval; i < n; i++) { \
                T temp = arr[i]; \
                size_t j = 0; \
                for(j = i; j >= interval && arr[j - interval] < temp; j -= interval) { \
                    arr[j] = arr[j - interval]; \
                } \
                arr[j] = temp; \
            } \
        } \
    } \
    \
    int lutd_##N##_init(Lutd_##N *l, size_t w) \
    { \
        if(!l) THROW("expected pointer to Lut_"#N" struct"); \
        if(l->c) THROW("expected null pointer"); \
        if(l->l) THROW("expected null pointer"); \
        if(l->items) THROW("expected null pointer"); \
        l->c = malloc(sizeof(*l->c) * (1UL << w)); \
        if(!l->c) THROW("failed to allocate memory"); \
        memset(l->c, 0, sizeof(*l->c) * (1UL << w)); \
        l->l = malloc(sizeof(*l->l) * (1UL << w)); \
        if(!l->l) THROW("failed to allocate memory"); \
        memset(l->l, 0, sizeof(*l->l) * (1UL << w)); \
        l->items = malloc(sizeof(*l->items) * (1UL << w)); \
        if(!l->items) THROW("failed to allocate memory"); \
        memset(l->items, 0, sizeof(*l->items) * (1UL << w)); \
        l->w = w; \
        return 0;   \
    error: \
        return -1; \
    } \
    \
    int lutd_##N##_add(Lutd_##N *l, T v) \
    { \
        if(!l) THROW("expected pointer to Lut_"#N" struct"); \
        bool exists = false; \
        size_t hash = lutd_##N##_static_hash(v) % (1UL << l->w); /* TODO this is stupid. */ \
        size_t exist_index = 0; \
        for(exist_index = 0; exist_index < l->l[hash]; exist_index++) { \
            if(l->items[hash][exist_index] != v) continue; \
            exists = true; \
            break; \
        } \
        if(!exists) { \
            size_t required = l->c[hash] ? l->c[hash] : LUTD_DEFAULT_SIZE;\
            size_t cap = exist_index + 1; \
            while(required < cap) required *= 2; \
            if(required > l->c[hash]) { \
                void *temp = realloc(l->items[hash], sizeof(T) * required); \
                if(!temp) THROW("failed to allocate memory"); \
                l->items[hash] = temp; \
                l->c[hash] = required; \
            } \
            l->items[hash][exist_index] = v; \
            l->l[hash]++; \
        } \
        return 0;   \
    error: \
        return -1; \
    } \
    \
    int lutd_##N##_xor(Lutd_##N *l, T v) \
    { \
        if(!l) THROW("expected pointer to Lut_"#N" struct"); \
        bool exists = false; \
        size_t hash = lutd_##N##_static_hash(v) % (1UL << l->w); /* TODO this is stupid. */ \
        size_t exist_index = 0; \
        for(exist_index = 0; exist_index < l->l[hash]; exist_index++) { \
            if(l->items[hash][exist_index] != v) continue; \
            exists = true; \
            break; \
        } \
        if(!exists) { \
            size_t required = l->c[hash] ? l->c[hash] : LUTD_DEFAULT_SIZE;\
            size_t cap = exist_index + 1; \
            while(required < cap) required *= 2; \
            if(required > l->c[hash]) { \
                void *temp = realloc(l->items[hash], sizeof(T) * required); \
                if(!temp) THROW("failed to allocate memory"); \
                l->items[hash] = temp; \
                l->c[hash] = required; \
            } \
            l->items[hash][exist_index] = v; \
            l->l[hash]++; \
        } else { \
            size_t len = l->l[hash]; \
            memmove(&l->items[hash][exist_index], &l->items[hash][1 + exist_index], (len - exist_index - 1) * sizeof(T)); \
            l->l[hash]--; \
        } \
        return 0;   \
    error: \
        return -1; \
    } \
    \
    int lutd_##N##_del(Lutd_##N *l, T v) \
    { \
        if(!l) THROW("expected pointer to Lut_"#N" struct"); \
        bool exists = false; \
        size_t hash = lutd_##N##_static_hash(v) % (1UL << l->w); \
        size_t exist_index = 0; \
        for(exist_index = 0; exist_index < l->l[hash]; exist_index++) { \
            if(l->items[hash][exist_index] != v) continue; \
            exists = true; \
            break; \
        } \
        if(exists) { \
            size_t len = l->l[hash]; \
            memmove(&l->items[hash][exist_index], &l->items[hash][1 + exist_index], (len - exist_index - 1) * sizeof(T)); \
            l->l[hash]--; \
        } \
        return 0;   \
    error: \
        return -1; \
    } \
    \
    /*int lutd_##N##_print(Lutd_##N *l) \
    { \
        if(!l) THROW("expected pointer to Lut_"#N" struct"); \
        for(size_t i = 0; i < 1UL << l->w; i++) { \
            if(l->l[i]) { \
                printf("[%zu] : ", i); \
                for(size_t j = 0; j < l->l[i]; j++) { \
                    printf("%zu", l->items[i][j]); \
                    if(j + 1 < l->l[i]) printf(", "); \
                } \
                printf("\n"); \
            } \
        } \
        return 0;   \
    error: \
        return -1; \
    }*/ \
    \
    int lutd_##N##_free(Lutd_##N *l) \
    { \
        if(!l) THROW("expected pointer to Lut_"#N" struct"); \
        for(size_t i = 0; i < 1UL << l->w; i++) { \
            free(l->items[i]); \
        } \
        memset(l->items, 0, sizeof(*l->items) * (1UL << l->w)); \
        memset(l->c, 0, sizeof(*l->c) * (1UL << l->w)); \
        memset(l->l, 0, sizeof(*l->l) * (1UL << l->w)); \
        free(l->c); \
        free(l->l); \
        free(l->items); \
        memset(l, 0, sizeof(*l)); \
        return 0;   \
    error: \
        return -1; \
    } \
    \
    int lutd_##N##_recycle(Lutd_##N *l) \
    { \
        if(!l) THROW("expected pointer to Lut_"#N" struct"); \
        memset(l->l, 0, sizeof(*l->l) * (1UL << l->w)); \
        return 0;   \
    error: \
        return -1; \
    } \
    \
    int lutd_##N##_dump_sorted(Lutd_##N *l, T **arr, size_t *len) \
    { \
        if(!l) THROW("expected pointer to Lut_"#N" struct"); \
        if(!arr) THROW("expected pointer to "#T); \
        if(!len) THROW("expected pointer to size_t"); \
        size_t used_total = 0; \
        for(size_t i = 0; i < 1UL << l->w; i++) { \
            used_total += l->l[i]; \
        } \
        if(*arr) THROW("expected null pointer"); \
        *arr = malloc(sizeof(T) * used_total); \
        if(!*arr) THROW("failed to allocate memory"); \
        *len = used_total; \
        size_t used_index = 0; \
        for(size_t i = 0; i < 1UL << l->w; i++) { \
            for(size_t j = 0; j < l->l[i]; j++) { \
                (*arr)[used_index++] = l->items[i][j]; \
            } \
        } \
        lutd_##N##_static_shellsort(*arr, used_total); \
        return 0;   \
    error: \
        return -1; \
    } \


/******************************************************************************/
/* FUNCTION IMPLEMENTATIONS ***************************************************/
/******************************************************************************/

static inline size_t lutd_size_static_hash(size_t v)
{
    int width = 16;
    return (v * 14695981039346656037UL) >> ((8 * sizeof(v)) - width);
}
LUTD_IMPL(size, size_t);

#endif

