/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi_op.hpp"
#include "private.hpp"
#include "smpi_datatype.hpp"
#include "src/smpi/include/smpi_actor.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(smpi_op, smpi, "Logging specific to SMPI (op)");

#define MAX_OP(a, b)  (b) = (a) < (b) ? (b) : (a)
#define MIN_OP(a, b)  (b) = (a) < (b) ? (a) : (b)
#define SUM_OP(a, b)  (b) += (a)
#define SUM_OP_COMPLEX(a, b)                                                                                           \
  {                                                                                                                    \
    ((b).value) += ((a).value);                                                                                        \
    ((b).index) += ((a).index);                                                                                        \
  }
#define PROD_OP(a, b) (b) *= (a)
#define PROD_OP_COMPLEX(a, b)                                                                                          \
  {                                                                                                                    \
    ((b).value) *= ((a).value);                                                                                        \
    ((b).index) *= ((a).index);                                                                                        \
  }
#define LAND_OP(a, b) (b) = (a) && (b)
#define LOR_OP(a, b)  (b) = (a) || (b)
#define LXOR_OP(a, b) (b) = bool(a) != bool(b)
#define BAND_OP(a, b) (b) &= (a)
#define BOR_OP(a, b)  (b) |= (a)
#define BXOR_OP(a, b) (b) ^= (a)
#define MAXLOC_OP(a, b)                                                                                                \
  (b) = ((a).value) < ((b).value) ? (b) : (((a).value) == ((b).value) ? (((a).index) < ((b).index) ? (a) : (b)) : (a))
#define MINLOC_OP(a, b)                                                                                                \
  (b) = ((a).value) < ((b).value) ? (a) : (((a).value) == ((b).value) ? (((a).index) < ((b).index) ? (a) : (b)) : (b))

#define APPLY_FUNC(a, b, length, type, func) \
{                                          \
  int i;                                   \
  type* x = (type*)(a);                    \
  type* y = (type*)(b);                    \
  for(i = 0; i < *(length); i++) {         \
    func(x[i], y[i]);                      \
  }                                        \
}

#define APPLY_OP_LOOP(dtype, type, op)                                                                                 \
  if (*datatype == (dtype)) {                                                                                          \
    APPLY_FUNC(a, b, length, type, op)                                                                                 \
  } else

#define APPLY_BASIC_OP_LOOP(op)\
APPLY_OP_LOOP(MPI_CHAR, char,op)\
APPLY_OP_LOOP(MPI_SHORT, short,op)\
APPLY_OP_LOOP(MPI_INT, int,op)\
APPLY_OP_LOOP(MPI_LONG, long,op)\
APPLY_OP_LOOP(MPI_LONG_LONG, long long,op)\
APPLY_OP_LOOP(MPI_SIGNED_CHAR, signed char,op)\
APPLY_OP_LOOP(MPI_UNSIGNED_CHAR, unsigned char,op)\
APPLY_OP_LOOP(MPI_UNSIGNED_SHORT, unsigned short,op)\
APPLY_OP_LOOP(MPI_UNSIGNED, unsigned int,op)\
APPLY_OP_LOOP(MPI_UNSIGNED_LONG, unsigned long,op)\
APPLY_OP_LOOP(MPI_UNSIGNED_LONG_LONG, unsigned long long,op)\
APPLY_OP_LOOP(MPI_WCHAR, wchar_t,op)\
APPLY_OP_LOOP(MPI_INT8_T, int8_t,op)\
APPLY_OP_LOOP(MPI_INT16_T, int16_t,op)\
APPLY_OP_LOOP(MPI_INT32_T, int32_t,op)\
APPLY_OP_LOOP(MPI_INT64_T, int64_t,op)\
APPLY_OP_LOOP(MPI_UINT8_T, uint8_t,op)\
APPLY_OP_LOOP(MPI_UINT16_T, uint16_t,op)\
APPLY_OP_LOOP(MPI_UINT32_T, uint32_t,op)\
APPLY_OP_LOOP(MPI_UINT64_T, uint64_t,op)\
APPLY_OP_LOOP(MPI_AINT, MPI_Aint,op)\
APPLY_OP_LOOP(MPI_OFFSET, MPI_Offset,op)\
APPLY_OP_LOOP(MPI_INTEGER1, int,op)\
APPLY_OP_LOOP(MPI_INTEGER2, int16_t,op)\
APPLY_OP_LOOP(MPI_INTEGER4, int32_t,op)\
APPLY_OP_LOOP(MPI_INTEGER8, int64_t,op)\
APPLY_OP_LOOP(MPI_COUNT, long long,op)


#define APPLY_BOOL_OP_LOOP(op)\
APPLY_OP_LOOP(MPI_C_BOOL, bool,op)

#define APPLY_BYTE_OP_LOOP(op)\
APPLY_OP_LOOP(MPI_BYTE, int8_t,op)

#define APPLY_FLOAT_OP_LOOP(op)\
APPLY_OP_LOOP(MPI_FLOAT, float,op)\
APPLY_OP_LOOP(MPI_DOUBLE, double,op)\
APPLY_OP_LOOP(MPI_LONG_DOUBLE, long double,op)\
APPLY_OP_LOOP(MPI_REAL, float,op)\
APPLY_OP_LOOP(MPI_REAL4, float,op)\
APPLY_OP_LOOP(MPI_REAL8, double,op)\
APPLY_OP_LOOP(MPI_REAL16, long double,op)

#define APPLY_COMPLEX_OP_LOOP(op)\
APPLY_OP_LOOP(MPI_C_FLOAT_COMPLEX, float _Complex,op)\
APPLY_OP_LOOP(MPI_C_DOUBLE_COMPLEX, double _Complex,op)\
APPLY_OP_LOOP(MPI_C_LONG_DOUBLE_COMPLEX, long double _Complex,op)

#define APPLY_PAIR_OP_LOOP(op)\
APPLY_OP_LOOP(MPI_FLOAT_INT, float_int,op)\
APPLY_OP_LOOP(MPI_LONG_INT, long_int,op)\
APPLY_OP_LOOP(MPI_DOUBLE_INT, double_int,op)\
APPLY_OP_LOOP(MPI_SHORT_INT, short_int,op)\
APPLY_OP_LOOP(MPI_2INT, int_int,op)\
APPLY_OP_LOOP(MPI_2FLOAT, float_float,op)\
APPLY_OP_LOOP(MPI_2DOUBLE, double_double,op)\
APPLY_OP_LOOP(MPI_LONG_DOUBLE_INT, long_double_int,op)\
APPLY_OP_LOOP(MPI_2LONG, long_long,op)\
APPLY_OP_LOOP(MPI_COMPLEX8, float_float,op)\
APPLY_OP_LOOP(MPI_COMPLEX16, double_double,op)\
APPLY_OP_LOOP(MPI_COMPLEX32, double_double,op)

#define APPLY_END_OP_LOOP(op)                                                                                          \
  {                                                                                                                    \
    xbt_die("Failed to apply " _XBT_STRINGIFY(op) " to type %s", (*datatype)->name());                                 \
  }

static void max_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_BASIC_OP_LOOP(MAX_OP)
  APPLY_FLOAT_OP_LOOP(MAX_OP)
  APPLY_END_OP_LOOP(MAX_OP)
}

static void min_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_BASIC_OP_LOOP(MIN_OP)
  APPLY_FLOAT_OP_LOOP(MIN_OP)
  APPLY_END_OP_LOOP(MIN_OP)
}

static void sum_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_BASIC_OP_LOOP(SUM_OP)
  APPLY_FLOAT_OP_LOOP(SUM_OP)
  APPLY_COMPLEX_OP_LOOP(SUM_OP)
  APPLY_PAIR_OP_LOOP(SUM_OP_COMPLEX)
  APPLY_END_OP_LOOP(SUM_OP)
}

static void prod_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_BASIC_OP_LOOP(PROD_OP)
  APPLY_FLOAT_OP_LOOP(PROD_OP)
  APPLY_COMPLEX_OP_LOOP(PROD_OP)
  APPLY_PAIR_OP_LOOP(PROD_OP_COMPLEX)
  APPLY_END_OP_LOOP(PROD_OP)
}

static void land_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_BASIC_OP_LOOP(LAND_OP)
  APPLY_FLOAT_OP_LOOP(LAND_OP)
  APPLY_BOOL_OP_LOOP(LAND_OP)
  APPLY_END_OP_LOOP(LAND_OP)
}

static void lor_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_BASIC_OP_LOOP(LOR_OP)
  APPLY_FLOAT_OP_LOOP(LOR_OP)
  APPLY_BOOL_OP_LOOP(LOR_OP)
  APPLY_END_OP_LOOP(LOR_OP)
}

static void lxor_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_BASIC_OP_LOOP(LXOR_OP)
  APPLY_FLOAT_OP_LOOP(LXOR_OP)
  APPLY_BOOL_OP_LOOP(LXOR_OP)
  APPLY_END_OP_LOOP(LXOR_OP)
}

static void band_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_BASIC_OP_LOOP(BAND_OP)
  APPLY_BOOL_OP_LOOP(BAND_OP)
  APPLY_BYTE_OP_LOOP(BAND_OP)
  APPLY_END_OP_LOOP(BAND_OP)
}

static void bor_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_BASIC_OP_LOOP(BOR_OP)
  APPLY_BOOL_OP_LOOP(BOR_OP)
  APPLY_BYTE_OP_LOOP(BOR_OP)
  APPLY_END_OP_LOOP(BOR_OP)
}

static void bxor_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_BASIC_OP_LOOP(BXOR_OP)
  APPLY_BOOL_OP_LOOP(BXOR_OP)
  APPLY_BYTE_OP_LOOP(BXOR_OP)
  APPLY_END_OP_LOOP(BXOR_OP)
}

static void minloc_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_PAIR_OP_LOOP(MINLOC_OP)
  APPLY_END_OP_LOOP(MINLOC_OP)
}

static void maxloc_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  APPLY_PAIR_OP_LOOP(MAXLOC_OP)
  APPLY_END_OP_LOOP(MAXLOC_OP)
}

static void replace_func(void *a, void *b, int *length, MPI_Datatype * datatype)
{
  memcpy(b, a, *length * (*datatype)->size());
}

static void no_func(void*, void*, int*, MPI_Datatype*)
{
  /* obviously a no-op */
}

#define CREATE_MPI_OP(name, func)                                                                                      \
  SMPI_Op _XBT_CONCAT(smpi_MPI_, name)(&(func) /* func */, true, true);                                              \

CREATE_MPI_OP(MAX, max_func)
CREATE_MPI_OP(MIN, min_func)
CREATE_MPI_OP(SUM, sum_func)
CREATE_MPI_OP(PROD, prod_func)
CREATE_MPI_OP(LAND, land_func)
CREATE_MPI_OP(LOR, lor_func)
CREATE_MPI_OP(LXOR, lxor_func)
CREATE_MPI_OP(BAND, band_func)
CREATE_MPI_OP(BOR, bor_func)
CREATE_MPI_OP(BXOR, bxor_func)
CREATE_MPI_OP(MAXLOC, maxloc_func)
CREATE_MPI_OP(MINLOC, minloc_func)
CREATE_MPI_OP(REPLACE, replace_func)
CREATE_MPI_OP(NO_OP, no_func)

namespace simgrid{
namespace smpi{

void Op::apply(const void* invec, void* inoutvec, const int* len, MPI_Datatype datatype) const
{
  if (smpi_cfg_privatization() == SmpiPrivStrategies::MMAP) {
    // we need to switch as the called function may silently touch global variables
    XBT_DEBUG("Applying operation, switch to the right data frame ");
    smpi_switch_data_segment(simgrid::s4u::Actor::self());
  }

  if (not smpi_process()->replaying() && *len > 0) {
    if (not is_fortran_op_)
      this->func_(const_cast<void*>(invec), inoutvec, const_cast<int*>(len), &datatype);
    else{
      XBT_DEBUG("Applying operation of length %d from %p and from/to %p", *len, invec, inoutvec);
      int tmp = datatype->c2f();
      /* Unfortunately, the C and Fortran version of the MPI standard do not agree on the type here,
         thus the reinterpret_cast. */
      this->func_(const_cast<void*>(invec), inoutvec, const_cast<int*>(len), reinterpret_cast<MPI_Datatype*>(&tmp));
    }
  }
}

Op* Op::f2c(int id){
  return static_cast<Op*>(F2C::f2c(id));
}

void Op::ref(){
  refcount_++;
}

void Op::unref(MPI_Op* op){
  if((*op)!=MPI_OP_NULL){
    (*op)->refcount_--;
    if ((*op)->refcount_ == 0 && not (*op)->is_predefined_){
      F2C::free_f((*op)->c2f());
      delete(*op);
    }
  }
}

}
}
