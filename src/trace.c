#include <stdio.h>
#include <stdlib.h>

#include "platform.h"

#ifdef PLATFORM_LINUX

#include <execinfo.h>

/* https://www.gnu.org/software/libc/manual/html_node/Backtraces.html */
/* Obtain a backtrace and print it to stdout. */
void trace_print(void)
{
    void *array[10];
    char **strings = 0;
    int size, i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);
    printf("\n");
    if(strings != NULL) {
        printf("Obtained %d stack frames.\n", size);
        for (i = 0; i < size; i++) printf ("%s\n", strings[i]);
    }

    free(strings);
}

#else

void trace_print(void)
{
}

#endif
