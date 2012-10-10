#ifndef timer_H_
#define timer_H_
/*!
 * \defgroup timer.h
 * here, we provides a generic interface to time some parts of the code
 */
# include <sys/time.h>

static inline double get_microsecond(struct timespec *res)
{
  return (res->tv_sec*1000000 + res->tv_nsec/1000);
}

static inline double get_nanosecond(struct timespec *res)
{
  return (res->tv_sec*1000000000 + res->tv_nsec);
}

static inline double get_second(struct timespec *res)
{
  return (res->tv_sec + res->tv_nsec/1000000000);
}

int get_time(struct timespec *tp);
double get_timediff(struct timespec *start, struct timespec * end);
#endif /*timer_H_*/
