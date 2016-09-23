/* Copyright (c) 2010, 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>

static int iterate = 10;
static int growsdown(int *x)
{
  int y = (x > &y);

  if (--iterate > 0)
    y = growsdown(&y);

  /* The stack sometimes changes at the 0th level. 
   * Original version did fail in this case, but I changed this around SimGrid 3.13 because of https://bugs.debian.org/814272
   * Every arch failed on that day :(
   */
  if (iterate != 0 && y != (x > &y)) {
    fprintf(stderr, "The stack changed its direction! (Iteration: %d. It was growing %s; &y=%p; &prevY=%p)\n",
	    (10-iterate), y?"down":"up", &y, x);
    exit(1);
  }
  return y;
}

int main(int argc, char *argv[])
{
  int x;
  printf("%s", growsdown(&x) ? "down" : "up");

  return 0;
}
