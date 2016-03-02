/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cerrno>

#include <system_error>

namespace simgrid {
namespace xbt {

inline
const std::error_category& errno_category() noexcept
{
  return std::generic_category();
}

inline
std::system_error errno_error(int errnum)
{
  return std::system_error(errnum, errno_category());
}

inline
std::system_error errno_error(int errnum, const char* what)
{
  return std::system_error(errnum, errno_category(), what);
}

}
}
