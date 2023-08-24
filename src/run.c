#include "run.h"
#include "ast.h"
#include "create.h"
#include "execute.h"
#include "expr.h"
#include "next.h"
#include "scope.h"
#include "timers.h"
#include "vec.h"
#include "arg.h"
#include "parse.h"

/* TODO for the run stuff:
 * - scopes, each scope holds:
 *   - scope info: which scope we're in
 *   - timers struct: the timers running
 *   it can be a stack like structure or
 *   just my vector thing
 * - the stack; aka. the stacc, global
 *   among all scopes
 *
 */

void run_free(Run *run)
{
    if(!run) return;
    for(size_t i = 0; i < vec_scope_length(&run->scopes); i++) {
        Scope *s_i = vec_scope_get_at(&run->scopes, i);
        timers_free_all(run, &s_i->timers);
    }
    expr_free_p(run->create_buf);
    vec_scope_free(&run->scopes);
    vec_val_free(&run->stack);
    memset(run, 0, sizeof(*run));
}

int run_process(Run *run, Ast *ast)
{
    int err = 0;
    if(!run) THROW("expected pointer to run struct");
    if(!ast) THROW("expected pointer to ast struct");

    /* set up run */
    TRY(parse_build_func_name_lut(ast), ERR_PARSE_BUILD_FUNC_NAME_LUT);
    TRY(expr_malloc(&run->create_buf), ERR_EXPR_MALLOC);

    /* together with: 'execute_func' ??? */
    TRY(scope_enter(run, ast, 0, 0), ERR_SCOPE_ENTER);
    while(vec_scope_length(&run->scopes)) {
        Scope *scope = vec_scope_get_back(&run->scopes);
        while(scope->timers.ll) {
            TriVal diff = {0};
            //printf("scope:");
            //timers_print2(&scope->timers);
            TRY(timers_execute(run, scope), ERR_TIMERS_EXECUTE);
            if(run->apply_delta) {
                diff.val = 0;
                TRY(timers_get_delta(run, scope, &diff), ERR_TIMERS_GET_DELTA);
                TRY(timers_apply_delta(run, &scope->timers, diff.val), ERR_TIMERS_APPLY_DELTA);
                run->apply_delta = false;
            }
            if(run->called_scope) {
                TRY(scope_enter(run, run->called_scope, 0, 0), ERR_SCOPE_ENTER);
                run->called_scope = 0;
                break;
            }
        }
        if(!scope->timers.ll) {
            scope_leave(run);
        }
    }

clean:
    return err;
error: 
    ERR_CLEAN;
}


