/* Copyright (c) 2018-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ker_actor);

namespace simgrid {

// DO NOT define destructors for exceptions in Exception.hpp.
// Defining it here ensures that the exceptions are defined only in libsimgrid, but not in libsimgrid-java.
// Doing otherwise naturally breaks things (at least on freebsd with clang).

Exception::~Exception()                             = default;
TimeoutException::~TimeoutException()               = default;
HostFailureException::~HostFailureException()       = default;
NetworkFailureException::~NetworkFailureException() = default;
StorageFailureException::~StorageFailureException() = default;
VmFailureException::~VmFailureException()           = default;
CancelException::~CancelException()                 = default;
TracingError::~TracingError()                       = default;
ParseError::~ParseError()                           = default;
ForcefulKillException::~ForcefulKillException()     = default;

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
  } catch (ForcefulKillException const& e) {
    XBT_DEBUG("Caught a ForcefulKillException: %s", e.what());
    res = false;
  }
  return res;
}
} // namespace simgrid
