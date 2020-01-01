/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include "smpi_comm.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(smpi_pmpi);

/* PMPI User level calls */

/* The topo part of MPI_COMM_WORLD should always be nullptr. When other topologies will be implemented, not only should we
 * check if the topology is nullptr, but we should check if it is the good topology type (so we have to add a
 *  MPIR_Topo_Type field, and replace the MPI_Topology field by an union)*/

int PMPI_Cart_create(MPI_Comm comm_old, int ndims, const int* dims, const int* periodic, int reorder, MPI_Comm* comm_cart) {
  CHECK_COMM2(1, comm_old)
  if (ndims > 0){
    CHECK_NULL(3, MPI_ERR_ARG, dims)
    CHECK_NULL(4, MPI_ERR_ARG, periodic)
    CHECK_NULL(6, MPI_ERR_ARG, comm_cart)
  }
  CHECK_NEGATIVE(2, MPI_ERR_ARG, ndims)
  for (int i = 0; i < ndims; i++)
    CHECK_NEGATIVE(2, MPI_ERR_ARG, dims[i])
  const simgrid::smpi::Topo_Cart* topo =
      new simgrid::smpi::Topo_Cart(comm_old, ndims, dims, periodic, reorder, comm_cart);
  if (*comm_cart == MPI_COMM_NULL) {
    delete topo;
  } else {
    xbt_assert((*comm_cart)->topo() == topo);
  }
  return MPI_SUCCESS;
}

int PMPI_Cart_rank(MPI_Comm comm, const int* coords, int* rank) {
  CHECK_COMM(1)
  CHECK_NULL(1, MPI_ERR_TOPOLOGY, comm->topo())
  CHECK_NULL(2, MPI_SUCCESS, coords)
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->rank(coords, rank);
}

int PMPI_Cart_shift(MPI_Comm comm, int direction, int displ, int* source, int* dest) {
  CHECK_COMM(1)
  CHECK_NULL(1, MPI_ERR_TOPOLOGY, comm->topo())
  CHECK_NEGATIVE(3, MPI_ERR_ARG, direction)
  CHECK_NULL(4, MPI_ERR_ARG, source)
  CHECK_NULL(5, MPI_ERR_ARG, dest)
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->shift(direction, displ, source, dest);
}

int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int* coords) {
  CHECK_COMM(1)
  CHECK_NULL(1, MPI_ERR_TOPOLOGY, comm->topo())
  CHECK_NEGATIVE(3, MPI_ERR_ARG, maxdims)
  CHECK_RANK(2, rank, comm)
  if(maxdims==0 || coords == nullptr) {
    return MPI_SUCCESS;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->coords(rank, maxdims, coords);
}

int PMPI_Cart_get(MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords) {
  if(dims == nullptr || periods == nullptr || coords == nullptr){
    return MPI_SUCCESS;
  }
  CHECK_COMM(1)
  CHECK_NULL(1, MPI_ERR_TOPOLOGY, comm->topo())
  CHECK_NEGATIVE(3, MPI_ERR_ARG, maxdims)
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->get(maxdims, dims, periods, coords);
}

int PMPI_Cartdim_get(MPI_Comm comm, int* ndims) {
  CHECK_COMM(1)
  CHECK_NULL(1, MPI_ERR_TOPOLOGY, comm->topo())
  CHECK_NULL(2, MPI_ERR_ARG, ndims)
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->dim_get(ndims);
}

int PMPI_Dims_create(int nnodes, int ndims, int* dims) {
  CHECK_NULL(3, MPI_SUCCESS, dims)
  if (ndims < 1 || nnodes < 1) {
    return MPI_ERR_DIMS;
  }
  return simgrid::smpi::Topo_Cart::Dims_create(nnodes, ndims, dims);
}

int PMPI_Cart_sub(MPI_Comm comm, const int* remain_dims, MPI_Comm* comm_new) {
  CHECK_COMM(1)
  CHECK_NULL(1, MPI_ERR_TOPOLOGY, comm->topo())
  CHECK_NULL(3, MPI_ERR_ARG, comm_new)
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  const simgrid::smpi::Topo_Cart* cart = topo->sub(remain_dims, comm_new);
  if(*comm_new==MPI_COMM_NULL)
      delete cart;
  if(cart==nullptr)
    return  MPI_ERR_ARG;
  return MPI_SUCCESS;
}
