#ifndef TIMERS_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "val.h"
#include "ast.h"
#include "next.h"

typedef struct Run Run; /* forward declaration */

typedef struct TimersLL {
    struct TimersLL *next;
    struct TimersLL *prev;
    size_t i0;
    size_t processed;
    size_t last;
    Val t;
} TimersLL;

typedef struct Timers {
    TimersLL *ll;
    size_t len;
    Val sum;
} Timers;

#define ERR_TIMERS_POINTER   "expected pointer to timer struct"
#define ERR_TIMERSNEW_POINTER "expected pointer to timer new struct"
/* public function error messages */
#define ERR_TIMERS_INIT      "failed to initialize timer"
#define ERR_TIMERS_INSERT    "failed to insert timer"
#define ERR_TIMERS_DELETE    "failed to delete timer"
#define ERR_TIMERS_FREE_ALL  "failed to free all timers"

/* utility */
//ErrDecl timers_init(TimersLL **timer, size_t i0, Val t);
ErrDecl timers_insert(Timers *timer, size_t i0, Val t);
//ErrDecl timers_delete(Timers **timer);
void timers_delete(Run *run, Timers *timer, TimersLL *del);
void timers_free_all(Run *run, Timers *timer);
void timers_print(Timers *timers);
void timers_print2(Timers *timer);
/* more advanced */
ErrDecl timers_from_new(Timers *timer, Ast *node);     /* create timers from an expr */

#define ERR_TIMERS_EXECUTE "failed executing timers"
ErrDecl timers_execute(Run *run, Scope *scope);

#define ERR_TIMERS_GET_DELTA "failed to get timers delta"
ErrDecl timers_get_delta(Run *run, Scope *scope, TriVal *t);
            
#define ERR_TIMERS_APPLY_DELTA "failed to apply delta to timers"
ErrDecl timers_apply_delta(Run *run, Timers *timer, Val diff);

int timers_get_func(Timers *timer, Ast **func);           /* ? */
int timers_get_next_func(Timers *timer, Ast **func);      /* ? */

#define TIMERS_H
#endif

