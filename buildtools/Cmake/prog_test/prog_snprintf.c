/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>

int main(void)
{
    char bufs[5] = { 'x', 'x', 'x', '\0', '\0' };
    char bufd[5] = { 'x', 'x', 'x', '\0', '\0' };
    int i;
    i = snprintf (bufs, 2, "%s", "111");
    if (strcmp (bufs, "1")) exit (1);
    if (i != 3) exit (1);
    i = snprintf (bufd, 2, "%d", 111);
    if (strcmp (bufd, "1")) exit (1);
    if (i != 3) exit (1);
    exit(0);
}
