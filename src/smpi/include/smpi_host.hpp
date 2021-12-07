/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SMPI_HOST_HPP_
#define SMPI_HOST_HPP_

#include "smpi_utils.hpp"

#include "simgrid/s4u/Host.hpp"
#include <string>
#include <vector>
#include <xbt/Extendable.hpp>

namespace simgrid {
namespace smpi {

class Host {
  std::vector<s_smpi_factor_t> orecv_parsed_values;
  std::vector<s_smpi_factor_t> osend_parsed_values;
  std::vector<s_smpi_factor_t> oisend_parsed_values;
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

}
}
#endif
