/* Copyright (c) 2016-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SMPI_FORWARD_HPP
#define SIMGRID_SMPI_FORWARD_HPP


#ifdef __cplusplus

#include <boost/intrusive_ptr.hpp>
namespace simgrid {
namespace smpi {

class Colls;
class Comm;
class Datatype;
class Errhandler;
class File;
class Group;
class Info;
class Keyval;
class Op;
class ActorExt;
class Request;
class Status;
class Topo;
class Topo_Cart;
class Topo_Graph;
class Topo_Dist_Graph;
class Win;

}
}

typedef simgrid::smpi::Comm SMPI_Comm;
typedef simgrid::smpi::Datatype SMPI_Datatype;
typedef simgrid::smpi::Errhandler SMPI_Errhandler;
typedef simgrid::smpi::File SMPI_File;
typedef simgrid::smpi::Group SMPI_Group;
typedef simgrid::smpi::Info SMPI_Info;
typedef simgrid::smpi::Op SMPI_Op;
typedef simgrid::smpi::Request SMPI_Request;
typedef simgrid::smpi::Topo SMPI_Topology;
typedef simgrid::smpi::Topo_Cart SMPI_Cart_topology;
typedef simgrid::smpi::Topo_Dist_Graph SMPI_Dist_Graph_topology;
typedef simgrid::smpi::Topo_Graph SMPI_Graph_topology;
typedef simgrid::smpi::Win SMPI_Win;

#else

typedef struct SMPI_Comm SMPI_Comm;
typedef struct SMPI_Datatype SMPI_Datatype;
typedef struct SMPI_Errhandler SMPI_Errhandler;
typedef struct SMPI_File SMPI_File;
typedef struct SMPI_Group SMPI_Group;
typedef struct SMPI_Info SMPI_Info;
typedef struct SMPI_Op SMPI_Op;
typedef struct SMPI_Request SMPI_Request;
typedef struct SMPI_Topology SMPI_Topology;
typedef struct SMPI_Win SMPI_Win;

#endif

#endif
