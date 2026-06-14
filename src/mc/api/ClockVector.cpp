/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/api/ClockVector.hpp"
#include "src/mc/api/static_config.hpp"

namespace simgrid::mc {

ClockVector ClockVector::max(const ClockVector& cv1, const ClockVector& cv2)
{
  auto max_vector = ClockVector(cv1);

  for (size_t aid = 0; aid < static_config::max_threads - 1; aid++)
    max_vector[aid] = std::max<Clock>(cv1.get(aid), cv2.get(aid));

  return max_vector;
}

} // namespace simgrid::mc
