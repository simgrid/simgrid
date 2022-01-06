/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_TOPO_HPP_INCLUDED
#define SMPI_TOPO_HPP_INCLUDED

#include "smpi_comm.hpp"
#include "smpi_status.hpp"
#include <memory>

using MPI_Topology = std::shared_ptr<SMPI_Topology>;

namespace simgrid{
namespace smpi{

class Topo {
  MPI_Comm comm_ = MPI_COMM_NULL;

public:
  virtual ~Topo() = default;
  MPI_Comm getComm() const { return comm_; }
  void setComm(MPI_Comm comm);
};

class Topo_Cart: public Topo {
  int nnodes_ = 0;
  int ndims_;
  std::vector<int> dims_;
  std::vector<int> periodic_;
  std::vector<int> position_;

public:
  explicit Topo_Cart(int ndims);
  Topo_Cart(MPI_Comm comm_old, int ndims, const int dims[], const int periods[], int reorder, MPI_Comm* comm_cart);
  Topo_Cart* sub(const int remain_dims[], MPI_Comm* newcomm);
  int coords(int rank, int maxdims, int coords[]);
  int get(int maxdims, int* dims, int* periods, int* coords);
  int rank(const int* coords, int* rank);
  int shift(int direction, int disp, int* rank_source, int* rank_dest);
  int dim_get(int* ndims) const;
  static int Dims_create(int nnodes, int ndims, int dims[]);
};


class Topo_Graph: public Topo {
  std::vector<int> index_;
  std::vector<int> edges_;
};

class Topo_Dist_Graph: public Topo {
  std::vector<int> in_;
  std::vector<int> in_weights_;
  std::vector<int> out_;
  std::vector<int> out_weights_;
};

}
}


#endif
