#ifndef NEXT_H

#include "ast.h"
#include "expr.h"
//#include "timers.h"

typedef struct Run Run; /* forward declaration */
typedef struct Scope Scope; /* forward declaration */

#define ERR_NEXT_IN_FUNC_TIME "failed getting next in func time"
//ErrDecl next_in_func_time(Ast *node, Val cmp, bool skip_current, TriVal *diff);
//ErrDecl next_in_func_time(VecVal *stack, Ast *node, Val cmp, bool skip_current, TriVal *diff);
ErrDecl next_in_func_time(VecVal *stack, Scope *scope, Ast *node, Val cmp, bool skip_current, TriVal *diff);

#define ERR_NEXT_IN_SCOPE "failed getting next in scope"
ErrDecl next_in_scope(Run *run, Scope *scope, Ast *node, bool skip_current, TriVal *diff);
//ErrDecl next_in_scope(Run *run, Ast *node, size_t i0, Val cmp, bool skip_current, TriVal *diff);

#define ERR_NEXT_RANDOM "failed getting random function"
ErrDecl next_random(Run *run, Scope *scope);

#define NEXT_H
#endif

