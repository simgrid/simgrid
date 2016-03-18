/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/Checker.hpp"

namespace simgrid {
namespace mc {

Checker::~Checker()
{
}

FunctionalChecker::~FunctionalChecker()
{
}

int FunctionalChecker::run()
{
  return function_(*session_);
}

}
}
