/* Copyright (c) 2015-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/string.hpp>
#include <xbt/sysdep.h>

namespace simgrid::xbt {

std::string string_vprintf(const char *fmt, va_list ap)
{
  // Get the size:
  va_list ap2;
  va_copy(ap2, ap);
  int size = std::vsnprintf(nullptr, 0, fmt, ap2);
  va_end(ap2);
  xbt_assert(size >= 0, "string_vprintf error");

  // Allocate the string and format:
  std::string res;
  res.resize(size);
  if (size != 0)
    xbt_assert(std::vsnprintf(&res[0], size + 1, fmt, ap) == size, "string_vprintf error");
  return res;
}

std::string string_printf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  std::string res;
  try {
    res = string_vprintf(fmt, ap);
  }
  catch(...) {
    va_end(ap);
    throw;
  }
  va_end(ap);
  return res;
}

} // namespace simgrid::xbt
