#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

#include "err.h"
#include "platform.h"
#include "str.h"
#include "file.h"
#include "lex.h"
#include "ast.h"
#include "parse.h"
#include "run.h"
#include "utils.h"
#include "arg.h"
#include "vec_lex.h"

#define ERR_EXECUTE_FILE "failed single file execution"
ErrDecl execute_file(Args *args, Str *filename)
{
    int err = 0;
    if(!args) THROW(ERR_ARGS_POINTER);
    if(!filename) THROW(ERR_STR_POINTER);
    Lex lex = {0};
    Ast ast = {0};
    Run run = {0};
    Str outbuf = {0};
    TRY(file_str_read(filename, &outbuf), ERR_FILE_STR_READ);
    TRY(str_app(&lex.fn, STR_F, STR_UNP(*filename)), ERR_STR_APP);
    TRY(lex_process(args, &outbuf, &lex), ERR_LEX_PROCESS);
    if(!args->skip_parse) {
        TRY(parse_process(args, &ast, &lex), ERR_PARSE_PROCESS);
        if(!args->skip_run) {
            TRY(run_process(&run, &ast), ERR_RUN_PROCESS);
        }
    }
clean:
    str_free(outbuf);
    lex_free(&lex);
    ast_free(&ast, false, true);
    run_free(&run);
    return err;
error: 
    ERR_CLEAN;
}

#define ERR_EXECUTE_MERGE "failed merged execution"
ErrDecl execute_merge(Args *args)
{
    int err = 0;
    if(!args) THROW(ERR_ARGS_POINTER);
    VecLex vlex = {0};
    Ast ast = {0};
    Run run = {0};
    Str outbuf = {0};
    for(size_t i = 0; i < vec_str_length(&args->files); i++) {
        Lex lex = {0};
        str_recycle(&outbuf);
        Str *file = vec_str_get_at(&args->files, i);
        /* lex */
        TRY(file_str_read(file, &outbuf), ERR_FILE_STR_READ);
        TRY(str_app(&lex.fn, STR_F, STR_UNP(*file)), ERR_STR_APP);
        TRY(lex_process(args, &outbuf, &lex), ERR_LEX_PROCESS);
        /* parse */
        if(!args->skip_parse) {
            TRY(parse_process(args, &ast, &lex), ERR_PARSE_PROCESS);
        }
        TRY(vec_lex_push_back(&vlex, &lex), ERR_VEC_PUSH_BACK);
    }
    if(!args->skip_parse) {
        if(!args->skip_run) {
            TRY(run_process(&run, &ast), ERR_RUN_PROCESS);
        }
    }
clean:
    str_free(outbuf);
    //lex_free(&lex);
    vec_lex_free(&vlex);
    ast_free(&ast, false, true);
    run_free(&run);
    return err;
error:
    ERR_CLEAN;
}


int main(int argc, const char **argv)
{
    srand((unsigned int)platform_seed());
    int err = 0;
    Args args = {0};
    TRY(platform_utf8_enable(), ERR_PLATFORM_UTF8_ENABLE);

    TRY(arg_parse(&args, argc, argv), ERR_ARG_PARSE);
    if(args.exit_early) {
        goto clean;
    }

    if(argc > 1 && !vec_str_length(&args.files)) THROW("no input files");
    if(!args.merge) {
        for(size_t i = 0; i < vec_str_length(&args.files); i++) {
            Str *file = vec_str_get_at(&args.files, i);
            TRY(execute_file(&args, file), ERR_EXECUTE_FILE);
        }
    } else {
        TRY(execute_merge(&args), ERR_EXECUTE_MERGE);
    }

clean:
    fflush(stdout);
    /* clean up */
    arg_free(&args);
    return err;

error: ERR_CLEAN;
}

/* OTHER TODO's
 * - replace error messages with defines
 * - after having the run: create an interactive course.c or tutorial.c
 */

