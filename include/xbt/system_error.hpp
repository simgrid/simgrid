/* Copyright (c) 2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cerrno>

#include <system_error>

#ifndef SIMGRID_MC_SYSTEM_ERROR_HPP
#define SIMGRID_MC_SYSTEM_ERROR_HPP

namespace simgrid {
namespace xbt {

/** A `error_category` suitable to be used with `errno`
 *
 *  It is not clear which error we are supposed to generate
 *  when getting a errno:
 *
 *  * `system_error` clearly cannot be used for this on Windows;
 *
 *  * `generic_error` might not be used for non-standard `errno`.
 *
 *  Let's just define a function which gives us the correct
 *  category.
 */
inline
const std::error_category& errno_category() noexcept
{
  return std::generic_category();
}

/** Create a `error_code` from an `errno` value
 *
 *  This is expected to to whatever is right to create a
 *  `error_code` from a given `errno` value.
 */
inline
std::error_code errno_code(int errnum)
{
  return std::error_code(errnum, errno_category());
}

/** Create an `error_code` from `errno` (and clear it) */
inline
std::error_code errno_code()
{
  int errnum = errno;
  errno = 0;
  return errno_code(errnum);
}

/** Create a `system_error` from an `errno` value
 *
 *  This is expected to to whatever is right to create a
 *  `system_error` from a given `errno` value.
 */
inline
std::system_error errno_error(int errnum)
{
  return std::system_error(errno_code(errnum));
}

inline
std::system_error errno_error(int errnum, const char* what)
{
  return std::system_error(errno_code(errnum), what);
}

/** Create a `system_code` from `errno` (and clear it) */
inline
std::system_error errno_error()
{
  return std::system_error(errno_code());
}

inline
std::system_error errno_error(const char* what)
{
  return std::system_error(errno_code(), what);
}

}
}

#endif
