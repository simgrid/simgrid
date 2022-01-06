/* Copyright (c) 2016-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/time.h>
#if _POSIX_TIMERS > 0
#include <time.h>
#endif

// Test if we correctly intercept gettimeofday and clock_gettime, and sleeps

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  // gettimeofday - First test that two consecutive calls will return the same
  // time, as computing power is set to -1

  struct timeval tv1;
  struct timeval tv2;
  gettimeofday(&tv1, NULL);
  gettimeofday(&tv2, NULL);
  if ((tv1.tv_sec != tv2.tv_sec) || (tv1.tv_usec != tv2.tv_usec))
    printf("Error, two consecutive calls to gettimeofday did not return same time (with cpu_threshold set to 0)\n");

  // sleep one 1 second
  gettimeofday(&tv1, NULL);
  sleep(1);
  gettimeofday(&tv2, NULL);
  long res = (tv2.tv_sec * 1000000 + tv2.tv_usec) - (tv1.tv_sec * 1000000 + tv1.tv_usec);
  if (res < 999998 || res > 1000002)
    printf("Error, sleep(1) did not exactly slept 1s\n");

  // usleep 100 us
  gettimeofday(&tv1, NULL);
  usleep(100);
  gettimeofday(&tv2, NULL);
  res = (tv2.tv_sec * 1000000 + tv2.tv_usec) - (tv1.tv_sec * 1000000 + tv1.tv_usec);
  if (res < 98 || res > 102)
    printf("Error, usleep did not really sleep 100us, but %ld\n", res);


  // clock_gettime, only if available
#if _POSIX_TIMERS > 0
  struct timespec tp1;
  struct timespec tp2;
  struct timespec tpsleep;
  clock_gettime(CLOCK_REALTIME, &tp1);
  clock_gettime(CLOCK_REALTIME, &tp2);
  if (tp1.tv_sec != tp2.tv_sec || tp1.tv_nsec != tp2.tv_nsec)
    printf("Error, two consecutive calls to clock_gettime did not return same time (with running power to 0)\n");

  // nanosleep for 100ns
  clock_gettime(CLOCK_REALTIME, &tp1);
  tpsleep.tv_sec  = 0;
  tpsleep.tv_nsec = 100;
  nanosleep(&tpsleep, NULL);
  clock_gettime(CLOCK_REALTIME, &tp2);
  res = (tp2.tv_sec * 1000000000 + tp2.tv_nsec) - (tp1.tv_sec * 1000000000 + tp1.tv_nsec);
  if (res < 98 || res > 102)
    printf("Error, nanosleep did not really sleep 100ns, but %ld\n", res);
#endif

  MPI_Finalize();
  return 0;
}
