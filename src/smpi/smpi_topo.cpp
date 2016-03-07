/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/sysdep.h"
#include "smpi/smpi.h"
#include "private.h"

#include <math.h>

typedef struct s_smpi_mpi_cart_topology {
    int nnodes;
    int ndims;
    int *dims;
    int *periodic;
    int *position;
} s_smpi_mpi_cart_topology_t;

typedef struct s_smpi_mpi_graph_topology {
    int nnodes;
    int nedges;
    int *index;
    int *edges;
} s_smpi_mpi_graph_topology_t;

typedef struct s_smpi_dist_graph_topology {
    int indegree;
    int *in;
    int *in_weights;
    int outdegree;
    int *out;
    int *out_weights;
    int is_weighted;
} s_smpi_mpi_dist_graph_topology_t;

typedef struct s_smpi_mpi_topology { 
    MPIR_Topo_type kind;
    union topo { 
        MPIR_Graph_Topology graph;
        MPIR_Cart_Topology  cart;
        MPIR_Dist_Graph_Topology dist_graph;
    } topo;
} s_smpi_mpi_topology_t;

void smpi_topo_destroy(MPI_Topology topo) {
    if(topo == NULL) {
        return;
    }
    switch (topo->kind) {
    case MPI_GRAPH:
        // Not implemented
        // smpi_graph_topo_destroy(topo->topo.graph);
        break;
    case MPI_CART:
        smpi_cart_topo_destroy(topo->topo.cart);
        break;
    case MPI_DIST_GRAPH:
        // Not implemented
        // smpi_dist_graph_topo_destroy(topo->topo.dist_graph);
        break;
    default:
        return;
        break;
    }
}

MPI_Topology smpi_topo_create(MPIR_Topo_type kind) {
    MPI_Topology newTopo = static_cast<MPI_Topology>(xbt_malloc(sizeof(*newTopo)));
    newTopo->kind = kind;
    // Allocate and initialize the right topo should be done by the caller
    return newTopo;
}

/*******************************************************************************
 * Cartesian topologies
 ******************************************************************************/
void smpi_cart_topo_destroy(MPIR_Cart_Topology cart) {
    if (cart) {
        if(cart->dims) {
            free(cart->dims);
        }
        if(cart->periodic) {
            free(cart->periodic);
        }
        if(cart->position) {
            free(cart->position);
        }
        free(cart);
    }
}

MPI_Topology smpi_cart_topo_create(int ndims) {
    MPI_Topology newTopo = smpi_topo_create(MPI_CART);
    MPIR_Cart_Topology newCart = static_cast<MPIR_Cart_Topology>(xbt_malloc(sizeof(*newCart)));
    newCart->nnodes = 0;
    newCart->ndims = ndims;
    newCart->dims = xbt_new(int, ndims);
    newCart->periodic = xbt_new(int, ndims);
    newCart->position = xbt_new(int, ndims);
    newTopo->topo.cart = newCart;
    return newTopo;
}

/* reorder is ignored, don't know what would be the consequences of a dumb
 * reordering but neither do I see the point of reordering*/
int smpi_mpi_cart_create(MPI_Comm comm_old, int ndims, int dims[], int periods[], int reorder, MPI_Comm *comm_cart) {
    int retval = MPI_SUCCESS;
    int i;
    MPI_Topology newCart;
    MPI_Group newGroup, oldGroup;
    int rank, nranks, newSize;

    rank = smpi_comm_rank(comm_old);

    newSize = 1;
    if(ndims != 0) {
        for (i = 0 ; i < ndims ; i++) {
            newSize *= dims[i];
        }
        if(rank >= newSize) {
            *comm_cart = MPI_COMM_NULL;
            return retval;
        }
        newCart = smpi_cart_topo_create(ndims);
        oldGroup = smpi_comm_group(comm_old);
        newGroup = smpi_group_new(newSize);
        for (i = 0 ; i < newSize ; i++) {
            smpi_group_set_mapping(newGroup, smpi_group_index(oldGroup, i), i);
        }

        newCart->topo.cart->nnodes = newSize;

        /* memcpy(newCart->topo.cart->dims, dims, ndims * sizeof(*newCart->topo.cart->dims)); */
        /* memcpy(newCart->topo.cart->periodic, periods, ndims * sizeof(*newCart->topo.cart->periodic)); */

        //  FIXME : code duplication... See smpi_mpi_cart_coords
        nranks = newSize;
        for (i=0; i<ndims; i++) {
            newCart->topo.cart->dims[i] = dims[i];
            newCart->topo.cart->periodic[i] = periods[i];
            nranks = nranks / dims[i];
            /* FIXME: nranks could be zero (?) */
            newCart->topo.cart->position[i] = rank / nranks;
            rank = rank % nranks;
        }

        *comm_cart = smpi_comm_new(newGroup, newCart);
    } else {
        if (rank == 0) {
            newCart = smpi_cart_topo_create(ndims);
            *comm_cart = smpi_comm_new(smpi_comm_group(MPI_COMM_SELF), newCart);
        } else {
            *comm_cart = MPI_COMM_NULL;
        }
    }
    return retval;
}

int smpi_mpi_cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm) {
    MPI_Topology oldTopo = smpi_comm_topo(comm);
    int oldNDims = oldTopo->topo.cart->ndims;
    int i, j = 0, newNDims, *newDims = NULL, *newPeriodic = NULL;
  
    if (remain_dims == NULL && oldNDims != 0) {
        return MPI_ERR_ARG;
    }
    newNDims = 0;
    for (i = 0 ; i < oldNDims ; i++) {
        if (remain_dims[i]) newNDims++;
    }
  
    if (newNDims > 0) {
        newDims = xbt_new(int, newNDims);
        newPeriodic = xbt_new(int, newNDims);

        // that should not segfault
        for (i = 0 ; j < newNDims ; i++) {
            if(remain_dims[i]) {
                newDims[j] = oldTopo->topo.cart->dims[i];
                newPeriodic[j] = oldTopo->topo.cart->periodic[i];
                j++;
            }
        }
    }
    return smpi_mpi_cart_create(comm, newNDims, newDims, newPeriodic, 0, newcomm);
}

int smpi_mpi_cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]) {
    int nnodes;

    MPI_Topology topo = smpi_comm_topo(comm);

    nnodes = topo->topo.cart->nnodes;
    for (int i=0; i < topo->topo.cart->ndims; i++ ) {
        nnodes    = nnodes / topo->topo.cart->dims[i];
        coords[i] = rank / nnodes;
        rank      = rank % nnodes;
    }
    return MPI_SUCCESS;
}

int smpi_mpi_cart_get(MPI_Comm comm, int maxdims, int* dims, int* periods, int* coords) {
    MPI_Topology topo = smpi_comm_topo(comm);

    for(int i = 0 ; i < maxdims ; i++) {
        dims[i] = topo->topo.cart->dims[i];
        periods[i] = topo->topo.cart->periodic[i];
        coords[i] = topo->topo.cart->position[i];
    }
    return MPI_SUCCESS;
}

int smpi_mpi_cart_rank(MPI_Comm comm, int* coords, int* rank) {
    MPI_Topology topo = smpi_comm_topo(comm);
    int ndims = topo->topo.cart->ndims;
    int multiplier, coord,i;
    *rank = 0;
    multiplier = 1;

    for ( i=ndims-1; i >=0; i-- ) {
        coord = coords[i];

        /* The user can give us whatever coordinates he wants. If one of them is out of range, either this dimension is
         * periodic, and we consider the equivalent coordinate inside the bounds, or it's not and then it's an error
         */
        if (coord >= topo->topo.cart->dims[i]) {
            if ( topo->topo.cart->periodic[i] ) {
                coord = coord % topo->topo.cart->dims[i];
            } else {
                // Should I do that ?
                *rank = -1; 
                return MPI_ERR_ARG;
            }
        } else if (coord <  0) {
            if(topo->topo.cart->periodic[i]) {
                coord = coord % topo->topo.cart->dims[i];
                if (coord) coord = topo->topo.cart->dims[i] + coord;
            } else {
                *rank = -1;
                return MPI_ERR_ARG;
            }
        }

        *rank += multiplier * coord;
        multiplier *= topo->topo.cart->dims[i];
    }
    return MPI_SUCCESS;
}

int smpi_mpi_cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest) {
    MPI_Topology topo = smpi_comm_topo(comm);
    int position[topo->topo.cart->ndims];

    if(topo->topo.cart->ndims == 0) {
        return MPI_ERR_ARG;
    }
    if (topo->topo.cart->ndims < direction) {
        return MPI_ERR_DIMS;
    }

    smpi_mpi_cart_coords(comm, smpi_comm_rank(comm), topo->topo.cart->ndims, position);
    position[direction] += disp;

    if(position[direction] < 0 ||
       position[direction] >= topo->topo.cart->dims[direction]) {
        if(topo->topo.cart->periodic[direction]) {
            position[direction] %= topo->topo.cart->dims[direction];
            smpi_mpi_cart_rank(comm, position, rank_dest);
        } else {
            *rank_dest = MPI_PROC_NULL;
        }
    } else {
        smpi_mpi_cart_rank(comm, position, rank_dest);
    }

    position[direction] =  topo->topo.cart->position[direction] - disp;
    if(position[direction] < 0 || position[direction] >= topo->topo.cart->dims[direction]) {
        if(topo->topo.cart->periodic[direction]) {
            position[direction] %= topo->topo.cart->dims[direction];
            smpi_mpi_cart_rank(comm, position, rank_source);
        } else {
            *rank_source = MPI_PROC_NULL;
        }
    } else {
        smpi_mpi_cart_rank(comm, position, rank_source);
    }

    return MPI_SUCCESS;
}

int smpi_mpi_cartdim_get(MPI_Comm comm, int *ndims) {
    MPI_Topology topo = smpi_comm_topo(comm);

    *ndims = topo->topo.cart->ndims;
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
static int getfactors(int num, int *nfactors, int **factors) {
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
