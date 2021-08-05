/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/ProfileBuilder.hpp"
#include "src/kernel/resource/profile/Profile.hpp"

namespace simgrid {
namespace kernel {
namespace profile {

Profile* ProfileBuilder::from_string(const std::string& name, const std::string& input, double periodicity)
{
  return Profile::from_string(name, input, periodicity);
}

Profile* ProfileBuilder::from_file(const std::string& path)
{
  return Profile::from_file(path);
}

} // namespace profile
} // namespace kernel
} // namespace simgrid