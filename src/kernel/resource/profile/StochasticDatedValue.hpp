/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_PROFILE_STOCHASTICDATEDVALUE
#define SIMGRID_KERNEL_PROFILE_STOCHASTICDATEDVALUE

#include "simgrid/forward.h"
#include "src/kernel/resource/profile/DatedValue.hpp"
#include <vector>

namespace simgrid {
namespace kernel {
namespace profile {

enum class Distribution { EXP, NORM, UNIF, DET };

class XBT_PUBLIC StochasticDatedValue {
public:
  Distribution date_law = Distribution::DET;
  std::vector<double> date_params;
  Distribution value_law = Distribution::DET;
  std::vector<double> value_params;
  DatedValue get_datedvalue() const;
  double get_date() const;
  double get_value() const;
  explicit StochasticDatedValue() = default;
  explicit StochasticDatedValue(double d, double v) : date_params({d}), value_params({v}) {}
  explicit StochasticDatedValue(Distribution dl, const std::vector<double>& dp, Distribution vl,
                                const std::vector<double>& vp)
      : date_law(dl), date_params(dp), value_law(vl), value_params(vp)
  {
  }
  bool operator==(StochasticDatedValue const& e2) const;

private:
  static double draw(Distribution law, std::vector<double> params);
};

} // namespace profile
} // namespace kernel
} // namespace simgrid

#endif
