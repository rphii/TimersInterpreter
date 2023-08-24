#include "scope.h"
#include "ast.h"
#include "timers.h"
#include "run.h"

VEC_IMPLEMENT(VecScope, vec_scope, Scope, BY_REF, 0);

ErrDecl scope_enter(Run *run, Ast *origin, int argc, char **argv)
{
    if(!run) THROW(ERR_RUN_POINTER);
    if(!origin) THROW(ERR_AST_POINTER);
    if(!(origin->id == AST_ID_PROGRAM || origin->id == AST_ID_SCOPE)) {
        THROW("didn't expect %s", ast_id_str(origin->id));
    }
    VecScope *scopes = &run->scopes;
    Scope scope = {0};
    scope.origin = origin;
    //scope.len = &scopes->len; /* danger is my second name */
    //scope.last_destroyed = &run->last_destroyed;
    scope.run = run;
    TRY(ast_get_scope_name(origin,  &scope.scope_name), ERR_AST_GET_SCOPE_NAME);
    TRY(timers_insert(&scope.timers, 0, 0), ERR_TIMERS_INIT);
    INFO("push scope");
    TRY(vec_scope_push_back(scopes, &scope), ERR_VEC_PUSH_BACK);
    return 0;
error:
    return -1;
}

void scope_leave(Run *run)
{
    if(!run) return;
    VecScope *scopes = &run->scopes;
    Scope scope = {0};
    vec_scope_pop_back(scopes, &scope);
    timers_free_all(run, &scope.timers);
}
