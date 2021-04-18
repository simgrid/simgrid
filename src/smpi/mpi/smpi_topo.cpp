/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi/smpi.h"
#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_topo.hpp"
#include "xbt/sysdep.h"

#include <algorithm>
#include <vector>

/* static functions */
static int assignnodes(int ndim, const std::vector<int>& factors, std::vector<int>& dims);
static int getfactors(int num, std::vector<int>& factors);

namespace simgrid{
namespace smpi{

void Topo::setComm(MPI_Comm comm)
{
  xbt_assert(not comm_);
  comm_ = comm;
}

/*******************************************************************************
 * Cartesian topologies
 ******************************************************************************/

Topo_Cart::Topo_Cart(int ndims) : ndims_(ndims), dims_(ndims), periodic_(ndims), position_(ndims)
{
}

/* reorder is ignored, don't know what would be the consequences of a dumb reordering but neither do I see the point of
 * reordering*/
Topo_Cart::Topo_Cart(MPI_Comm comm_old, int ndims, const int dims[], const int periods[], int /*reorder*/, MPI_Comm* comm_cart)
    : Topo_Cart(ndims)
{
  int rank = comm_old->rank();

  if(ndims != 0) {
    int newSize = 1;
    for (int i = 0 ; i < ndims ; i++) {
      newSize *= dims[i];
    }
    if(rank >= newSize) {
      if(comm_cart != nullptr)
        *comm_cart = MPI_COMM_NULL;
      return;
    }

    nnodes_ = newSize;

    //  FIXME : code duplication... See coords
    int nranks = newSize;
    for (int i=0; i<ndims; i++) {
      dims_[i] = dims[i];
     periodic_[i] = periods[i];
      nranks = nranks / dims[i];
      /* FIXME: nranks could be zero (?) */
      position_[i] = rank / nranks;
      rank = rank % nranks;
    }
    
    if(comm_cart != nullptr){
      const Group* oldGroup = comm_old->group();
      auto* newGroup        = new Group(newSize);
      for (int i = 0 ; i < newSize ; i++) {
        newGroup->set_mapping(oldGroup->actor_pid(i), i);
      }
      *comm_cart = new  Comm(newGroup, std::shared_ptr<Topo>(this));
    }
  } else {
    if(comm_cart != nullptr){
      if (rank == 0) {
        auto* group = new Group(MPI_COMM_SELF->group());
        *comm_cart  = new Comm(group, std::shared_ptr<Topo>(this));
      } else {
        *comm_cart = MPI_COMM_NULL;
      }
    }
  }
  if(comm_cart != nullptr){
    setComm(*comm_cart);
  }
}

Topo_Cart* Topo_Cart::sub(const int remain_dims[], MPI_Comm *newcomm) {
  int oldNDims = ndims_;
  std::vector<int> newDims;
  std::vector<int> newPeriodic;

  if (remain_dims == nullptr && oldNDims != 0) {
    return nullptr;
  }
  int newNDims = 0;
  for (int i = 0 ; i < oldNDims ; i++) {
    if (remain_dims[i])
      newNDims++;
  }

  if (newNDims > 0) {
    newDims.resize(newNDims);
    newPeriodic.resize(newNDims);

    // that should not segfault
    int j = 0;
    for (int i = 0; i < oldNDims; i++) {
      if(remain_dims[i]) {
        newDims[j] =dims_[i];
        newPeriodic[j] =periodic_[i];
        j++;
      }
    }
  }

  //split into several communicators
  int color = 0;
  for (int i = 0; i < oldNDims; i++) {
    if (not remain_dims[i]) {
      color = (color * dims_[i] + position_[i]);
    }
  }
  Topo_Cart* res;
  if (newNDims == 0){
    res = new Topo_Cart(getComm(), newNDims, newDims.data(), newPeriodic.data(), 0, newcomm);
  } else {
    *newcomm = getComm()->split(color, getComm()->rank());
    auto topo = std::make_shared<Topo_Cart>(getComm(), newNDims, newDims.data(), newPeriodic.data(), 0, nullptr);
    res       = topo.get();
    res->setComm(*newcomm);
    (*newcomm)->set_topo(topo);
  }
  return res;
}

int Topo_Cart::coords(int rank, int /*maxdims*/, int coords[])
{
  int nnodes = nnodes_;
  for (int i = 0; i< ndims_; i++ ) {
    nnodes    = nnodes /dims_[i];
    coords[i] = rank / nnodes;
    rank      = rank % nnodes;
  }
  return MPI_SUCCESS;
}

int Topo_Cart::get(int maxdims, int* dims, int* periods, int* coords) {
  int ndims=ndims_ < maxdims ?ndims_ : maxdims;
  for(int i = 0 ; i < ndims ; i++) {
    dims[i] =dims_[i];
    periods[i] =periodic_[i];
    coords[i] =position_[i];
  }
  return MPI_SUCCESS;
}

int Topo_Cart::rank(const int* coords, int* rank) {
  int ndims =ndims_;
  *rank = 0;
  int multiplier = 1;

  for (int i=ndims-1; i >=0; i-- ) {
    int coord = coords[i];

    /* The user can give us whatever coordinates he wants. If one of them is out of range, either this dimension is
     * periodic, and we consider the equivalent coordinate inside the bounds, or it's not and then it's an error
     */
    if (coord >=dims_[i]) {
      if (periodic_[i] ) {
        coord = coord %dims_[i];
      } else {
        // Should I do that ?
        *rank = -1;
        return MPI_ERR_ARG;
      }
    } else if (coord <  0) {
      if(periodic_[i]) {
        coord = coord %dims_[i];
        if (coord)
          coord =dims_[i] + coord;
      } else {
        *rank = -1;
        return MPI_ERR_ARG;
      }
    }

    *rank += multiplier * coord;
    multiplier *=dims_[i];
  }
  return MPI_SUCCESS;
}

int Topo_Cart::shift(int direction, int disp, int* rank_source, int* rank_dest)
{
  if(ndims_ == 0) {
    return MPI_ERR_ARG;
  }
  if (ndims_ < direction) {
    return MPI_ERR_DIMS;
  }

  std::vector<int> position(ndims_);
  this->coords(getComm()->rank(), ndims_, position.data());
  position[direction] += disp;

  if(position[direction] < 0 ||
      position[direction] >=dims_[direction]) {
    if(periodic_[direction]) {
      position[direction] %=dims_[direction];
      this->rank(position.data(), rank_dest);
    } else {
      *rank_dest = MPI_PROC_NULL;
    }
  } else {
    this->rank(position.data(), rank_dest);
  }

  position[direction] = position_[direction] - disp;
  if(position[direction] < 0 || position[direction] >=dims_[direction]) {
    if(periodic_[direction]) {
      position[direction] %=dims_[direction];
      this->rank(position.data(), rank_source);
    } else {
      *rank_source = MPI_PROC_NULL;
    }
  } else {
    this->rank(position.data(), rank_source);
  }
  return MPI_SUCCESS;
}

int Topo_Cart::dim_get(int* ndims) const
{
  *ndims =ndims_;
  return MPI_SUCCESS;
}

// Everything below has been taken from ompi, but could be easily rewritten (and partially was to follow sonar rules).

/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2014 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2012      Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2014      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/*
 * This is a utility function, no need to have anything in the lower layer for this at all
 */
int Topo_Cart::Dims_create(int nnodes, int ndims, int dims[])
{
  /* Get # of free-to-be-assigned processes and # of free dimensions */
  int freeprocs = nnodes;
  int freedims = 0;
  for (int i = 0; i < ndims; ++i) {
    if (dims[i] == 0) {
      ++freedims;
    } else if ((dims[i] < 0) || ((nnodes % dims[i]) != 0)) {
      return  MPI_ERR_DIMS;

    } else {
      freeprocs /= dims[i];
    }
  }

  if (freedims == 0) {
    if (freeprocs == 1) {
      return MPI_SUCCESS;
    }
    return MPI_ERR_DIMS;
  }

  if (freeprocs == 1) {
    for (int i = 0; i < ndims; ++i) {
      if (dims[i] == 0) {
        dims[i] = 1;
      }
    }
    return MPI_SUCCESS;
  }

  /* Factor the number of free processes */
  std::vector<int> factors;
  int err = getfactors(freeprocs, factors);
  if (MPI_SUCCESS != err)
    return  err;

  /* Assign free processes to free dimensions */
  std::vector<int> procs;
  err = assignnodes(freedims, factors, procs);
  if (MPI_SUCCESS != err)
    return err;

  /* Return assignment results */
  auto p = procs.begin();
  for (int i = 0; i < ndims; ++i) {
    if (dims[i] == 0) {
      dims[i] = *p++;
    }
  }

  /* all done */
  return MPI_SUCCESS;
}

}
}

/*
 *  assignnodes
 *
 *  Function:   - assign processes to dimensions
 *          - get "best-balanced" grid
 *          - greedy bin-packing algorithm used
 *          - sort dimensions in decreasing order
 *          - dimensions array dynamically allocated
 *  Accepts:    - # of dimensions
 *          - std::vector of prime factors
 *          - reference to std::vector of dimensions (returned value)
 *  Returns:    - 0 or ERROR
 */
static int assignnodes(int ndim, const std::vector<int>& factors, std::vector<int>& dims)
{
  if (0 >= ndim) {
    return MPI_ERR_DIMS;
  }

  /* Allocate and initialize the bins */
  dims.clear();
  dims.resize(ndim, 1);

  /* Loop assigning factors from the highest to the lowest */
  for (auto pfact = factors.crbegin(); pfact != factors.crend(); ++pfact) {
    /* Assign a factor to the smallest bin */
    auto pmin = std::min_element(dims.begin(), dims.end());
    *pmin *= *pfact;
  }

  /* Sort dimensions in decreasing order */
  std::sort(dims.begin(), dims.end(), std::greater<>());

  return MPI_SUCCESS;
}

/*
 *  getfactors
 *
 *  Function:   - factorize a number
 *  Accepts:    - number
 *              - reference to std::vector of prime factors
 *  Returns:    - MPI_SUCCESS or ERROR
 */
static int getfactors(int num, std::vector<int>& factors)
{
  factors.clear();
  if (num < 2) {
    return MPI_SUCCESS;
  }

  /* determine all occurrences of factor 2 */
  while((num % 2) == 0) {
    num /= 2;
    factors.push_back(2);
  }
  /* determine all occurrences of uneven prime numbers up to sqrt(num) */
  int d = 3;
  while ((num > 1) && (d * d < num)) {
    while((num % d) == 0) {
      num /= d;
      factors.push_back(d);
    }
    d += 2;
  }
  /* as we looped only up to sqrt(num) one factor > sqrt(num) may be left over */
  if(num != 1) {
    factors.push_back(num);
  }
  return MPI_SUCCESS;
}
