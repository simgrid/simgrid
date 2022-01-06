/* Copyright (c) 2004-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/profile/StochasticDatedValue.hpp"
#include "xbt.h"
#include "xbt/random.hpp"
#include <math.h>

namespace simgrid {
namespace kernel {
namespace profile {

double StochasticDatedValue::draw(Distribution law, std::vector<double> params)
{
  switch (law) {
    case Distribution::DET:
      return params[0];
    case Distribution::EXP:
      return simgrid::xbt::random::exponential(params[0]);
    case Distribution::UNIF:
      return simgrid::xbt::random::uniform_real(params[0], params[1]);
    case Distribution::NORM:
      return simgrid::xbt::random::normal(params[0], params[1]);
    default:
      xbt_die("Unimplemented distribution");
  }
}

double StochasticDatedValue::get_value() const
{
  return draw(value_law, value_params);
}

double StochasticDatedValue::get_date() const
{
  return draw(date_law, date_params);
}

DatedValue StochasticDatedValue::get_datedvalue() const
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

} // namespace profile
} // namespace kernel
} // namespace simgrid
