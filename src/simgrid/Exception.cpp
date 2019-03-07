/* Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(simix_context);

namespace simgrid {

ForcefulKillException::~ForcefulKillException() = default;

void ForcefulKillException::do_throw()
{
  throw ForcefulKillException();
}

bool ForcefulKillException::try_n_catch(const std::function<void()>& try_block)
{
  bool res;
  try {
    try_block();
    res = true;
  } catch (ForcefulKillException const&) {
    XBT_DEBUG("Caught a ForcefulKillException");
    res = false;
  }
  return res;
}
} // namespace simgrid
