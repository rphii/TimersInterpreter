#ifndef EXPR_H

#include <stddef.h>
#include <stdbool.h>

#include "val.h"
#include "ast.h"
#include "err.h"
#include "err.h"
#include "attr.h"

typedef struct Scope Scope; /* forward declaration */

#define ERR_TRI_VAL_POINTER "expected pointer to TriVal"
typedef struct TriVal {
    Val val;
    bool set;
} TriVal;
#define TRI_VAL_INIT   (TriVal){0}

struct ExprBuf; /* forward declaration */
#define ERR_EXPR_BUF_CALLBACK_NUMBER "failed number callback"
typedef int (*ExprBufCallbackNumber  )(struct ExprBuf *, Val t);
#define ERR_EXPR_BUF_CALLBACK_RANGE "failed range callback"
typedef int (*ExprBufCallbackRange   )(struct ExprBuf *, Val from, Val until);
#define ERR_EXPR_BUF_CALLBACK_SEQUENCE "failed sequence callback"
typedef int (*ExprBufCallbackSequence)(struct ExprBuf *, Val offset, Val step);
#define ERR_EXPR_BUF_CALLBACK_POWER "failed power callback"
typedef int (*ExprBufCallbackPower   )(struct ExprBuf *, Val base, Val exponent);
#define ERR_EXPR_BUF_CALLBACK_TIMES "failed times callback"
typedef int (*ExprBufCallbackTimes   )(struct ExprBuf *, Val t);
#define ERR_EXPR_BUF_CALLBACK_ZERO "failed times callback"
typedef int (*ExprBufCallbackZero    )(void *p);
#define ERR_EXPR_BUF_CALLBACKS_POINTER "expected pointer to ExprBufCallbacks"
typedef struct ExprBufCallbacks {
    ExprBufCallbackNumber number;
    ExprBufCallbackRange range;
    ExprBufCallbackSequence sequence;
    ExprBufCallbackPower power;
    ExprBufCallbackTimes times;
    ExprBufCallbackZero zero;
} ExprBufCallbacks;

#define ERR_EXPR_BUF_POINTER "expected pointer to ExprBuf"
typedef struct ExprBuf {
    /* stuff used for all */
    const ExprBufCallbacks *callbacks;
    VecVal order; /* used for decoding, lowest index = left, highest = right */
    TriVal until;
    TriVal times;
    AstIDList kind; /* sequence, exponential... */
    Ast *node; /* same as passed in .._eval */
    struct ExprBuf *prev; /* previous thing of creation */
    /* only for checking */
    const Val cmp; 
    const bool skip_current; 
    /* only for creating */
    VecVal pileup; /* all timers to be created */
    TriVal diff;
    size_t prev_op_count;
} ExprBuf;

#define ERR_EXPR_EVAL "failed evaluating ast"
ErrDecl expr_eval(VecVal *stack, Scope *scope, Ast *node, AstIDList id_decode, ExprBuf *buf);

#define ERR_EXPR_NUMBER_CHECK "failed to check number in expression"
ErrDecl expr_number_check(Ast *node, Val t, bool skip_current, TriVal *diff);

#define ERR_EXPR_GET_VAL_ID "failed getting val & id"
ErrDecl expr_get_val_id(ExprBuf *buf, Ast *node);

#define ERR_EXPR_DECODE "failed to decode expression"
ErrDecl expr_decode(VecVal *stack, Scope *scope, Ast *node, Ast **decoded, AstIDList id);

void expr_free(ExprBuf *buf);
void expr_free_p(ExprBuf *buf);

#define ERR_EXPR_MALLOC "failed to allocate ExprBuf"
ErrDecl expr_malloc(ExprBuf **buf);

#define EXPR_H
#endif

