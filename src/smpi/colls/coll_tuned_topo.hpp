/* Copyright (c) 2013-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 *
 * Additional copyrights may follow
 */

#ifndef MCA_COLL_TUNED_TOPO_H_HAS_BEEN_INCLUDED
#define MCA_COLL_TUNED_TOPO_H_HAS_BEEN_INCLUDED

#include "colls_private.hpp"

#define MAXTREEFANOUT 32

#define COLL_TUNED_COMPUTED_SEGCOUNT(SEGSIZE, TYPELNG, SEGCOUNT)                                                       \
  if (((SEGSIZE) >= (TYPELNG)) && ((SEGSIZE) < ((TYPELNG) * (SEGCOUNT)))) {                                            \
    size_t residual;                                                                                                   \
    (SEGCOUNT) = (int)((SEGSIZE) / (TYPELNG));                                                                         \
    residual   = (SEGSIZE) - (SEGCOUNT) * (TYPELNG);                                                                   \
    if (residual > ((TYPELNG) >> 1))                                                                                   \
      (SEGCOUNT)++;                                                                                                    \
  }

struct ompi_coll_tree_t {
  int32_t tree_root;
  int32_t tree_fanout;
  int32_t tree_bmtree;
  int32_t tree_prev;
  int32_t tree_next[MAXTREEFANOUT];
  int32_t tree_nextsize;
};

ompi_coll_tree_t* ompi_coll_tuned_topo_build_tree(int fanout, MPI_Comm com, int root);
ompi_coll_tree_t* ompi_coll_tuned_topo_build_in_order_bintree(MPI_Comm comm);

ompi_coll_tree_t* ompi_coll_tuned_topo_build_bmtree(MPI_Comm comm, int root);
ompi_coll_tree_t* ompi_coll_tuned_topo_build_in_order_bmtree(MPI_Comm comm, int root);
ompi_coll_tree_t* ompi_coll_tuned_topo_build_chain(int fanout, MPI_Comm com, int root);

int ompi_coll_tuned_topo_destroy_tree(ompi_coll_tree_t** tree);

/* debugging stuff, will be removed later */
int ompi_coll_tuned_topo_dump_tree(ompi_coll_tree_t* tree, int rank);

#endif /* MCA_COLL_TUNED_TOPO_H_HAS_BEEN_INCLUDED */
