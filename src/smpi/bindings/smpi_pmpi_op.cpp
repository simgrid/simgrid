/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_op.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

/* PMPI User level calls */

int PMPI_Op_create(MPI_User_function * function, int commute, MPI_Op * op)
{
  CHECK_NULL(1, MPI_ERR_ARG, function)
  CHECK_NULL(3, MPI_ERR_ARG, op)
  *op = new simgrid::smpi::Op(function, (commute!=0));
  return MPI_SUCCESS;
}

int PMPI_Op_free(MPI_Op * op)
{
  CHECK_NULL(1, MPI_ERR_ARG, op)
  CHECK_MPI_NULL(1, MPI_OP_NULL, MPI_ERR_OP, *op)
  simgrid::smpi::Op::unref(op);
  *op = MPI_OP_NULL;
  return MPI_SUCCESS;
}

int PMPI_Op_commutative(MPI_Op op, int* commute){
  CHECK_OP(1)
  CHECK_NULL(1, MPI_ERR_ARG, commute)
  *commute = op->is_commutative();
  return MPI_SUCCESS;
}

MPI_Op PMPI_Op_f2c(MPI_Fint op){
  if(op==-1)
    return MPI_OP_NULL;
  return simgrid::smpi::Op::f2c(op);
}

MPI_Fint PMPI_Op_c2f(MPI_Op op){
  if(op==MPI_OP_NULL)
    return -1;
  return op->c2f();
}
