#ifndef SCOPE_H

#include <stddef.h>
#include <stdlib.h>

#include "ast.h"
#include "timers.h"
#include "vec.h"

typedef struct Run Run;

#define SCOPE_DEFAULT_SIZE  2

typedef struct Scope {
    Ast *origin;
    Timers timers;
    struct {
        bool yes;
        Ast *ast_execute;
        Ast *ast_next;
        size_t exec_i;
        size_t next_i;
        size_t code_i;
    } resume;
    bool destroy_timer;
    bool created_timer;
    bool randomize;
    Str *scope_name;
    Run *run;
    //size_t *len;
    //TriVal *last_destroyed;
} Scope;

VEC_INCLUDE(VecScope, vec_scope, Scope, BY_REF);

#if 0
typedef struct Scope {
    ScopeItem *items;   /* the scopes */
    size_t l;           /* length of used scopes */
    size_t c;           /* capacity of allocated scopes */
} Scope;
#endif

#define ERR_SCOPE_POINTER       "expected pointer to scope"
//#define ERR_SCOPEITEM_POINTER   "expected pointer to scope item"

/* basic utility */
#define ERR_SCOPE_ENTER "failed to enter scope"
//ErrDecl scope_enter(VecScope *scopes, Ast *origin, int argc, char **argv);
ErrDecl scope_enter(Run *run, Ast *origin, int argc, char **argv);

#define ERR_SCOPE_LEAVE "failed to leave scope"
void scope_leave(Run *run);
//ErrDecl scope_peek(VecScope *scopes, ScopeItem **item, bool *empty);
//ErrDecl scope_free(Scope *scope);
//#endif

#define SCOPE_H
#endif
