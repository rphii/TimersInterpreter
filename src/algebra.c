#include <assert.h>

#include "algebra.h"
//#include <stdio.h>

/* https://stackoverflow.com/questions/101439/the-most-efficient-way-to-implement-an-integer-based-power-function-powint-int */
int algebra_size_pow(Val base, Val exp, Val *result)
{
    assert(result);
    Val scratch = 1;
    if(!base) {
        *result = 0;
        return 0;
    }
    //INFO("base " FMT_VAL ", exp " FMT_VAL "", base, exp);
    while(exp > 0) {
        if(exp & 1) {
            Val scratch_new = scratch * base;
            //INFO("scratch new " FMT_VAL " = scratch " FMT_VAL " * base " FMT_VAL "", scratch_new, scratch, base);
            if(scratch_new < base) goto overflow;
            if((scratch_new / base) != scratch) goto overflow;
            scratch = scratch_new;
        }
        exp = exp >> 1;
        if(exp > 0) {
            Val base_squared = base * base;
            //INFO("base² " FMT_VAL " = base * base " FMT_VAL "²", base_squared, base);
            //INFO("base² " FMT_VAL " / base != base " FMT_VAL "?", base_squared/base, base);
            //INFO("base² " FMT_VAL " / base != base " FMT_VAL "?", 1/base, base);
            if(base_squared < base) goto overflow;
            if((base_squared / base) != base) goto overflow;
            base = base_squared;
        }
    }
    *result = scratch;
    return 0;
overflow:
    return -1;
}


int algebra_sequence_power(VecVal *n, Val iters, Val n_0, Val *sum)
{
    /* TODO 
     * for future arbitrary precision, when there's a bignum implementation,
     * just add k2 and powed seperately and in the very end divide? or something. 
     * idk 
     */
    Val total = 0;
    INFO("iters " FMT_VAL ", n_0 " FMT_VAL "", iters, n_0);
    for(Val i = 0; i < iters; i++) {
        Val k2 = 0;
        Val powed = 0;
        int overflow = 0;
        for(size_t k = 1; k < vec_val_length(n); k++) {
            Val n_k = vec_val_get_at(n, k);
            overflow |= algebra_size_pow((Val)k, 2, &k2);
            overflow |= algebra_size_pow((Val)i + n_0, n_k, &powed);
            //total += (size_t)(pow((i + n_0), n_k) / (double)(k*k));
            INFO("powed " FMT_VAL ", k² " FMT_VAL "", powed, k2);
            Val total_next = total + (Val)((double)powed / (double)k2 + 0.5);
            if(total_next < total) overflow |= -1;
            if(overflow) goto overflow;
            total = total_next;
        }
    }
    INFO("total " FMT_VAL "", total);
    *sum = total;
    return 0;
overflow:
    INFO("overflow");
    return -1;
}

