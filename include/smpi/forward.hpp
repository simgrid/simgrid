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

using SMPI_Comm                = simgrid::smpi::Comm;
using SMPI_Datatype            = simgrid::smpi::Datatype;
using SMPI_Errhandler          = simgrid::smpi::Errhandler;
using SMPI_File                = simgrid::smpi::File;
using SMPI_Group               = simgrid::smpi::Group;
using SMPI_Info                = simgrid::smpi::Info;
using SMPI_Op                  = simgrid::smpi::Op;
using SMPI_Request             = simgrid::smpi::Request;
using SMPI_Topology            = simgrid::smpi::Topo;
using SMPI_Cart_topology       = simgrid::smpi::Topo_Cart;
using SMPI_Dist_Graph_topology = simgrid::smpi::Topo_Dist_Graph;
using SMPI_Graph_topology      = simgrid::smpi::Topo_Graph;
using SMPI_Win                 = simgrid::smpi::Win;

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
