/* Copyright (c) 2016-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/checker/Checker.hpp"
#include "src/mc/ModelChecker.hpp"
#include "xbt/asserts.h"

namespace simgrid {
namespace mc {

Checker::Checker(Session& s) : session_(&s)
{
  xbt_assert(mc_model_checker);
  xbt_assert(mc_model_checker->getChecker() == nullptr);
  mc_model_checker->setChecker(this);
}

} // namespace mc
} // namespace simgrid
