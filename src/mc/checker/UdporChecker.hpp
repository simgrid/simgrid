/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_CHECKER_HPP
#define SIMGRID_MC_UDPOR_CHECKER_HPP

#include "src/mc/checker/Checker.hpp"
#include "src/mc/mc_record.hpp"

namespace simgrid {
namespace mc {

class XBT_PRIVATE UdporChecker : public Checker {

public:
  explicit UdporChecker();
  ~UdporChecker() override = default;
  void run() override;
  RecordTrace get_record_trace() override;
  std::vector<std::string> get_textual_trace() override;
  void log_state() override;

  
};

} // namespace mc
} // namespace simgrid

#endif