/* smpi_mpi_dt.c -- MPI primitives to handle datatypes                        */
/* FIXME: a very incomplete implementation                                    */

/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"
#include "smpi_mpi_dt_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_mpi_dt, smpi,
                                "Logging specific to SMPI (datatype)");

typedef struct s_smpi_mpi_datatype {
  size_t size;
  MPI_Aint lb;
  MPI_Aint ub;
  int flags;
} s_smpi_mpi_datatype_t;

#define CREATE_MPI_DATATYPE(name, type)       \
  static s_smpi_mpi_datatype_t mpi_##name = { \
    sizeof(type),  /* size */                 \
    0,             /* lb */                   \
    sizeof(type),  /* ub = lb + size */       \
    DT_FLAG_BASIC  /* flags */                \
  };                                          \
  MPI_Datatype name = &mpi_##name;


//The following are datatypes for the MPI functions MPI_MAXLOC and MPI_MINLOC.
typedef struct {
  float value;
  int index;
} float_int;
typedef struct {
  long value;
  int index;
} long_int;
typedef struct {
  double value;
  int index;
} double_int;
typedef struct {
  short value;
  int index;
} short_int;
typedef struct {
  int value;
  int index;
} int_int;
typedef struct {
  long double value;
  int index;
} long_double_int;

// Predefined data types
CREATE_MPI_DATATYPE(MPI_CHAR, char);
CREATE_MPI_DATATYPE(MPI_SHORT, short);
CREATE_MPI_DATATYPE(MPI_INT, int);
CREATE_MPI_DATATYPE(MPI_LONG, long);
CREATE_MPI_DATATYPE(MPI_LONG_LONG, long long);
CREATE_MPI_DATATYPE(MPI_SIGNED_CHAR, signed char);
CREATE_MPI_DATATYPE(MPI_UNSIGNED_CHAR, unsigned char);
CREATE_MPI_DATATYPE(MPI_UNSIGNED_SHORT, unsigned short);
CREATE_MPI_DATATYPE(MPI_UNSIGNED, unsigned int);
CREATE_MPI_DATATYPE(MPI_UNSIGNED_LONG, unsigned long);
CREATE_MPI_DATATYPE(MPI_UNSIGNED_LONG_LONG, unsigned long long);
CREATE_MPI_DATATYPE(MPI_FLOAT, float);
CREATE_MPI_DATATYPE(MPI_DOUBLE, double);
CREATE_MPI_DATATYPE(MPI_LONG_DOUBLE, long double);
CREATE_MPI_DATATYPE(MPI_WCHAR, wchar_t);
CREATE_MPI_DATATYPE(MPI_C_BOOL, _Bool);
CREATE_MPI_DATATYPE(MPI_INT8_T, int8_t);
CREATE_MPI_DATATYPE(MPI_INT16_T, int16_t);
CREATE_MPI_DATATYPE(MPI_INT32_T, int32_t);
CREATE_MPI_DATATYPE(MPI_INT64_T, int64_t);
CREATE_MPI_DATATYPE(MPI_UINT8_T, uint8_t);
CREATE_MPI_DATATYPE(MPI_UINT16_T, uint16_t);
CREATE_MPI_DATATYPE(MPI_UINT32_T, uint32_t);
CREATE_MPI_DATATYPE(MPI_UINT64_T, uint64_t);
CREATE_MPI_DATATYPE(MPI_C_FLOAT_COMPLEX, float _Complex);
CREATE_MPI_DATATYPE(MPI_C_DOUBLE_COMPLEX, double _Complex);
CREATE_MPI_DATATYPE(MPI_C_LONG_DOUBLE_COMPLEX, long double _Complex);
CREATE_MPI_DATATYPE(MPI_AINT, MPI_Aint);
CREATE_MPI_DATATYPE(MPI_OFFSET, MPI_Offset);

CREATE_MPI_DATATYPE(MPI_FLOAT_INT, float_int);
CREATE_MPI_DATATYPE(MPI_LONG_INT, long_int);
CREATE_MPI_DATATYPE(MPI_DOUBLE_INT, double_int);
CREATE_MPI_DATATYPE(MPI_SHORT_INT, short_int);
CREATE_MPI_DATATYPE(MPI_2INT, int_int);
CREATE_MPI_DATATYPE(MPI_LONG_DOUBLE_INT, long_double_int);


size_t smpi_datatype_size(MPI_Datatype datatype)
{
  return datatype->size;
}

MPI_Aint smpi_datatype_lb(MPI_Datatype datatype)
{
  return datatype->lb;
}

MPI_Aint smpi_datatype_ub(MPI_Datatype datatype)
{
  return datatype->ub;
}

int smpi_datatype_extent(MPI_Datatype datatype, MPI_Aint * lb,
                         MPI_Aint * extent)
{
  int retval;

  if ((datatype->flags & DT_FLAG_COMMITED) != DT_FLAG_COMMITED) {
    retval = MPI_ERR_TYPE;
  } else {
    *lb = datatype->lb;
    *extent = datatype->ub - datatype->lb;
    retval = MPI_SUCCESS;
  }
  return MPI_SUCCESS;
}

int smpi_datatype_copy(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype)
{
  int retval, count;

  /* First check if we really have something to do */
  if (recvcount == 0) {
    retval = sendcount == 0 ? MPI_SUCCESS : MPI_ERR_TRUNCATE;
  } else {
    /* FIXME: treat packed cases */
    sendcount *= smpi_datatype_size(sendtype);
    recvcount *= smpi_datatype_size(recvtype);
    count = sendcount < recvcount ? sendcount : recvcount;
    memcpy(recvbuf, sendbuf, count);
    retval = sendcount > recvcount ? MPI_ERR_TRUNCATE : MPI_SUCCESS;
  }
  return retval;
}

typedef struct s_smpi_mpi_op {
  MPI_User_function *func;
} s_smpi_mpi_op_t;

#define MAX_OP(a, b)  (b) = (a) < (b) ? (b) : (a)
#define MIN_OP(a, b)  (b) = (a) < (b) ? (a) : (b)
#define SUM_OP(a, b)  (b) += (a)
#define PROD_OP(a, b) (b) *= (a)
#define LAND_OP(a, b) (b) = (a) && (b)
#define LOR_OP(a, b)  (b) = (a) || (b)
#define LXOR_OP(a, b) (b) = (!(a) && (b)) || ((a) && !(b))
#define BAND_OP(a, b) (b) &= (a)
#define BOR_OP(a, b)  (b) |= (a)
#define BXOR_OP(a, b) (b) ^= (a)
#define MAXLOC_OP(a, b)  (b) = (a.value) < (b.value) ? (b) : (a)
#define MINLOC_OP(a, b)  (b) = (a.value) < (b.value) ? (a) : (b)
//TODO : MINLOC & MAXLOC

#define APPLY_FUNC(a, b, length, type, func) \
  {                                          \
    int i;                                   \
    type* x = (type*)(a);                    \
    type* y = (type*)(b);                    \
    for(i = 0; i < *(length); i++) {         \
      func(x[i], y[i]);                      \
    }                                        \
  }

static void max_func(void *a, void *b, int *length,
                     MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, MAX_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, MAX_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, MAX_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, MAX_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, MAX_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, MAX_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, MAX_OP);
  } else if (*datatype == MPI_FLOAT) {
    APPLY_FUNC(a, b, length, float, MAX_OP);
  } else if (*datatype == MPI_DOUBLE) {
    APPLY_FUNC(a, b, length, double, MAX_OP);
  } else if (*datatype == MPI_LONG_DOUBLE) {
    APPLY_FUNC(a, b, length, long double, MAX_OP);
  }
}

static void min_func(void *a, void *b, int *length,
                     MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, MIN_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, MIN_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, MIN_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, MIN_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, MIN_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, MIN_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, MIN_OP);
  } else if (*datatype == MPI_FLOAT) {
    APPLY_FUNC(a, b, length, float, MIN_OP);
  } else if (*datatype == MPI_DOUBLE) {
    APPLY_FUNC(a, b, length, double, MIN_OP);
  } else if (*datatype == MPI_LONG_DOUBLE) {
    APPLY_FUNC(a, b, length, long double, MIN_OP);
  }
}

static void sum_func(void *a, void *b, int *length,
                     MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, SUM_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, SUM_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, SUM_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, SUM_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, SUM_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, SUM_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, SUM_OP);
  } else if (*datatype == MPI_FLOAT) {
    APPLY_FUNC(a, b, length, float, SUM_OP);
  } else if (*datatype == MPI_DOUBLE) {
    APPLY_FUNC(a, b, length, double, SUM_OP);
  } else if (*datatype == MPI_LONG_DOUBLE) {
    APPLY_FUNC(a, b, length, long double, SUM_OP);
  } else if (*datatype == MPI_C_FLOAT_COMPLEX) {
    APPLY_FUNC(a, b, length, float _Complex, SUM_OP);
  } else if (*datatype == MPI_C_DOUBLE_COMPLEX) {
    APPLY_FUNC(a, b, length, double _Complex, SUM_OP);
  } else if (*datatype == MPI_C_LONG_DOUBLE_COMPLEX) {
    APPLY_FUNC(a, b, length, long double _Complex, SUM_OP);
  }
}

static void prod_func(void *a, void *b, int *length,
                      MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, PROD_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, PROD_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, PROD_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, PROD_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, PROD_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, PROD_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, PROD_OP);
  } else if (*datatype == MPI_FLOAT) {
    APPLY_FUNC(a, b, length, float, PROD_OP);
  } else if (*datatype == MPI_DOUBLE) {
    APPLY_FUNC(a, b, length, double, PROD_OP);
  } else if (*datatype == MPI_LONG_DOUBLE) {
    APPLY_FUNC(a, b, length, long double, PROD_OP);
  } else if (*datatype == MPI_C_FLOAT_COMPLEX) {
    APPLY_FUNC(a, b, length, float _Complex, PROD_OP);
  } else if (*datatype == MPI_C_DOUBLE_COMPLEX) {
    APPLY_FUNC(a, b, length, double _Complex, PROD_OP);
  } else if (*datatype == MPI_C_LONG_DOUBLE_COMPLEX) {
    APPLY_FUNC(a, b, length, long double _Complex, PROD_OP);
  }
}

static void land_func(void *a, void *b, int *length,
                      MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, LAND_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, LAND_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, LAND_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, LAND_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, LAND_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, LAND_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, LAND_OP);
  } else if (*datatype == MPI_C_BOOL) {
    APPLY_FUNC(a, b, length, _Bool, LAND_OP);
  }
}

static void lor_func(void *a, void *b, int *length,
                     MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, LOR_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, LOR_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, LOR_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, LOR_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, LOR_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, LOR_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, LOR_OP);
  } else if (*datatype == MPI_C_BOOL) {
    APPLY_FUNC(a, b, length, _Bool, LOR_OP);
  }
}

static void lxor_func(void *a, void *b, int *length,
                      MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, LXOR_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, LXOR_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, LXOR_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, LXOR_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, LXOR_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, LXOR_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, LXOR_OP);
  } else if (*datatype == MPI_C_BOOL) {
    APPLY_FUNC(a, b, length, _Bool, LXOR_OP);
  }
}

static void band_func(void *a, void *b, int *length,
                      MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, BAND_OP);
  }
  if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, BAND_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, BAND_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, BAND_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, BAND_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, BAND_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, BAND_OP);
  } else if (*datatype == MPI_BYTE) {
    APPLY_FUNC(a, b, length, uint8_t, BAND_OP);
  }
}

static void bor_func(void *a, void *b, int *length,
                     MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, BOR_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, BOR_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, BOR_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, BOR_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, BOR_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, BOR_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, BOR_OP);
  } else if (*datatype == MPI_BYTE) {
    APPLY_FUNC(a, b, length, uint8_t, BOR_OP);
  }
}

static void bxor_func(void *a, void *b, int *length,
                      MPI_Datatype * datatype)
{
  if (*datatype == MPI_CHAR) {
    APPLY_FUNC(a, b, length, char, BXOR_OP);
  } else if (*datatype == MPI_SHORT) {
    APPLY_FUNC(a, b, length, short, BXOR_OP);
  } else if (*datatype == MPI_INT) {
    APPLY_FUNC(a, b, length, int, BXOR_OP);
  } else if (*datatype == MPI_LONG) {
    APPLY_FUNC(a, b, length, long, BXOR_OP);
  } else if (*datatype == MPI_UNSIGNED_SHORT) {
    APPLY_FUNC(a, b, length, unsigned short, BXOR_OP);
  } else if (*datatype == MPI_UNSIGNED) {
    APPLY_FUNC(a, b, length, unsigned int, BXOR_OP);
  } else if (*datatype == MPI_UNSIGNED_LONG) {
    APPLY_FUNC(a, b, length, unsigned long, BXOR_OP);
  } else if (*datatype == MPI_BYTE) {
    APPLY_FUNC(a, b, length, uint8_t, BXOR_OP);
  }
}

static void minloc_func(void *a, void *b, int *length,
                        MPI_Datatype * datatype)
{
  if (*datatype == MPI_FLOAT_INT) {
    APPLY_FUNC(a, b, length, float_int, MINLOC_OP);
  } else if (*datatype == MPI_LONG_INT) {
    APPLY_FUNC(a, b, length, long_int, MINLOC_OP);
  } else if (*datatype == MPI_DOUBLE_INT) {
    APPLY_FUNC(a, b, length, double_int, MINLOC_OP);
  } else if (*datatype == MPI_SHORT_INT) {
    APPLY_FUNC(a, b, length, short_int, MINLOC_OP);
  } else if (*datatype == MPI_2INT) {
    APPLY_FUNC(a, b, length, int_int, MINLOC_OP);
  } else if (*datatype == MPI_LONG_DOUBLE_INT) {
    APPLY_FUNC(a, b, length, long_double_int, MINLOC_OP);
  }
}

static void maxloc_func(void *a, void *b, int *length,
                        MPI_Datatype * datatype)
{
  if (*datatype == MPI_FLOAT_INT) {
    APPLY_FUNC(a, b, length, float_int, MAXLOC_OP);
  } else if (*datatype == MPI_LONG_INT) {
    APPLY_FUNC(a, b, length, long_int, MAXLOC_OP);
  } else if (*datatype == MPI_DOUBLE_INT) {
    APPLY_FUNC(a, b, length, double_int, MAXLOC_OP);
  } else if (*datatype == MPI_SHORT_INT) {
    APPLY_FUNC(a, b, length, short_int, MAXLOC_OP);
  } else if (*datatype == MPI_2INT) {
    APPLY_FUNC(a, b, length, int_int, MAXLOC_OP);
  } else if (*datatype == MPI_LONG_DOUBLE_INT) {
    APPLY_FUNC(a, b, length, long_double_int, MAXLOC_OP);
  }
}


#define CREATE_MPI_OP(name, func)                             \
  static s_smpi_mpi_op_t mpi_##name = { &(func) /* func */ }; \
  MPI_Op name = &mpi_##name;

CREATE_MPI_OP(MPI_MAX, max_func);
CREATE_MPI_OP(MPI_MIN, min_func);
CREATE_MPI_OP(MPI_SUM, sum_func);
CREATE_MPI_OP(MPI_PROD, prod_func);
CREATE_MPI_OP(MPI_LAND, land_func);
CREATE_MPI_OP(MPI_LOR, lor_func);
CREATE_MPI_OP(MPI_LXOR, lxor_func);
CREATE_MPI_OP(MPI_BAND, band_func);
CREATE_MPI_OP(MPI_BOR, bor_func);
CREATE_MPI_OP(MPI_BXOR, bxor_func);
CREATE_MPI_OP(MPI_MAXLOC, maxloc_func);
CREATE_MPI_OP(MPI_MINLOC, minloc_func);

MPI_Op smpi_op_new(MPI_User_function * function, int commute)
{
  MPI_Op op;

  //FIXME: add commute param
  op = xbt_new(s_smpi_mpi_op_t, 1);
  op->func = function;
  return op;
}

void smpi_op_destroy(MPI_Op op)
{
  xbt_free(op);
}

void smpi_op_apply(MPI_Op op, void *invec, void *inoutvec, int *len,
                   MPI_Datatype * datatype)
{
  op->func(invec, inoutvec, len, datatype);
}
