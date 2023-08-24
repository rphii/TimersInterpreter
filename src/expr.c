#include "expr.h"
#include "ast.h"
#include "algebra.h"
#include "val.h"
#include "lex.h"
#include "scope.h"
#include "run.h"

ErrDecl expr_decode(VecVal *stack, Scope *scope, Ast *node, Ast **decoded, AstIDList id)
{
    if(!stack) THROW(ERR_VEC_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    if(!decoded) THROW(ERR_AST_POINTER);
    /* fetch; verify it's a new node, copy if FNEW */
    AstIDList replace_with = AST_ID_NONE;
    switch(id) {
        case AST_ID_NEW: { replace_with = AST_ID_FNEW; } break;
        case AST_ID_FUNC_TIME: { replace_with = AST_ID_FFUNC_TIME; } break;
        default: THROW(ERR_UNHANDLED_ID" %s", ast_id_str(id));
    }
    bool alloc = (node->id == replace_with);
    INFO("fetch ...");
    if(!(node->id == id || node->id == replace_with)) {
        THROW("expected id %s or %s (is %s)", ast_id_str(id), ast_id_str(replace_with), ast_id_str(node->id));
    }
    Ast *fetch = node;
    Ast *numbers = 0; /* used for traversal when creating timers */
#if 0 /* TODO this should actually be good? maybe */
    if(replace_with == AST_ID_FFUNC_TIME) {
        printf("NEW/FNEW:\n");
        //platform_getchar();
        TRY(ast_child_get(node, 0, &node), ERR_AST_CHILD_GET);
        if(node->id != AST_ID_EXPRESSION) THROW(ERR_UNREACHABLE);
    }
#endif
    if(alloc) {
        TRY(ast_malloc(decoded), ERR_AST_MALLOC);
        TRY(ast_copy_deep(*decoded, false, node), ERR_AST_COPY_DEEP);
        numbers = *decoded;
#if 0
#if 1||DEBUG_INSPECT
    ast_inspect_mark(numbers, numbers);
#endif
#endif
    } else {
        *decoded = fetch;
        numbers = fetch;
    }
    if(!numbers) THROW(ERR_UNREACHABLE);
    /* decode; if necessary */
    INFO("decode ...");
    size_t child_len = 0;
    TRY(ast_child_len(numbers, &child_len), ERR_AST_CHILD_LEN);
    //printf("%zu...", child_len);
    //ast_inspect_mark(numbers, numbers);
    if(node->id == replace_with) {
        Ast *decode = numbers;
        Ast *decode_prev = decode;
        ssize_t layer = 0;
        TRY(ast_next(&decode, &layer), ERR_AST_NEXT);
        while(decode && layer > 0) {
            Ast *parent = 0;
            TRY(ast_parent_get(decode, &parent), ERR_AST_PARENT_GET);
            if(!ast_has_child(decode)) {
                INFO("%s:", ast_id_str(decode->id));
                if(decode->id == AST_ID_FNUM) {
                    size_t len = vec_val_length(stack);
                    INFO(F(" c=%c", FG_YL_B), decode->branch.ch);
                    switch(decode->branch.ch) {
                        case LEX_CH_MAX: {
                            decode->branch.num = VAL_MAX;
                        } break;
                        case LEX_CH_0_IF_EMPTY: {
                            if(!vec_val_length(stack)) {
                                decode->branch.num = 0;
                                decode->id = AST_ID_INT;
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_MAX_IF_NEMPTY: {
                            if(vec_val_length(stack)) {
                                decode->branch.num = VAL_MAX;
                                decode->id = AST_ID_INT;
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_TOP: {
                            if(len >= 1) {
                                Val v = vec_val_get_back(stack);
                                decode->branch.num = v;
                                decode->id = AST_ID_INT;
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_BELOW: {
                            if(len >= 2) {
                                Val v = vec_val_get_at(stack, len - 2);
                                decode->branch.num = v;
                                decode->id = AST_ID_INT;
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_AT_TOP: {
                            if(len >= 1) {
                                Val n = vec_val_get_back(stack);
                                if(n < (Val)len && (Val)((size_t)n) == n) {
                                    Val v = vec_val_get_at(stack, (size_t)n);
                                    decode->branch.num = v;
                                    decode->id = AST_ID_INT;
                                } else {
                                    decode->id = AST_ID_EMPTY;
                                }
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_AT_BELOW: {
                            if(len >= 2) {
                                Val n = vec_val_get_at(stack, len - 2);
                                if(n < (Val)len && (Val)((size_t)n) == n) {
                                    Val v = vec_val_get_at(stack, (size_t)n);
                                    decode->branch.num = v;
                                    decode->id = AST_ID_INT;
                                } else {
                                    decode->id = AST_ID_EMPTY;
                                }
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_TOP_IFN0: {
                            if(len >= 1) {
                                Val v = vec_val_get_back(stack);
                                if(v) {
                                    decode->branch.num = v;
                                    decode->id = AST_ID_INT;
                                } else {
                                    decode->id = AST_ID_EMPTY;
                                }
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_CMP_TMRS_GT: {
                            size_t len_timers = scope->timers.len;
                            if(len_timers > len) {
                                decode->branch.num = (Val)len_timers;
                                decode->id = AST_ID_INT;
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_CMP_TMRS_LT: {
                            size_t len_timers = scope->timers.len;
                            if(len_timers < len) {
                                decode->branch.num = (Val)len;
                                decode->id = AST_ID_INT;
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_SCOPE_DEPTH: {
//TODO wtf?
//#include "trace.h"
//trace_print();
//printf(";\n");
                            size_t depth = vec_scope_length(&scope->run->scopes); //*scope->len;
                            decode->branch.num = (Val)depth;
                            decode->id = AST_ID_INT;
                        } break;
                        case LEX_CH_SCOPE_NAME: {
                                //printf("?");
                            decode->branch.str = scope->scope_name;
                            if(decode->branch.str) {
                                decode->id = AST_ID_MATCH;
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_LAST_DEL: {
                            if(scope->run->last_destroyed.set) {
                                decode->branch.num = scope->run->last_destroyed.val;
                                decode->id = AST_ID_INT;
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_LAST_DIFF: {
                            if(scope->run->last_diff) {
                                decode->branch.num = scope->run->last_diff;
                                decode->id = AST_ID_INT;
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_LAST_VAL: {
                            ExprBuf *buf = scope->run->create_buf;
                            //printf("[a]");
                            if(buf->prev) {
                                //printf("%%");
                                //printf("[b]");
                                VecVal *last_created = &buf->prev->pileup;
                                //printf("[%zu]", vec_val_length(last_created));
                                decode->branch.vec = last_created;
                                decode->id = AST_ID_VEC;
                            } else {
                                decode->id = AST_ID_EMPTY;
                            }
                        } break;
                        case LEX_CH_NUM_TIMERS: {
                            Val n = (Val)scope->timers.len;
                            decode->branch.num = n;
                            decode->id = AST_ID_INT;
                        } break;
                        case LEX_CH_SUM_TIMERS: {
                            Val t = scope->timers.sum;
                            decode->branch.num = t;
                            decode->id = AST_ID_INT;
                        } break;
                        case LEX_CH_INC_TIMERS: {
                            decode->branch.num = scope->run->inc_timers;
                            decode->id = AST_ID_INT;
                        } break;
                        default: THROW("not implemented decode : %s (%c)", ast_id_str(decode->id), decode->branch.ch);
                    }
                } else if(decode->id == AST_ID_EMPTY) {
                    switch(parent->id) {
                        case AST_ID_RANGE: {
                            if(decode->arri == 0) {
                                /* empty left side */
                                decode->branch.num = 0;
                                decode->id = AST_ID_INT;
                            } else {
                                /* empty right side */
                                decode->branch.num = VAL_MAX;
                                decode->id = AST_ID_INT;
                            }
                        } break;
                        case AST_ID_SEQUENCE: {
                            if(decode->arri == 0) {
                                /* empty left side */
                                decode->branch.num = 0;
                                decode->id = AST_ID_INT;
                            } else {
                                /* empty right side */
                                //if(decode_prev->id != AST_ID_MATCH) /* TODO maybe... MAYBE just ... not care about prev->id ??? just always call? */ {
                                memcpy(&decode->branch, &decode_prev->branch, sizeof(decode->branch));
                                decode->id = decode_prev->id;
                                //} else THROW("what to do?");
                            }
                        } break;
                        case AST_ID_POWER: {
                            if(decode->arri == 0) {
                                /* empty left side */
                                decode->branch.num = 0;
                                decode->id = AST_ID_INT;
                            } else {
                                /* empty right side */
                                memcpy(&decode->branch, &decode_prev->branch, sizeof(decode->branch));
                                decode->id = decode_prev->id;
                            }
                        } break;
                        case AST_ID_TIMES: {
                            if(decode->arri == 0) {
                                /* empty left side */
                            } else {
                                /* empty right side */
                                //if(decode_prev->id != AST_ID_MATCH) /* TODO maybe... MAYBE just ... not care about prev->id ??? just always call? */ {
                                    memcpy(&decode->branch, &decode_prev->branch, sizeof(decode->branch));
                                    decode->id = decode_prev->id;
                                //} else THROW("what to do?");
                            }
                        } break;
                        case AST_ID_TERM: {
                            decode->id = AST_ID_INT;
                            decode->branch.num = 0;
                            /* TODO does it make sense to just set it to 0? I think yes :) */
                            //INFO("empty term... useless :)");
                        } break;
                        default: {
                            THROW("not handled all cases of %s with parent %s", ast_id_str(AST_ID_EMPTY), ast_id_str(parent->id)); /* TODO make sure each case is handled... */
                        } break;
                    }
                }
            }
            /* keep below */
            decode_prev = decode;
            TRY(ast_next(&decode, &layer), ERR_AST_NEXT);
        }
    }
#if 0
#if 1||DEBUG_INSPECT
    printf("THE FINAL AST:\n");
    ast_inspect_mark(numbers, numbers);
    platform_getchar();
#endif
#endif
    return 0;
error:
    return -1;
}

void expr_decode_free(Ast *node, Ast *decoded)
{
    if(!node) return;
    if(!decoded) return;
    if(node != decoded) {
        ast_free(decoded, false, false);
        free(decoded);
    }
}

#if 0
#define ERR_EXPR_CMP_SEQUENCE "failed comparing sequence"
ErrDeclStatic expr_static_cmp_sequence(ExprBuf *buf, Val cmp, bool skip_current, TriVal *diff)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(!diff) THROW(ERR_SIZE_T_POINTER);
    /* expecting two items in buf order */
    Val offset = 0;
    Val step = 0;
    size_t len = vec_val_length(&buf->order);
    if(buf->kind != AST_ID_SEQUENCE) THROW("should have kind set correctly");
    if(len >= 2) {
        //if(len < 1) THROW("expected buf order len to be at least 2 (is %zu)", len);
        for(size_t i = 0; i < len; i++) {
            vec_val_pop_front(&buf->order, &step);
            INFO("len %zu / popped " FMT_VAL "", vec_val_length(&buf->order), (step));
            /* keep above */
            if(i > 0) {
                if(!buf->times.set) {
#if 1
                    Val t = 0;
                    Val t_until = buf->until.set ? buf->until.val : VAL_MAX;
                    Val t_cmp = skip_current ? cmp + step : cmp;
                    INFO(F("sequence -> offset " FMT_VAL " / step " FMT_VAL " / until " FMT_VAL "", FG_BL_B), (offset), (step), (t_until));
                    if(t_cmp >= offset && t_cmp <= t_until) {
                        t = t_cmp;
                    } else {
                        t = offset;
                    }
                    /*

                       Val t = 0;
                       Val t_from = from <= until ? from : until;
                       Val t_until = from <= until ? until : from;
                       Val t_cmp = skip_current ? cmp + 1 : cmp;
                       INFO(F("from " FMT_VAL " until " FMT_VAL "", FG_BL_B), (t_from), (t_until));
                       if(t_cmp >= from && t_cmp <= until) {
                           t = t_cmp;
                       } else {
                           t = from <= until ? from : until;
                       }
                       TRY(expr_static_handle_diff(t_cmp, t, skip_current, diff), ERR_EXPR_HANDLE_DIFF);

                     */

#else
                    INFO(F("sequence -> offset " FMT_VAL " / step " FMT_VAL " / until " FMT_VAL "", FG_BL_B), (offset), (step), (until));
#if 1
                    Val t_next = 0;
                    for(Val t = offset; t <= until; t = t_next) {
                        //TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
                        t_next = t + step;
                        if(t_next < t) break;
                        if(t >= VAL_MAX) break;
                    }
#endif
#endif
                } else {
                    Val times = buf->times.val;
                    INFO(F("sequence -> offset " FMT_VAL " / step " FMT_VAL " / times " FMT_VAL "", FG_BL_B), (offset), (step), (times));
#if 1
                    Val t_next = 0;
                    Val t = offset;
                    for(Val i = 0; i < times; i++) {
                        //TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
                        t_next = t + step;
                        if(t_next < t) break;
                        if(t >= VAL_MAX) break;
                        t = t_next;
                    }
#endif
                }
            }
            /* keep last */
            offset = step;
        }
    } else if(len) {
        /* example case [''+2#] --> note, the empty LEADING string... */
        TRY(expr_static_cmp_number(buf, cmp, skip_current, diff), ERR_EXPR_CMP_NUMBER);
    }
    /* keep last */
    if(!buf->times.set && !buf->until.set) {
        TRY(vec_val_push_back(&buf->order, step), ERR_VEC_PUSH_BACK);
    }
    /* clear */
    buf->times.set = false;
    buf->until.set = false;
    buf->kind = AST_ID_NONE;
#if 0 /* IGNORE THE COMMENT BELOW ? */
    /* unlike with others, we don't want to push back the last t, since 
     * that's basically done in expr_static_limit_sequence, in where we
     * decided to not push that time, as it was added to buf->times/buf->until
     * which makes it easier to handle the necessary values within bur->order here
     */
#endif
    return 0;
error:
    return -1;
}
#endif

#if 0
ErrDecl expr_number_check(Ast *node, Val t, bool skip_current, TriVal *diff)
{
    /* TODO TODO TODO TODO TODO TODO TODO TODO
     * TODO break early whenever possible TODO
     * TODO TODO TODO TODO TODO TODO TODO TODO
     */
    if(!node) THROW(ERR_AST_POINTER);
    if(!diff) THROW(ERR_SIZE_T_POINTER);
    ssize_t layer = 0;
    ExprBuf buf = {0};
    Ast *parent_prev = 0;
    TRY(ast_parent_get(node, &parent_prev), ERR_AST_PARENT_GET);
    TRY(ast_next(&node, &layer), ERR_AST_NEXT);
    while(layer > 0) {
        Ast *parent = 0;
        TRY(ast_parent_get(node, &parent), ERR_AST_PARENT_GET);
        if(!ast_has_child(node)) {
            INFO("%s", ast_id_str(node->id));
            if((buf.kind == AST_ID_TIMES && buf.kind != parent->id)
                    || (buf.kind == AST_ID_SEQUENCE && buf.kind != parent_prev->id)
              ) {
                TRY(expr_static_cmp_handle_kind(&buf, t, skip_current, diff), ERR_EXPR_CMP_HANDLE_KIND);
            }
            TRY(expr_static_get_val_id(&buf, node), ERR_EXPR_GET_VAL_ID);
            size_t len_order = vec_val_length(&buf.order);
            bool did_something = false;
            /* sequence, power, times */
            if(!did_something && parent->id == AST_ID_SEQUENCE) {
                if(!buf.kind) INFO("set sequence...");
                buf.kind = parent->id;
                did_something = true;
            }
            if(!did_something && parent->id == AST_ID_POWER) {
                if(!buf.kind) INFO("set power...");
                buf.kind = parent->id;
                did_something = true;
            }
            if(!did_something && parent->id == AST_ID_TIMES) {
                if(!buf.kind) INFO("set times... / prev %s", ast_id_str(parent_prev->id));
                if(parent_prev->id != AST_ID_SEQUENCE && parent_prev->id != AST_ID_POWER) {
                    buf.kind = parent->id;
                    did_something = true;
                }
            }
            /* handle ranges */
            if(!did_something && parent->id == AST_ID_RANGE) {
                INFO("create range");
                TRY(expr_static_cmp_range(&buf, t, skip_current, diff), ERR_EXPR_CMP_RANGE);
                did_something = true;
            }
            /* handle sole numbers laying around */
            if(!did_something && parent->id == AST_ID_TERM) {
                if(len_order >= 1) { /* TODO this may be removed if I just always push all numbers available on this function call :) */
                    TRY(expr_static_cmp_number(&buf, t, skip_current, diff), ERR_EXPR_CMP_NUMBER);
                    did_something = true;
                }
            }
        }
        /* all the other stuff */
        if(parent->id == AST_ID_EXPRESSION || parent->id == AST_ID_TERM) {
            TRY(expr_static_cmp_handle_kind(&buf, t, skip_current, diff), ERR_EXPR_CMP_HANDLE_KIND);
            TRY(expr_static_purge_until(&buf, 0), ERR_EXPR_PURGE_UNTIL);
        }
        /* keep below last */
        parent_prev = parent;
        TRY(ast_next(&node, &layer), ERR_AST_NEXT);
    }
    /* clean up */
    vec_val_free(&buf.pileup);
    vec_val_free(&buf.order);
    return 0;
error:
    return -1;
}
#endif




/*****************************************************************/


/* TODO add 'static int' erroring functions to be of 'ErrDeclStatic'... (other way round doesn't work lol) */
 
#if 0
#define ERR_EXPR_PILEUP_RANGE "failed to create pileup range"
ErrDeclStatic expr_static_pileup_range(ExprBuf *buf, size_t from, size_t until)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(from <= until) {
        for(size_t i = from; i <= until; i++) {
            TRY(vec_val_push_back(&buf->pileup, i), ERR_VEC_PUSH_BACK);
            /* keep below last */
            if(i == SIZE_MAX) break;
        }
    } else {
        for(size_t i = from; i >= until; i--) {
            TRY(vec_val_push_back(&buf->pileup, i), ERR_VEC_PUSH_BACK);
            /* keep below last */
            if(i == 0) break;
        }
    }
    return 0;
error:
    return -1;
}
#endif

#define ERR_EXPR_SEQUENCE "failed to create sequence"
ErrDeclStatic expr_static_sequence(ExprBuf *buf);

#define ERR_EXPR_POWER "failed creating power"
ErrDeclStatic expr_static_power(ExprBuf *buf);

#define ERR_EXPR_NUMBER "failed to create number"
ErrDeclStatic expr_static_number(ExprBuf *buf);

#define ERR_EXPR_LIMIT_SEQUENCE "failed limiting sequence"
ErrDeclStatic expr_static_limit_sequence(ExprBuf *buf, Ast *node, Val t)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    if(buf->kind != AST_ID_SEQUENCE) THROW("kind should be correct");
    Ast *parent = 0;
    TRY(ast_parent_get(node, &parent), ERR_AST_PARENT_GET);
    if(parent->id == AST_ID_RANGE && !buf->until.set) {
        INFO("limit sequence [range]!!! " FMT_VAL "", (t));
        buf->until.val = t;
        buf->until.set = true;
        TRY(expr_static_sequence(buf), ERR_EXPR_SEQUENCE);
    } else if(parent->id == AST_ID_TIMES && !buf->times.set) {
        INFO("limit sequence [times]!!! " FMT_VAL "", (t));
        buf->times.val = t;
        buf->times.set = true;
        TRY(expr_static_sequence(buf), ERR_EXPR_SEQUENCE);
    }
    return 0;
error:
    return -1;
}

#define ERR_EXPR_LIMIT_POWER "failed limiting power"
ErrDeclStatic expr_static_limit_power(ExprBuf *buf, Ast *node, Val t)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    if(buf->kind != AST_ID_POWER) THROW("kind should be correct");
    Ast *parent = 0;
    TRY(ast_parent_get(node, &parent), ERR_AST_PARENT_GET);
    if(parent->id == AST_ID_RANGE && !buf->until.set) {
        INFO("limit power [range]!!! " FMT_VAL "", (t));
        buf->until.val = t;
        buf->until.set = true;
        TRY(expr_static_power(buf), ERR_EXPR_SEQUENCE);
    } else if(parent->id == AST_ID_TIMES && !buf->times.set) {
        INFO("limit power [times]!!! " FMT_VAL "", (t));
        buf->times.val = t;
        buf->times.set = true;
        TRY(expr_static_power(buf), ERR_EXPR_SEQUENCE);
    }
    return 0;
error:
    return -1;
}

#define ERR_EXPR_GET_VAL_ID "failed getting val & id"
static int expr_static_get_val_id(ExprBuf *buf, Ast *node)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    switch(node->id) {
        case AST_ID_INT: {
            Val t = node->branch.num;
            if(buf->kind == AST_ID_SEQUENCE) {
                TRY(expr_static_limit_sequence(buf, node, t), ERR_EXPR_LIMIT_SEQUENCE);
            } else if(buf->kind == AST_ID_POWER) {
                TRY(expr_static_limit_power(buf, node, t), ERR_EXPR_LIMIT_SEQUENCE);
            }
            /* add by default */
            //INFO("push t " FMT_VAL " (num)", (t));
            TRY(vec_val_push_back(&buf->order, t), ERR_VEC_PUSH_BACK);
        } break;
        case AST_ID_MATCH: {
            Str *ref = node->branch.str;
            U8Point u8p = {0};
            for(size_t i = 0; i < ref->l; i++) {
                char *s = &ref->s[i];
                TRY(str_to_u8_point(s, &u8p), ERR_STR_TO_U8_POINT);
                i += ((size_t)u8p.bytes - 1);
                Val t = u8p.val;
                /* keep last; check for buf, if SEQUENCE or EXPONENT */
                //INFO("buf->kind %s", ast_id_str(buf->kind));
                if(buf->kind == AST_ID_SEQUENCE) {
                    TRY(expr_static_limit_sequence(buf, node, t), ERR_EXPR_LIMIT_SEQUENCE);
                }
                /* add by default */
                //INFO("push t " FMT_VAL " (str)", (t));
                TRY(vec_val_push_back(&buf->order, t), ERR_VEC_PUSH_BACK);
            }
        } break;
        case AST_ID_VEC: {
            VecVal *vec = node->branch.vec;
            //printf("{%zu}", vec_val_length(vec));
            for(size_t i = 0; i < vec_val_length(vec); i++) {
                Val t = vec_val_get_at(vec, i);
                //printf("<%zu:"FMT_VAL ">", i, t);
                TRY(vec_val_push_back(&buf->order, t), ERR_VEC_PUSH_BACK);
            }
        } break;
        case AST_ID_EMPTY: {
            //THROW("skill issue"); /* TODO is it okay to not do anything here,,, really? */
            /* TODO fuck this shit, I have to know which mode I am in or what the fuck? */
        } break;
        default: THROW(ERR_UNHANDLED_ID" %s", ast_id_str(node->id));
    }
    //INFO("order len %zu", vec_val_length(&buf->order));
    return 0;
error:
    return -1;
}

#if 0
#define ERR_EXPR_INSERT_PILEUP_TO_TIMERS "failed inserting buf"
static int expr_static_insert_pileup_to_timers(ExprBuf *buf, Timers **ptimers, size_t i0)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(!ptimers) THROW(ERR_TIMERS_POINTER);
    if(!*ptimers) THROW(ERR_TIMERS_POINTER);
    //TRY(expr_static_push_to_pileup(buf, TRI_SIZE_INIT), ERR_EXPR_PUSH_TO_LUT);
    INFO("> PUSH PILEUP TO TIMERS");
    /* push previous timer */
#if 1
    Timers *timers = *ptimers;
    for(size_t i = 0; i < buf->pileup.len; i++) { /* TODO IMPORTANT make this a sorted thing; LUT! */
        Val t = vec_val_get_at(&buf->pileup, i);
        //INFO("  ... push %zu", t);
        TRY(timers_insert(timers, i0, t), ERR_TIMERS_INSERT);
        timers = timers->next; /* TODO make this a function */
    }
    /* prepare for next */
    *ptimers = timers;
    vec_val_clear(&buf->pileup);
#endif
    return 0;
error:
    return -1;
}
#endif

#define ERR_EXPR_RANGE "failed to create range"
ErrDeclStatic expr_static_range(ExprBuf *buf)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    /* expecting two items in buf order */
    Val until = 0;
    Val from = 0;
    size_t len = vec_val_length(&buf->order);
    if(len >= 2) {
        for(size_t i = 0; i < len; i++) {
            vec_val_pop_front(&buf->order, &until);
            INFO("popped " FMT_VAL "", (until));
            if(i > 0) {
                INFO(F("from " FMT_VAL " until " FMT_VAL "", FG_BL_B), (from), (until));
                /* EXPR_BUF_CALLBACK -> range */
                TRY(buf->callbacks->range(buf, from, until), ERR_EXPR_BUF_CALLBACK_NUMBER);
#if 0
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
#endif
                //printf("\n");
            }
            /* keep last */
            from = until;
        }
        /* keep last */
        vec_val_clear(&buf->order);
        TRY(vec_val_push_back(&buf->order, until), ERR_VEC_PUSH_BACK);
    } else if (len) {
        /* example case ['a'-''] --> note, the empty string... */
        /* NOTE don't do the thing below, as thought of above, 
         * or certain patterns get confusing and nonsensical. so
         * the example above just spawns nothing.
         * TRY(expr_static_number(buf), ERR_EXPR_NUMBER);
         */
    }

    return 0;
error:
    return -1;
}


ErrDeclStatic expr_static_sequence(ExprBuf *buf)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    /* expecting two items in buf order */
    Val offset = 0;
    Val step = 0;
    size_t len = vec_val_length(&buf->order);
    if(buf->kind != AST_ID_SEQUENCE) THROW("should have kind set correctly");
    if(len >= 2) {
        for(size_t i = 0; i < len; i++) {
            offset = step; /* keep first */
            vec_val_pop_front(&buf->order, &step);
            INFO("len %zu / popped " FMT_VAL "", vec_val_length(&buf->order), (step));
            if(!i) continue;
            TRY(buf->callbacks->sequence(buf, offset, step), ERR_EXPR_BUF_CALLBACK_SEQUENCE);
        }
    } else if(len) {
        /* example case [''+2#] --> note, the empty LEADING string... */
        TRY(expr_static_number(buf), ERR_EXPR_NUMBER);
    }
    /* keep last */
    if(!buf->times.set && !buf->until.set) {
        TRY(vec_val_push_back(&buf->order, step), ERR_VEC_PUSH_BACK);
    }
    /* clear */
    buf->times.set = false;
    buf->until.set = false;
    buf->kind = AST_ID_NONE;
#if 0 /* IGNORE THE COMMENT BELOW ? */
    /* unlike with others, we don't want to push back the last t, since 
     * that's basically done in expr_static_limit_sequence, in where we
     * decided to not push that time, as it was added to buf->times/buf->until
     * which makes it easier to handle the necessary values within bur->order here
     */
#endif
    return 0;
error:
    return -1;
}

ErrDeclStatic expr_static_power(ExprBuf *buf)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    /* expecting two items in buf order */
    Val offset = 0;
    Val exponent = 0;
    size_t len = vec_val_length(&buf->order);
    if(buf->kind != AST_ID_POWER) THROW("should have kind set correctly");
    if(len >= 2) {
        vec_val_pop_front(&buf->order, &offset);
        //INFO("OFFSET " FMT_VAL "", offset);
        for(size_t i = 1; i < len; i++) {
            //offset = exponent; /* keep first */
            vec_val_pop_front(&buf->order, &exponent);
            //INFO("len %zu / popped " FMT_VAL "", vec_val_length(&buf->order), (exponent));
            //if(!i) continue; /* TODO DO I NEED THIS ??? */
            TRY(buf->callbacks->power(buf, offset, exponent), ERR_EXPR_BUF_CALLBACK_SEQUENCE);
        }
    } else if(len) {
        /* example case [''*2#] --> note, the empty LEADING string... */
        TRY(expr_static_number(buf), ERR_EXPR_NUMBER);
    }
    /* keep last */
    if(!buf->times.set && !buf->until.set) {
        TRY(vec_val_push_back(&buf->order, exponent), ERR_VEC_PUSH_BACK);
    }
    /* clear */
    buf->kind = AST_ID_NONE;
    buf->times.set = false;
    buf->until.set = false;
    return 0;
error:
    return -1;
}

#define ERR_EXPR_TIMES "failed creating times"
ErrDeclStatic expr_static_times(ExprBuf *buf)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
#if 1
    /* expecting two items in buf order */
    Val n1 = 0;
    Val n2 = 0;
    size_t len = vec_val_length(&buf->order);
    if(buf->kind != AST_ID_TIMES) THROW("should have kind set correctly");
    if(len >= 2) {
        vec_val_pop_front(&buf->order, &n1);
        //INFO("OFFSET " FMT_VAL "", n1);
        for(size_t i = 1; i < len; i++) {
            //offset = exponent; /* keep first */
            vec_val_pop_front(&buf->order, &n2);
            INFO("len %zu / popped " FMT_VAL "", vec_val_length(&buf->order), (n2));
            //if(!i) continue;
            Val t = n1 * n2;
            if(t < n1 || t < n2) break;
            TRY(buf->callbacks->times(buf, t), ERR_EXPR_BUF_CALLBACK_SEQUENCE);
        }
    } else if(len) {
        //THROW("find equivalent example as shown in the code below");
        /* example case [''+2#] --> note, the empty LEADING string... */
        TRY(expr_static_number(buf), ERR_EXPR_NUMBER);
    }
    /* keep last */
    if(!buf->times.set && !buf->until.set) {
        TRY(vec_val_push_back(&buf->order, n2), ERR_VEC_PUSH_BACK);
    }
    /* clear */
    buf->kind = AST_ID_NONE;
#else
    Val t = 0;
    Val temp = 0;
    size_t len = vec_val_length(&buf->order);
    if(len < 1) THROW("expected buf order len to be at least 1 (is %zu)", len);
    vec_val_pop_front(&buf->order, &t);
    for(size_t i = 1; i < len; i++) {
        vec_val_pop_front(&buf->order, &temp);
        INFO("times sum up " FMT_VAL " *= " FMT_VAL "", t, temp);
        t *= temp;
        /* TODO what if(t * temp < t) ??? -> for now, ignore */
    }
    INFO(F("times result " FMT_VAL "", FG_BL_B), t);
    /* EXPR_BUF_CALLBACK -> times */ 
    TRY(buf->callbacks->times(buf, t), ERR_EXPR_BUF_CALLBACK_TIMES);
    //TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
    /* keep last */
    vec_val_clear(&buf->order);
    TRY(vec_val_push_back(&buf->order, temp), ERR_VEC_PUSH_BACK);
    /* clear */
    buf->kind = AST_ID_NONE;
#endif
    return 0;
error:
    return -1;
}

ErrDeclStatic expr_static_number(ExprBuf *buf)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    Val t = 0;
    size_t len = vec_val_length(&buf->order);
    if(len < 1) THROW("expected buf order len to be at least 1 (is %zu)", len);
    for(size_t i = 0; i < len; i++) {
        vec_val_pop_front(&buf->order, &t);
        INFO(F("number " FMT_VAL "", FG_BL_B), t);
        //printf(F("number " FMT_VAL "\n", FG_BL_B), t);
        /* EXPR_BUF_CALLBACK -> number */
        TRY(buf->callbacks->number(buf, t), ERR_EXPR_BUF_CALLBACK_NUMBER);
        //TRY(vec_val_push_back(&buf->pileup, t), ERR_VEC_PUSH_BACK);
    }
    /* keep last */
    vec_val_clear(&buf->order);
    TRY(vec_val_push_back(&buf->order, t), ERR_VEC_PUSH_BACK);
    return 0;
error:
    return -1;
}

#if 0
#define ERR_EXPR_SEQUENCE "failed to create sequence"
ErrDeclStatic expr_static_sequence(ExprBuf *buf)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    return 0;
error:
    return -1;
}
#endif

#define ERR_EXPR_PURGE_UNTIL "failed purging"
ErrDeclStatic expr_static_purge_until(ExprBuf *buf, size_t keep)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(keep) {
        while(vec_val_length(&buf->order) > keep) {
            vec_val_pop_front(&buf->order, 0);
        }
    } else {
        vec_val_clear(&buf->order);
    }
    return 0;
error:
    return -1;
}

#define ERR_EXPR_HANDLE_KIND "failed handling kind"
ErrDeclStatic expr_static_handle_kind(ExprBuf *buf)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    switch(buf->kind) {
        case AST_ID_SEQUENCE: {
            TRY(expr_static_sequence(buf), ERR_EXPR_SEQUENCE);
        } break;
        case AST_ID_POWER: {
            TRY(expr_static_power(buf), ERR_EXPR_POWER);
        } break;
        case AST_ID_TIMES: {
            TRY(expr_static_times(buf), ERR_EXPR_TIMES);
        } break;
        case AST_ID_NONE: {} break;
        default: THROW("unknown id %s in kind", ast_id_str(buf->kind));
    }
    if(buf->kind) THROW("make sure to clear kind (%s)", ast_id_str(buf->kind));
    return 0;
error:
    return -1;
}

#if 0
int expr_fetch_decode(Timers *timers, Create *create, Ast *node)
{
    if(!create) THROW(ERR_EXPR_BUF_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    INFO("create ..."); 
    //{
    ExprBuf buf = {.callbacks = (ExprBufCallbacks){
        .number = &expr_static_callback_number,
        .range = &expr_static_callback_range,
        .sequence = &expr_static_callback_sequence,
        .power = &expr_static_callback_number,
        .times = &expr_static_callback_number,
    }};
#endif

//int expr_eval(Ast *node, AstIDList id_decode, ExprBuf *buf)
/* TODO get rid of *node? since it's in scope->origin ..? */
ErrDecl expr_eval(VecVal *stack, Scope *scope, Ast *node, AstIDList id_decode, ExprBuf *buf)
{
    int err = 0;
    Ast *numbers = 0;
    if(!stack) THROW(ERR_VEC_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(!buf->callbacks) THROW(ERR_EXPR_BUF_CALLBACKS_POINTER);
    if(!buf->callbacks->number) THROW(ERR_EXPR_BUF_CALLBACK_NUMBER);
    if(!buf->callbacks->range) THROW(ERR_EXPR_BUF_CALLBACK_RANGE);
    if(!buf->callbacks->sequence) THROW(ERR_EXPR_BUF_CALLBACK_SEQUENCE);
    if(!buf->callbacks->power) THROW(ERR_EXPR_BUF_CALLBACK_POWER);
    if(!buf->callbacks->times) THROW(ERR_EXPR_BUF_CALLBACK_TIMES);
    //if(!cb) THROW(ERR_EXPR_BUF_CALLBACKS_POINTER);
    //ExprBuf buf = {.callbacks = *cb};
    TRY(expr_decode(stack, scope, node, &numbers, id_decode), ERR_EXPR_DECODE);
    //size_t i0 = numbers->arri;
    size_t child_len = 0;
    TRY(ast_child_len(numbers, &child_len), ERR_AST_CHILD_LEN);
    if(id_decode != AST_ID_NEW) {
        child_len = 1;
        INFO("child len %zu --> traverse from back to front!!! to keep the new order", child_len);
    }
    //printf("***********\n");
    //Ast *expr_top = 0;
    for(size_t i = child_len; i > 0; i--) {
        INFO("accessing index %zu", i-1);
        ssize_t layer = 0;
        Ast *expr = 0;
        Ast *parent_prev = 0;
        TRY(ast_child_get(numbers, i - 1, &expr), ERR_AST_CHILD_GET);
        TRY(ast_parent_get(expr, &parent_prev), ERR_AST_PARENT_GET);
        //printf("[[[%s]]]\n", ast_id_str(expr->id));
        TRY(ast_next(&expr, &layer), ERR_AST_NEXT);
        while(layer > 0) {
            //printf("[%s]\n", ast_id_str(expr->id));
            Ast *parent = 0;
            TRY(ast_parent_get(expr, &parent), ERR_AST_PARENT_GET);
            //if(!expr_top && parent->id == AST_ID_EXPRESSION) {
            //    expr_top = parent;
            //}
            if(!ast_has_child(expr)) {
                INFO("kind %s / parent %s / prev %s", ast_id_str(buf->kind), ast_id_str(parent->id), ast_id_str(parent_prev->id));
                if((buf->kind == AST_ID_TIMES && buf->kind != parent->id)
                        || (buf->kind == AST_ID_SEQUENCE && buf->kind != parent_prev->id)
                  ) {
                    TRY(expr_static_handle_kind(buf), ERR_EXPR_HANDLE_KIND);
                }
                TRY(expr_static_get_val_id(buf, expr), ERR_EXPR_GET_VAL_ID);
                size_t len_order = vec_val_length(&buf->order);
                bool did_something = false;
                /* sequence, power, times */
                if(!did_something && parent->id == AST_ID_SEQUENCE) {
                    if(!buf->kind) INFO("set sequence...");
                    buf->kind = parent->id;
                    did_something = true;
                }
                if(!did_something && parent->id == AST_ID_POWER) {
                    if(!buf->kind) INFO("set power...");
                    buf->kind = parent->id;
                    did_something = true;
                }
                if(!did_something && parent->id == AST_ID_TIMES) {
                    if(!buf->kind) INFO("set times... / prev %s", ast_id_str(parent_prev->id));
                    if(parent_prev->id != AST_ID_SEQUENCE && parent_prev->id != AST_ID_POWER) {
                        buf->kind = parent->id;
                        did_something = true;
                    }
                }
                /* handle ranges */
                if(!did_something && parent->id == AST_ID_RANGE) {
                    INFO("create range");
                    TRY(expr_static_range(buf), ERR_EXPR_RANGE);
                    did_something = true;
                }
                /* handle sole numbers laying around */
                if(!did_something && parent->id == AST_ID_TERM) {
                    if(len_order >= 1) { /* TODO this may be removed if I just always push all numbers available on this function call :) */
                        TRY(expr_static_number(buf), ERR_EXPR_NUMBER);
                        did_something = true;
                    }
                }
            }
            /* all the other stuff */
            if(parent->id == AST_ID_EXPRESSION || parent->id == AST_ID_TERM) {
            //if(parent == expr_top || parent->id == AST_ID_TERM) {
                TRY(expr_static_handle_kind(buf), ERR_EXPR_HANDLE_KIND);
                TRY(expr_static_purge_until(buf, 0), ERR_EXPR_PURGE_UNTIL);
            }
            /* keep below last */
            parent_prev = parent;
            TRY(ast_next(&expr, &layer), ERR_AST_NEXT);
        }
    }
    TRY(expr_static_handle_kind(buf), ERR_EXPR_HANDLE_KIND);
clean:
    expr_decode_free(node, numbers);
#if 0
    /* TODO check if i0 is correct???? */
    TRY(expr_static_handle_kind(&buf), ERR_EXPR_HANDLE_KIND);
    TRY(expr_static_insert_pileup_to_timers(&buf, &timers, i0), ERR_EXPR_INSERT_PILEUP_TO_TIMERS);
    /* TODO store recent one. you know what instruction I mean. */
    vec_val_free(&buf.pileup);
    vec_val_free(&buf.order);
    //}
    INFO("done ..."); 
#endif

    return err;
error: ERR_CLEAN;
}

void expr_free(ExprBuf *buf)
{
    if(!buf) return;
    //printf(" [free expr %p]\n", buf);
    vec_val_free(&buf->order);
    vec_val_free(&buf->pileup);
    memset(buf, 0, sizeof(*buf));
}

void expr_free_p(ExprBuf *buf)
{
    if(!buf) return;
    if(buf->prev) expr_free_p(buf->prev);
    expr_free(buf);
    free(buf);
}


ErrDecl expr_malloc(ExprBuf **buf)
{
    if(!buf) THROW(ERR_EXPR_BUF_POINTER);
    if(*buf) THROW("expected null");
    *buf = malloc(sizeof(**buf));
    if(!*buf) THROW(ERR_MALLOC);
    //printf(" [mallocd expr %p]\n", buf);
    memset(*buf, 0, sizeof(**buf));
    return 0;
error:
    return -1;
}

