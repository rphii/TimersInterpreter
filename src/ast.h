#ifndef AST_H

#include <stdbool.h>
#include <stdint.h>

#include "val.h"
#include "str.h"
#include "err.h"
#include "attr.h"
#include "lutd.h"
#include "lutd_ast.h"

#define AST_DEFAULT_CAP 1
#define AST_INT_MAX SIZE_MAX

#define ERR_AST_POINTER         "expected pointer to ast struct"
#define ERR_AST_ID_POINTER      "expected pointer to ast id list"
#define ERR_AST_BRANCH_POINTER  "expected pointer to ast branch struct"
/* errors for public functions */
#define ERR_AST_NEXT            "failed getting next node"
#define ERR_AST_NEXT_NEW        "failed getting next new node"
#define ERR_AST_CHILD_ENSURE    "failed ensuring child"
#define ERR_AST_CHILD_GET       "failed getting child"
#define ERR_AST_CHILD_LEN       "failed getting child length"
#define ERR_AST_PARENT_GET      "failed getting parent"
#define ERR_AST_INSERT          "failed inserting node"
#define ERR_AST_COPY            "failed copy node"
#define ERR_AST_COPY_DEEP       "failed deep copy node"
#define ERR_AST_ADJUST_IL       "failed adjusting index"
#define ERR_AST_CHILD_DELNOK    "failed deleting child"
#define ERR_AST_CHILD_OK        "failed getting ok"
#define ERR_AST_MALLOC          "failed to malloc ast"
#define ERR_AST_FUNC_GET        "failed getting function"

typedef enum
{
    AST_ID_NONE,
    /* IDs below */
    AST_ID_COMMENT, /* best to keep at 1 */
    AST_ID_PROGRAM,
    AST_ID_SCOPE,
    AST_ID_FUNC_TIME,
    AST_ID_FFUNC_TIME,
    AST_ID_SCOPE_DEF,
    AST_ID_FUNC_CALL,
    AST_ID_FUNC_NAME,
    AST_ID_NUM,
    AST_ID_STATEMENT,
    AST_ID_EXPRESSION,
    AST_ID_TERM,
    AST_ID_INT,
    AST_ID_EMPTY,
    AST_ID_TOP,
    AST_ID_BELOW,
    AST_ID_OR,
    AST_ID_XOR,
    AST_ID_RANGE,
    AST_ID_SEQUENCE,
    AST_ID_POWER,
    AST_ID_TIMES,
    AST_ID_MATCH,
    AST_ID_STR,
    AST_ID_CMD,
    AST_ID_NEW,
    AST_ID_FNEW,
    AST_ID_LEFTOVER,
    AST_ID_LUT, /* hold a lookup table to the functions */
    AST_ID_VEC, /* hold previously created values */
    AST_ID_LASTOP, /* repeat previous operation, allocates child -> free pointer */

    AST_ID_FNUM,
    AST_ID_RST,

    AST_ID_COND,
    /* IDs above */
    AST_ID__COUNT
} AstIDList;

typedef struct Ast *AstArray; 

typedef union AstBranch
{
    Val num;
    Str *str;
    char ch;
    struct Ast **child;
    LutdAst *lut;
    VecVal *vec;
    //size_t i;   /* parsing / traverse index */
} AstBranch;

typedef struct Ast
{
    AstBranch branch;
    AstIDList id;
    size_t i;       /* starting index within lex */
    size_t l;       /* length within lex */
    size_t ok;      /* parse state */
    size_t arri;    /* index within array */
    size_t arrc;    /* capacity of child array */
    size_t arrl;    /* length of child array */
    struct Ast *parent;
} Ast;

//LUTD_INCLUDE(LutdAst, lutd_ast, Ast, BY_REF);

/**********************************************************/
/* PUBLIC FUNCTION PROTOTYPES *****************************/
/**********************************************************/

bool ast_has_child(Ast *node) ATTR_PURE;
char *ast_id_str(AstIDList id) ATTR_CONST;
ErrDecl ast_next(Ast **node, ssize_t *layer);
ErrDecl ast_next_specific(Ast **node, AstIDList id);
void ast_inspect_mark(Ast *node, Ast *mark);
//void ast_inspect(Ast *ast);
void ast_free(Ast *ast, bool keep_current_id, bool final_clean);
ErrDecl ast_copy(Ast *dest, Ast *source, Ast *parent);
//ErrDecl ast_copy_deep(Ast *dst, Ast *src);
ErrDecl ast_copy_deep(Ast *dst, bool keep_current, Ast *src);
ErrDecl ast_child_ensure(Ast *node, size_t count);
ErrDecl ast_child_get(Ast *node, size_t index, Ast **child);
ErrDecl ast_child_len(Ast *node, size_t *len);
ErrDecl ast_child_ok(Ast *node, bool *ok);
ErrDecl ast_parent_get(Ast *node, Ast **parent);
ErrDecl ast_insert(Ast *node, AstIDList id);
ErrDecl ast_adjust_il(Ast *node, size_t index);
ErrDecl ast_child_delnok(Ast *node, size_t index);
ErrDecl ast_malloc(Ast **node);
ErrDecl ast_func_get(Ast *node, Ast **func);

#define ERR_AST_GET_SCOPE_NAME "failed getting scope name"
ErrDecl ast_get_scope_name(Ast *node, Str **str);

#define ERR_AST_FUNC_CALL "failed calling function"
ErrDecl ast_func_call(Ast *node, Str *str, Ast **func);

#define AST_H
#endif

