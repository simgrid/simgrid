/* Copyright (c) 2015-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionRandom.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_rand, mc_transition, "Logging specific to MC Random transitions");

namespace simgrid {
namespace mc {
std::string RandomTransition::to_string(bool verbose) const
{
  return xbt::string_printf("Random([%d;%d] ~> %d)", min_, max_, times_considered_);
}

RandomTransition::RandomTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::RANDOM, issuer, times_considered)
{
  xbt_assert(stream >> min_ >> max_);
}

} // namespace mc
} // namespace simgrid
