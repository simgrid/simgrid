/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Host.hpp>
#include <xbt/Extendable.hpp>

#include <set>
#include <string>
#include <vector>

#ifndef SIMDAG_PRIVATE_HPP
#define SIMDAG_PRIVATE_HPP
XBT_PRIVATE bool check_for_cycle(const std::vector<simgrid::s4u::ActivityPtr>& dag);
#endif
