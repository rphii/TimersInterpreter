#ifndef CREATE_H

#include <stdbool.h>

#include "ast.h"
#include "err.h"
#include "timers.h"

typedef struct Scope Scope; /* forward declaration */

typedef struct Create {
    Ast *fetch;
    Ast decoded;
} Create;

#define ERR_CREATE_FROM_NEW "failed to create from new"
//ErrDecl create_from_new(Timers **timers, Create *create, Ast *node);
ErrDecl create_from_new(VecVal *stack, Scope *scope, Create *create, Ast *node);
//ErrDecl create_from_new(Timers **timers, Create *create, Ast *node, ExprBuf *buf);

//#define ERR_CREATE_INSERT_PILEUP_TO_TIMERS "failed inserting buf"
//ErrDecl create_insert_pileup_to_timers(ExprBuf *buf, Timers **ptimers, size_t i0);

#define CREATE_H
#endif

