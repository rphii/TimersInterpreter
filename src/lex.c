#include <stdbool.h>
#include "lex.h"
#include "err.h"
#include "str.h"
#include "utf8.h"
#include "errno.h"
#include "platform.h"

/******************************************************************************/
/* PRIVATE FUNCTION PROTOTYPES ************************************************/
/******************************************************************************/

ErrDeclStatic lex_static_cap_ensure(Lex *lis, size_t cap);
ErrDeclStatic lex_static_append(Lex *lis, LexItem *li);
ErrDeclStatic lex_static_extract_next(Str *stream, Str *fn, size_t i, LexItem *li);
static void lex_hint_item(FILE *file, LexItem *item, Str *stream, Str *fn);

/******************************************************************************/
/* PRIVATE FUNCTION IMPLEMENTATIONS *******************************************/
/******************************************************************************/

ErrDeclStatic lex_static_cap_ensure(Lex *lex, size_t cap)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(cap) {
        /* calculate how much items we require */
        size_t required = lex->c ? lex->c : 16;
        while(required < cap) required *= 2;
        /* only increase */
        if(required > lex->c) {
            void *temp = realloc(lex->items, sizeof(*lex->items) * required);
            if(!temp) THROW("failed to expand lex capacity");
            /* initialize new memory */
            size_t new_size = required - lex->c;
            lex->items = temp;
            memset(&lex->items[lex->c], 0, sizeof(*lex->items) * new_size);
            lex->c = required;
        }
    }
    return 0;
error:
    return -1;
}

ErrDeclStatic lex_static_append(Lex *lex, LexItem *li)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!li) THROW(ERR_LEXITEM_POINTER);
    if(li->id == LEX_ID_COMMENT) return 0;
    LexItem *last = lex->l ? &lex->items[lex->l - 1] : 0;
    //if(last && last->id == LEX_ID_WHITESPACE && last->num && li->id == LEX_ID_WHITESPACE && li->num) {
    if(last && last->id == LEX_ID_WHITESPACE && li->id == LEX_ID_WHITESPACE) {
        last->num += li->num;
        return 0;
    }
    TRY(lex_static_cap_ensure(lex, lex->l + 1), "failed ensuring lex capacity");
    last = &lex->items[lex->l];
    last->i = li->i;
    last->l = li->l;
    last->line = li->line;
    last->num = li->num;
    last->id = li->id;
    TRY(str_app(&last->str, STR_F, STR_UNP(li->str)), ERR_STR_APP);
    lex->l++;
    return 0;
error:
    return 1;
}

ErrDeclStatic lex_static_search_def(Str *stream, size_t i, bool *found)
{
    if(!stream) THROW(ERR_STR_POINTER);
    if(!found) THROW(ERR_BOOL_POINTER);
    char *s = &stream->s[i];
    if(isspace((int)*s)) {
        while(*s && isspace((int)*s)) {
            s++;
        }
    }
    if(*s && (*s == LEX_CH_DEF_OPEN || *s == LEX_CH_DEF_CLOSE)) {
        *found = true;
    }
    return 0;
error:
    return -1;
}

ErrDeclStatic lex_static_extract_next(Str *stream, Str *fn, size_t i, LexItem *li)
{
    int err = 0;
    bool err_esc = false;
    Str temp = {0};
    if(!stream) THROW(ERR_STR_POINTER);
    if(!li) THROW(ERR_LEXITEM_POINTER);
    if(!fn) THROW(ERR_STR_POINTER);

    char *s = &stream->s[i];
    char c = *s;
    if(!li->line) li->line = 1;
    li->id = LEX_ID_NONE;
    li->l = 0;
    li->i = 0;
    li->num = 0;
    str_recycle(&li->str);
    if(c) {
        /* check if we're at a comment */
        if(c == LEX_CH_DEL) {
            char *next = s;
            if(*next) next++;
            if(*next == LEX_CH_DEL) {
                li->i = i;
                li->id = LEX_ID_COMMENT;
                /* find end of comment */
                while(*next && *next != '\n') next++;
                //if(*next && *next == '\r') next++;
                //if(*next) next++;
                li->l = (size_t)(next - s);
                return 0;
            }
        }
        /* check if we're at whitespace characters */
        if(isspace(c)) {
            li->id = LEX_ID_WHITESPACE;
            char *endptr = s;
            while(*endptr && isspace((int)*endptr)) {
                if(*endptr == '\n') {
                    li->num++;
                    li->line++;
                };
                endptr++;
            }
            li->l = (size_t)(endptr - s);
        }
        /* check if we're at a separator */
        else if(strchr(LEX_SEPARATORS, c)) li->id = LEX_ID_SEPARATOR;
        /* check if we're at an operator */
        else if(strchr(LEX_OPERATORS, c)) li->id = LEX_ID_OPERATOR;
        /* try to convert current position to a number */
        else {
            char *endptr = 0;
            size_t num = 0;
            errno = 0;
            char *goalptr = s;
            char *realend = s;
            bool ugly = false;
            if(*s == '0' && strchr(LEX_OCTAL "xX", (int)*(s + 1))) {
                if(strchr(LEX_OCTAL, (int)*(s + 1))) {
                    while(strchr(LEX_OCTAL, (int)*goalptr)) goalptr++;
                    realend = goalptr;
                } else if(strchr("xX", *(s + 1))) {
                    goalptr += 2;
                    while(strchr(LEX_HEX, (int)*goalptr)) goalptr++;
                }
                realend = goalptr;
                while(!(realend && (isspace((int)*realend) || strchr(LEX_SEPARATORS LEX_OPERATORS LEX_STR_STRING, (int)*realend)))) {
                    realend++;
                }
                TRY(str_app(&temp, "%.*s", (int)(goalptr - s), s), ERR_STR_APP);
                num = (size_t)strtoull(temp.s, &endptr, 0);
                li->l = (size_t)(realend - s);
                ugly = true;
            } else {
                num = (size_t)strtoull(s, &endptr, 0);
                li->l = (size_t)(endptr - s);
            }
            if(!*endptr || (*endptr && (isspace((int)*endptr) || strchr(LEX_SEPARATORS LEX_OPERATORS LEX_STR_STRING, (int)*endptr)))) {
                li->num = (Val)num;
                li->id = LEX_ID_LITERAL;
                /* special case - peek forward to understand */
                bool found = false;
                TRY(lex_static_search_def(stream, i + li->l, &found), "failed searching def");
                if(found) {
                    li->id = LEX_ID_IDENTIFIER;
                    li->num = 0;
                } else {
                    if(errno) {
                        err_esc = true;
                        li->i = i;
                        li->l = ugly ? (size_t)(endptr - temp.s) : (size_t)(endptr - s);
                        THROW("number out of range");
                    } else if(realend != goalptr) {
                        realend = s; /* recalculate this just in case idk which case this would actually trigger */
                        while(!(realend && (isspace((int)*realend) || strchr(LEX_SEPARATORS LEX_OPERATORS LEX_STR_STRING, (int)*realend)))) {
                            realend++;
                        }
                        err_esc = true;
                        li->i = i;
                        li->l = (size_t)(realend - s);
                        THROW("incorrect number");
                    }
                }
            }
        }
        /* check if we're at a string */
        if(c == LEX_CH_STRING) {
            char *endptr = s + 1;
            while(*endptr && *endptr != LEX_CH_STRING) {
                char *bgnptr = endptr;
                //printf("[%s]\n", bgnptr);
                if(lex_get_escape(&endptr, &li->str)) {
                    li->l = (size_t)(endptr - bgnptr);
                    li->i = (size_t)(bgnptr - stream->s);
                    err_esc = true;
                    THROW("failed lexing escape sequence");
                }
                if(*endptr && *endptr != '\\' && *endptr != LEX_CH_STRING) {
                    TRY(str_app(&li->str, "%c", *endptr), ERR_STR_APP);
                    endptr++;
                }
            }
            li->id = LEX_ID_STRING;
            li->l = (size_t)(endptr + 1 - s);
            //INFO("STRING LENGTH %zu/%zu : [%s]", li->l, li->str.l, li->str.s);
            if(*endptr != LEX_CH_STRING) {
                li->l = 1;
                li->i = i;
                err_esc = true;
                THROW("couldn't find matching string end character");
            }
            endptr++;
            /* now, check length of unicode character, if only one, save as num instead! */
            bool found = false;
            TRY(lex_static_search_def(stream, i + li->l, &found), "failed searching def");
            if(!found) {
                U8Point u8point = {0};
                //printf("li->str.s %s %zu\n", li->str.s, li->str.l);
                if(li->str.l) {
                    TRY(str_to_u8_point(li->str.s, &u8point), "failed getting utf-8 point");
                    if((size_t)u8point.bytes == li->str.l) { /* TODO more thoroughly investigate this cast */
                        li->id = LEX_ID_LITERAL;
                        li->num = u8point.val;
                    }
                }
            }
        }
        /* if any of the above aren't true, we're at an identifier */
        if(li->id == LEX_ID_NONE) {
            char *endptr = s;
            //while(*endptr && !isspace((int)*endptr) && !strchr(LEX_SEPARATORS, (int)*endptr) && !strchr(LEX_OPERATORS, (int)*endptr)) {
            while(*endptr && !isspace((int)*endptr) && !strchr(LEX_SEPARATORS LEX_OPERATORS LEX_STR_STRING, (int)*endptr)) {
                endptr++;
            }
            li->id = LEX_ID_IDENTIFIER;
            li->l = (size_t)(endptr - s);
        }
        //if(li->id == LEX_ID_STRING && li->str.l == 0) THROW("li->l %zu", li->l);
        li->i = i;
        if(li->id != LEX_ID_STRING) {
            if(!li->l) li->l = 1;
            if(!li->str.l) {
                TRY(str_app(&li->str, "%.*s", (int)li->l, s), ERR_STR_APP);
            }
            //if(li->l == 1 && li->str.s[0] == LEX_CH_LEFTOVER) {
            //    li->id = LEX_ID_OPERATOR;
            //}
        }
    } else {
        li->id = LEX_ID_END;
    }

clean:
    str_free_single(&temp);
    return err;
error:
    /* TODO add lex hinting... */
    if(err_esc) {
#if !DEBUG_DISABLE_ERR_MESSAGES
        lex_hint_item(ERR_FILE_STREAM, li, stream, fn);
#endif
    }
    ERR_CLEAN;
}

ErrDeclStatic lex_static_get_octal(char **s, Str *esc)
{
    int err = 0;
    Str str_val = {0};
    if(!s) THROW(ERR_CSTR_POINTER);
    if(!esc) THROW(ERR_STR_POINTER);
    int i = 0;
    char *endptr = *s;
    while(endptr && i < 3 && strchr(LEX_OCTAL, *endptr)) {
        i++;
        endptr++;
    }
    TRY(str_app(&str_val, "%.*s", (int)(endptr - *s), *s), ERR_STR_APP);
    *s = endptr;
    errno = 0;
    long num = strtol(str_val.s, &endptr, 8);
    if(errno) THROW("number out of range");
    TRY(str_app(esc, "%c", (char)num), ERR_STR_APP);
clean:
    str_free(&str_val);
    return err;
error: ERR_CLEAN;
}

ErrDeclStatic lex_static_get_hex(char **s, Str *esc, size_t num)
{
    int err = 0;
    Str str_val = {0};
    char *endptr = s ? *s : 0;
    if(!s) THROW(ERR_CSTR_POINTER);
    if(!esc) THROW(ERR_STR_POINTER);
    if(num) {
        size_t i = 0;
        while(endptr && i < num && strchr(LEX_HEX, *endptr)) {
            endptr++;
            i++;
        }
        if(i != num) {
            *s = endptr;
            THROW("expected more characters (%zu instead of %zu)", num, i);
        }
    } else {
        while(endptr && strchr(LEX_HEX, *endptr)) {
            endptr++;
        }
    }
    TRY(str_app(&str_val, "%.*s", (int)(endptr - *s), *s), ERR_STR_APP);
    INFO("[%zu:%s]", str_val.l, str_val.s);
    *s = endptr;
    U8Point u8p = {0};
    errno = 0;
    u8p.val = (uint32_t)strtoull(str_val.s, &endptr, 16);
    INFO("[%u]", u8p.val);
    if(errno) THROW("number out of range");
    char u8s[6] = {0};
    TRY(str_from_u8_point(u8s, &u8p), ERR_STR_FROM_U8_POINT);
    TRY(str_app(esc, "%.*s", u8p.bytes, u8s), ERR_STR_APP);
    INFO("[%.*s]", u8p.bytes, u8s);
clean:
    str_free(&str_val);
    return err;
error:
    if(s) {
        *s = endptr;
    }
    ERR_CLEAN;
}

ErrDecl lex_get_escape(char **s, Str *esc)
{
    if(!s) THROW(ERR_CSTR_POINTER);
    if(!*s) THROW(ERR_CSTR_INVALID);
    if(!esc) THROW(ERR_STR_POINTER);
    /* return if no escape */
    if(**s != '\\') return 0;
    switch(*(++(*s))) {
        case 'a': {
            TRY(str_app(esc, "\a"), ERR_STR_APP);
            (*s)++;
        } break;
        case 'b': {
            TRY(str_app(esc, "\b"), ERR_STR_APP);
            (*s)++;
        } break;
        case 'e': {
            TRY(str_app(esc, "\e"), ERR_STR_APP);
            (*s)++;
        } break;
        case 'f': {
            TRY(str_app(esc, "\f"), ERR_STR_APP);
            (*s)++;
        } break;
        case 'n': {
            TRY(str_app(esc, "\n"), ERR_STR_APP);
            (*s)++;
        } break;
        case 'r': {
            TRY(str_app(esc, "\r"), ERR_STR_APP);
            (*s)++;
        } break;
        case 't': {
            TRY(str_app(esc, "\t"), ERR_STR_APP);
            (*s)++;
        } break;
        case 'v': {
            TRY(str_app(esc, "\v"), ERR_STR_APP);
            (*s)++;
        } break;
        case '\\': {
            TRY(str_app(esc, "\\"), ERR_STR_APP);
            (*s)++;
        } break;
        case '\'': {
            TRY(str_app(esc, "\'"), ERR_STR_APP);
            (*s)++;
        } break;
        case '"': {
            TRY(str_app(esc, "\""), ERR_STR_APP);
            (*s)++;
        } break;
        case '?': {
            TRY(str_app(esc, "\?"), ERR_STR_APP);
            (*s)++;
        } break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7': {
            TRY(lex_static_get_octal(s, esc), "failed lexing octal escape sequence");
        } break;
            /* TODO add implementations */
        case 'x': {
            (*s)++;
            TRY(lex_static_get_hex(s, esc, 0), "failed lexing hex escape sequence");
        } break;
        case 'u': {
            (*s)++;
            TRY(lex_static_get_hex(s, esc, 4), "failed lexing hex escape sequence");
            //THROW("implementation yet to be added");
        } break;
        case 'U': {
            (*s)++;
            TRY(lex_static_get_hex(s, esc, 8), "failed lexing hex escape sequence");
            //THROW("implementation yet to be added");
        } break;
        default: {
            (*s)++;
            THROW("unknown escape sequence: \\%c", **s);
        } break;
    }
    return 0;
error:
    return -1;
}

/******************************************************************************/
/* PUBLIC FUNCTION IMPLEMENTATIONS ********************************************/
/******************************************************************************/

char *lex_id_str(LexIDList id)
{
    char *result = 0;
    switch(id) {
        case LEX_ID_NONE: result = "NONE"; break;
        case LEX_ID_LITERAL: result = "LITERAL"; break;
        case LEX_ID_SEPARATOR: result = "SEPARATOR"; break;
        case LEX_ID_IDENTIFIER: result = "IDENTIFIER"; break;
        case LEX_ID_STRING: result = "STRING"; break;
        case LEX_ID_OPERATOR: result = "OPERATOR"; break;
        case LEX_ID_WHITESPACE: result = "WHITESPACE"; break;
        case LEX_ID_END: result = "END"; break;
        case LEX_ID_COMMENT: result = "COMMENT"; break;
        default: THROW("unknown lex ID"); break;
    }
error:
    return result;
}


int lex_process(Args *args, Str *stream, Lex *lex)
{
    int err = 0;
    LexItem lexitem = {0};
    INFO("entered");
    if(!stream) THROW(ERR_STR_POINTER);
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!args) THROW(ERR_ARGS_POINTER);
    lex->stream = stream;
    if(!str_length(lex->stream)) goto clean;

    /*  */
    for(;;) {
        TRY(lex_static_extract_next(stream, &lex->fn, lexitem.i, &lexitem), "failed extracing next lex item");
        INFO("append lex id %s: %.*s", lex_id_str(lexitem.id), (int)lexitem.str.l, lexitem.str.s);
        if(args->inspect_lex == SPECIFY_STEP || args->inspect_lex == SPECIFY_CONFIRM) {
            printf("append lex id %s: %.*s\n", lex_id_str(lexitem.id), (int)lexitem.str.l, lexitem.str.s);
        }
        if(args->inspect_lex == SPECIFY_STEP) {
            platform_getch();
        }
        TRY(lex_static_append(lex, &lexitem), "failed appending next lex item");
        lexitem.i += lexitem.l;
        if(lexitem.id == LEX_ID_END) break;
    };
    if(args->inspect_lex == SPECIFY_STEP || args->inspect_lex == SPECIFY_CONFIRM) {
        platform_getchar();
    }
clean:
    str_free(&lexitem.str);
    return err;
error: ERR_CLEAN;
}

void lex_free(Lex *lex)
{
    if(!lex) return;
    for(size_t i = 0; i < lex->c; i++) {
        LexItem *item = &lex->items[i];
        str_free(&item->str);
        item->i = 0;
        item->l = 0;
        item->id = LEX_ID_NONE;
    }
    free(lex->items);
    str_free(&lex->fn);
    lex->items = 0;
    lex->c = 0;
    lex->l = 0;
}

void lex_inspect(Lex *lex)
{
    if(!lex) return;
    INFO("l = %zu / c = %zu / p = %zu", lex->l, lex->c, lex->p);
    for(size_t i = 0; i < lex->l; i++) {
        LexItem *item = &lex->items[i];
        char *idstr = lex_id_str(item->id);
        if(!idstr) THROW("couldn't get lex ID string");
        printf("[%5zu] %-10s @ %zu+%zu = ", i, idstr, item->i, item->l);
        switch(item->id) {
            case LEX_ID_IDENTIFIER: {
                printf("\""STR_F"\"\n", STR_UNP(item->str));
            } break;
            case LEX_ID_LITERAL: {
                printf("" FMT_VAL "\n", item->num);
            } break;
            case LEX_ID_SEPARATOR:
            case LEX_ID_OPERATOR: {
                printf("'"STR_F"'\n", STR_UNP(item->str));
            } break;
            case LEX_ID_WHITESPACE: {
                printf("{ " FMT_VAL " * '\\n' }\n", item->num);
            } break;
            case LEX_ID_STRING: {
                printf("'"STR_F"'\n", STR_UNP(item->str));
            } break;
            case LEX_ID_END: {
                printf("\n");
            } break;
            default: THROW("unhandled lex id: %d", item->id);
        }
    }
error:
    (void)lex; /* gets rid of warning */
}

/* TODO add an option to specify range! */
void lex_hint(FILE *file, Lex *lex, size_t index)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(index > lex->l) THROW("index out of range");
    LexItem *item = 0;
    TRY(lex_get(lex, index, &item), ERR_LEX_GET);
#if !DEBUG_DISABLE_ERR_MESSAGES
    lex_hint_item(file, item, lex->stream, &lex->fn);
#endif
error:
    (void)lex; /* gets rid of warning */
}

static inline void lex_hint_item(FILE *file, LexItem *item, Str *stream, Str *fn)
{
    int err = 0;
    Str line = {0};
    if(!item) THROW(ERR_LEXITEM_POINTER);
    if(!stream) THROW(ERR_STR_POINTER);
    if(!fn) THROW(ERR_STR_POINTER);
    char *s = stream->s;
    size_t len_hint = item->l;
    char *hint = &s[item->i];
    size_t i_pre = 0;
    size_t i_post = 0;
    for(i_pre = item->i; i_pre > 0; i_pre--) if(s[i_pre - 1] == '\n') break;
    for(i_post = item->i + len_hint; s[i_post]; i_post++) if(s[i_post] == '\n') break;
    size_t len_pre = item->i - i_pre;
    size_t len_post = i_post - (item->i + item->l);
    TRY(str_app(&line, " "STR_F":%zu ", fn ? (int)fn->l : 0, fn ? fn->s : "", item->line), ERR_STR_APP);
    int extra = (int)line.l;
    fprintf(file, " "STR_F":"F("%zu", FG_WT_B)" | ", fn ? (int)fn->l : 0, fn ? fn->s : "", item->line);
    fprintf(file, "%.*s", (int)len_pre, hint - len_pre);
    size_t wscount = platform_get_str_display_len(&(Str){.s = hint - len_pre, .l = len_pre});
    if(!wscount) wscount = len_pre; /* in case string doesn't exist but length is somewhat known / error reporting */
    bool print_post = true;
    if(item->id == LEX_ID_END) {
        fprintf(file, F("\\0", BG_BL_B));
        len_hint = 2;
        print_post = false;
    } else if(item->id == LEX_ID_WHITESPACE) {
        if(!item->num) {
            fprintf(file, F("%.*s", BG_RD_B), (int)len_hint, hint);
        } else {
            fprintf(file, F("\\n", BG_BL_B));
            len_hint = 2;
            print_post = false;
            //INFO("len_pre = %zu", len_pre);
        }
    } else {
        fprintf(file, F("%.*s", FG_RD_B), (int)len_hint, hint);
    }
    if(print_post) {
        fprintf(file, "%.*s", (int)len_post, hint + len_hint);
    }
    fprintf(file, "\n");
    size_t actual_len = platform_get_str_display_len(&item->str);
    if(!actual_len) actual_len = len_hint; /* in case string doesn't exist but length is somewhat known / error reporting */
    else if(item->id == LEX_ID_STRING) actual_len += 2;
    fprintf(file, "%*s| %*s", extra, "", (int)wscount, "");
    for(size_t i = 0; i < actual_len; i++) {
        if(print_post) {
            fprintf(file, F("^", FG_RD_B));
        } else {
            fprintf(file, F("^", FG_BL_B));
        }
    };
    fprintf(file, "\n");
clean:
    str_free_single(&line);
    return;
error: ERR_CLEAN;
    (void)err; /* get rid of warning */
}

int lex_get(Lex *lex, size_t index, LexItem **item)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!item) THROW(ERR_LEXITEM_POINTER);
    if(index >= lex->l) {
        THROW("index (%zu) out of range (%zu)", index, lex->l);
    }
    *item = &lex->items[index];
    return 0;
error:
    return -1;
}

int lex_skip_whitespace_on_line(Lex *lex, size_t *index)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    while(*index < lex->l && item->id == LEX_ID_WHITESPACE && !item->num) {
        (*index)++;
        if(*index < lex->l) {
            TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
        }
    }
    return 0;
error:
    return -1;
}

int lex_skip_whitespace(Lex *lex, size_t *index)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    while(*index < lex->l && item->id == LEX_ID_WHITESPACE) {
        (*index)++;
        if(*index < lex->l) {
            TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
        }
    }
    return 0;
error:
    return -1;
}

int lex_skip_whitespace_back(Lex *lex, size_t *index)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    while(*index > 0 && (item->id == LEX_ID_WHITESPACE || item->id == LEX_ID_END)) {
        (*index)--;
        if(*index > 0) {
            TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
        }
    }
    return 0;
error:
    return -1;
}

