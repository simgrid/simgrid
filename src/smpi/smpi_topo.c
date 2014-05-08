#include "xbt/sysdep.h"
#include "smpi/smpi.h"
#include "private.h"

#include <math.h>

typedef struct s_smpi_mpi_topology {
  int nnodes;
  int ndims;
  int *dims;
  int *periodic;
  int *position;
} s_smpi_mpi_topology_t;



void smpi_topo_destroy(MPI_Topology topo) {
  if (topo) {
    if(topo->dims) {
      free(topo->dims);
    }
    if(topo->periodic) {
      free(topo->periodic);
    }
    if(topo->position) {
      free(topo->position);
    }
    free(topo);
  }
}

MPI_Topology smpi_topo_create(int ndims) {
  MPI_Topology topo = xbt_malloc(sizeof(*topo));
  topo->nnodes = 0;
  topo->ndims = ndims;
  topo->dims = xbt_malloc(ndims * sizeof(*topo->dims));
  topo->periodic = xbt_malloc(ndims * sizeof(*topo->periodic));
  topo->position = xbt_malloc(ndims * sizeof(*topo->position));
  return topo;
}

/* reorder is ignored, don't know what would be the consequences of a dumb
 * reordering but neither do I see the point of reordering*/
int smpi_mpi_cart_create(MPI_Comm comm_old, int ndims, int dims[],
                     int periods[], int reorder, MPI_Comm *comm_cart) {
  int retval = MPI_SUCCESS;
  int i;
  MPI_Topology topo;
  MPI_Group newGroup, oldGroup;
  int rank, nranks,  newSize;



  rank = smpi_comm_rank(comm_old);

 
 
  newSize = 1;
  if(ndims != 0) {
    topo = smpi_topo_create(ndims);
    for (i = 0 ; i < ndims ; i++) {
      newSize *= dims[i];
    }
    if(rank >= newSize) {
      *comm_cart = MPI_COMM_NULL;
      return retval;
    }
    oldGroup = smpi_comm_group(comm_old);
    newGroup = smpi_group_new(newSize);
    for (i = 0 ; i < newSize ; i++) {
      smpi_group_set_mapping(newGroup, smpi_group_index(oldGroup, i), i);
    }
    
    topo->nnodes = newSize;

    memcpy(topo->dims, dims, ndims * sizeof(*topo->dims));
    memcpy(topo->periodic, periods, ndims * sizeof(*topo->periodic));
    
    //  code duplication... See smpi_mpi_cart_coords
    nranks = newSize;
    for (i=0; i<ndims; i++)
      {
        topo->dims[i]     = dims[i];
        topo->periodic[i] = periods[i];
        nranks = nranks / dims[i];
        /* FIXME: nranks could be zero (?) */
        topo->position[i] = rank / nranks;
        rank = rank % nranks;
      }
    
    *comm_cart = smpi_comm_new(newGroup, topo);      
  }
  else {
    if (rank == 0) {
      topo = smpi_topo_create(ndims);
      *comm_cart = smpi_comm_new(smpi_comm_group(MPI_COMM_SELF), topo);
    }
    else {
      *comm_cart = MPI_COMM_NULL;
    }
  }
  return retval;
}

int smpi_mpi_cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm) {
  MPI_Topology oldTopo = smpi_comm_topo(comm);
  int oldNDims = oldTopo->ndims;
  int i, j = 0, newNDims, *newDims = NULL, *newPeriodic = NULL;
  
  if (remain_dims == NULL && oldNDims != 0) {
    return MPI_ERR_ARG;
  }
  newNDims = 0;
  for (i = 0 ; i < oldNDims ; i++) {
    if (remain_dims[i]) newNDims++;
  }
  
  if (newNDims > 0) {
    newDims = malloc(newNDims * sizeof(*newDims));
    newPeriodic = malloc(newNDims * sizeof(*newPeriodic));

    // that should not segfault
    for (i = 0 ; j < newNDims ; i++) {
      if(remain_dims[i]) {
        newDims[j] = oldTopo->dims[i];
        newPeriodic[j] = oldTopo->periodic[i];
        j++;
      }
    }
  }
  return smpi_mpi_cart_create(comm, newNDims, newDims, newPeriodic, 0, newcomm);
}




int smpi_mpi_cart_coords(MPI_Comm comm, int rank, int maxdims,
                         int coords[]) {
  int nnodes;
  int i;
  MPI_Topology topo = smpi_comm_topo(comm);
  
  nnodes = topo->nnodes;
  for ( i=0; i < topo->ndims; i++ ) {
    nnodes    = nnodes / topo->dims[i];
    coords[i] = rank / nnodes;
    rank      = rank % nnodes;
  }
  return MPI_SUCCESS;
}

int smpi_mpi_cart_get(MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords) {
  MPI_Topology topo = smpi_comm_topo(comm);
  int i;
  for(i = 0 ; i < maxdims ; i++) {
    dims[i] = topo->dims[i];
    periods[i] = topo->periodic[i];
    coords[i] = topo->position[i];
  }
  return MPI_SUCCESS;
}

int smpi_mpi_cart_rank(MPI_Comm comm, int* coords, int* rank) {
  MPI_Topology topo = smpi_comm_topo(comm);
  int ndims = topo->ndims;
  int multiplier, coord,i;
  *rank = 0;
  multiplier = 1;



  for ( i=ndims-1; i >=0; i-- ) {
    coord = coords[i];

    /* Should we check first for args correction, then process,
     * or check while we work (as it is currently done) ? */
      if (coord >= topo->dims[i]) {
        if ( topo->periodic[i] ) {
        coord = coord % topo->dims[i];
        }
        else {
          // Should I do that ?
          *rank = -1; 
          return MPI_ERR_ARG;
        }
      }
      else if (coord <  0) {
        if(topo->periodic[i]) {
          coord = coord % topo->dims[i];
          if (coord) coord = topo->dims[i] + coord;
        }
        else {
          *rank = -1;
          return MPI_ERR_ARG;
        }
      }
    
      *rank += multiplier * coord;
      multiplier *= topo->dims[i];
  }
  return MPI_SUCCESS;
}

int smpi_mpi_cart_shift(MPI_Comm comm, int direction, int disp,
                        int *rank_source, int *rank_dest) {
  MPI_Topology topo = smpi_comm_topo(comm);
  int position[topo->ndims];


  if(topo->ndims == 0) {
    return MPI_ERR_ARG;
  }
  if (topo->ndims < direction) {
    return MPI_ERR_DIMS;
  }

  smpi_mpi_cart_coords(comm, smpi_comm_rank(comm), topo->ndims, position);
  position[direction] += disp;

  if(position[direction] < 0 || position[direction] >= topo->dims[direction]) {
    if(topo->periodic[direction]) {
      position[direction] %= topo->dims[direction];
      smpi_mpi_cart_rank(comm, position, rank_dest);
    }
    else {
      *rank_dest = MPI_PROC_NULL;
    }
  }
  else {
    smpi_mpi_cart_rank(comm, position, rank_dest);
  }

  position[direction] =  topo->position[direction] - disp;
  if(position[direction] < 0 || position[direction] >= topo->dims[direction]) {
    if(topo->periodic[direction]) {
      position[direction] %= topo->dims[direction];
      smpi_mpi_cart_rank(comm, position, rank_source);
    }
    else {
      *rank_source = MPI_PROC_NULL;
    }
  }
  else {
    smpi_mpi_cart_rank(comm, position, rank_source);
  }

  return MPI_SUCCESS;
}

int smpi_mpi_cartdim_get(MPI_Comm comm, int *ndims) {
  MPI_Topology topo = smpi_comm_topo(comm);

  *ndims = topo->ndims;
  return MPI_SUCCESS;
}



// Everything below has been taken from ompi, but could be easily rewritten.
 
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


/* static functions */
static int assignnodes(int ndim, int nfactor, int *pfacts,int **pdims);
static int getfactors(int num, int *nfators, int **factors);

/*
 * This is a utility function, no need to have anything in the lower
 * layer for this at all
 */
int smpi_mpi_dims_create(int nnodes, int ndims, int dims[])
{
    int i;
    int freeprocs;
    int freedims;
    int nfactors;
    int *factors;
    int *procs;
    int *p;
    int err;

    /* Get # of free-to-be-assigned processes and # of free dimensions */
    freeprocs = nnodes;
    freedims = 0;
    for (i = 0, p = dims; i < ndims; ++i,++p) {
        if (*p == 0) {
            ++freedims;
        } else if ((*p < 0) || ((nnodes % *p) != 0)) {
          return  MPI_ERR_DIMS;
                 
        } else {
            freeprocs /= *p;
        }
    }

    if (freedims == 0) {
       if (freeprocs == 1) {
          return MPI_SUCCESS;
       }
       return MPI_ERR_DIMS;
    }

    if (freeprocs == 1) {
        for (i = 0; i < ndims; ++i, ++dims) {
            if (*dims == 0) {
               *dims = 1;
            }
        }
        return MPI_SUCCESS;
    }

    /* Factor the number of free processes */
    if (MPI_SUCCESS != (err = getfactors(freeprocs, &nfactors, &factors))) {
      return  err;
    }

    /* Assign free processes to free dimensions */
    if (MPI_SUCCESS != (err = assignnodes(freedims, nfactors, factors, &procs))) {
      return err;
    }

    /* Return assignment results */
    p = procs;
    for (i = 0; i < ndims; ++i, ++dims) {
        if (*dims == 0) {
           *dims = *p++;
        }
    }

    free((char *) factors);
    free((char *) procs);

    /* all done */
    return MPI_SUCCESS;
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
static int
assignnodes(int ndim, int nfactor, int *pfacts, int **pdims)
{
    int *bins;
    int i, j;
    int n;
    int f;
    int *p;
    int *pmin;
          
    if (0 >= ndim) {
       return MPI_ERR_DIMS;
    }

    /* Allocate and initialize the bins */
    bins = (int *) malloc((unsigned) ndim * sizeof(int));
    if (NULL == bins) {
       return MPI_ERR_NO_MEM;
    }
    *pdims = bins;

    for (i = 0, p = bins; i < ndim; ++i, ++p) {
        *p = 1;
     }
    
    /* Loop assigning factors from the highest to the lowest */
    for (j = nfactor - 1; j >= 0; --j) {
        f = pfacts[j];
        /* Assign a factor to the smallest bin */
        pmin = bins;
        for (i = 1, p = pmin + 1; i < ndim; ++i, ++p) {
            if (*p < *pmin) {
                pmin = p;
            }
        }
        *pmin *= f;
     }
    
     /* Sort dimensions in decreasing order (O(n^2) for now) */
     for (i = 0, pmin = bins; i < ndim - 1; ++i, ++pmin) {
         for (j = i + 1, p = pmin + 1; j < ndim; ++j, ++p) {
             if (*p > *pmin) {
                n = *p;
                *p = *pmin;
                *pmin = n;
             }
         }
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
static int
getfactors(int num, int *nfactors, int **factors) {
    int size;
    int d;
    int i;
    int sqrtnum;

    if(num  < 2) {
        (*nfactors) = 0;
        (*factors) = NULL;
        return MPI_SUCCESS;
    }
    /* Allocate the array of prime factors which cannot exceed log_2(num) entries */
    sqrtnum = ceil(sqrt(num));
    size = ceil(log(num) / log(2));
    *factors = (int *) malloc((unsigned) size * sizeof(int));

    i = 0;
    /* determine all occurences of factor 2 */
    while((num % 2) == 0) {
        num /= 2;
        (*factors)[i++] = 2;
    }
    /* determine all occurences of uneven prime numbers up to sqrt(num) */
    d = 3;
    for(d = 3; (num > 1) && (d < sqrtnum); d += 2) {
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

