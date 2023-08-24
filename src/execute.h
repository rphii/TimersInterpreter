#ifndef EXECUTE_H

#include "ast.h"
#include "vec.h"
#include "create.h"

typedef struct Run Run;
typedef struct Timers Timers; /* forward declaration */

#define ERR_EXECUTE_STATEMENTS "failed executing function"
ErrDecl execute_statements(Run *run, Scope *scope, Ast *node);

void execute_hint_inspect_func_call(Run *run, Ast *node);

#define EXECUTE_H
#endif

