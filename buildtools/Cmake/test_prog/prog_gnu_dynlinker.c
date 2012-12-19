/* prog_gnu_dynlinker.c -- check that RTLD_NEXT is defined as in GNU linker */
/* Copyright (c) 2012. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define _GNU_SOURCE 1
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void * (*real_malloc) (size_t);

int main(void) {
   char *error;
   dlerror(); // clear any previous error
   real_malloc = (void * (*) (size_t)) dlsym(RTLD_NEXT, "malloc");
   error = dlerror();
   if (!error && real_malloc) {
      char *A = real_malloc(20);
      strcpy(A,"epic success");
      free(A);
      return 0; // SUCCESS
   } else {
      if (error)
	 printf("Error while checking for dlsym: %s\n",error);
      else
	 printf("dlsym did not return any error, but failed to find malloc()\n",error);
      return 1; // FAILED
   }
}
