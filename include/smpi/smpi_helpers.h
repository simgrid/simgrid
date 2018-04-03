#ifndef MPI_HELPERS_H
#define MPI_HELPERS_H

#include <unistd.h>
#include <sys/time.h> /* Load it before the define next line to not mess with the system headers */

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
