/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/transition/TransitionRandom.hpp"
#include "xbt/asserts.h"
#include "xbt/string.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_trans_rand, mc_transition, "Logging specific to MC Random transitions");

namespace simgrid::mc {
std::string RandomTransition::to_string(bool verbose) const
{
  return xbt::string_printf("Random([%d;%d] ~> %d)", min_, max_, times_considered_);
}

RandomTransition::RandomTransition(aid_t issuer, int times_considered, std::stringstream& stream)
    : Transition(Type::RANDOM, issuer, times_considered)
{
  xbt_assert(stream >> min_ >> max_);
}

bool RandomTransition::reversible_race(const Transition* other) const
{
  xbt_assert(type_ == Type::RANDOM, "Unexpected transition type %s", to_c_str(type_));

  return true; // Random is always enabled
}

} // namespace simgrid::mc
