/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @file mc_forward.hpp
 *
 *  Forward definitions for MC types
 */

#ifndef SIMGRID_MC_FORWARD_HPP
#define SIMGRID_MC_FORWARD_HPP

namespace simgrid::mc {

class PageStore;
class ChunkedData;
class AddressSpace;
class RemoteProcessMemory;
class Snapshot;
class ObjectInformation;
class Member;
class Type;
class Variable;
class Transition;
class Frame;

class Session;
class Exploration;
} // namespace simgrid::mc

#endif
