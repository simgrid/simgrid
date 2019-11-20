/* Copyright (c) 2004-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/profile/StochasticDatedValue.hpp"
#include "src/statmc/rng.hpp"
#include "xbt.h"
#include "xbt/random.hpp"
#include <math.h>

namespace simgrid {
namespace kernel {
namespace profile {

double StochasticDatedValue::draw(Distribution law, std::vector<double> params)
{
  switch (law) {
    case Dist_Det:
      return params[0];
    case Dist_Exp:
      return simgrid::xbt::random::exponential(params[0]);
    case Dist_Unif:
      return simgrid::xbt::random::uniform_real(params[0], params[1]);
    case Dist_Norm:
      return simgrid::xbt::random::normal(params[0], params[1]);
    default:
      xbt_assert(false, "Unimplemented distribution");
      return 0;
  }
}
double StochasticDatedValue::get_value()
{
  return draw(value_law, value_params);
}
double StochasticDatedValue::get_date()
{
  return draw(date_law, date_params);
}
DatedValue StochasticDatedValue::get_datedvalue()
{
  DatedValue event;
  event.date_  = get_date();
  event.value_ = get_value();
  return event;
}

bool StochasticDatedValue::operator==(StochasticDatedValue const& e2) const
{
  return (e2.date_law == date_law) && (e2.value_law == value_law) && (e2.value_params == value_params) &&
         (e2.date_params == date_params);
}

std::ostream& operator<<(std::ostream& out, const StochasticDatedValue& e)
{
  out << e.date_law << " (";
  for (unsigned int i = 0; i < e.date_params.size(); i++) {
    out << e.date_params[i];
    if (i != e.date_params.size() - 1) {
      out << ",";
    }
  }
  out << ") " << e.value_law << " (";
  for (unsigned int i = 0; i < e.value_params.size(); i++) {
    out << e.value_params[i];
    if (i != e.value_params.size() - 1) {
      out << ",";
    }
  }
  out << ")";
  return out;
}

} // namespace profile
} // namespace kernel
} // namespace simgrid
