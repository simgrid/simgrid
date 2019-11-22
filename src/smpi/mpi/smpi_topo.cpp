/* Copyright (c) 2014-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smpi/smpi.h"
#include "private.hpp"
#include "smpi_comm.hpp"
#include "smpi_topo.hpp"
#include "xbt/sysdep.h"
#include <cmath>
#include <vector>

/* static functions */
static int assignnodes(int ndim, int nfactor, int *pfacts,int **pdims);
static int getfactors(int num, int *nfators, int **factors);


namespace simgrid{
namespace smpi{

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
  MPI_Group newGroup;
  MPI_Group oldGroup;

  int rank = comm_old->rank();

  if(ndims != 0) {
    int newSize = 1;
    for (int i = 0 ; i < ndims ; i++) {
      newSize *= dims[i];
    }
    if(rank >= newSize) {
      *comm_cart = MPI_COMM_NULL;
      return;
    }
    oldGroup = comm_old->group();
    newGroup = new  Group(newSize);
    for (int i = 0 ; i < newSize ; i++) {
      newGroup->set_mapping(oldGroup->actor(i), i);
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

    *comm_cart = new  Comm(newGroup, this);
  } else {
    if (rank == 0) {
      *comm_cart = new  Comm(new  Group(MPI_COMM_SELF->group()), this);
    } else {
      *comm_cart = MPI_COMM_NULL;
    }
  }
  setComm(*comm_cart);
}

Topo_Cart* Topo_Cart::sub(const int remain_dims[], MPI_Comm *newcomm) {
  int oldNDims = ndims_;
  int *newDims = nullptr;
  int *newPeriodic = nullptr;

  if (remain_dims == nullptr && oldNDims != 0) {
    return nullptr;
  }
  int newNDims = 0;
  for (int i = 0 ; i < oldNDims ; i++) {
    if (remain_dims[i])
      newNDims++;
  }

  if (newNDims > 0) {
    newDims     = new int[newNDims];
    newPeriodic = new int[newNDims];

    // that should not segfault
    int j = 0;
    for (int i = 0 ; j < newNDims ; i++) {
      if(remain_dims[i]) {
        newDims[j] =dims_[i];
        newPeriodic[j] =periodic_[i];
        j++;
      }
    }
  }
  Topo_Cart* res = new Topo_Cart(getComm(), newNDims, newDims, newPeriodic, 0, newcomm);
  delete[] newDims;
  delete[] newPeriodic;
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

  int* position = new int[ndims_];
  this->coords(getComm()->rank(), ndims_, position);
  position[direction] += disp;

  if(position[direction] < 0 ||
      position[direction] >=dims_[direction]) {
    if(periodic_[direction]) {
      position[direction] %=dims_[direction];
      this->rank(position, rank_dest);
    } else {
      *rank_dest = MPI_PROC_NULL;
    }
  } else {
    this->rank(position, rank_dest);
  }

  position[direction] = position_[direction] - disp;
  if(position[direction] < 0 || position[direction] >=dims_[direction]) {
    if(periodic_[direction]) {
      position[direction] %=dims_[direction];
      this->rank(position, rank_source);
    } else {
      *rank_source = MPI_PROC_NULL;
    }
  } else {
    this->rank(position, rank_source);
  }
  delete[] position;
  return MPI_SUCCESS;
}

int Topo_Cart::dim_get(int *ndims) {
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
  int *p = dims;
  for (int i = 0; i < ndims; ++i) {
    if (*p == 0) {
      ++freedims;
    } else if ((*p < 0) || ((nnodes % *p) != 0)) {
      return  MPI_ERR_DIMS;

    } else {
      freeprocs /= *p;
    }
    p++;
  }

  if (freedims == 0) {
    if (freeprocs == 1) {
      return MPI_SUCCESS;
    }
    return MPI_ERR_DIMS;
  }

  if (freeprocs == 1) {
    for (int i = 0; i < ndims; ++i) {
      if (*dims == 0) {
        *dims = 1;
      }
      dims++;
    }
    return MPI_SUCCESS;
  }

  /* Factor the number of free processes */
  int nfactors;
  int *factors;
  int err = getfactors(freeprocs, &nfactors, &factors);
  if (MPI_SUCCESS != err)
    return  err;

  /* Assign free processes to free dimensions */
  int *procs;
  err = assignnodes(freedims, nfactors, factors, &procs);
  if (MPI_SUCCESS != err)
    return err;

  /* Return assignment results */
  p = procs;
  for (int i = 0; i < ndims; ++i) {
    if (*dims == 0) {
      *dims = *p++;
    }
    dims++;
  }

  delete[] factors;
  delete[] procs;

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
 *          - # of prime factors
 *          - array of prime factors
 *          - ptr to array of dimensions (returned value)
 *  Returns:    - 0 or ERROR
 */
static int assignnodes(int ndim, int nfactor, int *pfacts, int **pdims)
{
  int *pmin;

  if (0 >= ndim) {
    return MPI_ERR_DIMS;
  }

  /* Allocate and initialize the bins */
  int *bins = new int[ndim];

  *pdims = bins;
  int *p = bins;

  for (int i = 0 ; i < ndim; ++i) {
    *p = 1;
    p++;
  }
  /* Loop assigning factors from the highest to the lowest */
  for (int j = nfactor - 1; j >= 0; --j) {
    int f = pfacts[j];
    /* Assign a factor to the smallest bin */
    pmin = bins;
    p = pmin + 1;
    for (int i = 1; i < ndim; ++i) {
      if (*p < *pmin) {
        pmin = p;
      }
      p++;
    }
    *pmin *= f;
  }

  /* Sort dimensions in decreasing order (O(n^2) for now) */
  pmin = bins;
  for (int i = 0; i < ndim - 1; ++i) {
    p = pmin + 1;
    for (int j = i + 1; j < ndim; ++j) {
      if (*p > *pmin) {
        int n = *p;
        *p = *pmin;
        *pmin = n;
      }
      p++;
    }
    pmin++;
  }

  return MPI_SUCCESS;
}

/*
 *  getfactors
 *
 *  Function:   - factorize a number
 *  Accepts:    - number
 *          - # prime factors
 *          - array of prime factors
 *  Returns:    - MPI_SUCCESS or ERROR
 */
static int getfactors(int num, int *nfactors, int **factors) {
  if(num  < 2) {
    (*nfactors) = 0;
    (*factors) = nullptr;
    return MPI_SUCCESS;
  }
  /* Allocate the array of prime factors which cannot exceed log_2(num) entries */
  int sqrtnum = ceil(sqrt(double(num)));
  int size = ceil(log(double(num)) / log(2.0));
  *factors = new int[size];

  int i = 0;
  /* determine all occurrences of factor 2 */
  while((num % 2) == 0) {
    num /= 2;
    (*factors)[i++] = 2;
  }
  /* determine all occurrences of uneven prime numbers up to sqrt(num) */
  for(int d = 3; (num > 1) && (d < sqrtnum); d += 2) {
    while((num % d) == 0) {
      num /= d;
      (*factors)[i++] = d;
    }
  }
  /* as we looped only up to sqrt(num) one factor > sqrt(num) may be left over */
  if(num != 1) {
    (*factors)[i++] = num;
  }
  (*nfactors) = i;
  return MPI_SUCCESS;
}

