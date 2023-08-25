#include "next.h"
#include "ast.h"
#include "err.h"
#include "attr.h"
#include "expr.h"
#include "val.h"
#include "run.h"
#include "timers.h"
#include "algebra.h"
#include "execute.h"
#include "scope.h"
#include "rand64.h"

#define ERR_NEXT_HANDLE_DIFF "failed to handle diff"
//ErrDeclStatic next_static_handle_diff(Val cmp, Val t, bool skip_current, TriVal *diff)
ErrDeclStatic next_static_handle_diff(ExprBuf *buf, Val t)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    bool skip_current = buf->skip_current;
    Val cmp = buf->cmp;
    Val diff2 = t >= cmp ? t - cmp : VAL_MAX - cmp + t + 1;
    //INFO("diff 2 " FMT_VAL "", diff2);
    INFO(F("cmp " FMT_VAL ", t " FMT_VAL ", diff2 " FMT_VAL "", FG_BL_B), cmp, t, diff2);
    //printf(F("cmp " FMT_VAL ", t " FMT_VAL ", diff2 " FMT_VAL "\n", FG_BL_B), cmp, t, diff2);
    TriVal *diff = &buf->diff;
    if(!diff->set || diff2 < diff->val) {
        //INFO("diff2 " FMT_VAL ", skip_current %s", diff2, skip_current?"true":"false");
        if(!(skip_current && !diff2) || !skip_current) {
            diff->val = diff2;
            diff->set = true;
        }
    }
    return 0;
error:
    return -1;
}

ErrDeclStatic next_static_callback_range(ExprBuf *buf, Val from, Val until)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    Val t = 0;
    Val t_from = from <= until ? from : until;
    Val t_until = from <= until ? until : from;
    Val t_cmp = buf->skip_current ? buf->cmp + 1 : buf->cmp;
    INFO(F("from " FMT_VAL " until " FMT_VAL "", FG_BL_B), (t_from), (t_until));
    if(t_cmp >= t_from && t_cmp <= t_until) {
        t = t_cmp;
    } else {
        t = t_from;
    }
    //TRY(next_static_handle_diff(t_cmp, t, skip_current, diff), ERR_NEXT_HANDLE_DIFF);
    TRY(next_static_handle_diff(buf, t), ERR_NEXT_HANDLE_DIFF);
    return 0;
error:
    return -1;
}

ErrDeclStatic next_static_callback_sequence(ExprBuf *buf, Val from, Val step)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    INFO("from " FMT_VAL ", step " FMT_VAL "", from, step);
#if 1
    Val t_cmp = 0;
    if(buf->cmp < from) {
        t_cmp = from;
    } else {
        Val repeat = step ? (buf->cmp - from) / step : 0;
        if(step && (buf->cmp - from) % step) repeat++;
        t_cmp = from + repeat * step;
    }
#else
    Val t_mod = step ? buf->cmp % step : 0;
    Val t_add = (step - t_mod) % (step ? step : 1) + from;
    INFO("sequence t_add " FMT_VAL "", t_add);
    Val t_cmp = buf->cmp + t_add;
    INFO("sequence t_cmp " FMT_VAL "", t_cmp);
#endif
    if(buf->skip_current && t_cmp == buf->cmp) {
        t_cmp += step;
    }
    Val t_from = from;
    Val t_until = 0;
    if(buf->until.set) {
        t_until = buf->until.val;
    } else if(buf->times.set) {
        t_until = from + step * buf->until.val;
    } else {
        t_until = VAL_MAX;
    }
    INFO("sequence tcmp " FMT_VAL "", t_cmp);
    Val t = 0;
    if(t_cmp >= t_from && t_cmp <= t_until) {
        t = t_cmp;
        INFO("sequence1 t " FMT_VAL "", t);
    } else {
        t = t_from;
        INFO("sequence2 t " FMT_VAL "", t);
    }
    INFO("sequence t " FMT_VAL "", t);
    TRY(next_static_handle_diff(buf, t), ERR_NEXT_HANDLE_DIFF);
    return 0;
error:
    return -1;
}

int calculate_sum(VecVal *n, Val iters, Val offset, Val *sum)
{
    /* TODO 
     * for future arbitrary precision, when there's a bignum implementation,
     * just add k2 and powed seperately and in the very end divide? or something. 
     * idk 
     */
    Val total = 0;
    for(Val i = 0; i < iters; i++) {
        Val k2 = 0;
        Val powed = 0;
        int overflow = 0;
        for(size_t k = 1; k < vec_val_length(n) - 1; k++) {
            Val n_k = vec_val_get_at(n, k);
            overflow |= algebra_size_pow((Val)k, 2, &k2);
            overflow |= algebra_size_pow((Val)i + offset, n_k, &powed);
            //total += (size_t)(pow((i + offset), n_k) / (double)(k*k));
            Val total_next = total + (Val)((double)powed / (double)k2 + 0.5);
            if(total_next < total) overflow |= -1;
            if(overflow) goto overflow;
            total = total_next;
        }
    }
    *sum = total;
    return 0;
overflow:
    return -1;
}

#include <math.h>

ErrDeclStatic next_static_callback_power(ExprBuf *buf, Val offset, Val exponent)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    INFO("buf->cmp " FMT_VAL ", buf->times.val " FMT_VAL "", buf->cmp, buf->times.val);
    INFO("offset " FMT_VAL ", exponent " FMT_VAL "", offset, exponent);
    double exp = 1 / (double)exponent;
    Val iteration = (Val)pow((double)buf->cmp, exp);
    //if(iteration >= offset + 1) {
    //    iteration -= (offset + 1);
    //} else if(iteration < offset + 1) {
    //    iteration = 0;
    //}
    INFO(F("assumed iteration " FMT_VAL "", FG_YL), iteration);
    Val t = 0;
    int overflow = algebra_size_pow(iteration, exponent, &t);
    while(t < buf->cmp) { /* get correct value; manually correct floating point bs */
        overflow = algebra_size_pow(iteration, exponent, &t);
        //INFO(F("corrected value " FMT_VAL "", FG_YL), t);
        iteration++;
        if(overflow) break;
    }
    INFO(F("corrected iteration " FMT_VAL "", FG_YL), iteration);
    //INFO("buf->times %d " FMT_VAL "", buf->times.set, buf->times.val);
#if 1
    if(buf->skip_current && buf->cmp == t) {
        overflow = algebra_size_pow(++iteration, exponent, &t);
        INFO("increment iteration; t " FMT_VAL "", t);
    }
#endif
    bool bounds = false;
    if(buf->times.set && !(iteration >= offset && iteration < buf->times.val + offset)) {
        bounds = true;
        iteration = offset;
        INFO("bounds:%d, iteration " FMT_VAL "", bounds, iteration);
    } else if(buf->until.set && !(iteration >= offset && t <= buf->until.val)) {
        bounds = true;
        iteration = offset;
        INFO("bounds:%d", bounds);
    } else if(!(iteration >= offset)) {
        bounds = true;
        iteration = offset;
    }
    INFO("overflow:%d", overflow);
#if 1
    if(buf->skip_current && buf->cmp == t) {
        overflow = algebra_size_pow(++iteration, exponent, &t);
        INFO("increment iteration; t " FMT_VAL "", t);
    }
#endif
    INFO(F("final iteration " FMT_VAL "", FG_YL), iteration);

    if(!overflow && !bounds) {
        INFO("power next " FMT_VAL ", iteration " FMT_VAL "", t, iteration);
        TRY(next_static_handle_diff(buf, t), ERR_NEXT_HANDLE_DIFF);
    } else {
        overflow = algebra_size_pow(offset, exponent, &t);
        INFO(F("final value " FMT_VAL "", FG_YL), t);
        if(!overflow) {
            INFO("power next (2) " FMT_VAL ", iteration " FMT_VAL "", t, iteration);
            TRY(next_static_handle_diff(buf, t), ERR_NEXT_HANDLE_DIFF);
        }
    }

    return 0;
error:
    return -1;
}

ErrDeclStatic next_static_callback_random_num(ExprBuf *buf, Val t)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
    return 0;
error:
    return -1;
}

ErrDeclStatic next_static_callback_random_range(ExprBuf *buf, Val from, Val until)
{
    if(!buf) THROW(ERR_CREATE_POINTER);
    Val t = 0;
    Val t_from = from <= until ? from : until;
    Val t_until = from <= until ? until : from;
    Val t_delta = t_until - t_from + 1;
    if(t_delta) {
        t = (Val)(size_t)rand64() % (t_delta) + t_from;
    } else {
        t = (Val)(size_t)rand64(); /* overflowed t_delta; range is this: 0-. */
    }
    //printf("[%zi]\n", t);
    TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
    return 0;
error:
    return -1;
}

ErrDeclStatic next_static_callback_random_sequence(ExprBuf *buf, Val from, Val step)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    Val t_cmp = 0;
    if(buf->cmp < from) {
        t_cmp = from;
    } else {
        Val repeat = step ? (buf->cmp - from) / step : 0;
        if(step && (buf->cmp - from) % step) repeat++;
        t_cmp = from + repeat * step;
    }
    //if(buf->skip_current && t_cmp == buf->cmp) {
    //    t_cmp += step;
    //}
    Val t_from = from;
    Val t_until = 0;
    if(buf->until.set) {
        t_until = buf->until.val;
    } else if(buf->times.set) {
        t_until = from + step * buf->until.val;
    } else {
        t_until = VAL_MAX;
    }
    INFO("sequence tcmp " FMT_VAL "", t_cmp);
    /* pick random */
    /* approximate to closest one -> round down */
    Val t = 0;
    Val t_bad = 0;
    Val t_delta = t_until - t_from + 1;
    Val t_delta2 = t_delta + step;
    if(t_delta && t_delta2 >= t_delta) {
        t_bad = (Val)rand64() % (t_delta2) + t_from;
        //printf("1 bad %zu\n", t_bad);
        Val repeat = step ? ((t_bad - t_from) / step) : 0;
        t = step * repeat + t_from;
    } else {
        t_bad = (Val)rand64(); /* overflowed t_delta; range is this: 0-. */
        //printf("2 bad %zu\n", t_bad);
        Val repeat = step ? (t_bad / step) : 0;
        t = step * repeat;
    }
    TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
    return 0;
error:
    return -1;
}

ErrDeclStatic next_static_callback_random_power(ExprBuf *buf, Val offset, Val exponent)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    //THROW("implement this");
    /* figure out bounds */
    Val t_from = 0;
    Val t_until = 0;
    int overflow = false;
    double exp_inv = 1 / (double)exponent;
    Val iteration = 0;
    if(buf->times.set) {
        iteration = buf->times.val;
        overflow = algebra_size_pow(offset + iteration, exponent, &t_until);
    } else {
        Val t_max = buf->until.set ? buf->until.val : VAL_MAX;
        iteration = (Val)pow((double)t_max, exp_inv);
        overflow = algebra_size_pow(offset + iteration, exponent, &t_until);
        while(t_until < buf->until.set) { /* get correct value; manually correct floating point bs */
            overflow = algebra_size_pow(offset + iteration, exponent, &t_until);
            //INFO(F("corrected value " FMT_VAL "", FG_YL), t);
            iteration++;
            if(overflow) break;
        }
    }
    while(overflow && iteration) { /* get correct value; manually correct floating point bs */
        overflow = algebra_size_pow(--iteration, exponent, &t_until);
    }
    if(overflow) return 0 ;
    /* figure out random iteration */
    Val t_bad = 0;
    Val t_delta = t_until - t_from + 1;
    if(t_delta) {
        t_bad = (Val)(size_t)rand64() % (t_delta) + t_from;
    } else {
        t_bad = (Val)(size_t)rand64(); /* overflowed t_delta; range is this: 0-. */
    }
    //Val iteration = (Val)((double)pow((double)t_bad, exp_inv) + 0.5);
    iteration = (Val)((double)pow((double)t_bad, exp_inv));
    Val t = 0;
    overflow = algebra_size_pow(offset + iteration, exponent, &t);
    if(!overflow) {
        TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
    }
    return 0;
error:
    return -1;
}

ErrDecl next_random(Run *run, Scope *scope)
{
    int err = 0;
    if(!run) THROW(ERR_RUN_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    Ast *node = scope->origin;
    VecVal *stack = &run->stack;
    /* pick random child */
    size_t child_len = 0;
    TRY(ast_child_len(node, &child_len), ERR_AST_CHILD_LEN);
    //printf("child len %zu\n", child_len);
    if(!child_len) THROW("TODO"); /* TODO what the fuck do I have to do in this case? is this even possible?? */
    size_t random_child = (size_t)rand64() % child_len; /* TODO see the use case below. 
     * initially, I just thought I'd pick a random child and THEN check its expression,
     * but it is cooler if I go over all expressions and THEN pick a random child... or something, well, whatever,
     * this works I'm sure... or I'll just set it without modulo!... which also sucks, no??? */
    random_child = (size_t)rand64();
    /* TODO repeat that for that thing, if it lands on a '.' you know? :)
     * [..] the sole remaining timer takes a random value out of all the possibilities within that scope, 
     * >>> except for the max. count any timer can have. <<<
     */
    /* evaluate randomly */
    if(!stack) THROW(ERR_VEC_POINTER);
    //if(!diff) THROW(ERR_TRI_VAL_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    ExprBufCallbacks callbacks = {
        .number = &next_static_callback_random_num,
        .range = &next_static_callback_random_range,
        .sequence = &next_static_callback_random_sequence,
        .power = &next_static_callback_random_power,
        .times = &next_static_callback_random_num,
    };
    ExprBuf buf = {
        .callbacks = &callbacks,
    };
    Ast *child = 0;
    for(size_t i = 0; i < child_len; i++) {
        TRY(ast_child_get(node, i, &child), ERR_AST_CHILD_GET);
        TRY(expr_eval(stack, scope, child, AST_ID_FUNC_TIME, &buf), ERR_EXPR_EVAL);
    }
    /* pick one randomly from pileup */
    size_t len = vec_val_length(&buf.pileup);
    /* accept things, apply random */
    timers_free_all(run, &scope->timers);
    //if(!len) THROW("TODO"); /* TODO what the fuck do I have to do in this case? is this even possible?? */
    if(len) {
        size_t iR = (size_t)rand64() % len;
        Val t = vec_val_get_at(&buf.pileup, iR);
        TRY(timers_insert(&scope->timers, random_child, t), ERR_TIMERS_INSERT);
    }
    memset(&scope->resume, 0, sizeof(scope->resume));
    run->called_scope = 0;
    run->leave_func = false;
    scope->randomize = false;
    scope->created_timer = false;
    scope->destroy_timer = false;
    scope->resume.yes = false;
    //scope->resume.next_i = random_child;
    //printf(""FMT_VAL "\n", t);
clean:
    expr_free(&buf);
    return err;
error: ERR_CLEAN;
}


//int next_in_func_time(Ast *node)
ErrDecl next_in_func_time(VecVal *stack, Scope *scope, Ast *node, Val cmp, bool skip_current, TriVal *diff)
{
    int err = 0;
    if(!stack) THROW(ERR_VEC_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    if(!diff) THROW(ERR_TRI_VAL_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    ExprBufCallbacks callbacks = {
        .number = &next_static_handle_diff,
        .range = &next_static_callback_range,
        .sequence = &next_static_callback_sequence,
        .power = &next_static_callback_power,
        .times = &next_static_handle_diff,
    };
    ExprBuf buf = {
        .cmp = cmp,
        .skip_current = skip_current,
        .callbacks = &callbacks,
    };
    TRY(expr_eval(stack, scope, node, AST_ID_FUNC_TIME, &buf), ERR_EXPR_EVAL);
    /* TODO store recent one. you know what instruction I mean. */
    *diff = buf.diff;
    INFO(F("diff.val %zu", FG_GN), diff->val);
    INFO("done ..."); 
clean:
    expr_free(&buf);
    //}
    return err;
error: ERR_CLEAN;
}

//ErrDecl next_in_scope(Run *run, Ast *node, size_t i0, Val cmp, bool skip_current, TriVal *diff)
ErrDecl next_in_scope(Run *run, Scope *scope, Ast *node, bool skip_current, TriVal *diff)
{
    if(!run) THROW(ERR_RUN_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    //if(!*timer) THROW(ERR_TIMERS_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    //if(!*timers) THROW(ERR_TIMERS_POINTER);
    if(!diff) THROW(ERR_TRI_VAL_POINTER);
    //Timers *timer = *timers;
    size_t len = 0;
    TRY(ast_child_len(node, &len), ERR_AST_CHILD_LEN);
    if(!len) return 0; /* empty scope */
    size_t i0 = 0;
    if(!skip_current) {
        i0 = scope->timers.ll->processed % len;
    }
    size_t t_i0 = scope->timers.ll->i0;
    Val cmp = scope->timers.ll->t;
    if(scope->resume.yes) {
        i0 = scope->resume.next_i % len;
    }
    INFO(F("child len is %zu / t_i0 %zu", FG_BK_B), len, t_i0);
    //printf("t: " FMT_VAL ", %zu / %zu\n", scope->timers.ll->t, scope->timers.ll->processed, len);
    for(size_t i = i0; i < len; i++) {
        if(!skip_current) {
            scope->timers.ll->processed = i + 1;
        }
        Ast *child = 0;
        size_t i_real = (i + t_i0) % len; //(i + t_i0) % len;
        INFO("i_real = %zu", i_real);
        TRY(ast_child_get(node, i_real, &child), ERR_AST_CHILD_GET);
        //ast_inspect_mark(child, child);getchar();
        if(child->id != AST_ID_FUNC_TIME && child->id != AST_ID_FFUNC_TIME) {
            continue;
        }
        bool execute = scope->resume.yes;
        if(!scope->resume.yes) {
            TriVal diff_sub = {0};
            INFO(F("child id is %s (%p)", FG_BK_B), ast_id_str(child->id), child);
            //ast_inspect_mark(child, child);
            TRY(next_in_func_time(&run->stack, scope, child, cmp, skip_current, &diff_sub), ERR_NEXT_IN_FUNC_TIME);
            if(diff_sub.set) {
                if(!diff->set) {
                    diff->val = diff_sub.val;
                    diff->set = true;
                } else if(diff_sub.val < diff->val) {
                    diff->val = diff_sub.val;
                }
                if(!diff_sub.val) {
                    execute = true;
                }
                scope->timers.ll->last = i_real + 1;
            }
        }
        if(execute) {
            INFO(F("execute child [t = " FMT_VAL "]\n", FG_RD), cmp);
            Ast *child2 = 0;
            TRY(ast_child_get(child, 1, &child2), ERR_AST_CHILD_GET);
            if(skip_current) THROW(ERR_UNREACHABLE", trying to execute function while instructed to skip it"); /* if this is going to be an issue, also fix wherever this main function gets called from the second time in timers.c */
#if DEBUG_NEXT
            printf("[t="FMT_VAL ":%zu:", cmp, i_real);
#endif
            TRY(execute_statements(run, scope, child2), ERR_EXECUTE_STATEMENTS); /* resume here */
#if 0
            if(scope->created_timer) {
                scope->created_timer = false;
                scope->timers.ll = scope->timers.ll->next;
            }
#endif
            if(scope->randomize) {
                break;
            }
#if 0
            if(scope->created_timer && run->called_scope) {
                THROW("handle this, is it even possible?");
            }
#endif
            if(scope->created_timer) {
                scope->created_timer = false;
                break;
            }
            if(scope->destroy_timer) { /* I am fucking unsure if I need this */
                break;
            }
#if DEBUG_NEXT
            printf("]");
#endif
            if(run->called_scope) {
                scope->resume.next_i = i;
                INFO("exit function");
                break;
            }
#if 0
            scope->timers.ll->i0++;
            break;
#endif
        }
    }
    //if(!diff->set) {
    //    THROW("should be set by now :)");
    //}
    return 0;
error:
    return -1;
}

