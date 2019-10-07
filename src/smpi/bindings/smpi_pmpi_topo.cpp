/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

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
  if (comm_old == MPI_COMM_NULL){
    return MPI_ERR_COMM;
  } else if (ndims < 0 || (ndims > 0 && (dims == nullptr || periodic == nullptr)) || comm_cart == nullptr) {
    return MPI_ERR_ARG;
  } else {
    for (int i = 0; i < ndims; i++)
      if (dims[i] < 0)
        return MPI_ERR_ARG;

    simgrid::smpi::Topo_Cart* topo = new simgrid::smpi::Topo_Cart(comm_old, ndims, dims, periodic, reorder, comm_cart);
    if (*comm_cart == MPI_COMM_NULL) {
      delete topo;
    } else {
      xbt_assert((*comm_cart)->topo() == topo);
    }
    return MPI_SUCCESS;
  }
}

int PMPI_Cart_rank(MPI_Comm comm, const int* coords, int* rank) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (coords == nullptr) {
    return MPI_SUCCESS;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->rank(coords, rank);
}

int PMPI_Cart_shift(MPI_Comm comm, int direction, int displ, int* source, int* dest) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (source == nullptr || dest == nullptr || direction < 0 ) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->shift(direction, displ, source, dest);
}

int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int* coords) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (rank < 0 || rank >= comm->size()) {
    return MPI_ERR_RANK;
  }
  if (maxdims < 0) {
    return MPI_ERR_ARG;
  }
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
  if(comm == nullptr || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if(maxdims <= 0) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->get(maxdims, dims, periods, coords);
}

int PMPI_Cartdim_get(MPI_Comm comm, int* ndims) {
  if (comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (ndims == nullptr) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  return topo->dim_get(ndims);
}

int PMPI_Dims_create(int nnodes, int ndims, int* dims) {
  if(dims == nullptr) {
    return MPI_SUCCESS;
  }
  if (ndims < 1 || nnodes < 1) {
    return MPI_ERR_DIMS;
  }
  return simgrid::smpi::Topo_Cart::Dims_create(nnodes, ndims, dims);
}

int PMPI_Cart_sub(MPI_Comm comm, const int* remain_dims, MPI_Comm* comm_new) {
  if(comm == MPI_COMM_NULL || comm->topo() == nullptr) {
    return MPI_ERR_TOPOLOGY;
  }
  if (comm_new == nullptr) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology topo = static_cast<MPIR_Cart_Topology>(comm->topo());
  if (topo==nullptr) {
    return MPI_ERR_ARG;
  }
  MPIR_Cart_Topology cart = topo->sub(remain_dims, comm_new);
  if(*comm_new==MPI_COMM_NULL)
      delete cart;
  if(cart==nullptr)
    return  MPI_ERR_ARG;
  return MPI_SUCCESS;
}
