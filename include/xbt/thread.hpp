/* Copyright (c) 2014-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_THREAD_HPP
#define SIMGRID_XBT_THREAD_HPP

#include <string>

namespace simgrid::xbt {

/** Return a textual representation of the current thread (because gettid() does not exist on Mac OSX) */
std::string& gettid();

}; // namespace simgrid::xbt

#endif