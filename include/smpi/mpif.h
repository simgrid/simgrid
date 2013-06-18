! -*- fortran -*-
! Copyright (c) 2010. The SimGrid Team.
! All rights reserved.

! This program is free software; you can redistribute it and/or modify it
! under the terms of the license (GNU LGPL) which comes with this package.

! SMPI's Fortran 77 include file

      integer MPI_THREAD_SINGLE, MPI_THREAD_FUNNELED
      integer MPI_THREAD_SERIALIZED, MPI_THREAD_MULTIPLE
      parameter(MPI_THREAD_SINGLE=0)
      parameter(MPI_THREAD_FUNNELED=1)
      parameter(MPI_THREAD_SERIALIZED=2)
      parameter(MPI_THREAD_MULTIPLE=3)

      integer MPI_MAX_PROCESSOR_NAME, MPI_MAX_ERROR_STRING
      integer MPI_MAX_DATAREP_STRIN, MPI_MAX_INFO_KEY
      integer MPI_MAX_INFO_VAL, MPI_MAX_OBJECT_NAME, MPI_MAX_PORT_NAME
      integer MPI_ANY_SOURCE, MPI_PROC_NULL, MPI_ANY_TAG, MPI_UNDEFINED
      integer MPI_IN_PLACE, MPI_TAG_UB, MPI_TAG_LB
      parameter(MPI_MAX_PROCESSOR_NAME=100)
      parameter(MPI_MAX_ERROR_STRING=100)
      parameter(MPI_MAX_DATAREP_STRIN =100)
      parameter(MPI_MAX_INFO_KEY=100)
      parameter(MPI_MAX_INFO_VAL=100)
      parameter(MPI_MAX_OBJECT_NAME=100)
      parameter(MPI_MAX_PORT_NAME=100)
      parameter(MPI_ANY_SOURCE=-555)
      parameter(MPI_PROC_NULL=-666)
      parameter(MPI_ANY_TAG=-444)
      parameter(MPI_UNDEFINED=-333)
      parameter(MPI_IN_PLACE=-222)
      parameter(MPI_TAG_UB=0)
      parameter(MPI_TAG_LB=0)

      integer MPI_SUCCESS, MPI_ERR_COMM, MPI_ERR_ARG, MPI_ERR_TYPE
      integer MPI_ERR_REQUEST, MPI_ERR_INTERN, MPI_ERR_COUNT
      integer MPI_ERR_RANK, MPI_ERR_OTHER, MPI_ERR_UNKNOWN
      integer MPI_ERR_TAG, MPI_ERR_TRUNCATE, MPI_ERR_GROUP, MPI_ERR_OP
      integer MPI_IDENT, MPI_SIMILAR, MPI_UNEQUAL, MPI_CONGRUENT
      integer MPI_WTIME_IS_GLOBAL
      parameter(MPI_SUCCESS=0)
      parameter(MPI_ERR_COMM=1)
      parameter(MPI_ERR_ARG=2)
      parameter(MPI_ERR_TYPE=3)
      parameter(MPI_ERR_REQUEST=4)
      parameter(MPI_ERR_INTERN=5)
      parameter(MPI_ERR_COUNT=6)
      parameter(MPI_ERR_RANK=7)
      parameter(MPI_ERR_TAG=8)
      parameter(MPI_ERR_TRUNCATE=9)
      parameter(MPI_ERR_GROUP=10)
      parameter(MPI_ERR_OP=11)
      parameter(MPI_ERR_OTHER=12)
      parameter(MPI_ERR_UNKNOWN=13)
      parameter(MPI_IDENT=0)
      parameter(MPI_SIMILAR=1)
      parameter(MPI_UNEQUAL=2)
      parameter(MPI_CONGRUENT=3)
      parameter(MPI_WTIME_IS_GLOBAL=1)

! These should be ordered as in smpi_f77.c
      integer MPI_COMM_NULL, MPI_COMM_WORLD, MPI_COMM_SELF
      parameter(MPI_COMM_NULL=-1)
      parameter(MPI_COMM_SELF=-2)
      parameter(MPI_COMM_WORLD=0)

      integer MPI_GROUP_NULL, MPI_GROUP_EMPTY
      parameter(MPI_GROUP_NULL=-1)
      parameter(MPI_GROUP_EMPTY=-2)

! This should be equal to the number of int fields in MPI_Status
      integer MPI_STATUS_SIZE, MPI_STATUSES_IGNORE
      parameter(MPI_STATUS_SIZE=4)
      parameter(MPI_STATUSES_IGNORE=0)

      integer MPI_STATUS_IGNORE(MPI_STATUS_SIZE)
      common/smpi/ MPI_STATUS_IGNORE

      integer MPI_REQUEST_NULL
      parameter(MPI_REQUEST_NULL=-1)

! These should be ordered as in smpi_f77.c
      integer MPI_DATATYPE_NULL, MPI_BYTE, MPI_CHARACTER, MPI_LOGICAL
      integer MPI_INTEGER, MPI_INTEGER1, MPI_INTEGER2, MPI_INTEGER4
      integer MPI_INTEGER8, MPI_REAL, MPI_REAL4, MPI_REAL8
      integer MPI_DOUBLE_PRECISION, MPI_COMPLEX, MPI_DOUBLE_COMPLEX
      integer MPI_2INTEGER, MPI_LOGICAL1, MPI_LOGICAL2, MPI_LOGICAL4
      integer MPI_LOGICAL8, MPI_2REAL, MPI_2DOUBLE_PRECISION
      parameter(MPI_DATATYPE_NULL=-1)
      parameter(MPI_BYTE=0)
      parameter(MPI_CHARACTER=1)
      parameter(MPI_LOGICAL=2)
      parameter(MPI_INTEGER=3)
      parameter(MPI_INTEGER1=4)
      parameter(MPI_INTEGER2=5)
      parameter(MPI_INTEGER4=6)
      parameter(MPI_INTEGER8=7)
      parameter(MPI_REAL=8)
      parameter(MPI_REAL4=9)
      parameter(MPI_REAL8=10)
      parameter(MPI_DOUBLE_PRECISION=11)
      parameter(MPI_COMPLEX=12)
      parameter(MPI_DOUBLE_COMPLEX=13)
      parameter(MPI_2INTEGER=14)
      parameter(MPI_LOGICAL1=15)
      parameter(MPI_LOGICAL2=16)
      parameter(MPI_LOGICAL4=17)
      parameter(MPI_LOGICAL8=18)
      parameter(MPI_2REAL=19)
      parameter(MPI_2DOUBLE_PRECISION=19)

! These should be ordered as in smpi_f77.c
      integer MPI_OP_NULL,MPI_MAX, MPI_MIN, MPI_MAXLOC, MPI_MINLOC
      integer MPI_SUM, MPI_PROD, MPI_LAND, MPI_LOR, MPI_LXOR, MPI_BAND
      integer MPI_BOR, MPI_BXOR
      parameter(MPI_OP_NULL=-1)
      parameter(MPI_MAX=0)
      parameter(MPI_MIN=1)
      parameter(MPI_MAXLOC=2)
      parameter(MPI_MINLOC=3)
      parameter(MPI_SUM=4)
      parameter(MPI_PROD=5)
      parameter(MPI_LAND=6)
      parameter(MPI_LOR=7)
      parameter(MPI_LXOR=8)
      parameter(MPI_BAND=9)
      parameter(MPI_BOR=10)
      parameter(MPI_BXOR=11)
      
      INTEGER MPI_ADDRESS_KIND, MPI_OFFSET_KIND
      PARAMETER (MPI_ADDRESS_KIND=8)
      PARAMETER (MPI_OFFSET_KIND=8)
      
      INTEGER MPI_MODE_NOPRECEDE
      PARAMETER (MPI_MODE_NOPRECEDE=8192)

      external MPI_INIT, MPI_FINALIZE, MPI_ABORT
      external MPI_COMM_RANK, MPI_COMM_SIZE, MPI_COMM_DUP, MPI_COMM_SPLIT
      external MPI_SEND_INIT, MPI_ISEND, MPI_SEND
      external MPI_RECV_INIT, MPI_IRECV, MPI_RECV
      external MPI_START, MPI_STARTALL
      external MPI_WAIT, MPI_WAITANY, MPI_WAITALL
      external MPI_BCAST, MPI_BARRIER, MPI_REDUCE, MPI_ALLREDUCE
      external MPI_SCATTER, MPI_GATHER, MPI_ALLGATHER, MPI_SCAN
      external MPI_ALLTOALL, MPI_GATHERV, MPI_SENDRECV

      external MPI_WTIME
      external MPI_WTICK

      double precision MPI_WTIME
      double precision MPI_WTICK
