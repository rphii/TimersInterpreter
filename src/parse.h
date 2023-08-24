#ifndef PARSE_H

#include "err.h"
#include "lex.h"
#include "ast.h"
#include "arg.h"

#define ERR_PARSE_PROCESS "failed to parse"
ErrDecl parse_process(Args *args, Ast *ast, Lex *lex);

#define ERR_PARSE_BUILD_FUNC_NAME_LUT "failed building lookup table for function names"
ErrDecl parse_build_func_name_lut(Ast *root);

#define PARSE_ID_STATEMENT_START  AST_ID_FUNC_CALL

#define PARSE_H
#endif
