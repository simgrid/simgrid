#ifndef MPI_H
#define MPI_H

#define SEED 221238

#include <smpi/smpi.h>
#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/asserts.h>

#define sleep(x) smpi_sleep(x)
#define gettimeofday(x, y) smpi_gettimeofday(x, y)
#define main(x, y) smpi_simulated_main(x, y)

#endif
