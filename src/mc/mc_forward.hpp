/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file mc_forward.hpp
 *
 *  Forward definitions for MC types
 */

#ifndef SIMGRID_MC_FORWARD_HPP
#define SIMGRID_MC_FORWARD_HPP

#ifndef __has_feature
  #define __has_feature(x) 0
#endif

#if __has_feature(cxx_override_control)
  #define MC_OVERRIDE override
#else
  #define MC_OVERRIDE
#endif

namespace simgrid {
namespace mc {

class PageStore;
class ModelChecker;
class AddressSpace;
class Process;
class Snapshot;
class ObjectInformation;
class Type;
class Variable;
class Frame;

}
}

// TODO, try to get rid of the global ModelChecker variable
extern simgrid::mc::ModelChecker* mc_model_checker;

#endif
