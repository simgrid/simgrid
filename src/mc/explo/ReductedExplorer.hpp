/* Copyright (c) 2008-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_REDUCTED_EXPLORER_HPP
#define SIMGRID_MC_REDUCTED_EXPLORER_HPP

#include "src/mc/explo/Exploration.hpp"
#include "src/mc/explo/reduction/Reduction.hpp"

#include <memory>

namespace simgrid::mc {

class XBT_PRIVATE ReductedExplorer : public Exploration {
public:
  explicit ReductedExplorer(std::unique_ptr<Reduction> reduction) : reduction_(std::move(reduction)) {}
  explicit ReductedExplorer(std::unique_ptr<RemoteApp> remote_app, std::unique_ptr<Reduction> reduction)
      : Exploration(std::move(remote_app)), reduction_(std::move(reduction))
  {
  }

  std::unique_ptr<Reduction> reduction_;
};

} // namespace simgrid::mc

#endif
