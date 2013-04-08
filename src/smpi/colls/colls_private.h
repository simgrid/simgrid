#ifndef SMPI_COLLS_PRIVATE_H
#define SMPI_COLLS_PRIVATE_H

#include "colls.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_colls);
void star_reduction(MPI_Op op, void *src, void *target, int *count, MPI_Datatype *dtype);

#endif
