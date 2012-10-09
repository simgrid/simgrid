# include "timer.h"
#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include "xbt/log.h"
 XBT_LOG_NEW_DEFAULT_CATEGORY(MM_timer,
                             "Messages specific for this msg example");

/* this could be specific for some processors
 * the default solution seems to be accurate enough
#define CLOCK_TIMER CLOCK_MONOTONIC_RAW
 */

inline double get_microsecond(struct timespec *res){
  return (res->tv_sec*1000000 + res->tv_nsec/1000);
}
inline double get_nanosecond(struct timespec *res){
  return (res->tv_sec*1000000000 + res->tv_nsec);
}
inline double get_second(struct timespec *res){
  return (res->tv_sec + res->tv_nsec/1000000000);
}

inline int get_time(struct timespec *tp){
  double time = MPI_Wtime();
  time_t value = (time_t)floor(time);
  time -= (double) value;
  time = time * 1000000000;
  tp->tv_nsec = (long) time;
  tp->tv_sec = value ;
}

double get_timediff(struct timespec *start, struct timespec *end){
	return (double)(-start->tv_sec - ((double)start->tv_nsec)/1000000000 + end->tv_sec + ((double)end->tv_nsec)/1000000000);
}
