/* Copyright (c) 2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_SMPI_FORWARD_HPP
#define SIMGRID_SMPI_FORWARD_HPP


#ifdef __cplusplus

#include <boost/intrusive_ptr.hpp>
namespace simgrid {
namespace SMPI {

class Group;
class Comm;

}
}

typedef simgrid::SMPI::Group SMPI_Group;
typedef simgrid::SMPI::Comm SMPI_Comm;

#else

typedef struct SMPI_Group SMPI_Group;
typedef struct SMPI_Comm SMPI_Comm;

#endif

#endif
