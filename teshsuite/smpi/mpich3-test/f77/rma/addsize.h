C -*- Mode: Fortran; -*-
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C     SimGrid comment :
C     This file holds a value which should have the same size as an MPI_Aint
C     that can hold any pointer. In f90 it's a integer (kind=MPI_ADDRESS_KIND)
C     Original mpich testsuite uses autconf to configure the right size to use
C     Integer is not right on some systems, we set it to integer*8 by default
      integer*8 asize
