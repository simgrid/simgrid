/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>

#include <xbt/asserts.h>

#include "src/mc/Checker.hpp"
#include "src/mc/ModelChecker.hpp"

namespace simgrid {
namespace mc {

Checker::Checker(Session& session) : session_(&session)
{
  xbt_assert(mc_model_checker);
  xbt_assert(mc_model_checker->getChecker() == nullptr);
  mc_model_checker->setChecker(this);
}

Checker::~Checker()
{
}

// virtual
RecordTrace Checker::getRecordTrace()
{
  return {};
}

// virtual
std::vector<std::string> Checker::getTextualTrace()
{
  return {};
}

}
}
