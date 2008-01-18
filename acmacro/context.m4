dnl AC_CHECK_UCONTEXT: Check whether ucontext are working

dnl Most of the code is stolen from the GNU pth autoconf macros by
dnl Ralf S. Engelschall. 
dnl # ``"Reuse an expert's code" is the right
dnl #   advice for most people. But it's a useless
dnl #   advice for the experts writing the code
dnl #   in the first place.'
dnl #               -- Dan J. Bernstein
dnl
dnl OK, you're definitely the expert on this point... :)

dnl ##
dnl ##  Display Configuration Headers
dnl ##
dnl ##  configure.ac:
dnl ##    AC_MSG_PART(<text>)
dnl ##

m4_define(AC_MSG_PART,[dnl
if test ".$enable_subdir" != .yes; then
    AC_MSG_RESULT()
    AC_MSG_RESULT(${TB}$1:${TN})
fi
])dnl

dnl ##
dnl ##  Display a message under --verbose
dnl ##
dnl ##  configure.ac:
dnl ##    AC_MSG_VERBOSE(<text>)
dnl ##

m4_define(AC_MSG_VERBOSE,[dnl
if test ".$verbose" = .yes; then
    AC_MSG_RESULT([  $1])
fi
])

dnl ##
dnl ##  Do not display message for a command
dnl ##
dnl ##  configure.ac:
dnl ##    AC_MSG_SILENT(...)
dnl ##

m4_define(AC_FD_TMP, 9)
m4_define(AC_MSG_SILENT,[dnl
exec AC_FD_TMP>&AC_FD_MSG AC_FD_MSG>/dev/null
$1
exec AC_FD_MSG>&AC_FD_TMP AC_FD_TMP>&-
])

dnl ##
dnl ##  Check for direction of stack growth
dnl ##
dnl ##  configure.ac:
dnl ##    AC_CHECK_STACKGROWTH(<define>)
dnl ##  acconfig.h:
dnl ##    #undef <define>
dnl ##  source.c:
dnl ##    #include "config.h"
dnl ##    #if <define> < 0
dnl ##        ...stack grow down...
dnl ##    #else
dnl ##        ...stack grow up...
dnl ##    #endif
dnl ##

AC_DEFUN([AC_CHECK_STACKGROWTH],[dnl
AC_MSG_CHECKING(for direction of stack growth)
AC_CACHE_VAL(ac_cv_check_stackgrowth, [
cross_compiling=no
AC_TRY_RUN(
changequote(<<, >>)dnl
<<
#include <stdio.h>
#include <stdlib.h>
static int iterate = 10;
static int growsdown(int *x)
{
    auto int y;
    y = (x > &y);
    if (--iterate > 0)
        y = growsdown(&y);
    if (y != (x > &y))
        exit(1);
    return y;
}
int main(int argc, char *argv[])
{
    FILE *f;
    auto int x;
    if ((f = fopen("conftestval", "w")) == NULL)
        exit(1);
    fprintf(f, "%s\n", growsdown(&x) ? "down" : "up");;
    fclose(f);
    exit(0);
}
>>
changequote([, ])dnl
,
ac_cv_check_stackgrowth=`cat conftestval`,
ac_cv_check_stackgrowth=down,
ac_cv_check_stackgrowth=down
)dnl
])dnl
AC_MSG_RESULT([$ac_cv_check_stackgrowth])
if test ".$ac_cv_check_stackgrowth" = ".down"; then
    val="-1"
else
    val="+1"
fi
AC_DEFINE_UNQUOTED($1, $val, [define for stack growth])
])

dnl ##
dnl ##  Check whether SVR4/SUSv2 makecontext(2), swapcontext(2) and
dnl ##  friends can be used for user-space context switching
dnl ##
dnl ##  configure.ac:
dnl ##     AC_CHECK_MCSC(<success-action>, <failure-action>)
dnl ##

AC_DEFUN([AC_CHECK_MCSC], [
AC_MSG_CHECKING(for usable SVR4/SUSv2 makecontext(2)/swapcontext(2))
AC_CACHE_VAL(ac_cv_check_mcsc, [
AC_TRY_RUN([

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>

ucontext_t uc_child;
ucontext_t uc_main;

void child(void)
{
    if (swapcontext(&uc_child, &uc_main) != 0)
        exit(2);
}

int main(int argc, char *argv[])
{
    FILE *fp;
    void *stack;

    /* the default is that it fails */
    if ((fp = fopen("conftestval", "w")) == NULL)
        exit(3);
    fprintf(fp, "no\n");
    fclose(fp);

    /* configure a child user-space context */
    if ((stack = malloc(64*1024)) == NULL)
        exit(4);
    if (getcontext(&uc_child) != 0)
        exit(5);
    uc_child.uc_link = NULL;
    uc_child.uc_stack.ss_sp = (char *)stack+(32*1024);
    uc_child.uc_stack.ss_size = 32*1024;
    uc_child.uc_stack.ss_flags = 0;
    makecontext(&uc_child, child, 0);

    /* switch into the user context */
    if (swapcontext(&uc_main, &uc_child) != 0)
        exit(6);

    /* Fine, child came home */
    if ((fp = fopen("conftestval", "w")) == NULL)
        exit(7);
    fprintf(fp, "yes\n");
    fclose(fp);

    /* die successfully */
    exit(0);
}
],
ac_cv_check_mcsc=`cat conftestval`,
ac_cv_check_mcsc=no,
ac_cv_check_mcsc=no
)dnl
])dnl
AC_MSG_RESULT([$ac_cv_check_mcsc])
if test ".$ac_cv_check_mcsc" = .yes; then
    ifelse([$1], , :, [$1])
else
    ifelse([$2], , :, [$2])
fi
])dnl

dnl ##
dnl ##  Check how stacks have to be setup for the functions
dnl ##  sigstack(2), sigaltstack(2) and makecontext(2).
dnl ##
dnl ##  configure.ac:
dnl ##    AC_CHECK_STACKSETUP(sigstack|sigaltstack|makecontext, <macro-addr>, <macro-size>)
dnl ##  acconfig.h:
dnl ##    #undef HAVE_{SIGSTACK|SIGALTSTACK|MAKECONTEXT}
dnl ##    #undef HAVE_STACK_T
dnl ##  header.h.in:
dnl ##    @<macro-addr>@
dnl ##    @<macro-size>@
dnl ##  source.c:
dnl ##    #include "header.h"
dnl ##    xxx.sp_ss   = <macro-addr>(skaddr, sksize);
dnl ##    xxx.sp_size = <macro-size>(skaddr, sksize);
dnl ##

AC_DEFUN([AC_CHECK_STACKSETUP],[dnl
dnl #   check for consistent usage
ifelse($1,[sigstack],,[
ifelse($1,[sigaltstack],,[
ifelse($1,[makecontext],,[
errprint(__file__:__line__: [AC_CHECK_STACKSETUP: only sigstack, sigaltstack and makecontext supported
])])])])
dnl #   we require the C compiler and the standard headers
AC_REQUIRE([AC_HEADER_STDC])dnl
dnl #   we at least require the function to check
AC_CHECK_FUNCS($1)
dnl #   sigaltstack on some platforms uses stack_t instead of struct sigaltstack
ifelse($1, sigaltstack, [
    AC_ONCE(stacksetup_stack_t, [
        AC_CHECK_TYPEDEF(stack_t, signal.h)
    ])
])
dnl #   display processing header
AC_MSG_CHECKING(for stack setup via $1)
dnl #   but cache the whole results
AC_CACHE_VAL(ac_cv_stacksetup_$1,[
if test ".$ac_cv_func_$1" = .no; then
    dnl #   no need to check anything when function is already missing
    ac_cv_stacksetup_$1="N.A.:/*N.A.*/,/*N.A.*/"
else
    dnl #   setup compile environment
    OCFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -DTEST_$1"
    cross_compiling=no
    dnl #   compile and run the test program
    AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(TEST_sigstack) || defined(TEST_sigaltstack)
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#endif
#if defined(TEST_makecontext)
#include <ucontext.h>
#endif
union alltypes {
    long   l;
    double d;
    void  *vp;
    void (*fp)(void);
    char  *cp;
};
static volatile char *handler_addr = (char *)0xDEAD;
#if defined(TEST_sigstack) || defined(TEST_sigaltstack)
static volatile int handler_done = 0;
void handler(int sig)
{
    char garbage[1024];
    int i;
    auto int dummy;
    for (i = 0; i < 1024; i++)
        garbage[i] = 'X';
    handler_addr = (char *)&dummy;
    handler_done = 1;
    return;
}
#endif
#if defined(TEST_makecontext)
static ucontext_t uc_handler;
static ucontext_t uc_main;
void handler(void)
{
    char garbage[1024];
    int i;
    auto int dummy;
    for (i = 0; i < 1024; i++)
        garbage[i] = 'X';
    handler_addr = (char *)&dummy;
    swapcontext(&uc_handler, &uc_main);
    return;
}
#endif
int main(int argc, char *argv[])
{
    FILE *f;
    char *skaddr;
    char *skbuf;
    int sksize;
    char result[1024];
    int i;
    sksize = 32768;
    skbuf = (char *)malloc(sksize*2+2*sizeof(union alltypes));
    if (skbuf == NULL)
        exit(1);
    for (i = 0; i < sksize*2+2*sizeof(union alltypes); i++)
        skbuf[i] = 'A';
    skaddr = skbuf+sizeof(union alltypes);
#if defined(TEST_sigstack) || defined(TEST_sigaltstack)
    {
        struct sigaction sa;
#if defined(TEST_sigstack)
        struct sigstack ss;
#elif defined(TEST_sigaltstack) && defined(HAVE_STACK_T)
        stack_t ss;
#else
        struct sigaltstack ss;
#endif
#if defined(TEST_sigstack)
        ss.ss_sp      = (void *)(skaddr + sksize);
        ss.ss_onstack = 0;
        if (sigstack(&ss, NULL) < 0)
            exit(1);
#elif defined(TEST_sigaltstack)
        ss.ss_sp    = (void *)(skaddr + sksize);
        ss.ss_size  = sksize;
        ss.ss_flags = 0;
        if (sigaltstack(&ss, NULL) < 0)
            exit(1);
#endif
        memset((void *)&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = handler;
        sa.sa_flags = SA_ONSTACK;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGUSR1, &sa, NULL);
        kill(getpid(), SIGUSR1);
        while (!handler_done)
            /*nop*/;
    }
#endif
#if defined(TEST_makecontext)
    {
        if (getcontext(&uc_handler) != 0)
            exit(1);
        uc_handler.uc_link = NULL;
        uc_handler.uc_stack.ss_sp    = (void *)(skaddr + sksize);
        uc_handler.uc_stack.ss_size  = sksize;
        uc_handler.uc_stack.ss_flags = 0;
        makecontext(&uc_handler, handler, 0);
        swapcontext(&uc_main, &uc_handler);
    }
#endif
    if (handler_addr == (char *)0xDEAD)
        exit(1);
    if (handler_addr < skaddr+sksize) {
        /* stack was placed into lower area */
        if (*(skaddr+sksize) != 'A')
             sprintf(result, "(skaddr)+(sksize)-%d,(sksize)-%d",
                     sizeof(union alltypes), sizeof(union alltypes));
        else
             strcpy(result, "(skaddr)+(sksize),(sksize)");
    }
    else {
        /* stack was placed into higher area */
        if (*(skaddr+sksize*2) != 'A')
            sprintf(result, "(skaddr),(sksize)-%d", sizeof(union alltypes));
        else
            strcpy(result, "(skaddr),(sksize)");
    }
    if ((f = fopen("conftestval", "w")) == NULL)
        exit(1);
    fprintf(f, "%s\n", result);
    fclose(f);
    exit(0);
}
],[
dnl #   test successully passed
ac_cv_stacksetup_$1=`cat conftestval`
ac_cv_stacksetup_$1="ok:$ac_cv_stacksetup_$1"
],[
dnl #   test failed
ac_cv_stacksetup_$1='guessed:(skaddr),(sksize)'
],[
dnl #   cross-platform => failed
ac_cv_stacksetup_$1='guessed:(skaddr),(sksize)'
])
dnl #   restore original compile environment
CFLAGS="$OCFLAGS"
])dnl
fi
dnl #   extract result ingredients of single cached result value
type=`echo $ac_cv_stacksetup_$1 | sed -e 's;:.*$;;'`
addr=`echo $ac_cv_stacksetup_$1 | sed -e 's;^.*:;;' -e 's;,.*$;;'`
size=`echo $ac_cv_stacksetup_$1 | sed -e 's;^.*:;;' -e 's;^.*,;;'`
dnl #   export result ingredients
$2="#define $2(skaddr,sksize) ($addr)"
$3="#define $3(skaddr,sksize) ($size)"
AC_SUBST($2)dnl
AC_SUBST($3)dnl
dnl #   display result indicator
AC_MSG_RESULT([$type])
dnl #   display results in detail
AC_MSG_VERBOSE([$]$2)
AC_MSG_VERBOSE([$]$3)
])
