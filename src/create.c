#include "create.h"
#include "ast.h"

#include "scope.h"
#include "err.h"
#include "attr.h"
#include "lutd.h"
#include "str.h"
#include "utf8.h"
#include "expr.h"
#include "val.h"
#include "vec.h"
#include "algebra.h"
#include "run.h"

/* TODO when creating timers 
 * step 1) pick node to create timers from
 * step 2) truly insert all numbers into a new ast (talking about things like %@$ etc.)
 * step 3) now just traverse the ast again and add timers to the list
 * */

///* TODO move to different location??? and also the AstID struct */
//VEC_INCLUDE(VecAst, vec_ast, Ast *, BY_VAL);
//VEC_IMPLEMENT(VecAst, vec_ast, Ast *, BY_VAL, 0);

/* TODO add 'static int' erroring functions to be of 'ErrDeclStatic'... (other way round doesn't work lol) */
 
#define ERR_CREATE_INSERT_PILEUP_TO_TIMERS "failed inserting buf"
ErrDeclStatic create_static_insert_pileup_to_timers(ExprBuf *buf, Timers *ptimers, size_t i0)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(!ptimers) THROW(ERR_TIMERS_POINTER);
    INFO("> PUSH PILEUP TO TIMERS");
    /* push previous timer */
    TimersLL *timers = ptimers->ll;
    for(size_t i = 0; i < buf->pileup.len; i++) { 
        Val t = vec_val_get_at(&buf->pileup, i);
        INFO("  ... push %zu", t);
        TRY(timers_insert(ptimers, i0, t), ERR_TIMERS_INSERT);
        ptimers->ll = ptimers->ll->next;
    }
    ptimers->ll = timers;
    /* prepare for next */
    return 0;
error:
    return -1;
}

ErrDeclStatic create_static_callback_number(ExprBuf *buf, Val t)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
    return 0;
error:
    return -1;
}

ErrDeclStatic create_static_callback_range(ExprBuf *buf, Val from, Val until)
{
    if(!buf) THROW(ERR_CREATE_POINTER);
    if(from > until) {
        for(Val t = from; t >= until; t--) {
            //printf("%zu ", t);
            TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
            if(t == 0) break;
        }
    } else {
        for(Val t = from; t <= until; t++) {
            TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
            //printf("%zu ", t);
            if(t >= VAL_MAX) break;
        }
    }
    return 0;
error:
    return -1;
}

ErrDeclStatic create_static_callback_sequence(ExprBuf *buf, Val offset, Val step)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(!buf->times.set) {
        Val until = buf->until.set ? buf->until.val : VAL_MAX;
        INFO(F("sequence -> offset " FMT_VAL " / step " FMT_VAL " / until " FMT_VAL "", FG_BL_B), (offset), (step), (until));
#if 1
        Val t_next = 0;
        for(Val t = offset; t <= until; t = t_next) {
            TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
            t_next = t + step;
            if(t_next < t) break;
            if(t >= VAL_MAX) break;
        }
#endif
    } else {
        Val times = buf->times.val;
        INFO(F("sequence -> offset " FMT_VAL " / step " FMT_VAL " / times " FMT_VAL "", FG_BL_B), (offset), (step), (times));
#if 1
        Val t_next = 0;
        Val t = offset;
        for(Val i = 0; i < times; i++) {
            TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
            t_next = t + step;
            if(t_next < t) break;
            if(t >= VAL_MAX) break;
            t = t_next;
        }
#endif
    }
    return 0;
error:
    return -1;
}


ErrDeclStatic create_static_callback_power(ExprBuf *buf, Val offset, Val exponent)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    Val times = buf->times.val;
    Val until = buf->until.val;
    for(Val i = 0; buf->times.set ? i < times : true; i++) {
        Val powed = 0;
        int overflow = algebra_size_pow(offset + i, exponent, &powed);
        //INFO("offset " FMT_VAL ", exponent " FMT_VAL "", offset + i, exponent);
        if(overflow) break;
        if(buf->until.set && powed > until) break;
        TRY(vec_val_push_back(&buf->pileup, powed), ERR_VEC_PUSH_BACK);
    }
    return 0;
error:
    return -1;
}
#if 0
ErrDeclStatic create_static_callback_power(ExprBuf *buf)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    size_t len = vec_val_length(&buf->order);
    Val n_0 = vec_val_get_at(&buf->order, 0); /* directly write into n_0 since we can guarantee 2 vals -> for keeping last */
    Val times = buf->times.val;
    Val until = buf->until.val;
    INFO(F("power -> offset " FMT_VAL " / times " FMT_VAL "", FG_BL_B), n_0, times);
    bool break_pls = false;
    for(Val i = 0; buf->times.set ? i < times : true; i++) {
        Val v_i = 0;
        for(size_t k = 1; k < len; k++) {
            Val n_k = vec_val_get_at(&buf->order, k);
            Val k2 = 0;
            Val powed = 0;
            int overflow = algebra_size_pow((Val)i + n_0, n_k, &powed);
            overflow |= algebra_size_pow((Val)k, 2, &k2);
            Val v_i_new = v_i + (Val)(((double)powed / (double)k2) + 0.5);
            if(overflow || v_i_new < v_i || (buf->until.set && v_i_new > until)) {
                break_pls = true;
                break;
            }
            v_i = v_i_new;
        } if(break_pls) break;
        /* EXPR_BUF_CALLBACK -> power */
        //TRY(buf->callbacks.power(buf, v_i), ERR_EXPR_BUF_CALLBACK_POWER);
        TRY(vec_val_push_back(&buf->pileup, v_i), ERR_VEC_PUSH_BACK);
    }
    return 0;
error:
    return -1;
}
#endif

static ExprBufCallbacks create_static_expr_callbacks = {
    .number = &create_static_callback_number,
    .range = &create_static_callback_range,
    .sequence = &create_static_callback_sequence,
    .power = &create_static_callback_power,
    .times = &create_static_callback_number,
};

ErrDecl create_from_new(VecVal *stack, Scope *scope, Create *create, Ast *node)
{
    //printf("[begin %s]\n", __func__);
    if(!stack) THROW(ERR_VEC_POINTER);
    if(!create) THROW(ERR_CREATE_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    Ast *parent = node;
    do {
        TRY(ast_parent_get(parent, &parent), ERR_AST_PARENT_GET);
    } while(parent && !(parent->id == AST_ID_FUNC_TIME || parent->id == AST_ID_FFUNC_TIME));
    if(!parent || !(parent->id == AST_ID_FUNC_TIME || parent->id == AST_ID_FFUNC_TIME)) {
        THROW(ERR_UNREACHABLE);
    }
    //ast_inspect_mark(parent, parent);
    size_t i0 = (parent->arri + 1);
    //INFO("i0 %zu", i0);getchar();
    ExprBuf **buf = &scope->run->create_buf;
    (*buf)->callbacks = &create_static_expr_callbacks;
    //printf("=");
    TRY(expr_eval(stack, scope, node, AST_ID_NEW, *buf), ERR_EXPR_EVAL);
    TRY(create_static_insert_pileup_to_timers(*buf, &scope->timers, i0), ERR_CREATE_INSERT_PILEUP_TO_TIMERS);
    /* TODO store recent one. you know what instruction I mean. */
    INFO("done ..."); 
    if(!(*buf)->prev_op_count) {
        /* didn't use the previous one, get rid of it and replace it with a new one (?) */
        ExprBuf *keep = *buf;
        *buf = 0;
        TRY(expr_malloc(buf), ERR_EXPR_MALLOC);
        (*buf)->prev = keep;
        keep->node = node;
        if(keep->prev) expr_free_p(keep->prev);
        keep->prev = 0;
    } else {
        ExprBuf *keep = *buf;
        *buf = 0;
        TRY(expr_malloc(buf), ERR_EXPR_MALLOC);
        (*buf)->prev = keep;
        keep->node= node;
        //THROW("todo");
    }
    //printf("CREATED\n");
    //printf("[end %s]\n", __func__);
    //expr_free(*buf);
    return 0;
error:
    return -1;
}

