/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.h"

extern int xargc;
extern char** xargv;

void* mpi_byte__             = &MPI_BYTE;
void* mpi_character__        = &MPI_CHAR;
void* mpi_logical__          = &MPI_INT;
void* mpi_integer__          = &MPI_INT;
void* mpi_integer1__         = &MPI_INT8_T;
void* mpi_integer2__         = &MPI_INT16_T;
void* mpi_integer4__         = &MPI_INT32_T;
void* mpi_integer8__         = &MPI_INT64_T;
void* mpi_real__             = &MPI_FLOAT;
void* mpi_real4__            = &MPI_FLOAT;
void* mpi_real8__            = &MPI_DOUBLE;
void* mpi_double_precision__ = &MPI_DOUBLE;
void* mpi_complex__          = &MPI_C_FLOAT_COMPLEX;
void* mpi_double_complex__   = &MPI_C_DOUBLE_COMPLEX;
void* mpi_2integer__         = &MPI_2INT;
void* mpi_logical1__         = &MPI_UINT8_T;
void* mpi_logical2__         = &MPI_UINT16_T;
void* mpi_logical4__         = &MPI_UINT32_T;
void* mpi_logical8__         = &MPI_UINT64_T;

void* mpi_comm_world__ = &MPI_COMM_WORLD;

void mpi_init__(int* ierr) {
   /* smpif2c is responsible for generating a call with the final arguments */
   *ierr = MPI_Init(NULL, NULL);
}

void mpi_finalize__(int* ierr) {
   *ierr = MPI_Finalize();
}

void mpi_comm_rank__(MPI_Comm** comm, int* rank, int* ierr) {
   /* Yes, you really get a MPI_Comm** here */
   *ierr = MPI_Comm_rank(**comm, rank);
}

void mpi_comm_size__(MPI_Comm** comm, int* size, int* ierr) {
   /* Yes, you really get a MPI_Comm** here */
   *ierr = MPI_Comm_size(**comm, size);
}

double mpi_wtime__(void) {
   return MPI_Wtime();
}

void mpi_send__(void* buf, int* count, MPI_Datatype** datatype, int* dst,
                int* tag, MPI_Comm** comm, int* ierr) {
   *ierr = MPI_Send(buf, *count, **datatype, *dst, *tag, **comm);
}

void mpi_recv__(void* buf, int* count, MPI_Datatype** datatype, int* src,
                int* tag, MPI_Comm** comm, MPI_Status* status, int* ierr) {
   *ierr = MPI_Recv(buf, *count, **datatype, *src, *tag, **comm, status);
}
