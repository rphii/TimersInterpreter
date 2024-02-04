#include "parse.h"
#include "ast.h"
#include "err.h"
#include "lex.h"
#include "lutd_ast.h"
#include "platform.h"

#define ERR_PARSE_INCREMENT_INDEX   "failed to increment index"

/******************************************************************************/
/* PRIVATE FUNCTION IMPLEMENTATIONS *******************************************/
/******************************************************************************/

ErrDeclStatic parse_static_recall_index(Ast *node, size_t *index)
{
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    size_t child_len = 0;
    Ast *child = 0;
    TRY(ast_child_len(node, &child_len), ERR_AST_CHILD_LEN);
    if(child_len) {
        TRY(ast_child_get(node, 0, &child), ERR_AST_CHILD_GET);
        *index = child->i + child->l;
    }
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_leftopened(Lex *lex, size_t index, Ast *node)
{
    bool err_nest = false;
    size_t found_new = false;
    size_t found_fun = false;
    size_t found_def = false;
    size_t i_new = 0;
    char c = 0;
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    do {
        LexItem *item = 0;
        TRY(lex_get(lex, index, &item), ERR_LEX_GET);
        if(item->id == LEX_ID_SEPARATOR) {
            c = str_at(&item->str, 0);
            switch(c) {
                case LEX_CH_NEW_CLOSE: {
                    found_new++;
                } break;
                case LEX_CH_NEW_OPEN: {
                    if(!found_new) i_new = index;
                    found_new--;
                } break;
                case LEX_CH_FUN_CLOSE: {
                    found_fun++;
                } break;
                case LEX_CH_FUN_OPEN: {
                    if(!found_fun) err_nest = true;
                    found_fun--;
                } break;
                case LEX_CH_DEF_CLOSE: {
                    found_def++;
                } break;
                case LEX_CH_DEF_OPEN: {
                    if(!found_def) err_nest = true;
                    found_def--;
                } break;
                default: break;
            }
        }
        if(err_nest) break;
        if(!index) break;
        index--;
    } while(true);
    if(i_new) {
        c = LEX_CH_NEW_OPEN;
        index = i_new;
    }
    if(err_nest) {
        /* announce the error */
        switch(c) {
            case LEX_CH_NEW_OPEN: {
                THROW("left %c%c opened", LEX_CH_NEW_OPEN, LEX_CH_NEW_CLOSE);
            } break;
            case LEX_CH_FUN_OPEN: {
                THROW("left %c%c opened", LEX_CH_FUN_OPEN, LEX_CH_FUN_CLOSE);
            } break;
            case LEX_CH_DEF_OPEN: {
                THROW("left %c%c opened", LEX_CH_DEF_OPEN, LEX_CH_DEF_CLOSE);
            } break;
            default: {
                err_nest = false;
                THROW(ERR_UNREACHABLE);
            } break;
        }
    }
    return 0;
error:
    if(err_nest) {
        lex_hint(ERR_FILE_STREAM, lex, index);
    }
    return -1;
}

ErrDeclStatic parse_static_increment_index(size_t *index)
{
    if(!index) THROW(ERR_SIZE_T_POINTER);
    (*index)++;
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_int(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* don't require child, but num */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    if(item->id == LEX_ID_LITERAL) {
        (*node)->branch.num = item->num;
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
    }
    else if(item->id == LEX_ID_OPERATOR && item->str.l && str_at(&item->str, 0) == LEX_CH_MAX) {
        (*node)->branch.num = VAL_MAX;
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
    }
    TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_fnum(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* don't require child */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    //if(strchr(LEX_FNUM LEX_STR_LEFTOVER, str_at(&item->str, 0))) {
    if(item->str.l == 1 && strchr(LEX_FNUM, str_at(&item->str, 0))) { /* TODO maybe, *just maybe* check in other places for item->str.l where I use str_at(...) */
        (*node)->branch.ch = str_at(&item->str, 0);
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
    }
    TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_top(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* don't require child */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    if(item->id == LEX_ID_OPERATOR && item->str.l && str_at(&item->str, 0) == LEX_CH_TOP) {
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
    }
    TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_below(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* don't require child */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    if(item->id == LEX_ID_OPERATOR && item->str.l && str_at(&item->str, 0) == LEX_CH_BELOW) {
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
    }
    TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_empty(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* don't require child - manually set ok on empty */
    /* TODO... why manually? */
    TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
    TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_num(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* add a child to hold node */
    Ast *child = 0;
    TRY(ast_child_ensure(*node, 1), ERR_AST_CHILD_ENSURE);
    TRY(ast_child_get(*node, 0, &child), ERR_AST_CHILD_GET);
    /* go over each possible case */
    switch(child->id) {
        case AST_ID_NONE: {
            child->id = AST_ID_INT;
        } break;
        case AST_ID_INT: {
            if(!child->ok) child->id = AST_ID_TOP;
        } break;
        case AST_ID_TOP: {
            if(!child->ok) child->id = AST_ID_BELOW;
        } break;
        case AST_ID_BELOW: {
            if(!child->ok) child->id = AST_ID_EMPTY;
        } break;
        case AST_ID_EMPTY: {
            child->ok = true;
        } break;
        default: THROW(ERR_UNHANDLED_ID);
    }
    if(!child->ok) *node = child;
    else {
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    }
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_match(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* don't require child */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    if(item->id == LEX_ID_STRING) {
        (*node)->branch.str = &item->str;
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
    }
    TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    return 0;
error:
    /* TODO... can I uncomment this..? */
    //lex_hint(stdout, lex, *index);
    return -1;
}

ErrDeclStatic parse_static_term(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
#if 1
    Ast *parent = 0;
    TRY(ast_parent_get(*node, &parent), ERR_AST_PARENT_GET);
    if(!parent) THROW(ERR_AST_POINTER);
    Ast *child = 0;
    size_t child_len = 0;
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    if(child_len) TRY(ast_child_get(*node, child_len - 1, &child), ERR_AST_CHILD_GET);
    if(!(child_len && child->ok)) {
        if(((parent->id != AST_ID_OR && parent->id != AST_ID_XOR) || (*node)->ok)
                && (item->id == LEX_ID_IDENTIFIER
                    || (item->id == LEX_ID_OPERATOR
                        && item->str.l && (str_at(&item->str, 0) != LEX_CH_OR /*&& str_at(&item->str, 0) != LEX_CH_XOR*/ && !strchr(LEX_TERM, str_at(&item->str, 0)))))) {
            TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
            TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
            ast_free(*node, true, false);
            return 0;
        }
    }
#endif
    /* add a child to hold node */
    bool try_b = false;
    bool child_ok = false;
    TRY(ast_child_ensure(*node, 1), ERR_AST_CHILD_ENSURE);
    TRY(ast_child_get(*node, 0, &child), ERR_AST_CHILD_GET);
    TRY(ast_child_ok(child, &child_ok), ERR_AST_CHILD_OK);
    if(!child_ok) {
        /* go over each case */
        switch(child->id) { /* TODO add filepath */
            case AST_ID_NONE: {
                child->id = AST_ID_INT;
                *node = child;
                return 0;
            } break;
            case AST_ID_INT: {
                child->id = AST_ID_FNUM;
                *node = child;
                return 0;
            } break;
            case AST_ID_FNUM: {
                child->id = AST_ID_MATCH;
                *node = child;
                return 0;
            } break;
            case AST_ID_MATCH: {
                child->id = AST_ID_EMPTY;
                *node = child;
                return 0;
            } break;
            case AST_ID_EMPTY: {
                child->ok = true;
                TRY(ast_child_ok(child, &child_ok), ERR_AST_CHILD_OK);
            } break;
            default: THROW(ERR_UNHANDLED_ID);
        }
    }
    if(item->str.l && strchr(LEX_RSPT, str_at(&item->str, 0))) try_b = true;
    if(child_ok && try_b) {
        AstIDList id = AST_ID_NONE;
        switch(str_at(&item->str, 0)) {
            case LEX_CH_RANGE: id = AST_ID_RANGE; break;
            case LEX_CH_SEQUENCE: id = AST_ID_SEQUENCE; break;
            case LEX_CH_POWER: id = AST_ID_POWER; break;
            case LEX_CH_TIMES: id = AST_ID_TIMES; break;
            default: break;
        }
        if(id) {
            TRY(ast_insert(child, id), ERR_AST_INSERT);
            *node = child;
            return 0;
        } else {
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
            TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
            return 0;
        }
    }
#if 0
        AstIDList id = AST_ID_NONE;
        switch(str_at(&item->str, 0)) {
            case LEX_CH_RANGE: id = AST_ID_RANGE; break;
            case LEX_CH_SEQUENCE: id = AST_ID_SEQUENCE; break;
            case LEX_CH_TIMES: id = AST_ID_TIMES; break;
            default: break;
        }
        if(id) {
            INFO(">> TO-INSERT-ID: %s [%s]\n", ast_id_str(id), item->str.s);
            TRY(ast_insert(child, id), ERR_AST_INSERT);
            *node = child;
            TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
            return 0;
        }
#endif
    /* TODO below... don't always go to the parent :) what the hell did I mean when I wrote that?! */
    if(child_ok) {
        (*node)->ok = true;;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    }
    return 0;
error:
    return -1;
}

static int parse_static_rsmt(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    Ast *child = 0;
    //Ast *child2 = 0;
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* properly step through each case */
    bool child_ok = false;
    size_t child_len = 0;
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    TRY(ast_child_ok(*node, &child_ok), ERR_AST_CHILD_OK);
    if(!child_len || (child_len == 1 && !child_ok)) {
        /* we expect to always insert, so there should be a child already */
        THROW(ERR_UNREACHABLE);
    } else if(child_len == 1) {
        /* verify beginning */
        if(item->str.l && !strchr(LEX_RSPT, str_at(&item->str, 0))) {
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
            TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
            return 0;
        }
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
        /* initiate first state */
        TRY(ast_child_ensure(*node, 2), ERR_AST_CHILD_ENSURE);
        TRY(ast_child_get(*node, 1, &child), ERR_AST_CHILD_GET);
        child->id = AST_ID_INT;
        *node = child;
    } else if(child_len == 2 && !child_ok) {
        TRY(ast_child_get(*node, 1, &child), ERR_AST_CHILD_GET);
        /* TODO : this is similar to the term part : go over each case */
        switch(child->id) {
            //case AST_ID_NONE: {
            //    child->id = AST_ID_INT;
            //    *node = child;
            //    return 0;
            //} break;
            case AST_ID_INT: {
                child->id = AST_ID_FNUM;
                *node = child;
                return 0;
            } break;
            case AST_ID_FNUM: {
                child->id = AST_ID_MATCH;
                *node = child;
                return 0;
            } break;
            case AST_ID_MATCH: {
                child->id = AST_ID_EMPTY;
                *node = child;
                return 0;
            } break;
            case AST_ID_EMPTY: {
                child->ok = true;
                TRY(ast_child_ok(child, &child_ok), ERR_AST_CHILD_OK);
            } break;
            default: THROW(ERR_UNHANDLED_ID);
        }
    } else {
        /* succes, exit */
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    }
#if 0
    if(!strchr(LEX_RST, str_at(&item->str, 0))) {
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
        return 0;
    }
    bool child_ok = false;
    TRY(ast_child_ok(*node, &child_ok), ERR_AST_CHILD_OK);
    if(child_ok) {
        (*node)->branch.ch = str_at(&item->str, 0);
    }
    /* check if we're already done */
    if((*node)->arrl > 1) {
        TRY(ast_child_get(*node, 0, &child), ERR_AST_CHILD_GET);
        TRY(ast_child_get(*node, 1, &child2), ERR_AST_CHILD_GET);
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
        return 0;
    }
    /* set up next */
    size_t child_len = 0;
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    TRY(ast_child_ensure(*node, child_len + 1), ERR_AST_CHILD_ENSURE);
    TRY(ast_child_get(*node, child_len, &child), ERR_AST_CHILD_GET);
    child->id = AST_ID_NUM;
    *node = child;
#endif
    return 0;
error:
    return -1;
}

/* TODO CLEANUP FROM HERE ON */

static int parse_static_expression(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* add a child to hold node */
    size_t child_len = 0;
    bool try_b = false;
    Ast *child = 0;
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    bool add_node = !child_len;
    TRY(ast_child_ensure(*node, child_len + add_node), ERR_AST_CHILD_ENSURE);
    TRY(ast_child_get(*node, child_len - !add_node, &child), ERR_AST_CHILD_GET);
    /* go over each case */
    switch(child->id) {
        case AST_ID_NONE: {
            child->id = AST_ID_TERM;
            *node = child;
        } break;
        case AST_ID_TERM: {
            try_b = true;
        } break;
        case AST_ID_OR: {
            if(!child->ok) {
                ast_free(child, true, false);
                child->id = AST_ID_XOR;
                *node = child;
            } else {
                try_b = true;
            }
        } break;
        case AST_ID_XOR: {
            if(!child->ok) {
                ast_free(child, true, false);
                child->id = AST_ID_TERM;
                *node = child;
            } else {
                try_b = true;
            }
        } break;
        default: THROW(ERR_UNHANDLED_ID);
    }
    if(!try_b) return 0;
    /* check if we're at an or / xor, if not we're done */
    AstIDList id_insert = AST_ID_NONE;
    if(item->id == LEX_ID_OPERATOR && item->str.l) {
        if(str_at(&item->str, 0) == LEX_CH_OR/* || str_at(&item->str, 0) == LEX_CH_XOR*/) {
            if(child->id == AST_ID_TERM || child_len % 2 == 0) {
                id_insert = AST_ID_OR;
            }
            else {
                id_insert = AST_ID_XOR;
            }
        }
    }
    if(!id_insert) {
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
        return 0;
    }
    bool ensure_instead = (child->id == AST_ID_OR || child->id == AST_ID_XOR);
    if(id_insert) {
        if(!ensure_instead) {
            TRY(ast_insert(child, id_insert), ERR_AST_INSERT);
        } else {
            TRY(ast_child_ensure(*node, child_len + 1), ERR_AST_CHILD_ENSURE);
            TRY(ast_child_get(*node, child_len, &child), ERR_AST_CHILD_GET);
            child->id = id_insert;
        }
    }
    *node = child;
    return 0;
error:
    return -1;
}

static int parse_static_or_xor(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* check if we're at an or / xor */
    //char lex_ch = 0;
    size_t child_len = 0;
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    //if((*node)->id == AST_ID_XOR && !child_len) lex_ch = LEX_CH_XOR;
    //else lex_ch = LEX_CH_OR;
    if(item->id == LEX_ID_OPERATOR && item->str.l && (str_at(&item->str, 0) == LEX_CH_OR /*|| (!child_len && str_at(&item->str, 0) == LEX_CH_XOR)*/)) {
        /* add a child */
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
        size_t child_len = 0;
        TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
        TRY(ast_child_ensure(*node, child_len + 1), ERR_AST_CHILD_ENSURE);
        Ast *child = 0;
        TRY(ast_child_get(*node, child_len, &child), ERR_AST_CHILD_GET);
        child->id = AST_ID_TERM;
        *node = child;
    } else {
        (*node)->ok = true;// (child_len);
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    }
    return 0;
error:
    return -1;
}

static int parse_static_cmd(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* check if we're at a cmd */
    if(item->id == LEX_ID_OPERATOR && item->str.l) {
        (*node)->branch.ch = str_at(&item->str, 0);
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
    }
    TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_new(Lex *lex, size_t *index, Ast **node)
{
    bool err_nest = false;
    bool add_expr = false;
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    size_t child_len = 0;
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* check if we left it open */
    if(item->id == LEX_ID_END) {
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
        return 0;
    }
    /* check if we're at a new */
    INFO("STR LEN %zu/%zu", item->l, item->str.l);
    if(item->id == LEX_ID_SEPARATOR && item->str.l && str_at(&item->str, 0) == LEX_CH_NEW_OPEN) {
        /* check for nesting error */
        if((*node)->arrl) {
            err_nest = true;
            THROW("nesting of %c%c unsupported", LEX_CH_NEW_OPEN, LEX_CH_NEW_CLOSE);
        }
        add_expr = true;
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
        TRY(lex_skip_whitespace(lex, index), ERR_LEX_SKIP_WS);
        /* check for error */
        TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
        /* check if we left it open */
        if(item->id == LEX_ID_END) {
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
            TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
            return 0;
        }
    }
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    if((add_expr || child_len) && item->id == LEX_ID_SEPARATOR && (item->str.l && (str_at(&item->str, 0) == LEX_CH_DEF_OPEN || str_at(&item->str, 0) == LEX_CH_FUN_OPEN))) { /* TODO IMPORTANT do that outside of when something was added, too! */
        err_nest = true;
        THROW("can't nest %c in %c%c", item->str.l ? str_at(&item->str, 0) : 0, LEX_CH_NEW_OPEN, LEX_CH_NEW_CLOSE);
    }
    if((add_expr || child_len) && (item->str.l && (str_at(&item->str, 0) == LEX_CH_DEF_CLOSE || str_at(&item->str, 0) == LEX_CH_FUN_CLOSE))) { /* TODO IMPORTANT do that outside of when something was added, too! */
        err_nest = true;
        *index = (*node)->i;
        THROW("couldn't find matching %c", LEX_CH_NEW_CLOSE);
    }
    /* check if we're done already */
    if(item->id == LEX_ID_SEPARATOR && item->str.l && str_at(&item->str, 0) == LEX_CH_NEW_CLOSE) {
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
    } else if(add_expr || child_len) {
        /* else, add child */
        Ast *child = 0;
        TRY(ast_child_ensure(*node, child_len + 1), ERR_AST_CHILD_ENSURE);
        TRY(ast_child_get(*node, child_len, &child), ERR_AST_CHILD_GET);
        child->id = AST_ID_EXPRESSION;
        *node = child;
        return 0;
    } else {
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
    }
    TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    return 0;
error:
    if(err_nest) {
        lex_hint(ERR_FILE_STREAM, lex, *index);
    }
    return -1;
}

ErrDeclStatic parse_static_func_call(Lex *lex, size_t *index, Ast **node, bool *whitespace_sensitive)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    if(!whitespace_sensitive) THROW(ERR_BOOL_POINTER);
    /* retrieve lex item */
    AstIDList add_id = AST_ID_NONE;
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* check if we're at a func call */
    if(item->id == LEX_ID_IDENTIFIER || item->id == LEX_ID_LITERAL || item->id == LEX_ID_STRING) {
        add_id = AST_ID_STR;
    } else if(item->id == LEX_ID_OPERATOR && str_at(&item->str, 0) == LEX_CH_TOP) {
        add_id = AST_ID_CMD;
    }
    if(add_id != AST_ID_NONE) {
        *whitespace_sensitive = true;
        Ast *child = 0;
        size_t child_len = 0;
        TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
        TRY(ast_child_ensure(*node, child_len + 1), ERR_AST_CHILD_ENSURE);
        TRY(ast_child_get(*node, child_len, &child), ERR_AST_CHILD_GET);
        child->id = add_id;
        *node = child;
    } else {
        *whitespace_sensitive = false;
        (*node)->ok = ((*node)->arrl != 0);
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    }
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_str(Lex *lex, size_t *index, Ast **node)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* check if we really want to add a str */
    if(item->id == LEX_ID_IDENTIFIER || item->id == LEX_ID_LITERAL || item->id == LEX_ID_STRING) {
        (*node)->branch.str = &item->str;
#if 0
        /* check special case for single _ */
        if(!(*node)->branch.str) THROW(ERR_UNREACHABLE);
        Ast *parent = {0};
        TRY(ast_parent_get(*node, &parent), ERR_AST_PARENT_GET);
        if(!parent) THROW(ERR_AST_POINTER);
        if((parent->id == AST_ID_SCOPE_DEF) && (*node)->branch.str->l == 1 && (*node)->branch.str->s[0] == LEX_CH_LEFTOVER) {
            item->id = LEX_ID_OPERATOR;
        //    if(parent->id == AST_ID_PROGRAM || parent->id == AST_ID_SCOPE) {
        //        (*node)->id = AST_ID_LEFTOVER;
        //    }
        } else {
#endif
            /* special case done*/
            (*node)->ok = true;
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
            TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
        //}
    } else {
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
    }
    TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_statement(Lex *lex, size_t *index, Ast **node)
{
    bool err_nest = false;
    bool add_stmnt = false;
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    size_t child_len = 0;
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* check for left opened bracket */
    if(item->id == LEX_ID_END) {
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
        return 0;
    }
    /* check if we're at the beginning or end of statement */
    if(item->id == LEX_ID_SEPARATOR && str_at(&item->str, 0) == LEX_CH_FUN_OPEN) {
        /* check for nesting error */
        if((*node)->arrl) {
            err_nest = true;
            THROW("nesting of %c%c unsupported", LEX_CH_FUN_OPEN, LEX_CH_FUN_CLOSE);
        }
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
        TRY(lex_skip_whitespace(lex, index), ERR_LEX_SKIP_WS); /* TODO I blindfoldedly added this; does it break anything?`*/
        TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
        /* check for left opened bracket */
        if(item->id == LEX_ID_END) {
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
            TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
            return 0;
        }
        add_stmnt = true;
    }
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    /* check for stray unwanted characters */
    if((add_stmnt || child_len) && item->id == LEX_ID_SEPARATOR && str_at(&item->str, 0) == LEX_CH_NEW_CLOSE) {
        err_nest = true;
        THROW("stray %c found", LEX_CH_NEW_CLOSE);
    }
    if((add_stmnt || child_len) && item->id == LEX_ID_SEPARATOR && str_at(&item->str, 0) == LEX_CH_DEF_CLOSE) {
        err_nest = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
        TRY(parse_static_recall_index(*node, index), "failed recalling index");
        THROW("couldn't find matching %c", LEX_CH_FUN_CLOSE);
    }
    /* check if we're done already */
    if(item->id == LEX_ID_SEPARATOR && str_at(&item->str, 0) == LEX_CH_FUN_CLOSE) {
        if(!add_stmnt && !child_len) {
            err_nest = true;
            THROW("couldn't find matching %c", LEX_CH_FUN_OPEN);
        }
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
        add_stmnt = false;
    } else if(child_len) {
        Ast *child = 0;
        TRY(ast_child_get(*node, child_len - 1, &child), ERR_AST_CHILD_GET);
        if(child->ok) {
            add_stmnt = true;
        } else {
            switch(child->id) {
                case PARSE_ID_STATEMENT_START: {
                    child->id = AST_ID_NEW;
                } break;
                case AST_ID_NEW: {
                    child->id = AST_ID_CMD;
                } break;
                case AST_ID_CMD: {
                    child->id = AST_ID_SCOPE;
                } break;
                default: THROW(ERR_UNHANDLED_ID);
            }
            *node = child;
            return 0;
        }
    } else {
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
    }
    if(add_stmnt) {
        TRY(ast_child_ensure(*node, child_len + 1), ERR_AST_CHILD_ENSURE);
        TRY(ast_child_get(*node, child_len, node), ERR_AST_CHILD_GET);
        (*node)->id = PARSE_ID_STATEMENT_START;
        return 0;
    }
    TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
    return 0;
error:
    if(err_nest) {
        lex_hint(ERR_FILE_STREAM, lex, *index);
    }
    return -1;
}

ErrDeclStatic parse_static_func_time(Lex *lex, size_t *index, Ast **node, bool *newline_sensitive)
{
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    if(!newline_sensitive) THROW(ERR_BOOL_POINTER);
    bool ok = false;
    bool go_up = false;
    /* check how many children there are */
    Ast *child = 0;
    size_t child_len = 0;
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    if(!child_len) {
        /* set the first to be the expression (time) */
        TRY(ast_child_ensure(*node, 1), ERR_AST_CHILD_ENSURE);
        TRY(ast_child_get(*node, 0, &child), ERR_AST_CHILD_GET);
        child->id = AST_ID_EXPRESSION;
        *node = child;
        *newline_sensitive = true;
    } else if(child_len == 1) {
        TRY(ast_child_get(*node, 0, &child), ERR_AST_CHILD_GET);
        if(!child->ok) THROW(ERR_UNREACHABLE);
        /* retrieve lex item, because whitespace sensitive */
        LexItem *item = 0;
        TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
        if(item->id == LEX_ID_WHITESPACE && item->num > 1) {
            *newline_sensitive = false;
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
            TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
            return 0;
        } else {
            *newline_sensitive = false;
            TRY(lex_skip_whitespace(lex, index), ERR_LEX_SKIP_WS);
            /* set next to be the statements*/
            TRY(ast_child_ensure(*node, 2), ERR_AST_CHILD_ENSURE);
            TRY(ast_child_get(*node, 1, &child), ERR_AST_CHILD_GET);
            child->id = AST_ID_STATEMENT;
            TRY(ast_child_get(*node, 1, node), ERR_AST_CHILD_GET);
            return 0;
        }
    } else {
        if(child_len == 2) {
            ok = true; /* TODO how about... checking the child ??? */
        }
        go_up = true;
    }
    if(go_up) {
        TRY(ast_child_get(*node, 1, &child), ERR_AST_CHILD_GET);
        if(!child->ok) {
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
            if(!(*node)->l) {
                INFO("node length was 0, increment index");
                TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
            }
        } else {
            (*node)->ok = ok;
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        }
        *newline_sensitive = false;
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET); /* TODO I moved this from the above else block down here... did it destroy anything ??? */
    }
    return 0;
error:
    return -1;
}

ErrDeclStatic parse_static_scope(Lex *lex, size_t *index, Ast **node)
{
    bool err_def = false;
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* check the parent scope to determine if we're in main scope */
    Ast *parent = 0;
    TRY(ast_parent_get(*node, &parent), ERR_AST_PARENT_GET);
    /* get children */
    size_t child_len = 0;
    Ast *child = 0;
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* check for left opened bracket */
    if(item->id == LEX_ID_END) {
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
        return 0;
    }
    /* make sure we're at a scope */
    bool add_scope = false;
    if(!child_len && item->id == LEX_ID_SEPARATOR && str_at(&item->str, 0) == LEX_CH_DEF_OPEN) {
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
        add_scope = true;
    } else if(child_len && item->id == LEX_ID_SEPARATOR && str_at(&item->str, 0) == LEX_CH_DEF_CLOSE) {
        /* check if child is not ok */
        //printf("HELLO!\n");getchar();
        if(child_len) {
            TRY(ast_child_get(*node, child_len - 1, &child), ERR_AST_CHILD_GET);
            if(!child->ok) {
                ast_free(child, false, false);
                //ast_free(*node, true, false);
            }
        }
        /* finished, node is ok */
        (*node)->ok = true;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
        TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
        return 0;
    } else if(child_len) {
        /* go over each case */
        TRY(ast_child_get(*node, child_len - 1, &child), ERR_AST_CHILD_GET);
        switch(child->id) {
            case AST_ID_SCOPE_DEF: {
                if(child->ok) {
                    add_scope = true;
                    break;
                }
                if(!child->ok && item->id == LEX_ID_SEPARATOR && str_at(&item->str, 0) == LEX_CH_DEF_OPEN) {
                    err_def = true;
                    THROW("expected scope name");
                } else {
                    //ast_free(child, true, false);
                    child->id = AST_ID_FUNC_TIME;
                    *node = child;
                    break;
                }
            } break;
            case AST_ID_FFUNC_TIME:
            case AST_ID_FUNC_TIME: {
                if(child->ok) {
                    add_scope = true;
                    break;
                } else {
                    ast_free(child, true, false);
                    child->id = AST_ID_SCOPE_DEF;
                    *node = child;
                    break;
                }
            } break;
            default: THROW(ERR_UNHANDLED_ID);
        }
    }
    if(add_scope) {
        /* set the first to be the expression (time) */
        TRY(ast_child_ensure(*node, child_len + 1), ERR_AST_CHILD_ENSURE);
        TRY(ast_child_get(*node, child_len, &child), ERR_AST_CHILD_GET);
        child->id = AST_ID_SCOPE_DEF;
        *node = child;
    } else {
        if(!child_len) {
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
            TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
            TRY(parse_static_increment_index(index), ERR_PARSE_INCREMENT_INDEX);
        }
        return 0;
    }
    return 0;
error:
    if(err_def) {
        lex_hint(ERR_FILE_STREAM, lex, *index);
    }
    return -1;
}

ErrDeclStatic parse_static_scope_def(Lex *lex, size_t *index, Ast **node)
{
    bool err_nest = false;
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    bool ok = false;
    bool go_up = false;
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* check how many children there are */
    Ast *child = 0;
    size_t child_len = 0;
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    INFO("child_len = %zu", child_len);
    if(!child_len) {
        INFO("item->id = %s", lex_id_str(item->id));
        //if(item->id != LEX_ID_IDENTIFIER) {
        if(item->id != LEX_ID_IDENTIFIER && item->id != LEX_ID_STRING) {
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
            TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
        } else {
            /* set the first to be the name (str) */
            TRY(ast_child_ensure(*node, 1), ERR_AST_CHILD_ENSURE);
            TRY(ast_child_get(*node, 0, &child), ERR_AST_CHILD_GET);
            child->id = AST_ID_STR;
            *node = child;
        }
    } else if(child_len == 1) {
        TRY(ast_child_get(*node, 0, &child), ERR_AST_CHILD_GET);
        if(!child->ok) {
            INFO(F("change time!!!!!! (mark as comment, aka. remove)", BG_WT_B FG_BK));
            ast_free(*node, true, false);
            TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
            TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
            return 0;
        } else {
            /* confirm pattern for definition */
            //TRY(ast_adjust_il(child, *index), ERR_AST_ADJUST_IL);
            LexItem *item = 0;
            TRY(lex_skip_whitespace(lex, index), ERR_LEX_SKIP_WS);
            TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
            INFO("lex id %s", lex_id_str(item->id));
            if(item->id == LEX_ID_IDENTIFIER || (item->id == LEX_ID_SEPARATOR && item->str.l && str_at(&item->str, 0) == LEX_CH_DEF_CLOSE)) {
                TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
                return 0;
            }
            if(item->id != LEX_ID_IDENTIFIER && item->str.l && str_at(&item->str, 0) != LEX_CH_DEF_OPEN) {
                INFO("go up since pattern doesn't match");
                *index = child->i;
                ast_free(*node, true, false);
                TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
                TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
                return 0;
            } else {
                /* set next to be the statements*/
                INFO("do statements now");
                TRY(ast_child_ensure(*node, 2), ERR_AST_CHILD_ENSURE);
                TRY(ast_child_get(*node, 1, &child), ERR_AST_CHILD_GET);
                child->id = AST_ID_SCOPE;
                TRY(ast_child_get(*node, 1, node), ERR_AST_CHILD_GET);
            }
        }
    } else {
        if(child_len == 2) {
            ok = true; /* TODO how about... checking the child ??? */
        }
        go_up = true;
    }
    if(go_up) {
        Ast *parent = 0;
        TRY(ast_adjust_il(*node, *index), ERR_AST_ADJUST_IL);
        TRY(ast_parent_get(*node, &parent), ERR_AST_PARENT_GET); /* TODO I moved this from the above else block down here... did it destroy anything ??? */
        TRY(ast_child_get(*node, 1, &child), ERR_AST_CHILD_GET);
        if(child->ok) {
            (*node)->ok = ok;
        }
        *node = parent;
    }
    return 0;
error:
    if(err_nest) {
        lex_hint(ERR_FILE_STREAM, lex, *index);
    }
    return -1;
}

ErrDeclStatic parse_static_program(Lex *lex, size_t *index, Ast **node)
{
    bool err_def = false;
    if(!lex) THROW(ERR_LEX_POINTER);
    if(!index) THROW(ERR_SIZE_T_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* retrieve lex item */
    LexItem *item = 0;
    TRY(lex_get(lex, *index, &item), ERR_LEX_GET);
    /* check if we're done */
    if(item->id == LEX_ID_END) {
        TRY(ast_adjust_il(*node, *index), "failed adjusting index");
        TRY(ast_parent_get(*node, node), ERR_AST_PARENT_GET);
        return 0;
    }
    /* check how many children there are */
    Ast *child = 0;
    size_t child_len = 0;
    bool add_scope = false;
    TRY(ast_child_len(*node, &child_len), ERR_AST_CHILD_LEN);
    if(!child_len) {
        add_scope = true;
    } else {
        /* check for stray characters */
        if(item->id == LEX_ID_SEPARATOR && item->str.l && str_at(&item->str, 0) == LEX_CH_DEF_CLOSE) {
            err_def = true;
            THROW("stray %c found", LEX_CH_DEF_CLOSE);
        }
        /* go over each case */
        TRY(ast_child_get(*node, child_len - 1, &child), ERR_AST_CHILD_GET);
        switch(child->id) {
            case AST_ID_SCOPE_DEF: {
                if(child->ok) {
                    add_scope = true;
                    break;
                }
                if(!child->ok && item->id == LEX_ID_SEPARATOR && item->str.l && str_at(&item->str, 0) == LEX_CH_DEF_OPEN) {
                    err_def = true;
                    THROW("expected scope name");
                } else {
                    //ast_free(child, true, false);
                    child->id = AST_ID_FUNC_TIME;
                    *node = child;
                    break;
                }
            } break;
            case AST_ID_FFUNC_TIME:
            case AST_ID_FUNC_TIME: {
                if(child->ok) {
                    add_scope = true;
                    break;
                } else {
                    ast_free(child, true, false);
                    child->id = AST_ID_SCOPE_DEF;
                    *node = child;
                    break;
                }
            } break;
            default: THROW(ERR_UNHANDLED_ID": %s", ast_id_str(child->id));
        }
    }
    if(add_scope) {
        /* set the first to be the expression (time) */
        TRY(ast_child_ensure(*node, child_len + 1), ERR_AST_CHILD_ENSURE);
        TRY(ast_child_get(*node, child_len, &child), ERR_AST_CHILD_GET);
        child->id = AST_ID_SCOPE_DEF;
        *node = child;
    }
    return 0;
error:
    if(err_def) {
        lex_hint(ERR_FILE_STREAM, lex, *index);
    }
    return -1;
}


ErrDeclStatic parse_static_optimize_new_fnew(Ast *root, AstIDList id)
{
    if(!root) THROW(ERR_AST_POINTER);
    INFO("=== OPTIMIZE NEW/FNEW ===");
    AstIDList replace_with = AST_ID_NONE;
    switch(id) {
        case AST_ID_NEW: { replace_with = AST_ID_FNEW; } break;
        case AST_ID_FUNC_TIME: { replace_with = AST_ID_FFUNC_TIME; } break;
        default: THROW(ERR_UNHANDLED_ID" %s", ast_id_str(id));
    }
    Ast *node = root;
    //ssize_t layer = 0;
    //TRY(ast_next(&node, &layer), ERR_AST_CHILD_NEXT);
    //while(layer > 0) {
    //}
#if 1
    if(node->id != id) {
        TRY(ast_next_specific(&node, id), ERR_AST_NEXT_NEW);
    }
    while(node) {
        if(node->id != id) THROW(ERR_UNREACHABLE);
        size_t child_len = 0;
        TRY(ast_child_len(node, &child_len), ERR_AST_CHILD_LEN);
        for(size_t i = 0; i < child_len; i++) {
            Ast *child = 0;
            ssize_t layer = 0;
            TRY(ast_child_get(node, i, &child), ERR_AST_CHILD_GET);
            //ast_inspect_mark(child, child);
            if(child->id != AST_ID_EXPRESSION) continue;
            //if(child->id != AST_ID_EXPRESSION) THROW(ERR_UNREACHABLE);
            //child = node;
            TRY(ast_next(&child, &layer), ERR_AST_NEXT);
            while(layer > 0) {
                //INFO("... %s", ast_id_str(child->id));
                if(!ast_has_child(child) && (child->id == AST_ID_FNUM || child->id == AST_ID_EMPTY)) {
                    INFO("mark %p as %s", node, ast_id_str(replace_with));
                    //if(id == AST_ID_NEW) ast_inspect_mark(node, node);
                    node->id = replace_with;
                    //goto done;
                }
                TRY(ast_next(&child, &layer), ERR_AST_NEXT);
            }
        }
        TRY(ast_next_specific(&node, id), ERR_AST_NEXT_NEW);
    }
//done:
#endif
    return 0;
error:
    return -1;
}

/* TODO check for duplicate function names and error that baby. */
ErrDecl parse_build_func_name_lut(Ast *root)
{
    int err = 0;
    if(!root) THROW(ERR_AST_POINTER);
    size_t width = 7; /* maybe make this an argument option?? */
    Ast *node = root;
    AstIDList id = AST_ID_SCOPE_DEF;
    INFO("build func name lut");
    TRY(ast_next_specific(&node, id), ERR_AST_NEXT_NEW);
    while(node) {
        Ast *child = 0;
        Ast *parent = 0;
        Ast *alut = 0;
        LutdAst *lut = 0;
        size_t parent_len = 0;
        TRY(ast_parent_get(node, &parent), ERR_AST_PARENT_GET);
        TRY(ast_child_len(parent, &parent_len), ERR_AST_CHILD_LEN);
        TRY(ast_child_get(node, 0, &child), ERR_AST_CHILD_GET);
        /* check if there is already a LUT */
        TRY(ast_child_get(parent, parent_len - 1, &alut), ERR_AST_CHILD_GET);
        if(alut->id != AST_ID_LUT) {
            INFO("init lut on ast");
            TRY(ast_child_ensure(parent, parent_len + 1), ERR_AST_CHILD_ENSURE);
            TRY(ast_child_get(parent, parent_len, &alut), ERR_AST_CHILD_GET);
            alut->id = AST_ID_LUT;
            lut = alut->branch.lut;
            if(lut) THROW(ERR_UNREACHABLE);
            lut = malloc(sizeof(*lut));
            if(!lut) THROW(ERR_MALLOC);
            memset(lut, 0, sizeof(*lut));
            alut->branch.lut = lut;
            TRY(lutd_ast_init(lut, width), ERR_LUTD_INIT);
        } else {
            lut = alut->branch.lut;
        }
        INFO("func name : %s", child->branch.str->s);
        if(lutd_ast_has(lut, child)) THROW("duplicate scope definition: '%.*s'", (int)child->branch.str->l, child->branch.str->s);
        INFO("[add %s]", child->branch.str->s);
        TRY(lutd_ast_add(lut, child), ERR_LUTD_ADD);
        /* keep last */
        alut->ok = true;
        TRY(ast_next_specific(&node, id), ERR_AST_NEXT_NEW);
    }
clean:
    return err;
error: ERR_CLEAN;
}

/******************************************************************************/
/* PUBLIC FUNCTION IMPLEMENTATIONS ********************************************/
/******************************************************************************/

int parse_process(Args *args, Ast *ast, Lex *lex)
{
#if DEBUG_INSPECT
    platform_clear();
#endif
    INFO("entered");
    if(!args) THROW(ERR_ARGS_POINTER);
    if(!ast) THROW(ERR_AST_POINTER);
    if(!lex) THROW(ERR_LEX_POINTER);

    Ast *root = ast;
    Ast *node = ast;
    if(node->id) {
        if(args->merge && node->id == AST_ID_PROGRAM) {
            node->ok = false;
            //TRY(ast_child_get(node, 0, &node), ERR_AST_CHILD_GET);
        } else {
            THROW("wrongly initialized");
        }
    } else {
        node->id = AST_ID_PROGRAM;
    }
    bool whitespace_sensitive = false;
    bool newline_sensitive = false;
    size_t index = 0;

    while(node && index < lex->l) {
        if(!whitespace_sensitive) {
            if(newline_sensitive) {
                TRY(lex_skip_whitespace_on_line(lex, &index), ERR_LEX_SKIP_WS_L);
            } else {
                TRY(lex_skip_whitespace(lex, &index), ERR_LEX_SKIP_WS);
            }
        }
        INFO("parse %p %s", node, ast_id_str(node->id));
        AstIDList id = node->id;
        char *id_str = ast_id_str(id);
#if DEBUG
#if DEBUG_INSPECT
        platform_clear();
#endif
        printf("whitespace_sensitive = %s\n", whitespace_sensitive ? "true" : "false");
        printf("newline_sensitive = %s\n", newline_sensitive ? "true" : "false");
        printf("index = %zu\n", index);
        lex_hint(stdout, lex, index);
        platform_getch();
#if DEBUG_INSPECT
        ast_inspect_mark(root, node);
#endif
#if DEBUG_GETCH
        platform_getch();
#endif
#endif

        if(args->inspect_ast == SPECIFY_STEP) {
#if !DEBUG
            platform_clear();
#endif
            ast_inspect_mark(root, node);
            printf("whitespace_sensitive = %s\n", whitespace_sensitive ? "true" : "false");
            printf("newline_sensitive = %s\n", newline_sensitive ? "true" : "false");
            printf("index = %zu\n", index);
            lex_hint(stdout, lex, index);
            platform_getch();
        }

        switch(id) {
            case AST_ID_INT: {
                TRY(parse_static_int(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_TOP: {
                TRY(parse_static_top(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_BELOW: {
                TRY(parse_static_below(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            //case AST_ID_LEFTOVER: {
            //    TRY(parse_static_leftover(lex, &index, &node), "failed parsing %s", id_str);
            //} break;
            case AST_ID_EMPTY: {
                TRY(parse_static_empty(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_NUM: {
                TRY(parse_static_num(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_FNUM: {
                TRY(parse_static_fnum(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_MATCH: {
                TRY(parse_static_match(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_TERM: {
                TRY(parse_static_term(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_TIMES:
            case AST_ID_POWER:
            case AST_ID_SEQUENCE:
            case AST_ID_RANGE: {
                TRY(parse_static_rsmt(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_XOR:
            case AST_ID_OR: {
                TRY(parse_static_or_xor(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_EXPRESSION: {
                TRY(parse_static_expression(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_FUNC_CALL: {
                TRY(parse_static_func_call(lex, &index, &node, &whitespace_sensitive), "failed parsing %s", id_str);
            } break;
            case AST_ID_STR: {
                TRY(parse_static_str(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_CMD: {
                TRY(parse_static_cmd(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_NEW: {
                TRY(parse_static_new(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_STATEMENT: {
                TRY(parse_static_statement(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_FUNC_TIME: {
                TRY(parse_static_func_time(lex, &index, &node, &newline_sensitive), "failed parsing %s", id_str);
            } break;
            case AST_ID_SCOPE: {
                TRY(parse_static_scope(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_SCOPE_DEF: {
                TRY(parse_static_scope_def(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            case AST_ID_PROGRAM: {
                TRY(parse_static_program(lex, &index, &node), "failed parsing %s", id_str);
            } break;
            default: {
                THROW("unhandled ast id '%s'", id_str ? id_str : "(null)");
            } break;
        }
    }
    TRY(parse_static_leftopened(lex, index, root), "left something opened");
    /* cleanup */
    Ast *child = 0;
    size_t child_len = 0;
    TRY(ast_child_len(root, &child_len), ERR_AST_CHILD_LEN);
    if(child_len) {
        TRY(ast_child_get(root, child_len - 1, &child), ERR_AST_CHILD_GET);
        if(!child->ok) {
            ast_free(child, false, false);
        }
    }
    /* optimize for new/fnew */
    TRY(parse_static_optimize_new_fnew(root, AST_ID_NEW), "failed to optimize NEW/FNEW");
    TRY(parse_static_optimize_new_fnew(root, AST_ID_FUNC_TIME), "failed to optimize NEW/FNEW");
    /* TODO merge those two function calls PLEASE and
     * please, please: DON'T (!!!!!!!!) TRAVERSE THE TREE TWICE (2)
     * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    /* finish */
    TRY(ast_adjust_il(root, 0), ERR_AST_ADJUST_IL);
    root->ok = true; /* just... like this ??? */
#if DEBUG_INSPECT
    platform_clear();
#endif
#if DEBUG
    INFO("built ast");
    lex_hint(stdout, lex, index);
#if DEBUG_INSPECT
    ast_inspect_mark(root, root);
#endif
#if DEBUG_GETCHAR
    platform_getchar();
#endif
#endif

    if(args->inspect_ast == SPECIFY_CONFIRM
            || args->inspect_ast == SPECIFY_STEP) {
#if !DEBUG
        platform_clear();
#endif
        ast_inspect_mark(root, root);
        printf("%s\n", lex->fn.s);
        platform_getchar();
    }

    INFO("reached end (node was NONE)");
    return 0;
error:
    return -1;
}


