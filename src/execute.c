#include "execute.h"
#include "ast.h"
#include "create.h"
#include "run.h"
#include "lex.h"
#include "str.h"
#include "timers.h"
#include "utf8.h"
#include "val.h"

void stack_print(VecVal *stack)
{
#if DEBUG
    if(!stack) return;
    INFO("stack:");
    printf("[bot] ");
    for(size_t i = 0; i < vec_val_length(stack); i++) {
        Val t = vec_val_get_at(stack, i);
        //printf("" FMT_VAL " ", t);
        printf("%c", (char)t);
    }
    printf(" [top]");
    printf("\n");
#endif
}

#define ERR_EXECUTE_OPERATION "failed executing operation"
ErrDeclStatic execute_static_operation(Scope *scope, VecVal *stack, char op)
{
    if(!scope) THROW(ERR_SCOPE_POINTER);
    if(!stack) THROW(ERR_VEC_POINTER);
    size_t len = vec_val_length(stack);
    Val res = 0;
    Val v1 = 0; /* top */
    Val v2 = 0;
    if(len >= 1) vec_val_pop_back(stack, &v1);
    if(len >= 2) vec_val_pop_back(stack, &v2);
    if(len) {
        switch(op) {
            case LEX_CH_ADD:    { res = (v1 + v2) & VAL_MAX; } break;
            case LEX_CH_SUB:    { res = (v2 - v1) & VAL_MAX; } break;
            case LEX_CH_MUL:    { res = (v1 * v2) & VAL_MAX; } break;
            case LEX_CH_CMP_GT: { res = (v1 > v2); } break;
            case LEX_CH_CMP_LT: { res = (v1 < v2); } break;
            case LEX_CH_CMP_EQ: { res = (v1 == v2); } break;
            case LEX_CH_DIV:    {
                if(!v1) { scope->randomize = true; } 
                else { res = (v2 / v1) & VAL_MAX; }
            } break;
            case LEX_CH_MOD:    {
                if(!v1) { scope->randomize = true; } 
                else { res = (v2 % v1) & VAL_MAX; }
            } break;
            default: THROW("unimplemented operation: '%c'", op);
        }
        TRY(vec_val_push_back(stack, res), ERR_VEC_PUSH_BACK);
    }
    return 0;
error:
    return -1;
}

static void execute_static_swap(VecVal *stack, size_t from)
{
    assert(stack);
    if(!stack) return;
    size_t until = vec_val_length(stack);
    size_t diff = until - from;
    size_t half = diff / 2;
    for(size_t i = 0; i < half; i++) {
        Val v1 = vec_val_get_at(stack, from+i);
        Val v2 = vec_val_get_at(stack, until-i-1);
        INFO("swap %zu " FMT_VAL " with %zu " FMT_VAL "", from+i, v1, until-i-1, v2);
        vec_val_set_at(stack, from+i, v2);
        vec_val_set_at(stack, until-i-1, v1);
    }
    //INFO("new len %zu / len %zu", len_new, len);
}

#define ERR_EXECUTE_PUSH_STR "failed to push back string"
ErrDeclStatic execute_static_push_str(VecVal *stack, Str *str)
{
    if(!stack) THROW(ERR_VEC_POINTER);
    if(!str) THROW(ERR_STR_POINTER);
    for(size_t i = 0; i < str->l; i++) {
        char *s = &str->s[i];
        U8Point u8p = {0};
        TRY(str_to_u8_point(s, &u8p), ERR_STR_TO_U8_POINT);
        i += (size_t)(u8p.bytes - 1);
        TRY(vec_val_push_back(stack, (Val)u8p.val), ERR_VEC_PUSH_BACK);
    }
    return 0;
error:
    return -1;
}

#define ERR_EXECUTE_CMD "failed executing command"
ErrDeclStatic execute_static_cmd(Run *run, Scope *scope, char cmd, bool ignore_wrong)
{
    int err = 0;
    Str input = {0};
    if(!run) THROW(ERR_RUN_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    Timers *timers = &scope->timers;
    bool touched_stack = true;
    VecVal *stack = &run->stack;
    size_t len = vec_val_length(stack);
    INFO("execute cmd '%c'", cmd);
#if DEBUG_EXECUTE
    printf("%c", cmd);
#endif
    switch(cmd) {
        case LEX_CH_DEL: {
            //TRY(timers_delete(timers), ERR_TIMERS_DELETE);
            scope->destroy_timer = true;
            INFO("flag timer "FMT_VAL " to be destroyed\n", timers->ll->t);
#if DEBUG
            timers_print2(&scope->timers);
#endif
        } break;
        case LEX_CH_PUSH_CALLER: {
            Val t = timers->ll->t;
            TRY(vec_val_push_back(stack, t), ERR_VEC_PUSH_BACK);
            touched_stack = true;
        } break;
        case LEX_CH_POP_TOP: {
            if(len) {
                vec_val_pop_back(stack, 0);
            }
            touched_stack = true;
        } break;
        case LEX_CH_SWAP: {
            if(len >= 2) {
                Val v1 = 0;
                Val v2 = 0;
                vec_val_pop_back(stack, &v1);
                vec_val_pop_back(stack, &v2);
                TRY(vec_val_push_back(stack, v1), ERR_VEC_PUSH_BACK);
                TRY(vec_val_push_back(stack, v2), ERR_VEC_PUSH_BACK);
            }
            touched_stack = true;
        } break;
        case LEX_CH_PUSH_TOP: {
            if(len >= 1) {
                Val v = vec_val_get_back(stack);
                TRY(vec_val_push_back(stack, v), ERR_VEC_PUSH_BACK);
            }
            touched_stack = true;
        } break;
        case LEX_CH_PUSH_SIZE: {
            Val v = (Val)len;
            TRY(vec_val_push_back(stack, v), ERR_VEC_PUSH_BACK);
            touched_stack = true;
        } break;
        case LEX_CH_POP_INT: {
            if(len >= 1) {
                Val v = 0;
                vec_val_pop_back(stack, &v);
                /* flush stdout at end of program or if input requested */
                printf("" FMT_VAL "", v);
            }
            //touched_stack = true;
        } break;
        case LEX_CH_POP_CHAR: {
            if(len >= 1) {
                Val v = 0;
                vec_val_pop_back(stack, &v);
                /* TODO this is not uniform with: execute.c extract_from_string */
                for(size_t i = 0; i < 8 * sizeof(Val); i += 32) {
                    U8Point u8p = {0};
                    u8p.val = (uint32_t)(v >> i);
                    char out[6] = {0};
                    TRY(str_from_u8_point(out, &u8p), ERR_STR_FROM_U8_POINT);
                    /* flush stdout at end of program or if input requested */
                    if(u8p.bytes) {
                        printf("%.*s", u8p.bytes, out);
                    } else {
                        printf("?");
                    }
                }
            }
            //touched_stack = true;
        } break;
        case LEX_CH_NEWLINE: {
            printf("\n");
        } break;
        case LEX_CH_PUSH_N: {
            if(len >= 1) {
                Val n = 0;
                vec_val_pop_back(stack, &n);
                if(n < (Val)vec_val_length(stack) && (Val)((size_t)n) == n) {
                    Val n_th = vec_val_get_at(stack, (size_t)n);
                    TRY(vec_val_push_back(stack, n_th), ERR_VEC_PUSH_BACK);
                } else {
                    /* revert pop, TODO figure out if that is what I really want to do... */
                    TRY(vec_val_push_back(stack, n), ERR_VEC_PUSH_BACK);
                }
            }
        } break;
        case LEX_CH_SET_N: {
            Val n = 0;
            Val b = 0;
            if(len >= 1) vec_val_pop_back(stack, &n);
            if(len >= 2) vec_val_pop_back(stack, &b);
            if(len) {
                if(n < (Val)vec_val_length(stack) && (Val)((size_t)n) == n) {
                    vec_val_set_at(stack, (size_t)n, b);
                } else {
                    /* revert pop, TODO figure out if that is what I really want to do... */
                    if(len <= 2) TRY(vec_val_push_back(stack, b), ERR_VEC_PUSH_BACK);
                    if(len <= 1) TRY(vec_val_push_back(stack, n), ERR_VEC_PUSH_BACK);
                }
            }
        } break;
        //case LEX_CH_CODE: {
        //} break;
        case LEX_CH_GET_INT: {
            /* TODO make this function on like... the ranges etc...!!! 
             * or: algernatively, just add or try to add a new command???
             * TODO 2: skip whitespaces or not? useful for when entering
             * things like: '98 99 100 101 102'
             * */
            fflush(stdout);
            TRY(str_get_str(&input), ERR_STR_GET_STR);
            INFO("input len %zu : %.*s\n", input.l, (int)input.l, input.s);
            size_t i = 0;
            size_t i_next = 0;
            char *endptr = 0;
            char *r = 0;
            size_t r_len = 0;
            while(i < input.l) {
                //while(isspace((int)input.s[i])) i++;
                //while(isspace((int)input.s[i_next])) i_next++;
                while(i < input.l && !strchr("0123456789", input.s[i])) i++;
                if(!(i < input.l)) break;
                r = &input.s[i_next];
                r_len = i - i_next;
                INFO("[%zu] %.*s <== r\n", i_next, (int)r_len, r);
                TRY(execute_static_push_str(stack, &(Str){.s = r, .l = r_len}), ERR_EXECUTE_PUSH_STR);
                char *s = &input.s[i];
                Val v = (Val)strtoull(s, &endptr, 0);
                TRY(vec_val_push_back(stack, v), ERR_VEC_PUSH_BACK);
                INFO("[%zu] %s -> val is " FMT_VAL "\n",i, s, v);
                i_next = (size_t)(endptr - input.s);
                if(i_next == i) i_next++;
                i = i_next;
            }
            r = &input.s[i_next];
            r_len = i - i_next;
            INFO("[%zu] %.*s <== r\n", i_next, (int)r_len, r);
            TRY(execute_static_push_str(stack, &(Str){.s = r, .l = r_len}), ERR_EXECUTE_PUSH_STR);
            execute_static_swap(stack, len);
        } break;
        case LEX_CH_GET_STR: {
            fflush(stdout);
            TRY(str_get_str(&input), ERR_STR_GET_STR);
            INFO("input len %zu : %.*s\n", input.l, (int)input.l, input.s);
            TRY(execute_static_push_str(stack, &input), ERR_EXECUTE_PUSH_STR);
            execute_static_swap(stack, len);
            touched_stack = true;
        } break;
        case LEX_CH_ADD: 
        case LEX_CH_SUB: 
        case LEX_CH_MUL: 
        case LEX_CH_DIV: 
        case LEX_CH_MOD: 
        case LEX_CH_CMP_GT:
        case LEX_CH_CMP_LT:
        case LEX_CH_CMP_EQ: {
            TRY(execute_static_operation(scope, stack, cmd), ERR_EXECUTE_OPERATION);
            touched_stack = true;
        } break;
        case LEX_CH_CMP_NOT: {
            if(len >= 1) {
                Val v = 0;
                vec_val_pop_back(stack, &v);
                v = !v;
                TRY(vec_val_push_back(stack, v), ERR_VEC_PUSH_BACK);
            }
        } break;
        case LEX_CH_CMP_NEQ0: {
            if(len >= 1) {
                Val v = 0;
                vec_val_pop_back(stack, &v);
                if(v != 0) {
                    run->leave_func = true;
                }
            }
        } break;
        default: {
            if(!ignore_wrong) {
                THROW("unimplemented cmd: '%c'", cmd);
            }
        } break;
    }
    if(touched_stack) {
        stack_print(stack);
    }
clean:
    str_free(input);
    return err;
error: ERR_CLEAN;
}

#define ERR_EXTRACT_STRING_FROM_STACK "failed extracting string from stack"
ErrDeclStatic execute_static_extract_string_from_stack(VecVal *stack, Str *extracted)
{
    int err = 0;
    Str temp = {0};
    if(!stack) THROW(ERR_VEC_POINTER);
    if(!extracted) THROW(ERR_STR_POINTER);
    size_t len = vec_val_length(stack);
    /* find ending of string */
    //size_t len_to_pop = 0;
    size_t len_of_string = 0;
    bool invalid = true;
    bool escape = false;
    for(size_t i = 0; i < len; i++) {
        /* first convert to utf8 sequence */
        Val v = vec_val_get_at(stack, len - i - 1);
        U8Point u8p = {.val = (uint32_t)v};
        char u8s[6] = {0};
        TRY(str_from_u8_point(u8s, &u8p), ERR_STR_FROM_U8_POINT);
        if(!u8p.bytes) break;
        /* does the string start? */
        //len_to_pop++;
        if(!i) {
            if(u8p.bytes == 1 && u8s[0] == LEX_CH_STRING) continue;
            else break;
        }
        /* does the string end? */
        if(i && !escape && u8p.bytes == 1 && u8s[0] == LEX_CH_STRING) {
            invalid = false;
            break;
        } else {
            len_of_string += (size_t)u8p.bytes;
        }
        /* is the current character an escape? */
        if(!escape && u8p.bytes == 1 && u8s[0] == '\\') {
            escape = true;
        } else {
            escape = false;
        }
    }
    if(invalid) return 0;
    vec_val_pop_back(stack, 0); /* beginning of string */
    for(size_t i = 0; i < len_of_string; i++) {
        /* first convert to utf8 sequence */
        Val v = 0;
        vec_val_pop_back(stack, &v);
        U8Point u8p = {.val = (uint32_t)v};
        char u8s[6] = {0};
        TRY(str_from_u8_point(u8s, &u8p), ERR_STR_FROM_U8_POINT);
        /* append string */
        TRY(str_app(&temp, "%.*s", u8p.bytes, u8s), ERR_STR_APP);
    }
    vec_val_pop_back(stack, 0); /* end of string */
    /* construct final string */
    for(size_t i = 0; i < len_of_string; i++) {
        char *s = &temp.s[i];
        char *endptr = s;
        TRY(lex_get_escape(&endptr, extracted), ERR_LEX_GET_ESCAPE);
        if(*endptr && *endptr != '\\') {
            TRY(str_app(extracted, "%c", *endptr), ERR_STR_APP);
            endptr++;
        }
        size_t inc = (size_t)(endptr - s);
        i += inc ? inc - 1 : 0;
    }
clean:
    str_free_single(&temp);
    return err;
error: ERR_CLEAN;
}


#define ERR_EXECUTE_FUNC_CALL "failed executing function call"
ErrDeclStatic execute_static_func_call(Run *run, Scope *scope, Ast *node)
{
    /* INFO (stack descriptions are from top->bot)
     * we try to first and foremost call a function. for that we need strings.
     * valid strings are:
     * - stack : function
     * - stack : 'function can have spaces'
     * invalid strings are:
     * - missing end string terminator, stack : 'function
     * - any command character (*+-/ etc) or brackets ()[]{}
     * we try to create the longest string. example:
     * - stack : 12+3
     * - code commands : (function?x?x?)
     * - will result in : 
     *   - call : function1x2x
     *   - if stack is unchanged (else just new top of stack), cmd : +
     */
    int err = 0;
    Str temp = {0};
    Str callstring = {0};
    if(!run) THROW(ERR_RUN_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    size_t child_len = 0;
    TRY(ast_child_len(node, &child_len), ERR_AST_CHILD_LEN);
    Ast *child = 0;
    VecVal *stack = &run->stack;
    size_t i0 = 0;
    if(scope->resume.yes) {
        i0 = scope->resume.code_i + 1;
        memset(&scope->resume, 0, sizeof(scope->resume));
    }
    bool callstring_do = false;
    bool callstring_force = false;
    size_t i = 0;
    for(i = i0; i < child_len; i++) {
        size_t len = vec_val_length(stack);
        /* resume here */
        TRY(ast_child_get(node, i, &child), ERR_AST_CHILD_GET);
        if(child->id == AST_ID_STR) {
            TRY(str_app(&callstring, "%.*s", STR_UNP(*child->branch.str)), ERR_STR_APP);
            callstring_do = true;
        } else if(child->id == AST_ID_CMD) {
            if(len >= 1) {
                Val cmd = vec_val_get_back(stack);
                if(cmd == (Val)LEX_CH_STRING) {
                    str_recycle(&temp);
                    TRY(execute_static_extract_string_from_stack(stack, &temp), ERR_EXTRACT_STRING_FROM_STACK);
                    if(temp.l) {
                        INFO("extracted:%.*s", (int)temp.l, temp.s);
                        /* okay string to append to callstring */
                        TRY(str_app(&callstring, "%.*s", STR_UNP(temp)), ERR_STR_APP);
                        callstring_do = true;
                    } else {
                        /* invalid string -> don't care */
                        //callstring_force = true;
                    }
                } else {
                    /* convert to utf8 */
                    Val v = 0;
                    vec_val_pop_back(stack, &v);
                    U8Point u8p = {.val = (uint32_t)v};
                    char u8s[6] = {0};
                    TRY(str_from_u8_point(u8s, &u8p), ERR_STR_FROM_U8_POINT);
                    /* check what to do */
                    bool is_cmd = (u8p.bytes == 1 && strchr(LEX_OPERATORS, u8s[0]));
                    bool is_sep = (u8p.bytes == 1 && strchr(LEX_SEPARATORS, u8s[0]));
                    if(is_cmd || !u8p.bytes) {
                        if(is_cmd && callstring_do) {
                            break;
                        }
                        /* execute command -> then, continue */
                        TRY(execute_static_cmd(run, scope, u8s[0], true), ERR_EXECUTE_CMD);
                    } else if(!is_sep || u8p.bytes > 1) {
                        /* single okay string to append to callstring */
                        TRY(str_app(&callstring, "%.*s", u8p.bytes, u8s), ERR_STR_APP);
                        callstring_do = true;
                    } else {
                        callstring_force = true;
                    }
                }
            } else {
                TRY(str_app(&callstring, "_"), ERR_STR_APP);
            }
        } else {
            THROW(ERR_UNHANDLED_ID", %s, likely a parser error", ast_id_str(child->id));
        }
        if(callstring_force) break;
    }
    if(callstring_do || callstring_force) {
        INFO("CALLSTRING:%.*s", STR_UNP(callstring));
        scope->resume.code_i = i;
        TRY(ast_func_call(node, &callstring, &run->called_scope), ERR_AST_FUNC_CALL);
    }
clean:
    str_free_single(&temp);
    str_free_single(&callstring);
    return err;
error: ERR_CLEAN;
}

ErrDecl execute_statements(Run *run, Scope *scope, Ast *node)
{
    //printf(F("!", FG_RD_B));
    if(!run) THROW(ERR_RUN_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    size_t len = 0;
    TRY(ast_child_len(node, &len), ERR_AST_CHILD_LEN);
    INFO(F("run this node's children (%zu) : %p", FG_BK BG_YL), len, node);
    //ast_inspect_mark(node, node);
    size_t i0 = 0;
    if(scope->randomize) {
        scope->randomize = false;
        memset(&scope->resume, 0, sizeof(scope->resume));
    }
    if(scope->resume.yes) {
        i0 = scope->resume.exec_i;
    }
#if 0
    if(scope->ast_continue) {
        if(node != scope->ast_continue) THROW("expected to continue on the correct node");
        i0 = scope->i_continue;
        INFO("i0 = %zu\n", i0);
    }
#endif
    for(size_t i = i0; i < len; i++) {
        Ast *child = 0;
        TRY(ast_child_get(node, i, &child), ERR_AST_CHILD_GET);
        if(scope->resume.yes && child->id != AST_ID_FUNC_CALL) {
            //scope->resume.yes = false;
            memset(&scope->resume, 0, sizeof(scope->resume));
            continue;
        }
        /* resume here */
        INFO("executing %s ...\n", ast_id_str(child->id));
        switch(child->id) {
            case AST_ID_FNEW:
            case AST_ID_NEW: {
                Create create = {0};
                //TRY(create_from_new(timers, &create, child, &run->buf), ERR_CREATE_FROM_NEW);
                //ast_inspect_mark(child, child);
                TimersLL *next = scope->timers.ll->next;
                TRY(create_from_new(&run->stack, scope, &create, child), ERR_CREATE_FROM_NEW);
                TimersLL *next_now = scope->timers.ll->next;
#if DEBUG_EXECUTE
                printf("NEW");
#endif
                //printf("next %p / now %p\n", next, next_now);platform_getchar();
                scope->created_timer = (next != next_now); /* needed in case we don't really create any timer */
                //timers_print2(&scope->timers);
            } break;
            case AST_ID_CMD: {
                if(child->id != AST_ID_CMD) THROW("expected id %s (is %s)", ast_id_str(AST_ID_CMD), ast_id_str(child->id));
                char cmd = child->branch.ch;
                TRY(execute_static_cmd(run, scope, cmd, false), ERR_EXECUTE_CMD);
            } break;
#if 1
            case AST_ID_FUNC_CALL: {
                TRY(execute_static_func_call(run, scope, child), ERR_EXECUTE_FUNC_CALL);
            } break;
            case AST_ID_SCOPE: {
                run->called_scope = child;
            } break;
#endif
            default: THROW(ERR_UNHANDLED_ID" %s", ast_id_str(child->id));
        }
        if(scope->randomize) {
            //THROW("todo implement");
            break;
        }
        if(run->called_scope) {
            scope->resume.yes = true;
            scope->resume.exec_i = i;
            run->leave_func = true;
        }
        if(run->leave_func) {
            run->leave_func = false;
            break;
        }
    }
    return 0;
error:
    return -1;
}

