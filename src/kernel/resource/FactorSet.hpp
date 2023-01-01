/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef RESOURCE_FACTORSET_HPP
#define RESOURCE_FACTORSET_HPP
#include <xbt/base.h>

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

// Methods used to parse and store the values for timing injections in smpi
struct s_smpi_factor_t {
  size_t factor = 0;
  std::vector<double> values;
};

namespace simgrid::kernel::resource {

class FactorSet {
  const std::string name_;
  std::vector<s_smpi_factor_t> factors_;
  double default_value_;
  const std::function<double(std::vector<double> const&, double)> lambda_;
  bool initialized_ = false;

public:
  // Parse the factor from a string
  FactorSet(
      const std::string& name, double default_value = 1,
      std::function<double(std::vector<double> const&, double)> const& lambda = [](std::vector<double> const& values,
                                                                                   double) { return values.front(); });
  void parse(const std::string& string_values);
  bool is_initialized() const { return initialized_; }
  // Get the default value
  double operator()() const;
  // Get the factor to use for the provided size
  double operator()(double size) const;
};

} // namespace simgrid::kernel::resource
#endif
