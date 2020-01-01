/* Copyright (c) 2004-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/profile/DatedValue.hpp"
#include <math.h>

namespace simgrid {
namespace kernel {
namespace profile {

bool DatedValue::operator==(DatedValue const& e2) const
{
  return (fabs(date_ - e2.date_) < 0.0001) && (fabs(value_ - e2.value_) < 0.0001);
}
std::ostream& operator<<(std::ostream& out, const DatedValue& e)
{
  out << e.date_ << " " << e.value_;
  return out;
}

} // namespace profile
} // namespace kernel
} // namespace simgrid
