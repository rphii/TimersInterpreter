#include "lutd_ast.h"
#include "ast.h"
#include "err.h"
#include "trace.h"

static inline int lutd_ast_static_cmp_impl(Ast *a, Ast *b)
{
    if(a->id != AST_ID_STR) {
        ABORT("expected %s, is %s", ast_id_str(AST_ID_STR), ast_id_str(a->id));
    }
    if(b->id != AST_ID_STR) {
        ABORT("expected %s, is %s", ast_id_str(AST_ID_STR), ast_id_str(b->id));
    }
    if(a->branch.str->l != b->branch.str->l) return -1;
    return memcmp(a->branch.str->s, b->branch.str->s, a->branch.str->l);
error:
    trace_print();
    exit(-1);
    return 0; /* get rid of warning from tcc */
}

static inline size_t lutd_ast_static_hash_impl(Ast *a)
{
    size_t hash = 5381;
    if(a->id != AST_ID_STR) {
        ABORT("expected %s, is %s", ast_id_str(AST_ID_STR), ast_id_str(a->id));
    }
    size_t i = 0;
    while(i < a->branch.str->l) {
        unsigned char c = (unsigned char)a->branch.str->s[i++];
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
error:
    trace_print();
    exit(-1);
    return 0; /* get rid of warning from tcc */
}


LUTD_IMPLEMENT(LutdAst, lutd_ast, Ast, BY_REF, \
        lutd_ast_static_hash_impl, \
        lutd_ast_static_cmp_impl, 0);

