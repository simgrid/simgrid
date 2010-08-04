/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#if defined OSX
#define _XOPEN_SOURCE
#endif

#ifdef _XBT_WIN32
	#include "win32_ucontext.h"
	#include "win32_ucontext.c"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(TEST_sigstack) || defined(TEST_sigaltstack)
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#endif

#if defined(TEST_makecontext)
#ifdef _XBT_WIN32
	#include "win32_ucontext.h"
#else
	#include <ucontext.h>
#endif

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
	return 1;
}
