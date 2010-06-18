/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#ifdef _XBT_WIN32
#include <win32_ucontext.h>
#else
#include <ucontext.h>
#endif

void child(void);

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
	return 1;
}
