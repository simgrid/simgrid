/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/ClockVector.hpp"

namespace simgrid::mc {

size_t ClockVector::max_size = 3;

ClockVector ClockVector::max(const ClockVector& cv1, const ClockVector& cv2)
{
  auto max_vector = ClockVector(cv1);

  for (size_t aid = 0; aid < cv2.size(); aid++)
    max_vector[aid] = std::max<signed long>(cv2.get(aid).value(), cv1.get(aid).value_or(-1));

  return max_vector;
}

} // namespace simgrid::mc
