/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/ClockVector.hpp"

namespace simgrid::mc {

ClockVector ClockVector::max(const ClockVector& cv1, const ClockVector& cv2)
{
  auto max_vector = ClockVector();

  for (const auto& [aid, value] : cv1.contents)
    max_vector[aid] = std::max(value, cv2.get(aid).value_or(0));

  for (const auto& [aid, value] : cv2.contents)
    max_vector[aid] = std::max(value, cv1.get(aid).value_or(0));

  return max_vector;
}

} // namespace simgrid::mc