#ifndef MPI_HELPERS_H
#define MPI_HELPERS_H

#ifndef _GNU_SOURCE
  #define _GNU_SOURCE
#endif

#include <unistd.h>
#include <sys/time.h> /* Load it before the define next line to not mess with the system headers */
#if _POSIX_TIMERS
#include <time.h>
#endif

int smpi_usleep(useconds_t usecs);
#if _POSIX_TIMERS > 0
int smpi_nanosleep(const struct timespec* tp, struct timespec* t);
int smpi_clock_gettime(clockid_t clk_id, struct timespec* tp);
#endif
unsigned int smpi_sleep(unsigned int secs);
int smpi_gettimeofday(struct timeval* tv, struct timezone* tz);

struct option;
int smpi_getopt_long (int argc,  char *const *argv,  const char *options,  const struct option *long_options, int *opt_index);
int smpi_getopt (int argc,  char *const *argv,  const char *options);

#define sleep(x) smpi_sleep(x)
#define usleep(x) smpi_usleep(x)
#define gettimeofday(x, y) smpi_gettimeofday(x, 0)
#if _POSIX_TIMERS > 0
#define nanosleep(x, y) smpi_nanosleep(x, y)
#define clock_gettime(x, y) smpi_clock_gettime(x, y)
#endif

#define getopt(x,y,z) smpi_getopt(x,y,z)
#define getopt_long(x,y,z,a,b) smpi_getopt_long(x,y,z,a,b)

#endif
