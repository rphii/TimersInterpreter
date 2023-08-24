#include <stddef.h>

#include "platform.h"
#include "file.h"
#include "str.h"
#include "colorprint.h"

static size_t platform_static_str_display_len(Str *str)
{
    if(!str) return 0;
    size_t actual_len = 0;
    for(size_t i = 0; i < str->l; i++) {
        char *s = &str->s[i];
        U8Point u8p = {0};
        TRY(str_to_u8_point(s, &u8p), ERR_STR_TO_U8_POINT);
        i += (size_t)(u8p.bytes - 1);
        actual_len++;
    }
error:
    return actual_len;
}

size_t platform_get_str_display_len(Str *str)
{
    if(!str) return 0;
    size_t actual_len = platform_static_str_display_len(str);
    return actual_len;
}


/******************************************************************************/
/* getch **********************************************************************/
/******************************************************************************/

#if defined(PLATFORM_WINDOWS)
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#endif

int platform_getch(void)
{
    printf(F("[press any key] ", IT FG_CY_B));
    fflush(stdout);

#if defined(PLATFORM_WINDOWS)

    return _getch();

#else

    int buf = 0;
    struct termios old = {0};
    fflush(stdout);
    if(tcgetattr(0, &old) < 0) {
        perror("tcsetattr()");
    }
    old.c_lflag &= (tcflag_t)~ICANON;
    old.c_lflag &= (tcflag_t)~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if(tcsetattr(0, TCSANOW, &old) < 0) {
        perror("tcsetattr ICANON");
    }
    if(read(0, &buf, 1) < 0) {
        perror("read()");
    }
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if(tcsetattr(0, TCSADRAIN, &old) < 0) {
        perror("tcsetattr ~ICANON");
    }
    printf("%c\n", buf);
    return buf;

#endif
}


/******************************************************************************/
/* getchar - not really platform depandant but in here for consistency ********/
/******************************************************************************/

int platform_getchar(void)
{
    printf(F("[press enter] ", IT FG_CY_B));
    fflush(stdout);
    fflush(stdin);
    return getchar();
}


/******************************************************************************/
/* clear - terminal clearing **************************************************/
/******************************************************************************/

void platform_clear(void)
{
    printf("\033[H\033[J\n");
}


/******************************************************************************/
/* utf8_enable - mainly windows ***********************************************/
/******************************************************************************/

int platform_utf8_enable(void)
{
    int err = 0;
#if defined(PLATFORM_WINDOWS) && !defined(COLORPRINT_DISABLE)
    err = system("chcp 65001 >nul");
    if(err) {
        THROW("failed enabling utf-8 codepage. Try compiling with -D"S(PLATFORM_DISABLE)"");
    }
clean:
    return err;
error: ERR_CLEAN;
#else
    return err;
#endif
}


/******************************************************************************/
/* seed ***********************************************************************/
/******************************************************************************/


#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
    #include <unistd.h>
    #include <time.h>
// https://stackoverflow.com/questions/322938/recommended-way-to-initialize-srand
// Robert Jenkins' 96 bit Mix Function
static unsigned long mix(unsigned long a, unsigned long b, unsigned long c)
{
    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return c;
}
#else
    #include <time.h>
#endif

unsigned long platform_seed(void)
{
    unsigned long seed = 0;
#if defined(PLATFORM_LINUX) || defined(PLATFORM_ANDROID)
    seed = mix((long unsigned)clock(), (long unsigned)time(0), (long unsigned)getpid());
#else
    seed = (unsigned long)time(0);
#endif
    return seed;
}
