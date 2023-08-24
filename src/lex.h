#ifndef LEX_H

#include <ctype.h>
#include <limits.h>

#include "str.h"
#include "val.h"
#include "arg.h"

#define LEX_CAP_DEFAULT   16
#define LEX_OPERATORS   "~^$\\:;.,\"#`?&@+-*/%><=!#|"
#define LEX_SEPARATORS  "()[]{}"
#define LEX_OCTAL       "01234567"
#define LEX_HEX         "0123456789abcdefABCDEF"

#define LEX_CH_MAX          '.'
#define LEX_CH_0_IF_EMPTY   ',' 
#define LEX_CH_TOP          '?'
#define LEX_CH_BELOW        '!'
#define LEX_CH_LAST_DEL     '$' 
#define LEX_CH_LAST_DIFF    '&'
#define LEX_CH_LAST_VAL     '%' 
#define LEX_CH_AT_TOP       '^'
#define LEX_CH_AT_BELOW     '@'
#define LEX_CH_TOP_IFN0     '\\'
#define LEX_CH_MAX_IF_NEMPTY '/'
#define LEX_CH_SUM_TIMERS   '"'
#define LEX_CH_NUM_TIMERS   '='
#define LEX_CH_INC_TIMERS   '`'
#define LEX_CH_CMP_TMRS_GT  '>' 
#define LEX_CH_CMP_TMRS_LT  '<'
#define LEX_CH_SCOPE_NAME   ':' 
#define LEX_CH_SCOPE_DEPTH  ';' 

#define LEX_CH_RANGE        '-'
#define LEX_CH_SEQUENCE     '+'
#define LEX_CH_POWER        '*'
#define LEX_CH_TIMES        '#'
#define LEX_CH_OR           '|'
#define LEX_CH_NEW_OPEN     '['
#define LEX_CH_NEW_CLOSE    ']'
#define LEX_CH_FUN_OPEN     '('
#define LEX_CH_FUN_CLOSE    ')'
#define LEX_CH_DEF_OPEN     '{'
#define LEX_CH_DEF_CLOSE    '}'

#define LEX_CH_DEL          '~'
#define LEX_CH_PUSH_CALLER  '^'
#define LEX_CH_POP_TOP      '$'
#define LEX_CH_SWAP         '\\'
#define LEX_CH_PUSH_TOP     ':'
#define LEX_CH_PUSH_SIZE    ';'
#define LEX_CH_POP_INT      '.'
#define LEX_CH_POP_CHAR     ','
#define LEX_CH_NEWLINE      '"'
#define LEX_CH_PUSH_N       '#'
#define LEX_CH_SET_N        '`'
#define LEX_CH_CODE         '?' // E) how intense do I do this shit => maximum intenso ? recursion of recursion ???
#define LEX_CH_GET_INT      '&' 
#define LEX_CH_GET_STR      '@'
#define LEX_CH_ADD          '+'
#define LEX_CH_SUB          '-'
#define LEX_CH_MUL          '*'
#define LEX_CH_DIV          '/'
#define LEX_CH_MOD          '%'
#define LEX_CH_CMP_GT       '>'
#define LEX_CH_CMP_LT       '<'
#define LEX_CH_CMP_EQ       '='
#define LEX_CH_CMP_NOT      '!'
#define LEX_CH_CMP_NEQ0     '|'

#define LEX_CH_STRING       '\''
#define LEX_STR_STRING      "\'"

#define LEX_CH_LEFTOVER     '_'
#define LEX_STR_LEFTOVER    "_"

//#define LEX_TERM  "-+*#:.?!$&%^@=\\" /* TODO why is | not in there..? why does it work...? */
#define LEX_RSPT    "-+*#"
#define LEX_FNUM    ".,?!$&%%^@=\\/><=:;\"`"
#define LEX_TERM    LEX_RSPT LEX_FNUM ":"

typedef enum LexIDList
{
    LEX_ID_NONE,
    /* IDs below */
    LEX_ID_LITERAL,     /* numbers, hex numbers, ... */
    LEX_ID_SEPARATOR,   /* (){}[] */
    LEX_ID_IDENTIFIER,  /* any string which is not a number */
    LEX_ID_STRING,      /* string encapsulated by ' */
    LEX_ID_OPERATOR,    /* any instruction */
    LEX_ID_WHITESPACE,  /* any whitespace. num = how many newlines there */
    LEX_ID_COMMENT,     /* placeholder, since it's not added to the structure */
    LEX_ID_END,         /* end of lex structure */
    /* IDs above */
    LEX_ID__COUNT
} LexIDList;

typedef struct LexItem
{
    Str str;        /* snippet of data */
    Val num;     /* potentially a number */
    size_t i;       /* index within inputstream */
    size_t l;       /* length of bytes after i */
    size_t line;    /* line within the stream */
    LexIDList id;   /* id of the snippet */
} LexItem;

typedef struct Lex
{
    Str *stream;
    Str fn;         /* filename from where data is */
    LexItem *items;
    size_t l;   /* length of items array */
    size_t c;   /* capacity */
    size_t p;   /* last parse index */
} Lex;

#define ERR_LEX_POINTER     "expected pointer to lex struct"
#define ERR_LEXITEM_POINTER "expected pointer to lex item"
/* error messages for public functions */
#define ERR_LEX_GET         "failed getting lex item"
#define ERR_LEX_SKIP_WS     "failed skipping whitespace"
#define ERR_LEX_SKIP_WS_L   "failed skipping whitespace on line"
#define ERR_LEX_SKIP_WS_B   "failed skipping whitespace backwards"

char *lex_id_str(LexIDList id);

#define ERR_LEX_PROCESS "failed to lex"
ErrDecl lex_process(Args *args, Str *stream, Lex *lex);
void lex_free(Lex *lex);
void lex_inspect(Lex *lex);
void lex_hint(FILE *file, Lex *lex, size_t index);
//void lex_hint_item(LexItem *item, char *s, Str *fn);
ErrDecl lex_get(Lex *lex, size_t index, LexItem **item);
//int lex_get_skipped(Lex *lex, size_t *index, LexItem **item);
int lex_skip_whitespace_on_line(Lex *lex, size_t *index);
int lex_skip_whitespace(Lex *lex, size_t *index);
int lex_skip_whitespace_back(Lex *lex, size_t *index);

#define ERR_LEX_GET_ESCAPE "failed getting escape"
ErrDecl lex_get_escape(char **s, Str *esc);

#define LEX_H
#endif
