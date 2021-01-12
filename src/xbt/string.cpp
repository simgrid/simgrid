/* Copyright (c) 2015-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/config.h>
#include <xbt/string.hpp>
#include <xbt/sysdep.h>

#include <cstdarg>
#include <cstdio>

namespace simgrid {
namespace xbt {

#if SIMGRID_HAVE_MC

char string::NUL = '\0';

#endif

std::string string_vprintf(const char *fmt, va_list ap)
{
  // Get the size:
  va_list ap2;
  va_copy(ap2, ap);
  int size = std::vsnprintf(nullptr, 0, fmt, ap2);
  va_end(ap2);
  if (size < 0)
    xbt_die("string_vprintf error");

  // Allocate the string and format:
  std::string res;
  res.resize(size);
  if (size != 0 && std::vsnprintf(&res[0], size + 1, fmt, ap) != size)
    xbt_die("string_vprintf error");
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

}
}
