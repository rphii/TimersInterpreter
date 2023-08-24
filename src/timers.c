#include "timers.h"
#include "ast.h"
#include "err.h"
#include "val.h"
#include "next.h"
#include "run.h"

int timers_init(TimersLL **timer, size_t i0, Val t)
{
    if(!timer) THROW(ERR_TIMERS_POINTER);
    if(*timer) THROW("wrongly initialized");
    *timer = malloc(sizeof(**timer));
    if(!*timer) THROW("failed to allocate memory");
    (*timer)->t = t;
    (*timer)->next = *timer;
    (*timer)->prev = *timer;
    (*timer)->i0 = i0;
    (*timer)->last = i0;
    (*timer)->processed = 0;
    //(*timer)->iE = i0;
    return 0;
error:
    return -1;
}

int timers_insert(Timers *timer, size_t i0, Val t)
{
    if(!timer) THROW(ERR_TIMERS_POINTER);
    if(timer->ll) {
        TimersLL *next = timer->ll->next;
        TimersLL *prev = timer->ll;
        INFO("init timer with " FMT_VAL " / next " FMT_VAL " / prev " FMT_VAL "", t, next->t, prev->t);
        timer->ll->next = 0;
        TRY(timers_init(&timer->ll->next, i0, t), ERR_TIMERS_INIT);
        //if(next && prev) {
        timer->ll->next->next = next;
        timer->ll->next->prev = prev;
        next->prev = timer->ll->next;
        prev->next = timer->ll->next;
    } else {
        TRY(timers_init(&timer->ll, i0, t), ERR_TIMERS_INIT);
    }
    timer->len++;
    timer->sum += t;
        //timer->prev->next = prev;
        //timer->next-> = next;
        //prev->next = timer;
        //next->prev = timer;
    //} else {
        /* first timer */
    //    timer->prev = timer;
    //    timer->next = timer;
    //}
    return 0;
error:
    return -1;
}

void timers_delete(Run *run, Timers *timer, TimersLL *del)
{
    if(!timer) return;
    if(!run) return;
    //if(!*timer) THROW(ERR_TIMERS_POINTER);
    if(!del) return;
    INFO(F("delete timer with t="FMT_VAL "", FG_MG), del->t);
    TimersLL *clear = del;
    TimersLL *next = clear->next;
    TimersLL *prev = clear->prev;
    run->last_destroyed.set = true;
    run->last_destroyed.val = clear->t;
    timer->sum -= clear->t;
    //if(next == del || prev == del) {
    //    THROW(ERR_UNREACHABLE);
    //}
    if(clear == timer->ll) {
        timer->ll = next;
    }
    if(next == clear) {
        timer->ll = 0; /* deleted last timer */
    } else {
        prev->next = next;
        next->prev = prev;
    }
    free(clear);
    timer->len--;
    return;
}

void timers_free_all(Run *run, Timers *timer)
{
    if(!timer) return;
    if(!run) return;
    INFO("free %p", timer);
    while(timer->ll) {
        timers_delete(run, timer, timer->ll);
    }
}

void timers_print(Timers *timer)
{
#if DEBUG_INSPECT
    printf("timers: ");
    if(!timer) return; //THROW(ERR_TIMERS_POINTER);
    Timers *root = timer;
    do {
        //printf("PRINT THE TIMERS %p\n", timer);
        printf("[" FMT_VAL " @ %zu] ", timer->t, timer->i0);
        timer = timer->next;
    } while(timer != root);
    printf("\n");
//error:
#endif
}

void timers_print2(Timers *timers)
{
#if 0
    if(!timers) return; //THROW(ERR_TIMERS_POINTER);
    printf("Timers: ");
    TimersLL *root = timers->ll;
    TimersLL *timer = timers->ll;
    do {
        //printf("PRINT THE TIMERS %p\n", timer);
        printf("[" FMT_VAL " @ %zu] ", timer->t, timer->i0);
        timer = timer->next;
    } while(timer != root);
    printf("\n");
    //getchar();
//error:
#endif
}

ErrDecl timers_execute(Run *run, Scope *scope)
{
    if(!run) THROW(ERR_RUN_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    //if(!*timers) THROW(ERR_TIMERS_POINTER);
    Timers *timers = &scope->timers;
    TimersLL *root = timers->ll;
    bool not_done = true;
    bool alive = false;
    size_t len = 0;
    TRY(ast_child_len(scope->origin, &len), ERR_AST_CHILD_LEN);
    do {
        TriVal diff = {0};
        TimersLL *called_timer = timers->ll;
        INFO(F("execute next functions ...", BG_GN));
        TRY(next_in_scope(run, scope, scope->origin, false, &diff), ERR_NEXT_IN_SCOPE); /* resume here;  */
        if(scope->randomize) {
            TRY(next_random(run, scope), ERR_NEXT_RANDOM);
            scope->randomize = false;
            break;
        }
        if(run->called_scope) {
            //printf("called a scope\n");
            break;
        }
        if(scope->destroy_timer) {
            timers_delete(run, timers, called_timer);
            scope->destroy_timer = false;
#if DEBUG
            timers_print2(&scope->timers);
#endif
            //printf("destroy scope timer\n");
            break;
        }
#define DEBUG_RUN 0
#if DEBUG_RUN
        printf("%s " FMT_VAL " %zu == %zu\n", diff.set?"diff is set":"diff is not set", diff.val, timers->ll->processed, len);
#endif
        if(!(diff.set && diff.val == 0 && timers->ll->processed == len)) {
            alive |= true;
        }
        alive=false;
        timers->ll = timers->ll->next;
        if(timers->ll == root) {
#if DEBUG_RUN
            printf("alive: %s\n", alive ? "true":"false");
#endif
            not_done = alive;
            run->apply_delta = !not_done;
            alive = false;
        }
    } while(not_done);
#if DEBUG_RUN
    printf("exit... apply_delta %s\n", run->apply_delta?"true":"false");
#endif
    return 0;
error: 
    return -1;

#if 0
    if(!run) THROW(ERR_RUN_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    Timers *timers = &scope->timers;
    TimersLL *root = timers->ll;
    bool not_done = true;
    do {
        TriVal diff = {0};
        TimersLL *called_timer = timers->ll;
        TRY(next_in_scope(run, scope, scope->origin, false, &diff), ERR_NEXT_IN_SCOPE); /* resume here;  */
        if(run->called_scope) {
            printf("called a scope\n");
            break;
        }
        if(scope->destroy_timer) {
            TRY(timers_delete(timers, called_timer), ERR_TIMERS_DELETE);
            scope->destroy_timer = false;
            printf("destroy scope timer\n");
            break;
        }
        if(timers->ll == root) {
        }
    } while(not_done);
    return 0;
error:
    return -1;
#endif
}

ErrDecl timers_get_delta(Run *run, Scope *scope, TriVal *t)
{
    int err = 0;
    if(!run) THROW(ERR_RUN_POINTER);
    if(!scope) THROW(ERR_SCOPE_POINTER);
    if(!t) THROW(ERR_SIZE_T_POINTER);
    Timers *timers = &scope->timers;
    TimersLL *root = timers->ll;
    VecVal vals = {0};
    do {
        TriVal diff = {0};
        diff.set = false;
        TRY(next_in_scope(run, scope, scope->origin, true, &diff), ERR_NEXT_IN_SCOPE);
        if(diff.set) {
            if(!t->set) {
                t->set = true;
                t->val = diff.val;
            } else if(diff.val < t->val) {
                t->val = diff.val;
            }
        }
        timers->ll = timers->ll->next;
    } while(timers->ll != root);
    /* clean up */
clean:
    vec_val_free(&vals);
    return err;
error: ERR_CLEAN;
}

#if 0
#define ERR_TIMERS_GET_VAL_ID  "failed to get val and id"
static int timers_static_get_val_id(TimersNewItem *tn, Ast *node, AstBranch *val, AstIDList *id)
{
    if(!tn) THROW(ERR_TIMERSNEW_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    if(!val) THROW(ERR_AST_BRANCH_POINTER);
    if(!id) THROW(ERR_AST_ID_POINTER);
    /* manage numbers */
    switch(node->id) {
        case AST_ID_INT: {
            val->num = node->branch.num;
            *id = AST_ID_INT;
        } break;
        case AST_ID_MATCH: {
            val->str = node->branch.str;
            *id = AST_ID_MATCH;
        } break;
        case AST_ID_EMPTY: {
            val->num = tn->t[TIMERS_NEW_RECENT].num;
            *id = tn->ok[TIMERS_NEW_RECENT];
        } break;
        default: break;//THROW(ERR_UNHANDLED_ID" %s", ast_id_str(node->id));
    }
    //printf("{%zu} ", *num);
#if 0
    /* manage id */
    Ast *parent = 0;
    AstIDList id = AST_ID_INT;
    if(!tn->id) {
        TRY(ast_parent_get(node, &parent), ERR_AST_PARENT_GET);
        if(parent->id == AST_ID_RANGE || parent->id == AST_ID_SEQUENCE || parent->id == AST_ID_TIMES) {
            id = parent->id;
        }
        tn->id = id;
    }
#endif
    return 0;
error:
    return -1;
}

#define ERR_TIMERS_TN_SET_RECENT    "failed to set new timer recent"
static int timers_static_tn_set_recent(TimersNewItem *tn, AstBranch *val, AstIDList id)
{
    if(!tn) THROW(ERR_TIMERSNEW_POINTER);
    if(!val) THROW(ERR_AST_BRANCH_POINTER);
    INFO("set recent to %zu", val->num);
    tn->t[TIMERS_NEW_RECENT].num = val->num;
    tn->ok[TIMERS_NEW_RECENT] = id;
    return 0;
error:
    return -1;
}

#define ERR_TIMERS_TN_SET_RANGE   "failed to set new timer range"
static int timers_static_tn_set_range(TimersNewItem *tn, AstBranch *val, AstIDList id)
{
    if(!tn) THROW(ERR_TIMERSNEW_POINTER);
    if(!val) THROW(ERR_AST_BRANCH_POINTER);
    size_t *t_a = &tn->t[TIMERS_NEW_A].num;
    size_t *t_b = &tn->t[TIMERS_NEW_B].num;
    size_t *t_c = &tn->t[TIMERS_NEW_C].num;
    AstIDList *id_a = &tn->ok[TIMERS_NEW_A];
    AstIDList *id_b = &tn->ok[TIMERS_NEW_B];
    AstIDList *id_c = &tn->ok[TIMERS_NEW_C];
    /* TODO maybe remove all assignments of id's since we can guarantee it's an int... */
    /* TODO handle assignments of match things to recent items */
    if(!tn->id || tn->id == AST_ID_INT) tn->id = AST_ID_RANGE;
    if(id != AST_ID_INT) THROW("make sure this is an INT and not %s, otherwise bad shit happens", ast_id_str(id));
    if(tn->id == AST_ID_RANGE) {
        if(tn->ok[TIMERS_NEW_RECENT]) { /* TODO why did I add this? */ }
        if(!*id_a) {
            INFO("> set t_a : %zu", val->num);
            *t_a = val->num;
            *id_a = id;
        } else if(!*id_b) {
            INFO("> set t_b : %zu", val->num);
            *t_b = val->num;
            *id_b = id;
        } else if(!*id_c) {
            INFO("> set t_c : %zu", val->num);
            *t_c = val->num;
            *id_c = id;
        } else {
            /* sort b and c, set c = this */
            if(*t_c > *t_b) *t_b = *t_c;
            /* c = this */
            INFO("> set t_c : %zu", val->num);
            *t_c = val->num;
            *id_c = id;
        }
    }
    return 0;
error:
    return -1;
}

#define ERR_TIMERS_TN_SET_SEQUENCE  "failed to set new timer sequence"
static int timers_static_tn_set_sequence(TimersNewItem *tn, AstBranch *val, AstIDList id)
{
    if(!tn) THROW(ERR_TIMERSNEW_POINTER);
    size_t *t_a = &tn->t[TIMERS_NEW_A].num;
    size_t *t_b = &tn->t[TIMERS_NEW_B].num;
    AstIDList *id_a = &tn->ok[TIMERS_NEW_A];
    AstIDList *id_b = &tn->ok[TIMERS_NEW_B];
    /* TODO maybe remove all assignments of id's since we can guarantee it's an int... */
    /* TODO handle assignments of match things to recent items */
    if(!tn->id) tn->id = AST_ID_SEQUENCE;
    if(id != AST_ID_INT) THROW("make sure this is an INT and not %s, otherwise bad shit happens", ast_id_str(id));
    if(tn->id == AST_ID_SEQUENCE) {
        if(tn->ok[TIMERS_NEW_RECENT]){
        }
        if(!*id_a) {
            INFO("> set t_a : %zu", val->num);
            *t_a = val->num;
            *id_a = id;
        } else if(!*id_b) {
            INFO("> set t_b : %zu", val->num);
            *t_b = val->num;
            *id_b = id;
        } else {
        }
    }
    return 0;
error:
    return -1;
}

#define ERR_TIMERS_TN_SET_TIMES   "failed to set new timer times"
static int timers_static_tn_set_times(TimersNewItem *tn, AstBranch *val, AstIDList id)
{
    if(!tn) THROW(ERR_TIMERSNEW_POINTER);
    if(!val) THROW(ERR_AST_BRANCH_POINTER);
    size_t *t_a = &tn->t[TIMERS_NEW_A].num;
    size_t *t_b = &tn->t[TIMERS_NEW_B].num;
    size_t *t_c = &tn->t[TIMERS_NEW_C].num;
    AstIDList *id_a = &tn->ok[TIMERS_NEW_A];
    AstIDList *id_b = &tn->ok[TIMERS_NEW_B];
    AstIDList *id_c = &tn->ok[TIMERS_NEW_C];
    /* TODO maybe remove all assignments of id's since we can guarantee it's an int... */
    /* TODO handle assignments of match things to recent items */
    if(!tn->id) tn->id = AST_ID_INT;
    if(id != AST_ID_INT) THROW("make sure this is an INT and not %s, otherwise bad shit happens", ast_id_str(id));
    switch(tn->id) {
        case AST_ID_RANGE: {
            if(*id_c) *t_c *= val->num;
            else if(*id_b) *t_b *= val->num;
            else THROW(ERR_UNREACHABLE);
        } break;
        case AST_ID_SEQUENCE: {
        } break;
        case AST_ID_INT: {
            if(*id_a) *t_a *= val->num;
            else {
                *t_a = val->num;
                *id_a = id;
            }
        } break;
        default: break;
    }
    return 0;
error:
    return -1;
}

#define ERR_TIMERS_TN_SET_NUM   "failed to set new timer num"
static int timers_static_tn_set_num(TimersNewItem *tn, AstBranch *val, AstIDList id)
{
    if(!tn) THROW(ERR_TIMERSNEW_POINTER);
    if(!val) THROW(ERR_AST_BRANCH_POINTER);
    size_t *t_a = &tn->t[TIMERS_NEW_A].num;
    size_t *t_b = &tn->t[TIMERS_NEW_B].num;
    AstIDList *id_a = &tn->ok[TIMERS_NEW_A];
    AstIDList *id_b = &tn->ok[TIMERS_NEW_B];
    /* TODO maybe remove all assignments of id's since we can guarantee it's an int... */
    /* TODO handle assignments of match things to recent items */
    printf("THIS IS THE ID? %s\n", ast_id_str(id));
    if(!tn->id) tn->id = AST_ID_INT;
    else THROW(ERR_UNREACHABLE);
    *t_a = val->num;
    return 0;
error:
    return -1;
}

void timers_new_print(TimersNewItem *tn)
{
#if DEBUG
    if(!tn) return;
    if(!tn->id) return;
    printf(F("[", FG_GN_B));
    printf(F("%s %s+%s, ", FG_GN_B), ast_id_str(tn->id), ast_id_str(tn->mode), tn->mode_xor ? "xor" : "or");
    if(tn->ok[TIMERS_NEW_A] == AST_ID_INT) printf(F("a=%zu", FG_GN_B), tn->t[TIMERS_NEW_A].num);
    else if(tn->ok[TIMERS_NEW_A] == AST_ID_MATCH) printf(F("a=%s", FG_GN_B), tn->t[TIMERS_NEW_A].str->s);
    if(tn->ok[TIMERS_NEW_B] == AST_ID_INT) printf(F(", b=%zu", FG_GN_B), tn->t[TIMERS_NEW_B].num);
    if(tn->ok[TIMERS_NEW_C] == AST_ID_INT) printf(F(", c=%zu", FG_GN_B), tn->t[TIMERS_NEW_C].num);
    printf(F("] ", FG_GN_B));
#endif
}

#define SWAP(a, b)  do { \
    (a) ^= (b); \
    (b) ^= (a); \
    (a) ^= (b); } while(0)

static int timers_static_tn_sort_range(TimersNewItem *tn)
{
    if(!tn) THROW(ERR_TIMERSNEW_POINTER);
    size_t *t_a = &tn->t[TIMERS_NEW_A].num;
    size_t *t_b = &tn->t[TIMERS_NEW_B].num;
    size_t *t_c = &tn->t[TIMERS_NEW_C].num;
    AstIDList *id_c = &tn->ok[TIMERS_NEW_C];
    if(tn->id == AST_ID_RANGE) {
        if(*id_c && *t_c < *t_b) SWAP(*t_c, *t_b);
        if(*t_b < *t_a) SWAP(*t_b, *t_a);
        if(*id_c && *t_c < *t_a) SWAP(*t_c, *t_a);
        if(*id_c && *t_c > *t_b) *t_b = *t_c;
        *id_c = AST_ID_NONE;
    }
    return 0;
error:
    return -1;
}

#define ERR_TIMERS_TN_CLEAR     "failed to clear new timer"
static int timers_static_tn_clear(TimersNewItem *tn)
{
    if(!tn) THROW(ERR_TIMERSNEW_POINTER);
    memset(&tn, 0, sizeof(tn));
    return 0;
error:
    return -1;
}

int timers_from_new(Timers *timer, Ast *node)
{
    if(!timer) THROW(ERR_TIMERS_POINTER);
    if(!node) THROW(ERR_AST_POINTER);
    /* make sure the context is right */
    if(node->id != AST_ID_NEW) THROW("decide if this is an error or not");
    size_t child_len = 0;
    TRY(ast_child_len(node, &child_len), ERR_AST_CHILD_LEN);
    TimersNewItem tn = {0};
    for(size_t i = 0; i < child_len; i++) {
        Ast *child = 0;
        Ast *parent = 0;
        Ast *pprev = 0;
        ssize_t layer = 0;
        //ssize_t prev_layer = 0;
        TRY(ast_child_get(node, i, &child), ERR_AST_CHILD_GET);
        if(child->id != AST_ID_EXPRESSION) THROW(ERR_UNREACHABLE);
        TRY(ast_parent_get(child, &pprev), ERR_AST_PARENT_GET);
        TRY(ast_next(&child, &layer), ERR_AST_NEXT);
        TRY(ast_parent_get(child, &parent), ERR_AST_PARENT_GET);
        INFO("--- new child (push timers) ! ---");
        TRY(timers_static_tn_clear(&tn), ERR_TIMERS_TN_CLEAR);
        while(layer > 0) {
            TRY(ast_parent_get(child, &parent), ERR_AST_PARENT_GET);
            /* keep above first */
            if(layer == 1) {
                if(child->id == AST_ID_OR) {
                    tn.mode_xor = false;
                    goto next;
                } else if(child->id == AST_ID_XOR) {
                    tn.mode_xor = true;
                    goto next;
                }
            }
            /* check to see if we're actually done for this iteration */
            if(child->id == AST_ID_TERM) {
                INFO("- push previous tn -");
                timers_static_tn_sort_range(&tn);
                timers_new_print(&tn);
                printf("\n");
                goto next;
            }
            /* determine the mode - determined by the very first item */
            if(tn.mode == AST_ID_NONE) {
                if(parent->id == AST_ID_TERM) {
                    if(child->id == AST_ID_MATCH) {
                        tn.mode = AST_ID_MATCH;
                        INFO("> set mode to MATCH");
                    } else {
                        tn.mode = AST_ID_INT;
                        INFO("> set mode to INT");
                    }
                }
            }
            if(!ast_has_child(child)) {
                AstBranch val = {0};
                AstIDList id = AST_ID_NONE;
                TRY(timers_static_get_val_id(&tn, child, &val, &id), ERR_TIMERS_GET_VAL_ID);
                //TRY(timers_static_tn_set_recent(&tn, &val, id), ERR_TIMERS_TN_SET_RECENT);
                if(parent->id == AST_ID_RANGE) {
                    INFO("> set range modifier");
                    TRY(timers_static_tn_set_range(&tn, &val, id), ERR_TIMERS_TN_SET_RANGE);
                } else if(parent->id == AST_ID_SEQUENCE) {
                    INFO("> set sequence modifier");
                    TRY(timers_static_tn_set_sequence(&tn, &val, id), ERR_TIMERS_TN_SET_SEQUENCE);
                } else if(parent->id == AST_ID_TIMES) {
                    INFO("> set times modifier");
                    TRY(timers_static_tn_set_times(&tn, &val, id), ERR_TIMERS_TN_SET_TIMES);
                } else {
                    INFO("> set number");
                    TRY(timers_static_tn_set_num(&tn, &val, id), ERR_TIMERS_TN_SET_NUM);
                }
                TRY(timers_static_tn_set_recent(&tn, &val, id), ERR_TIMERS_TN_SET_RECENT);
                goto next;
            }
            INFO("%s", ast_id_str(child->id));
#if 0
            //if(parent->id != pprev->id && !ast_has_child(child)) {
            if(parent->id == AST_ID_EXPRESSION || parent->id == AST_ID_TERM) {
                do {
                    if(pprev->id == AST_ID_SEQUENCE && parent->id == AST_ID_TIMES) break;
                    if(pprev->id == AST_ID_TIMES && parent->id == AST_ID_TIMES) break;
                    if(parent->id == AST_ID_TIMES) break;
                    if(!tn.id) break;
                    timers_new_print(&tn);
                    tn.id = AST_ID_NONE;
                    tn.id_c = AST_ID_NONE;
                    tn.or_xor = false;
                    tn.t[TIMERS_NEW_A] = 0;
                    tn.t[TIMERS_NEW_B] = 0;
                    tn.t[TIMERS_NEW_C] = 0;
                    tn.ok[TIMERS_NEW_A] = false;
                    tn.ok[TIMERS_NEW_B] = false;
                    tn.ok[TIMERS_NEW_C] = false;
                } while(0);
            }
            /* TODO handle last range, when empty... */
            /* TODO handle match... introduce pointer? */
            /* manage initializing of t_a, t_b */
            if(child->id == AST_ID_OR || child->id == AST_ID_XOR) {
                printf("{clear tn} ");
                memset(&tn, 0, sizeof(tn));
                tn.or_xor = (child->id == AST_ID_XOR);;
            }
            if(!tn.id && tn.ok[TIMERS_NEW_RECENT]) {
                tn.id = parent->id;
                tn.t[TIMERS_NEW_A] = tn.t[TIMERS_NEW_RECENT];
                tn.ok[TIMERS_NEW_A] = true;
            }
            if(!tn.id && !ast_has_child(child)) {
                size_t num = 0;
                TRY(timers_static_get_num(&tn, child, &num), ERR_TIMERS_GET_NUM);
                tn.t[TIMERS_NEW_A] = num;
                tn.ok[TIMERS_NEW_A] = true;
                tn.t[TIMERS_NEW_RECENT] = num;
                tn.ok[TIMERS_NEW_RECENT] = true;
                //tn.id = parent->id;
            } else if(tn.id && !ast_has_child(child)) {
                size_t num = 0;
                TRY(timers_static_get_num(&tn, child, &num), ERR_TIMERS_GET_NUM);
                tn.t[TIMERS_NEW_RECENT] = num;
                tn.ok[TIMERS_NEW_RECENT] = true;
                if(pprev->id == AST_ID_SEQUENCE && parent->id == AST_ID_TIMES) {
                    tn.t[TIMERS_NEW_C] = num;
                    tn.ok[TIMERS_NEW_C] = true;
                    tn.id_c = AST_ID_TIMES;
                } else if(pprev->id == AST_ID_SEQUENCE && parent->id == AST_ID_RANGE) {
                    tn.t[TIMERS_NEW_C] = num;
                    tn.ok[TIMERS_NEW_C] = true;
                    tn.id_c = AST_ID_RANGE;
                } else if(parent->id == AST_ID_TIMES) {
                    if(tn.ok[TIMERS_NEW_C]) tn.t[TIMERS_NEW_C] *= num;
                    else if(tn.ok[TIMERS_NEW_B]) tn.t[TIMERS_NEW_B] *= num;
                    //else if(tn.ok[TIMERS_NEW_A]) tn.t[TIMERS_NEW_A] = num;
                } else if(!ast_has_child(child)) {
                    if(!tn.ok[TIMERS_NEW_B]) {
                        if(num < tn.t[TIMERS_NEW_A]) {
                            size_t swp = tn.t[TIMERS_NEW_A];
                            tn.t[TIMERS_NEW_A] = num;
                            tn.t[TIMERS_NEW_B] = swp;
                        } else {
                            tn.t[TIMERS_NEW_B] = num;
                        }
                    } else {
                        if(num > tn.t[TIMERS_NEW_B]) {
                            tn.t[TIMERS_NEW_B] = num;
                        } else if(num < tn.t[TIMERS_NEW_A]) {
                            tn.t[TIMERS_NEW_A] = num;
                        }
                    }
                    tn.ok[TIMERS_NEW_B] = true;
                }
            }
            printf("%s%c%s", ast_id_str(parent->id), tn.or_xor ? ':' : '|', ast_id_str(child->id));
            printf(", ");
            //}
            if(child->id == AST_ID_INT) {
                printf("\b\b [%zu], ", child->branch.num);
            } else if(child->id == AST_ID_EMPTY) {
                printf("\b\b [], ");
            }
            /* manage shifting of t_a, t_b */
            //TRY(timers_static_shift_abc(&tn, parent, pprev), ERR_TIMERS_SHIFT_ABC);
#endif
            /* keep last */
next:
            //prev_layer = layer;
            pprev = parent;
            TRY(ast_next(&child, &layer), ERR_AST_NEXT);
        }
        INFO("- push last tn -");
        timers_static_tn_sort_range(&tn);
        timers_new_print(&tn);
        printf("\n");
    }
    INFO("--- done ---");

    return 0;
error:
    return -1;
}
#endif

int timers_from_new(Timers *timer, Ast *node)
{
    return 0;
}

ErrDecl timers_apply_delta(Run *run, Timers *timers, Val diff)
{
    if(!run) THROW(ERR_RUN_POINTER);
    if(!timers) THROW(ERR_TIMERS_POINTER);
    //if(!diff) THROW(ERR_SIZE_T_POINTER);
    run->last_diff = diff;
    run->inc_timers++;
    timers->sum = 0;
    TimersLL *root = timers->ll;
    TimersLL *timer = timers->ll;
    //printf("diff "FMT_VAL "\n", diff);
    do {
        timer->t += diff;
        timers->sum += timer->t;
        timer->t &= VAL_MAX; /* danger is my second name */
        timer->i0 = timer->last; /* reset last */
        timer->processed = 0;
        timer = timer->next;
    } while(timer != root);
    return 0;
error:
    return -1;
}

