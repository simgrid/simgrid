/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_TOPO_HPP_INCLUDED
#define SMPI_TOPO_HPP_INCLUDED

#include "private.h"


namespace simgrid{
namespace smpi{

class Topo {
  protected:
  MPI_Comm m_comm;
};


class Cart: public Topo {
  private:
    int m_nnodes;
    int m_ndims;
    int *m_dims;
    int *m_periodic;
    int *m_position;
  public:
    Cart(int ndims);
    ~Cart();
    Cart(MPI_Comm comm_old, int ndims, int dims[], int periods[], int reorder, MPI_Comm *comm_cart);
    Cart* sub(const int remain_dims[], MPI_Comm *newcomm) ;
    int coords(int rank, int maxdims, int coords[]) ;
    int get(int maxdims, int* dims, int* periods, int* coords);
    int rank(int* coords, int* rank);
    int shift(int direction, int disp, int *rank_source, int *rank_dest) ;
    int dim_get(int *ndims);
};


class Graph: public Topo {
  private:
    int m_nnodes;
    int m_nedges;
    int *m_index;
    int *m_edges;
  public:
    Graph();
    ~Graph();
};

class Dist_Graph: public Topo {
  private:
    int m_indegree;
    int *m_in;
    int *m_in_weights;
    int m_outdegree;
    int *m_out;
    int *m_out_weights;
    int m_is_weighted;
  public:
    Dist_Graph();
    ~Dist_Graph();
};

/*
 * This is a utility function, no need to have anything in the lower layer for this at all
 */
extern int Dims_create(int nnodes, int ndims, int dims[]);

}
}


#endif
