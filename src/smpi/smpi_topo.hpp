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
  MPI_Comm _comm;
};


class Cart: public Topo {
  private:
    int _nnodes;
    int _ndims;
    int *_dims;
    int *_periodic;
    int *_position;
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
    int _nnodes;
    int _nedges;
    int *_index;
    int *_edges;
  public:
    Graph();
    ~Graph();
};

class Dist_Graph: public Topo {
  private:
    int _indegree;
    int *_in;
    int *_in_weights;
    int _outdegree;
    int *_out;
    int *_out_weights;
    int _is_weighted;
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
