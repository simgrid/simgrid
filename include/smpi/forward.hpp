/* Copyright (c) 2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SMPI_FORWARD_HPP
#define SIMGRID_SMPI_FORWARD_HPP


#ifdef __cplusplus

#include <boost/intrusive_ptr.hpp>
namespace simgrid {
namespace smpi {

class Group;
class Comm;
class Topo;
class Cart;
class Graph;
class Dist_Graph;

}
}

typedef simgrid::smpi::Group SMPI_Group;
typedef simgrid::smpi::Comm SMPI_Comm;
typedef simgrid::smpi::Topo SMPI_Topology;
typedef simgrid::smpi::Graph SMPI_Graph_topology;
typedef simgrid::smpi::Cart SMPI_Cart_topology;
typedef simgrid::smpi::Dist_Graph SMPI_Dist_Graph_topology;

#else

typedef struct SMPI_Group SMPI_Group;
typedef struct SMPI_Comm SMPI_Comm;
typedef struct SMPI_Topology SMPI_Topology;
typedef struct SMPI_Graph_topology SMPI_Graph_topology;
typedef struct SMPI_Cart_topology SMPI_Cart_topology;
typedef struct SMPI_Dist_Graph_topology SMPI_Dist_Graph_topology;

#endif

#endif
