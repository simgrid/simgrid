/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_COMB_HPP
#define SIMGRID_MC_UDPOR_COMB_HPP

#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"
#include "src/xbt/utils/iter/variable_for_loop.hpp"

#include <boost/iterator/function_input_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <functional>
#include <vector>

namespace simgrid::mc::udpor {

using Spike = std::vector<const UnfoldingEvent*>;

class Comb {
public:
  explicit Comb(unsigned k) : k(k), spikes(k) {}
  Comb(const Comb& other)            = default;
  Comb(Comb&& other)                 = default;
  Comb& operator=(const Comb& other) = default;
  Comb& operator=(Comb&& other)      = default;

  Spike& operator[](unsigned i) { return spikes[i]; }
  const Spike& operator[](unsigned i) const { return spikes[i]; }

  auto combinations_begin() const
  {
    std::vector<std::reference_wrapper<const Spike>> references;
    std::transform(spikes.begin(), spikes.end(), std::back_inserter(references),
                   [](const Spike& spike) { return std::cref(spike); });
    return simgrid::xbt::variable_for_loop<const Spike>(std::move(references));
  }

  auto combinations_end() const { return simgrid::xbt::variable_for_loop<const Spike>(); }
  auto begin() const { return spikes.begin(); }
  auto end() const { return spikes.end(); }

private:
  unsigned k;
  std::vector<Spike> spikes;
};

} // namespace simgrid::mc::udpor
#endif
