#ifndef CLI_H

#include <stdbool.h>

#include "str.h"
VEC_INCLUDE(VecStr, vec_str, Str, BY_REF);

typedef enum {
    ARG_NONE,
    /* keep above */
    ARG_HELP,
    ARG_VERSION,
    ARG_SKIP_PARSE,
    ARG_SKIP_RUN,
    ARG_MERGE,
    ARG_INSPECT_LEX,
    ARG_INSPECT_AST,
    /* keep below */
    ARG__COUNT
} ArgList;

typedef enum {
    SPECIFY_NONE,
    SPECIFY_OPTION,
    SPECIFY_OPTIONAL,
    SPECIFY_CONFIRM,
    SPECIFY_STEP,
    SPECIFY_FILE,
    /* keep below */
    SPECIFY__COUNT
} SpecifyList;

#define ERR_ARGS_POINTER "expected pointer to Args struct"
typedef struct Args {
    Str unknown;
    VecStr files;
    ArgList priority;
    bool exit_early;
    bool skip_parse;
    bool skip_run;
    SpecifyList merge;
    SpecifyList inspect_ast;
    SpecifyList inspect_lex;
    struct {
        int tiny; /* tiny, because short is reserved */
        int main;
        int ext; /* abbr. for extended, because long is reserved  */
        int spec; /* specification tabs */
        int max;
    } tabs;
} Args;

#define LINK_WIKI "https://esolangs.org/wiki/Timers"
#define LINK_GITHUB "https://github.com/rphii/TimersInterpreter"

#define ERR_ARG_PARSE "failed to parse arguments"
ErrDecl arg_parse(Args *args, int argc, const char **argv);

void arg_free(Args *args);


#define CLI_H
#endif

