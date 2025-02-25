/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @file mc_forward.hpp
 *
 *  Forward definitions for MC types
 */

#ifndef SIMGRID_MC_FORWARD_HPP
#define SIMGRID_MC_FORWARD_HPP

#include "simgrid/forward.h"
#include "src/mc/xbt_intrusiveptr.hpp"
#include <deque>

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
class SleepSetState;

using StatePtr = boost::intrusive_ptr<State>;
XBT_PUBLIC void intrusive_ptr_release(State* o);
XBT_PUBLIC void intrusive_ptr_add_ref(State* o);

using stack_t = std::deque<StatePtr>;

} // namespace simgrid::mc

#endif
