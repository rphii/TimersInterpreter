#include <ctype.h>

#include "arg.h"
#include "err.h"
#include "str.h"
#include "vec.h"
#include "file.h"
#include "platform.h"

typedef struct SpecifyGroup {
    size_t l;
    //char **s;
    SpecifyList *s;
} SpecifyGroup;
//#define STR_GROUP(...)  (StrGroup){.s = (char *[]){__VA_ARGS__}, .l = sizeof((char *[]){__VA_ARGS__})/sizeof(*(char *[]){__VA_ARGS__})}
#define SPECIFY_GROUP(...)  (SpecifyGroup){ \
    .s = (SpecifyList []){__VA_ARGS__}, \
    .l = sizeof((SpecifyList []){__VA_ARGS__}) / sizeof(*(SpecifyList []){__VA_ARGS__})}

#define PRINTF_TABBED(tabs, fmt, ...)  printf("%*s"fmt, tabs, "", ##__VA_ARGS__)

static const char *static_args[][2] = {
    [ARG_NONE] = {0, 0},
    [ARG_HELP] = {"-h", "--help"},
    [ARG_VERSION] = {"-v", "--version"},
    [ARG_SKIP_PARSE] = {0, "--skip-parse"},
    [ARG_SKIP_RUN] = {0, "--skip-run"},
    [ARG_MERGE] = {"-m", "--merge"},
    [ARG_INSPECT_LEX] = {"-l", "--inspect-lex"},
    [ARG_INSPECT_AST] = {"-p", "--inspect-ast"},
};

static const char *static_explained[] = {
    [ARG_NONE] = 0,
    [ARG_HELP] = "print this help",
    [ARG_VERSION] = "display the version",
    [ARG_SKIP_PARSE] = "skip parsing the program",
    [ARG_SKIP_RUN] = "skip running the program",
    [ARG_MERGE] = "merge inputs into one program\noptionally specify a file in which each line itself is a filename that shall all get merged together",
    [ARG_INSPECT_LEX] = "inspect the lexer",
    [ARG_INSPECT_AST] = "inspect the parser (abstract syntax tree)",
};

/* zero-th element is description, first-th element is default; leave out if no defaults; any following are all other options */
static const SpecifyGroup static_specify[ARG__COUNT] = {
    [ARG_INSPECT_LEX] = SPECIFY_GROUP(SPECIFY_OPTION, SPECIFY_NONE, SPECIFY_CONFIRM, SPECIFY_STEP),
    [ARG_INSPECT_AST] = SPECIFY_GROUP(SPECIFY_OPTION, SPECIFY_NONE, SPECIFY_CONFIRM, SPECIFY_STEP),
    [ARG_MERGE] = SPECIFY_GROUP(SPECIFY_OPTIONAL, SPECIFY_FILE),
};

static const char *static_specify_str[] = {
    [SPECIFY_NONE] = "none",
    [SPECIFY_OPTION] = "OPTION",
    [SPECIFY_OPTIONAL] = "OPTIONAL",
    [SPECIFY_STEP] = "step",
    [SPECIFY_CONFIRM] = "confirm",
    [SPECIFY_FILE] = "file",
};

_Static_assert((sizeof(static_args) / sizeof(*static_args)) == ARG__COUNT, "missing arguments");
_Static_assert((sizeof(static_explained) / sizeof(*static_explained)) == ARG__COUNT, "missing argument explanations");
_Static_assert((sizeof(static_specify) / sizeof(*static_specify)) == ARG__COUNT, "missing argument specification");
_Static_assert((sizeof(static_specify_str) / sizeof(*static_specify_str)) == SPECIFY__COUNT, "missing argument specification string");

VEC_IMPLEMENT(VecStr, vec_str, Str, BY_REF, str_free_single);

int print_line(int max, int current, int tabs, Str *str)
{
    if(!str) return 0;
    int result = 0;
    int printed = 0;
    int length = 0;
    char *until = 0;
    for(size_t i = 0; i < str->l; i += (size_t)length) { 
        //bool skipped_space = false;
        while(isspace((int)str->s[i])) {
            //skipped_space = true;
            i++;
        }
        if(i >= str->l) break;
        if(!until && i) {
#if 0
            if(skipped_space) {
#endif
                printf("\n");
#if 0
            } else {
                i--;
                printf("\b-\n");
            }
#endif
        }
        char *s = &str->s[i];
        until = strchr(s, '\n');
        length = until ? (int)(until + 1 - s) : (int)strlen(s);
        if(length + current > max) length = max - current;
        printed = printf("%*s%.*s", tabs, "", length, s);
        //printed = printf("%*s%s", tabs, "", s);
        //printf("[printed=%zu]\n", printed);
        //printed = length;
        //printf("[%i]", printed);
        tabs = current;
        result += printed;
    }
    return result;
}

void print_help(Args *args)
{
    if(!args) return;
    int err = 0;
    Str ts = {0};
    print_line(args->tabs.max, 0, 0, &STR(F("timers:", BOLD)" command-line interpreter for the Timers language.\n\n"));
    print_line(args->tabs.max, 0, 0, &STR("Usage:\n"));
    print_line(args->tabs.max, args->tabs.main, args->tabs.main, &STR("timers [options] [file]\n"));
    //PRINTF_TABBED(args->tabs.main, "timers [options]\n");
    //PRINTF_TABBED(args->tabs.main, "[string] | timers\n");
    printf("\n");
    print_line(args->tabs.max, 0, 0, &STR("Options:\n"));
    for(size_t i = 0; i < ARG__COUNT; i++) {
        int tabs_offs = 0;
        int tp = 0;
        if(!i) continue;
        const char *arg_short = static_args[i][0];
        const char *arg_long = static_args[i][1];
        if(arg_short) {
            str_recycle(&ts);
            TRY(str_app(&ts, "%s,", arg_short), ERR_STR_APP);
            tp = print_line(args->tabs.max, args->tabs.tiny, args->tabs.tiny, &ts);
            tabs_offs += tp;
            //tp += 3; /* length of e.g. "-h," */
        }
        if(arg_long) {
            str_recycle(&ts);
            TRY(str_app(&ts, "%s", arg_long), ERR_STR_APP);
            tp = print_line(args->tabs.max, args->tabs.main, args->tabs.main-tabs_offs, &ts);
            tabs_offs += tp;
            const char *explained = static_explained[i];
            const SpecifyGroup *specify = &static_specify[i];
            if(specify->l) {
                str_recycle(&ts);
                const char *sgl = static_specify_str[specify->s[0]];
                TRY(str_app(&ts, "=%s", sgl ? sgl : F("!!! missing string representation !!!", FG_RD_B)), ERR_STR_APP);
                tp = print_line(args->tabs.max, args->tabs.spec, 0, &ts);
                tabs_offs += tp;
            }
            if(!explained) {
                PRINTF_TABBED(args->tabs.ext-tabs_offs, F("!!! missing argument explanation !!!", BOLD FG_RD_B));
            } else {
                str_recycle(&ts);
                TRY(str_app(&ts, "%s ", explained), ERR_STR_APP);
                tp = print_line(args->tabs.max, args->tabs.ext, args->tabs.ext-tabs_offs, &ts);
                tabs_offs += tp;
                if(specify->l > 1) {
                    printf("\n");
                    PRINTF_TABBED(args->tabs.ext+2, "");
                    //printf("(");
                    str_recycle(&ts);
                    for(size_t j = 1; j < specify->l; j++) {
                        const char *spec = static_specify_str[specify->s[j]];
                        if(!spec) continue;
                        if(j == 1) {
                            //printf("default: %s", spec);
                            TRY(str_app(&ts, "=%s", spec), ERR_STR_APP);
                            //printf("=%s", spec);
                            if(specify->s[0] == SPECIFY_OPTION) {
                                TRY(str_app(&ts, ""F(" (default)", IT)""), ERR_STR_APP);
                            } else if(specify->s[0] == SPECIFY_OPTIONAL) {
                                TRY(str_app(&ts, ""F(" (optional)", IT)""), ERR_STR_APP);
                            } else {
                                printf(F("!!! missing behavior hint !!!", FG_RD_B));
                            }
                        } else {
                            TRY(str_app(&ts, ", =%s", spec), ERR_STR_APP);
                        }
                    }
                    tp = print_line(args->tabs.max, args->tabs.spec, 0, &ts);
                    //printf(")");
                }
            }
        }
        printf("\n");
    }
    printf("\n");
    printf("Wiki: %s\n", LINK_WIKI);
    printf("GitHub: %s\n", LINK_GITHUB);
clean:
    str_free_single(&ts);
    return; (void)err;
error: ERR_CLEAN;
}

#define ERR_ARG_ADD_TO_UNKNOWN "failed to add to unknown"
ErrDeclStatic arg_static_add_to_unknown(Args *args, char *s)
{
    if(!args) THROW(ERR_ARGS_POINTER);
    if(!s) THROW(ERR_CSTR_POINTER);
    bool add_comma = (args->unknown.l != 0);
    TRY(str_app(&args->unknown, "%s%s", add_comma ? ", " : "", s), ERR_STR_APP);
    return 0;
error:
    return -1;
}

#define ERR_ARG_EXECUTE "failed executing argument"
ErrDeclStatic arg_static_execute(Args *args, ArgList id)
{
    if(!args) THROW(ERR_ARGS_POINTER);
    if(id >= ARG__COUNT) THROW("incorrect id (%u)", id);
    SpecifyList *spec = 0;
    switch(id) {
        case ARG_HELP: {
            print_help(args);
            args->exit_early = true;
        } break;
        case ARG_VERSION: {
#if defined(VERSION)
            printf("%s\n", VERSION);
            args->exit_early = true;
#else
            /* https://stackoverflow.com/questions/1704907/how-can-i-get-my-c-code-to-automatically-print-out-its-git-version-hash */
            THROW("missing version (added in Make as a preprocessor definition)");
#endif
        } break;
        case ARG_SKIP_PARSE: {
            args->skip_parse = true;
        } break;
        case ARG_SKIP_RUN: {
            args->skip_run = true;
        } break;
        case ARG_INSPECT_AST: {
            spec = &args->inspect_ast;
        } break;
        case ARG_INSPECT_LEX: {
            spec = &args->inspect_lex;
        } break;
        case ARG_MERGE: {
            spec = &args->merge;
        } break;
        default: THROW(ERR_UNHANDLED_ID" (%d)", id);
    }
    if(spec) {
        switch(*spec) {
            case SPECIFY_OPTION: {
                args->exit_early = true;
                /* list options available */
                const SpecifyGroup *sg = &static_specify[id];
                printf("%*s"F("%s", BOLD)" (one of %zu below)\n", args->tabs.tiny, "", static_args[id][1], sg->l-1);
                for(size_t i = 1; i < sg->l; i++) {
                    SpecifyList h = sg->s[i];
                    printf("%*s=%s\n", args->tabs.main, "", static_specify_str[h]);
                }
            } break;
            default: break;
        }
    }
    return 0;
error:
    return -1;
}

#define ERR_ARG_SPECIFIC_OPTION "failed specifying argument"
ErrDeclStatic arg_static_specific_option(Args *args, ArgList id, SpecifyList spec)
{
    if(!args) THROW(ERR_ARGS_POINTER);
    SpecifyList *set_this = 0;
    switch(id) {
        case ARG_INSPECT_AST: { set_this = &args->inspect_ast; } break;
        case ARG_INSPECT_LEX: { set_this = &args->inspect_lex; } break;
        case ARG_MERGE: { set_this = &args->merge; } break;
        default: THROW("id (%u) doesn't have a corresponding specific option", id);
    }
    if(set_this) {
        if(spec >= SPECIFY__COUNT) THROW("specific option out of range");
        *set_this = spec;
    }
    return 0;
error:
    return -1;
}

#define ERR_ARG_STATIC_READ_MERGE_LIST "failed reading merge list"
ErrDeclStatic arg_static_read_merge_list(Args *args, const char *filename)
{
    int err = 0;
    Str contents = {0};
    Str temp = {0};
    if(!args) THROW(ERR_ARGS_POINTER);
    if(!filename) THROW(ERR_CSTR_POINTER);
    Str file = STR_L((char *)filename);
    TRY(file_str_read(&file, &contents), ERR_FILE_STR_READ);
    /* get base name of that filename's directory */
    size_t last_subdir = 0;
    for(size_t i = 0; i < file.l; i++) {
        if(file.s[i] == '/') {
            last_subdir = i;
        }
    }
    /* go over each line in the contents */
    size_t i0 = 0;
    size_t iE = 0;
    for(size_t i = 0; true; i++) {
        char *s = contents.s;
        char c = s[i];
        if(c == '\n' || c == '\r' || c == 0 || c == '#') {
            iE = i;
        }
        if(iE > i0) {
            while(isspace((int)s[i0])) {
                i0++;
            } 
            if(i0 < iE) {
                if(s[i0] != '#') {
                    memset(&temp, 0, sizeof(temp));
                    if(s[i0] == '"' && s[iE - 1] == '"') {
                        i0++;
                        iE--;
                    } else if(s[i0] == '"' || s[iE] == '"') {
                        THROW("expected end and beginning '\"'");
                    } else while(iE && isspace((int)s[iE - 1])) iE--;
                    TRY(str_app(&temp, "%.*s%c%.*s", 
                                (int)(last_subdir), filename,
                                PLATFORM_CH_SUBDIR,
                                (int)(iE - i0), &s[i0]), ERR_STR_APP);
                    INFO("[MERGE] [%s]", temp.s);
                    TRY(vec_str_push_back(&args->files, &temp), ERR_VEC_PUSH_BACK);
                    if(iE < contents.l && s[iE] == '"') iE++;
                }
                i0 = iE;
            }
        }
        if(iE >= contents.l) break;
    }
clean:
    str_free_single(&contents);
    return err;
error:
    str_free_single(&temp);
    ERR_CLEAN;
}

#define ERR_ARG_SPECIFIC_OPTIONAL "failed specifying optional argument"
ErrDeclStatic arg_static_specific_optional(Args *args, ArgList id, const char *specify)
{
    if(!args) THROW(ERR_ARGS_POINTER);
    switch(id) {
        case ARG_MERGE: {
            TRY(arg_static_read_merge_list(args, specify), ERR_ARG_STATIC_READ_MERGE_LIST);
        } break;
        default: THROW("id (%u) doesn't have a corresponding specific optional", id);
    }
    return 0;
error:
    return -1;
}

ErrDecl arg_parse(Args *args, int argc, const char **argv)
{
    Str temp = {0};
    if(!args) THROW(ERR_ARGS_POINTER);
    args->tabs.tiny = 2;
    args->tabs.main = 7;
    args->tabs.ext = 32;
    args->tabs.spec = args->tabs.ext + 2;
    args->tabs.max = 80;
    if(argc == 1) {
        print_help(args);
    } else for(int i = 1; i < argc; i++) {
        /* verify command line arguments */
        const char *arg = argv[i];
        if(arg[0] != '-') continue;
        bool unknown_args = true;
        size_t arg_opt = 1; /* assume that argument is a full-string flag */
        if(arg[1] && !arg[2]) arg_opt = 0; /* argument is a one-character flag */
        for(ArgList j = 0; j < ARG__COUNT; j++) {
            if(arg[1] && arg[2] == '=' && static_specify[j].l) arg_opt = 0;
            const char *cmp = static_args[j][arg_opt];
            if(!cmp) continue;
            size_t cmp_len = strlen(cmp);
            size_t arg_len = strlen(arg);
            if(strncmp(arg, cmp, cmp_len)) continue;
            const SpecifyGroup *sg = &static_specify[j];
            if(sg->l) {
                if(arg_len > cmp_len && arg[cmp_len] == '=') {
                    if(sg->s[0] == SPECIFY_OPTION) {
                        for(SpecifyList k = 1; k < sg->l; k++) {
                            const char *specify_str = static_specify_str[sg->s[k]];
                            const char *arg_specify = &arg[cmp_len + 1];
                            if(!strcmp(arg_specify, specify_str)) {
                                TRY(arg_static_specific_option(args, j, sg->s[k]), ERR_ARG_SPECIFIC_OPTION);
                                unknown_args = false;
                                break;
                            }
                        }
                    } else {
                        const char *arg_specify = &arg[cmp_len + 1];
                        TRY(arg_static_specific_optional(args, j, arg_specify), ERR_ARG_SPECIFIC_OPTIONAL);
                    }
                }
                if(unknown_args) {
                    TRY(arg_static_specific_option(args, j, sg->s[0]), ERR_ARG_SPECIFIC_OPTION); 
                    unknown_args = false;
                }
            } else if(arg_len == cmp_len) {
                unknown_args = false;
            }
        }
        if(unknown_args) {
            //print_help();
            TRY(arg_static_add_to_unknown(args, (char *)arg), ERR_ARG_ADD_TO_UNKNOWN);
        }
    }

    if(args->unknown.l) {
        THROW("unknown arguments: %s", args->unknown.s);
    } else for(int i = 1; i < argc; i++) {
        /* run arguments */
        const char *arg = argv[i];
        if(arg[0] != '-') {
            memset(&temp, 0, sizeof(temp));
            TRY(str_app(&temp, "%.*s", (int)strlen(arg), (char *)arg), ERR_STR_APP);
            //TRY(vec_str_push_back(&args->files, &(Str){
                        //.l = strlen(arg),
                        //.s = (char *)arg}), ERR_VEC_PUSH_BACK);
            TRY(vec_str_push_back(&args->files, &temp), ERR_VEC_PUSH_BACK);
            continue;
        }
        size_t arg_opt = 1; /* assume that argument is a full-string flag */
        if(arg[1] && !arg[2]) arg_opt = 0; /* argument is a one-character flag */
        for(ArgList j = 0; j < ARG__COUNT; j++) {
            if(arg[1] && arg[2] == '=' && static_specify[j].l) arg_opt = 0;
            const char *cmp = static_args[j][arg_opt];
            if(!cmp) continue;
            size_t cmp_len = strlen(cmp);
            if(strncmp(arg, cmp, cmp_len)) continue;
            TRY(arg_static_execute(args, j), ERR_ARG_EXECUTE);
            if(args->exit_early) break;
        }
        if(args->exit_early) break;
    }
    return 0;
error:
    str_free_single(&temp);
    return -1;
}

void arg_free(Args *args)
{
    if(!args) return;
    str_free_single(&args->unknown);
    vec_str_free(&args->files);
}

