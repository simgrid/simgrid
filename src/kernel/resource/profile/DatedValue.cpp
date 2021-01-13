/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/profile/DatedValue.hpp"
#include <cmath>

namespace simgrid {
namespace kernel {
namespace profile {

bool DatedValue::operator==(DatedValue const& e2) const
{
  return (fabs(date_ - e2.date_) < 0.0001) && (fabs(value_ - e2.value_) < 0.0001);
}

} // namespace profile
} // namespace kernel
} // namespace simgrid
