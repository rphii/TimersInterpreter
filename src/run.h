#ifndef RUN_H

#include "err.h"
#include "ast.h"
#include "vec.h"
#include "scope.h"
#include "create.h"
#include "expr.h"
#include "lex.h"

typedef struct Args Args;

#define ERR_RUN_POINTER "expected pointer to Run struct"
typedef struct Run
{
    VecVal stack;    /* global stack */
    VecScope scopes; /* all scopes */
    /* settings per scope; theoretically those should be in the 
     * scopes struct but I don't want to, since this part won't get
     * multithreaded anyways */
    //bool destroy_timer;
    bool leave_func;
    bool apply_delta;
    ExprBuf *create_buf;
    Ast *called_scope;
    TriVal last_destroyed;
    Val last_diff;
    Val inc_timers;
    //Lex *lex;
    Args *args;
    //int (*execute_func)(struct Run *run); /* callback to when we need to call a func */
    //Scope scope;
    //TimersNew tn[2];   /* one is the recent, one is the current */
    //Create create;
} Run;

#define ERR_RUN_PROCESS "failed to run"
ErrDecl run_process(Run *run, Ast *ast);
void run_free(Run *run);


#define RUN_H
#endif

