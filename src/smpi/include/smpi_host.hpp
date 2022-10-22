/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_HOST_HPP_
#define SMPI_HOST_HPP_

#include "smpi_utils.hpp"

#include "simgrid/s4u/Host.hpp"
#include <string>
#include <vector>
#include <xbt/Extendable.hpp>

namespace simgrid::smpi {
static auto factor_lambda(std::vector<double> const& values, double size)
{
  return values[0] + values[1] * static_cast<size_t>(size);
}
class Host {
  utils::FactorSet orecv_{"smpi/or", 0.0, factor_lambda};
  utils::FactorSet osend_{"smpi/os", 0.0, factor_lambda};
  utils::FactorSet oisend_{"smpi/ois", 0.0, factor_lambda};
  s4u::Host* host = nullptr;
  /**
   * @brief Generates warning message if user's config is conflicting (callback vs command line/xml)
   * @param op String with config name (smpi/os, smpi/or, smpi/ois)
   */
  void check_factor_configs(const std::string& op) const;

public:
  static xbt::Extension<s4u::Host, smpi::Host> EXTENSION_ID;

  explicit Host(s4u::Host* ptr);

  double orecv(size_t size, s4u::Host* src, s4u::Host* dst);
  double osend(size_t size, s4u::Host* src, s4u::Host* dst);
  double oisend(size_t size, s4u::Host* src, s4u::Host* dst);
};

} // namespace simgrid::smpi
#endif
