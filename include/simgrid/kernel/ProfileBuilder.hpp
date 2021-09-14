/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_PROFILEBUILDER_HPP
#define SIMGRID_KERNEL_PROFILEBUILDER_HPP

#include <simgrid/forward.h>

namespace simgrid {
namespace kernel {
namespace profile {

/**
 * @brief Simple builder for Profile classes.
 *
 * It can be used to create profiles for links, hosts or disks.
 */
class XBT_PUBLIC ProfileBuilder {
public:
  static Profile* from_file(const std::string& path);
  static Profile* from_string(const std::string& name, const std::string& input, double periodicity);
};

} // namespace profile
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_PROFILEBUILDER_HPP */