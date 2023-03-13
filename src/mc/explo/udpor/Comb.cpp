/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Comb.hpp"

#include <functional>

namespace simgrid::mc::udpor {

auto Comb::combinations_begin() const
{
  std::vector<std::reference_wrapper<const Spike>> references;
  std::transform(spikes.begin(), spikes.end(), std::back_inserter(references),
                 [](const Spike& spike) { return std::cref(spike); });
  return simgrid::xbt::variable_for_loop<const Spike>(std::move(references));
}

auto Comb::combinations_end() const
{
  return simgrid::xbt::variable_for_loop<const Spike>();
}

} // namespace simgrid::mc::udpor