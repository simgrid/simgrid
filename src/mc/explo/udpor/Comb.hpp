/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_CONFIGURATION_HPP
#define SIMGRID_MC_UDPOR_CONFIGURATION_HPP

#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"
#include "src/xbt/utils/iter/variable_for_loop.hpp"

#include <vector>

namespace simgrid::mc::udpor {

class Spike : public std::vector<const UnfoldingEvent*> {
public:
  Spike(unsigned cap) : std::vector<const UnfoldingEvent*>() { reserve(cap); }
};

class Comb {
public:
  explicit Comb(unsigned k) : k(k) {}
  Comb(const Comb& other)             = default;
  Comb(const Comb&& other)            = default;
  Comb& operator=(const Comb& other)  = default;
  Comb& operator=(const Comb&& other) = default;

  Spike& operator[](unsigned i) { return spikes[i]; }
  const Spike& operator[](unsigned i) const { return spikes[i]; }

  auto combinations_begin() const;
  auto combinations_end() const;
  auto begin() const { return spikes.begin(); }
  auto end() const { return spikes.end(); }

private:
  unsigned k;
  std::vector<Spike> spikes;
};

} // namespace simgrid::mc::udpor
#endif
