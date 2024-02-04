#include "ast.h"
#include "lutd_ast.h"
#include "str.h"
#include <stdint.h>

static int ast_static_child_get_shadowed(Ast *node, size_t index, Ast **child);

/******************************************************************************/
/* PRIVATE FUNCTION IMPLEMENTATIONS *******************************************/
/******************************************************************************/

ErrDeclStatic ast_static_child_update_parent(Ast *parent)
{
    if(!parent) THROW("expected pointer to node");
    size_t arrc = parent->arrc;
    for(size_t i = 0; i < arrc; i++) {
        Ast *child = 0;
        TRY(ast_static_child_get_shadowed(parent, i, &child), "failed getting child");
        INFO(F("child %p %s : parent = %p %s\n", FG_YL), child, ast_id_str(child->id), parent, ast_id_str(parent->id));
        child->parent = parent;
        /* now do that for the child */
        size_t arrc2 = child->arrc;
        for(size_t j = 0; j < arrc2; j++) {
            Ast *child2 = 0;
            TRY(ast_static_child_get_shadowed(child, j, &child2), "failed getting child");
            INFO(F("child2 %p %s : parent %p => %p %s\n", FG_YL), child2, ast_id_str(child2->id), child2->parent, child, ast_id_str(child->id));
            child2->parent = child;
        }
    }
    return 0;
error:
    return -1;
}

ErrDeclStatic ast_static_child_get_shadowed(Ast *node, size_t index, Ast **child)
{
    if(!node) THROW("expected pointer to ast struct");
    if(!child) THROW("expected pointer to child struct");
    if(index >= node->arrc) THROW("index (%zu) out of range (%zu)", index, node->arrc);
    *child = node->branch.child[index];
    return 0;
error:
    return -1;
}

static size_t ast_static_arr_len(Ast *node)
{
    if(!node) return 0;
    Ast *parent = node->parent;
    size_t result = parent ? parent->arrl : 0;
    return result;
}

static size_t ast_static_arr_pos(Ast *node)
{
    if(!node) return 0;
    return node->arri;
}

ErrDeclStatic ast_static_free_range(Ast *clear, size_t iE, bool final_clean)
{
    if(!clear) THROW("expected pointer to an ast struct");
    if(iE) {
        INFO("free range of %p %s (%zu)", clear, ast_id_str(clear->id), iE);
        Ast **unassign_base = clear->branch.child;
        for(size_t i = 0; i < iE; i++) {
            Ast *unassign = unassign_base[i];
            INFO("  %zu > free %p %s", i, unassign, ast_id_str(unassign->id));
            if(final_clean) {
                if(unassign->id == AST_ID_LUT) {
                    lutd_ast_free(unassign->branch.lut);
                    free(unassign->branch.lut);
                }
            }
            free(unassign);
            unassign_base[i] = 0;
        }
        //INFO("> free %p\n", unassign_base);
        free(unassign_base);
        if(!final_clean) {
            memset(clear, 0, sizeof(*clear));
        }
    }
    return 0;
error:
    return -1;
}

bool ast_has_child(Ast *node)
{
    if(!node) return false;
    AstIDList id = node->id;
    switch(id) {
        case AST_ID_NUM:
        case AST_ID_RANGE:
        case AST_ID_TERM:
        case AST_ID_RST:
        case AST_ID_TIMES:
        case AST_ID_SEQUENCE:
        case AST_ID_POWER:
        case AST_ID_EXPRESSION:
        case AST_ID_OR:
        case AST_ID_FUNC_CALL:
        case AST_ID_NEW:
        case AST_ID_FNEW:
        case AST_ID_STATEMENT:
        case AST_ID_FUNC_TIME:
        case AST_ID_FFUNC_TIME:
        case AST_ID_SCOPE:
        case AST_ID_SCOPE_DEF:
        case AST_ID_PROGRAM:
        case AST_ID_XOR:
            return true;
        default: return false;
    }
}

/******************************************************************************/
/* PUBLIC FUNCTION IMPLEMENTATIONS ********************************************/
/******************************************************************************/

char *ast_id_str(AstIDList id)
{
    char *result = 0;
    switch(id) {
        case AST_ID_NONE: result = "NONE"; break;
        case AST_ID_PROGRAM: result = "PROGRAM"; break;
        case AST_ID_COMMENT: result = "COMMENT"; break;
        case AST_ID_SCOPE: result = "SCOPE"; break;
        case AST_ID_FUNC_TIME: result = "FUNC_TIME"; break;
        case AST_ID_FFUNC_TIME: result = "FFUNC_TIME"; break;
        case AST_ID_SCOPE_DEF: result = "SCOPE_DEF"; break;
        case AST_ID_FUNC_CALL: result = "FUNC_CALL"; break;
        case AST_ID_FUNC_NAME: result = "FUNC_NAME"; break;
        case AST_ID_NUM: result = "NUM"; break;
        case AST_ID_FNUM: result = "FNUM"; break;
        case AST_ID_STATEMENT: result = "STATEMENT"; break;
        case AST_ID_RST: result = "RST"; break;
        case AST_ID_EXPRESSION: result = "EXPRESSION"; break;
        case AST_ID_TERM: result = "TERM"; break;
        case AST_ID_INT: result = "INT"; break;
        case AST_ID_EMPTY: result = "EMPTY"; break;
        case AST_ID_TOP: result = "TOP"; break;
        case AST_ID_BELOW: result = "BELOW"; break;
        case AST_ID_OR: result = "OR"; break;
        case AST_ID_XOR: result = "XOR"; break;
        case AST_ID_RANGE: result = "RANGE"; break;
        case AST_ID_SEQUENCE: result = "SEQUENCE"; break;
        case AST_ID_POWER: result = "POWER"; break;
        case AST_ID_TIMES: result = "TIMES"; break;
        case AST_ID_MATCH: result = "MATCH"; break;
        case AST_ID_STR: result = "STR"; break;
        case AST_ID_CMD: result = "CMD"; break;
        case AST_ID_NEW: result = "NEW"; break;
        case AST_ID_FNEW: result = "FNEW"; break;
        case AST_ID_COND: result = "COND"; break;
        case AST_ID_LUT: result = "LUT"; break;
        case AST_ID_VEC: result = "VEC"; break;
        case AST_ID_LEFTOVER: result = "LEFTOVER"; break;
        default: THROW("unknown ast id (%u)", id); break;
    }
error:
    return result;
}

int ast_child_ensure(Ast *node, size_t count)
{
    if(!node) THROW("expeted pointer to ast struct");
    size_t n = count;
    if(n) {
        /* calculate required memory */
        size_t required = node->arrc ? node->arrc : AST_DEFAULT_CAP;
        while(required < n) required *= 2;
        /* only increase capacity */
        if(required > node->arrc) {
            AstArray **child_array = &node->branch.child;
            if(!node->arrl && *child_array) THROW("invalid initialization");
            void *temp = realloc(*child_array, sizeof(**child_array) * required);
            if(!temp) THROW("could not increase capacity of child");
            /* initialize new memory */
            *child_array = temp;
            size_t new_size = required - node->arrc;
            memset(&(*child_array)[node->arrc], 0, sizeof(*child_array) * new_size);
            for(size_t i = node->arrc; i < required; i++) {
                Ast **child = &(*child_array)[i];
                if(*child) THROW("not initialiazed correctly");
                *child = malloc(sizeof(**child));
                if(!*child) THROW("could not allocate memory");
                memset(*child, 0, sizeof(**child));
                (*child)->parent = node;
                (*child)->arri = i;
            }
            node->arrc = required;
        }
    }
    return 0;
error:
    return -1;
}

int ast_child_get(Ast *node, size_t index, Ast **child)
{
    if(!node) THROW("expected pointer to ast struct");
    if(!child) THROW("expected pointer to child struct");
    if(index >= node->arrc) THROW("index (%zu) out of range (%zu)", index, node->arrc);
    *child = node->branch.child[index];
    if(index + 1 > node->arrl) node->arrl = index + 1; // maybe set that here???
    return 0;
error:
    return -1;
}

int ast_child_len(Ast *node, size_t *len)
{
    if(!node) THROW("expected pointer to ast struct");
    if(!len) THROW("expected pointer to size_t");
    *len = node->arrl;
    return 0;
error:
    return -1;
}

int ast_child_ok(Ast *node, bool *ok)
{
    if(!node) THROW(ERR_AST_POINTER);
    if(!ok) THROW(ERR_BOOL_POINTER);
    if(ast_has_child(node)) {
        size_t child_len = 0;
        TRY(ast_child_len(node, &child_len), ERR_AST_CHILD_LEN);
        bool this_ok = (child_len);
        for(size_t i = 0; i < child_len; i++) {
            Ast *child = 0;
            TRY(ast_child_get(node, i, &child), ERR_AST_CHILD_GET);
            this_ok &= child->ok;
        }
        *ok = this_ok;
    } else {
        *ok = node->ok;
    }
    return 0;
error:
    return -1;
}

int ast_child_cap(Ast *node, size_t *cap)
{
    if(!node) THROW("expected pointer to ast struct");
    if(!cap) THROW("expected pointer to size_t");
    *cap = node->arrc;
    return 0;
error:
    return -1;
}

int ast_parent_get(Ast *node, Ast **parent)
{
    if(!node) THROW("expected pointer to ast struct");
    if(!parent) THROW("expected pointer to ast struct");
    *parent = node->parent;
    return 0;
error:
    return -1;
}

int ast_copy(Ast *dest, Ast *source, Ast *parent)
{
    if(!dest) THROW("expected pointer to ast struct");
    if(!source) THROW("expected pointer to ast struct");
    /* don't overwrite parent ! therefore, save it */
    memcpy(dest, source, sizeof(*dest));
    dest->parent = parent;
    return 0;
error:
    return -1;
}

int ast_insert(Ast *node, AstIDList id)
{
    if(!node) THROW("expected pointer to ast struct");
    if(!id) THROW("don't insert id NONE");
    /* TODO make this one memcpy or something */
    Ast **keep = node->branch.child;
    Ast copy = {0};
    memcpy(&copy, node, sizeof(copy));
    node->branch.child = 0;
    node->arri = 0;
    node->arrl = 0;
    node->arrc = 0;
    node->ok = 0;
    Ast *child = 0;
    TRY(ast_child_ensure(node, 1), "failed ensuring child");
    TRY(ast_child_get(node, 0, &child), "failed getting child");
    child->id = copy.id;
    child->branch.child = keep;
    child->arrl = copy.arrl;
    child->arrc = copy.arrc;
    child->ok = copy.ok;
    child->i = copy.i;
    child->l = copy.l;
    node->id = id;
    TRY(ast_static_child_update_parent(node), "failed updating parent");
    return 0;
error:
    return -1;
}

int ast_array_get(Ast *node, size_t index, Ast **got)
{
    if(!node) THROW("expected pointer to ast struct");
    if(!got) THROW("expected pointer to ast struct");
    Ast *parent = 0;
    TRY(ast_parent_get(node, &parent), "failed getting parent");
    TRY(ast_child_get(parent, index, got), "failed getting child");
    return 0;
error:
    return -1;
}

int ast_next(Ast **node, ssize_t *layer)
{
    if(!node) THROW("expected pointer to ast struct");
    if(!layer) THROW("expected pointer to size_t");
    /* note to self: when not entering child, check ast_has_child */
    bool has_child = ast_has_child(*node);
    uint64_t arr_len = ast_static_arr_len(*node);
    uint64_t arr_pos = ast_static_arr_pos(*node);
    if(has_child && (*node)->arrl) {
        TRY(ast_child_get(*node, 0, node), "failed getting child");
        (*layer)++;
    } else if(arr_pos + 1 < arr_len) {
        if((size_t)arr_pos > (size_t)1100) THROW("sus arr_pos %zu %p", arr_pos, *node);
        Ast *got_next = 0;
        TRY(ast_array_get(*node, arr_pos + 1, &got_next), "failed getting node");
        if(*node == got_next) THROW("something's wrong (maybe arri is set wrongly somewhere");
        *node = got_next;
    } else while(*node) {
        TRY(ast_parent_get(*node, node), "failed getting parent");
        (*layer)--;
        if(!*node) break;
        arr_len = ast_static_arr_len(*node);
        arr_pos = ast_static_arr_pos(*node);
        if((size_t)arr_pos > (size_t)1100) THROW("sus arr_pos %zu %p", arr_pos, *node);
        bool is_it_too_large = arr_pos > (uint64_t)1100;
        if(is_it_too_large) THROW("sus arr_pos %zu %p", arr_pos, *node);
        if(arr_pos + 1 < arr_len) {
            Ast *got_next = 0;
            TRY(ast_array_get(*node, arr_pos + 1, &got_next), "failed getting node");
            if(*node == got_next) THROW("something's wrong (maybe arri is set wrongly somewhere");
            *node = got_next;
            break;
        }
    }
    return 0;
error:
    return -1;
}

int ast_next_specific(Ast **node, AstIDList id)
{
    if(!node) THROW("expected pointer to ast struct");
    ssize_t layer = 0;
    TRY(ast_next(node, &layer), ERR_AST_NEXT);
    while(*node) {
        if((*node)->id == id) {
            break;
        }
        TRY(ast_next(node, &layer), ERR_AST_NEXT);
    }
    return 0;
error:
    return -1;
}


void ast_inspect_mark(Ast *node, Ast *mark)
{
    if(!node) return;
    ssize_t spaces = 0;
    Ast **peek = &node;
    printf("\e[25l"); /* disable cursor https://stackoverflow.com/questions/30126490/how-to-hide-console-cursor-in-c */
    while(node) {
        node = *peek;
        char *idstr = ast_id_str(node->id);
        TRY(!idstr, "couldn't get ast ID string");
        if(mark == node) {
            if(node->ok) {
                printf(F("%*s%-*s", FG_GN_B), (int)spaces, "", 12, idstr);
            } else {
                printf(F("%*s%-*s", FG_RD_B), (int)spaces, "", 12, idstr);
            }
            printf(F("%p", FG_MG_B), node);
            printf(", ");
            printf(F("%zu+%zu", FG_BL_B), node->i, node->l);
        } else {
            if(node->ok) {
                printf(F("%*s%-*s", FG_GN), (int)spaces, "", 12, idstr);
            } else {
                printf(F("%*s%-*s", FG_RD), (int)spaces, "", 12, idstr);
            }
            printf(F("%p", FG_MG), node);
            printf(", ");
            printf(F("%zu+%zu", FG_BL), node->i, node->l);
        }
        printf(", ");
        switch(node->id) {
            case AST_ID_NONE: {
                printf("(arrc=%zu", node->arrc);
                THROW("should not encounter ID 'NONE'");
            } break;
            case AST_ID_INT: {
                printf(FMT_VAL, (node->branch.num));
            } break;
            case AST_ID_FNUM:
            case AST_ID_CMD: {
                printf("%c", node->branch.ch);
            } break;
            case AST_ID_STR:
            case AST_ID_MATCH: {
                if(node->branch.str) printf("'%s' (%zu)", node->branch.str->s, node->branch.str->l);
                else printf("(null)");
            } break;
            case AST_ID_PROGRAM:
            case AST_ID_EXPRESSION:
            case AST_ID_TERM:
            case AST_ID_SCOPE_DEF:
            case AST_ID_SCOPE:
            case AST_ID_FUNC_TIME:
            case AST_ID_FFUNC_TIME:
            case AST_ID_LEFTOVER:
            case AST_ID_STATEMENT:
            case AST_ID_NEW:
            case AST_ID_FNEW:
            case AST_ID_FUNC_CALL:
            case AST_ID_OR:
            case AST_ID_XOR:
            case AST_ID_TIMES:
            case AST_ID_POWER:
            case AST_ID_SEQUENCE:
            case AST_ID_RANGE: {
                printf("%zu", node->arrl);
            } break;
            case AST_ID_VEC:
            case AST_ID_LUT: 
            case AST_ID_EMPTY:
            case AST_ID_BELOW:
            case AST_ID_TOP:
            case AST_ID_NUM: {
                printf("\b\b ");
            } break;
            case AST_ID_COMMENT: {} break;
            default: THROW("unknown / unhandled ast ID: %d", node->id);
        }
        printf(", arri %zu", node->arri);
        printf("\n");
        TRY(ast_next(&node, &spaces), "failed getting next node");
        if(spaces <= 0) break;
    }
    printf("\n");
error:
    printf("\e[25h"); /* reenable the cursor */
}

void ast_inspect(Ast *node)
{
    if(!node) return;
    ssize_t spaces = 0;
    INFO(">arrl: %zu\n", node->arrl);
    INFO(">arrc: %zu\n", node->arrc);
    Ast **peek = &node;
    while(node) {
        node = *peek;
        char *idstr = ast_id_str(node->id);
        TRY(!idstr, "couldn't get ast ID string");
        printf("%*s%-*s", (int)spaces, "", 12, idstr);
        printf(F("node = %p", FG_MG_B)" ", node);
        switch(node->id) {
            case AST_ID_NONE: {
                printf("(arrc=%zu", node->arrc);
                THROW("should not encounter ID 'NONE'");
            } break;
            case AST_ID_INT: {
                printf("= " FMT_VAL " (%s)\n", (node->branch.num), node->ok ? "true" : "false");
            } break;
            case AST_ID_EMPTY: {
                printf("(%s)\n", node->ok ? "true" : "false");
            } break;
            case AST_ID_BELOW:
            case AST_ID_TOP: {
                printf("(%s)\n", node->ok ? "true" : "false");
            } break;
            case AST_ID_CMD: {
                printf("= %c (%s)\n", node->branch.ch, node->ok ? "true" : "false");
            } break;
            case AST_ID_NUM: {
                printf("(%s)\n", node->ok ? "true" : "false");
            } break;
            case AST_ID_EXPRESSION:
            case AST_ID_TERM: {
                printf("(%zu, %s)\n", node->arrl, node->ok ? "true" : "false");
            } break;
            case AST_ID_STR:
            case AST_ID_MATCH: {
                if(node->branch.str) printf("= '%s' (%s)\n", node->branch.str->s, node->ok ? "true" : "false");
                else printf("(null)\n");
            } break;
            case AST_ID_SCOPE_DEF:
            case AST_ID_SCOPE:
            case AST_ID_FUNC_TIME:
            case AST_ID_FFUNC_TIME:
            case AST_ID_STATEMENT:
            case AST_ID_NEW:
            case AST_ID_FNEW:
            case AST_ID_FUNC_CALL:
            case AST_ID_OR:
            case AST_ID_RST:
            case AST_ID_TIMES:
            case AST_ID_SEQUENCE:
            case AST_ID_RANGE: {
                printf("(%zu, %s)\n", node->arrl, node->ok ? "true" : "false");
            } break;
            case AST_ID_COMMENT: {} break;
            default: THROW("unknown / unhandled ast ID: %d", node->id);
        }
        TRY(ast_next(&node, &spaces), "failed getting next node");
    }
    printf("\n");
error:
    (void)node; /* gets rid of warning */
}

void ast_free(Ast *ast, bool keep_current_id, bool final_clean)
{
    if(!ast) THROW("expected pointer to an ast struct");
#ifdef DEBUG_GETCHAR
    //if(ast) ast_inspect_mark(ast, ast);
#endif
    ssize_t prev_layer = 0;
    ssize_t ast_layer = 0;
    Ast copy = {0};
    memcpy(&copy, ast, sizeof(copy));
    INFO("free this:");
    Ast *stop = ast;
    Ast *prev = ast;
    bool exit_pls = 0;
    TRY(ast_next(&ast, &ast_layer), "failed getting next");
    while(ast_layer > 0) {
        TRY(ast_next(&ast, &ast_layer), "failed getting next");
        if(ast_layer < prev_layer) {
            prev = prev->parent;
            while(ast ? ast->parent != prev : stop->parent != prev) {
                Ast *clear = prev;
                size_t child_cap = 0;
                size_t arri = clear->arri;
                TRY(ast_child_cap(clear, &child_cap), "failed getting child length");
                TRY(ast_parent_get(clear, &prev), "failed getting parent");
                //INFO("FREE IN LOOP");
                //printf("FREE CHILDREN OF %s %p (%zu)\n", ast_id_str(clear->id), clear, child_cap);
                TRY(ast_static_free_range(clear, child_cap, final_clean), "failed freeing range");
                clear->arrc = 0;
                if(stop == clear) {
                    clear->parent = prev;
                    clear->arri = arri;
                    ast_layer = -1; /* is that solution clean ??? */
                    break;
                }
            }
            if(exit_pls) break;
        }
        prev = ast;
        prev_layer = ast_layer;
    }
    if(keep_current_id) {
        stop->id = copy.id;
        stop->parent = copy.parent;
        stop->arri = copy.arri;
    } else {
        size_t child_cap = 0;
        //INFO("FREE LAST %p", stop);
        //TRY(ast_parent_get(stop, &stop), ERR_AST_PARENT_GET);
        TRY(ast_child_cap(stop, &child_cap), "failed getting child length");
        TRY(ast_static_free_range(stop, child_cap, final_clean), "failed freeing range");
        stop->arrc = 0;
        //printf("stop->arrl = %zu\n", stop->arrl);
        if(copy.parent) { /* copy is equal stop */
            copy.parent->arrl--;
        }
    }
#ifdef DEBUG_GETCHAR
#endif
error:
    (void)ast; /* gets rid of warning */
}

#define clear() printf("\033[H\033[J\n")
int ast_copy_deep(Ast *dst, bool keep_current, Ast *src)
{
    if(!dst) THROW(ERR_AST_POINTER);
    if(!src) THROW(ERR_AST_POINTER);
    //printf("CREATE COPY OF: \n");
    //ast_inspect_mark(src,src);
    ast_free(dst, keep_current, false);
    Ast *root_dst = dst;
    dst->id = src->id;
    if(keep_current) {
        memset(&dst->branch, 0, sizeof(dst->branch));
        /* the great coping */
        dst->id = src->id;
        dst->i = src->i;
        dst->l = src->l;
        dst->ok = true;
        if(!ast_has_child(dst)) {
            //printf("  src->branch.num %zu\n", src->branch.num);
            memcpy(&dst->branch, &src->branch, sizeof(dst->branch));
        }
    }
    ssize_t layer = 0;
    ssize_t layer_prev = 0;
    //printf("%zi       - %s\n", layer, ast_id_str(src->id));
    TRY(ast_next(&src, &layer), ERR_AST_NEXT);
    while(src && layer > 0) {
        ssize_t layer_diff = layer - layer_prev;
        Ast *parent_src = 0;
        Ast *parent_dst = 0;
        TRY(ast_parent_get(src, &parent_src), ERR_AST_PARENT_GET);
        //printf("%zi%s%zi:%zu/%zu c=%zu - %s\n", layer, layer_diff >= 0 ? "+" : "", layer_diff, src->arri+1, parent_src->arrc, src->arrc, ast_id_str(src->id));
        //printf("LAYER DIFF %zi\n", layer_diff);
        /* keep above */
        if(layer_diff > 0) {
            TRY(ast_child_ensure(dst, parent_src->arrc), ERR_AST_CHILD_ENSURE);
            //printf("  A) ensured %zu children on %s\n", dst->arrc, ast_id_str(dst->id));
            //printf("  A) get child index %zu on %s\n", dst->arrl, ast_id_str(dst->id));
            TRY(ast_child_get(dst, dst->arrl, &dst), ERR_AST_CHILD_GET);
        } else if(layer_diff < 0) {
            for(ssize_t i = 0; i < -layer_diff; i++) {
                TRY(ast_parent_get(dst, &dst), ERR_AST_PARENT_GET);
            }
            if(ast_has_child(src)) {
                TRY(ast_child_ensure(dst, src->arrc), ERR_AST_CHILD_ENSURE);
                //printf("  B) ensured %zu children on %s\n", dst->arrc, ast_id_str(dst->id));
            }
            TRY(ast_parent_get(dst, &parent_dst), ERR_AST_PARENT_GET);
            TRY(ast_child_get(parent_dst, dst->arri+1, &dst), ERR_AST_CHILD_GET);
        } else {
            TRY(ast_parent_get(dst, &parent_dst), ERR_AST_PARENT_GET);
            //printf("  C) get child index %zu on %s\n", dst->arri+1, ast_id_str(parent_dst->id));
            TRY(ast_child_get(parent_dst, dst->arri+1, &dst), ERR_AST_CHILD_GET);
        }
        /* the great coping */
        //printf("  id %s, i %zu, l %zu\n", ast_id_str(src->id), src->i, src->l);
        //printf("     id %s, i %zu, l %zu, c %zu\n", ast_id_str(src->id), src->arri, src->arrl, src->arrc);
        dst->id = src->id;
        dst->i = src->i;
        dst->l = src->l;
        dst->ok = true;
        if(!ast_has_child(dst)) {
            //printf("  src->branch.num %zu\n", src->branch.num);
            memcpy(&dst->branch, &src->branch, sizeof(dst->branch));
        }
        /* keep below */
        layer_prev = layer;
        TRY(ast_next(&src, &layer), ERR_AST_NEXT);
    }
    root_dst->ok = true;
    return 0;
error:
    return -1;
}

int ast_adjust_il(Ast *node, size_t index)
{
    if(!node) THROW("expected pointer to ast struct");
    size_t child_len = 0;
    Ast *child = 0;
    TRY(ast_child_len(node, &child_len), "failed getting child length");
    if(node->id == AST_ID_EMPTY) {
        node->i = index;
        node->l = 0;
    } else if(!child_len) {
        node->i = index;
        switch(node->id) {
            case AST_ID_INT:
            case AST_ID_TOP:
            case AST_ID_BELOW:
            case AST_ID_FNUM:
            case AST_ID_CMD:
            case AST_ID_STR: {
                node->l = 1;
            } break;
            default: {
                node->l = 0;
            } break;
        }
    } else {
        /* TODO maybe check for child->ok ? ??? */
        if(!node->i) {
            TRY(ast_child_get(node, 0, &child), "failed getting child");
            node->i = child->i;
        }
        TRY(ast_child_get(node, child_len - 1, &child), "failed getting child");
        node->l = child->i - node->i + child->l;
    }
    return 0;
error:
    return -1;
}


ErrDecl ast_malloc(Ast **node)
{
    if(!node) THROW(ERR_AST_POINTER);
    if(*node) THROW("expected node to be empty");
    *node = malloc(sizeof(**node));
    if(!*node) THROW(ERR_MALLOC);
    memset(*node, 0, sizeof(**node));
    return 0;
error:
    return -1;
}

#define ERR_AST_FIND_LUT "failed finding in lookup table"
ErrDeclStatic ast_static_find_lut(Ast *node, Ast *find, Ast **func)
{
    if(!node) THROW(ERR_AST_POINTER);
    if(!func) THROW(ERR_AST_POINTER);
    if(!find) THROW(ERR_STR_POINTER);
    while(1) {
        Ast *parent = 0;
        TRY(ast_parent_get(node, &parent), ERR_AST_PARENT_GET);
        if(!parent) break;
        size_t parent_len = 0;
        TRY(ast_child_len(parent, &parent_len), ERR_AST_CHILD_LEN);
        for(size_t i = parent_len; i > 0; i--) {
            Ast *child = 0;
            TRY(ast_child_get(parent, i - 1, &child), ERR_AST_CHILD_GET);
            if(child->id != AST_ID_LUT) continue;
            LutdAst *lut = child->branch.lut;
            INFO("got a lut");
            size_t i = 0;
            size_t j = 0;
            if(lutd_ast_find(lut, find, &i, &j)) break;
            INFO("found: %zu, %zu", i, j);
            *func = lut->buckets[i].items[j];
            return 0;
        }
        node = parent;
    }
    return 0;
error:
    return -1;
}

ErrDecl ast_func_get(Ast *node, Ast **func)
{
    if(!node) THROW(ERR_AST_POINTER);
    if(!func) THROW(ERR_AST_POINTER);
    //if(node->id != AST_ID_FUNC_CALL) THROW("expected %s, is %s", ast_id_str(AST_ID_FUNC_CALL), ast_id_str(node->id));
    //Ast *child = 0;
    //TRY(ast_child_get(node, 0, &child), ERR_AST_CHILD_GET);
    if(node->id != AST_ID_STR) THROW("expected %s, is %s", ast_id_str(AST_ID_STR), ast_id_str(node->id));
    if(node->arrl > 1) THROW("to be handled");
    Str *str = node->branch.str;
    TRY(ast_func_call(node, str, func), ERR_AST_FUNC_CALL);
    return 0;
error:
    return -1;
}

ErrDecl ast_func_call(Ast *node, Str *str, Ast **func)
{
    if(!node) THROW(ERR_AST_POINTER);
    if(!str) THROW(ERR_STR_POINTER);
    if(!func) THROW(ERR_AST_POINTER);
    Ast find = {
        .id = AST_ID_STR,
        .branch.str = str,
    };
    INFO("search for function: %.*s", (int)str->l, str->s);
    TRY(ast_static_find_lut(node, &find, func), ERR_AST_FIND_LUT);
    if(*func) {
        TRY(ast_parent_get(*func, func), ERR_AST_PARENT_GET);
        TRY(ast_child_get(*func, 1, func), ERR_AST_CHILD_GET);
    }
#if DEBUG_INSPECT
    ast_inspect_mark(*func, *func);
    platform_getchar();
#endif
    return 0;
error:
    return -1;
}

ErrDecl ast_get_scope_name(Ast *node, Str **str)
{
    if(!node) THROW(ERR_AST_POINTER);
    if(!str) THROW(ERR_STR_POINTER);
    //TRY(str_app(str, "TEST_SCOPE_NAME"), ERR_STR_APP);
    str_recycle(*str);
    while(node) {
#if 0
        size_t child_len = 0;
        TRY(ast_child_len(node, &child_len), ERR_AST_CHILD_LEN);
        printf("%zu children %p %s\n", child_len, node, ast_id_str(node->id));
        for(size_t i = 0; i < child_len; i++) {
            Ast *child = 0;
            TRY(ast_child_get(node, i, &child), ERR_AST_CHILD_GET);
            printf(" %s\n", ast_id_str(child->id));
            if(child->id != AST_ID_SCOPE_DEF) continue;
            printf(" ...\n");
            //ast_inspect_mark(child, child);
            TRY(ast_child_get(child, 0, &child), ERR_AST_CHILD_GET);
            if(child->id != AST_ID_STR) THROW(ERR_STR_POINTER);
            *str = child->branch.str;
        }
#endif

        INFO("%p %s", node, ast_id_str(node->id));
        if(node->id == AST_ID_SCOPE_DEF) {
            Ast *child = 0;
            size_t child_len = 0;
            TRY(ast_child_len(node, &child_len), ERR_AST_CHILD_LEN);
            if(child_len) {
                TRY(ast_child_get(node, 0, &child), ERR_AST_CHILD_GET);
                if(child->id != AST_ID_STR) THROW(ERR_STR_POINTER);
                *str = child->branch.str;
            }
            break;
        }
        TRY(ast_parent_get(node, &node), ERR_AST_PARENT_GET);
        INFO("%p %s (parent)", node, node?ast_id_str(node->id):"");
    }
    return 0;
error:
    return -1;
}

