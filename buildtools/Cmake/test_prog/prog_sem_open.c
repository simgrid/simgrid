/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>

#ifndef SEM_FAILED
#define SEM_FAILED (-1)
#endif

int main(void) {   
#ifdef WIN32
	int s;
#else
	sem_t * s;
#endif
   s = sem_open("/0", O_CREAT, 0644, 10);
   if (s == SEM_FAILED){
//     printf("sem_open failed\n");
     return 1;
   }
//   printf("sem_open succeeded\n");   
   return 0;
}
