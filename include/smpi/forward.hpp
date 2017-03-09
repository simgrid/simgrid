/* Copyright (c) 2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SMPI_FORWARD_HPP
#define SIMGRID_SMPI_FORWARD_HPP


#ifdef __cplusplus

#include <boost/intrusive_ptr.hpp>
namespace simgrid {
namespace smpi {

class Comm;
class Datatype;
class Group;
class Op;
class Request;
class Topo;
class Topo_Cart;
class Topo_Graph;
class Topo_Dist_Graph;
class Type_Contiguous;
class Type_Hindexed;
class Type_Hvector;
class Type_Indexed;
class Type_Struct;
class Type_Vector;
class Win;

}
}

typedef simgrid::smpi::Comm SMPI_Comm;
typedef simgrid::smpi::Datatype SMPI_Datatype;
typedef simgrid::smpi::Group SMPI_Group;
typedef simgrid::smpi::Op SMPI_Op;
typedef simgrid::smpi::Request SMPI_Request;
typedef simgrid::smpi::Topo SMPI_Topology;
typedef simgrid::smpi::Topo_Cart SMPI_Cart_topology;
typedef simgrid::smpi::Topo_Dist_Graph SMPI_Dist_Graph_topology;
typedef simgrid::smpi::Topo_Graph SMPI_Graph_topology;
typedef simgrid::smpi::Win SMPI_Win;
typedef simgrid::smpi::Type_Contiguous SMPI_Type_Contiguous;
typedef simgrid::smpi::Type_Hindexed SMPI_Type_Hindexed;
typedef simgrid::smpi::Type_Hvector SMPI_Type_Hvector;
typedef simgrid::smpi::Type_Indexed SMPI_Type_Indexed;
typedef simgrid::smpi::Type_Struct SMPI_Type_Struct;
typedef simgrid::smpi::Type_Vector SMPI_Type_Vector;

#else

typedef struct SMPI_Comm SMPI_Comm;
typedef struct SMPI_Datatype SMPI_Datatype;
typedef struct SMPI_Group SMPI_Group;
typedef struct SMPI_Op SMPI_Op;
typedef struct SMPI_Request SMPI_Request;
typedef struct SMPI_Topology SMPI_Topology;
typedef struct SMPI_Win SMPI_Win;
typedef struct SMPI_Graph_topology SMPI_Graph_topology;
typedef struct SMPI_Cart_topology SMPI_Cart_topology;
typedef struct SMPI_Dist_Graph_topology SMPI_Dist_Graph_topology;
typedef struct SMPI_Type_Contiguous SMPI_Type_Contiguous;
typedef struct SMPI_Type_Hindexed SMPI_Type_Hindexed;
typedef struct SMPI_Type_Hvector SMPI_Type_Hvector;
typedef struct SMPI_Type_Indexed SMPI_Type_Indexed;
typedef struct SMPI_Type_Struct SMPI_Type_Struct;
typedef struct SMPI_Type_Vector SMPI_Type_Vector;

#endif

#endif
