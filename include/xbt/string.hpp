/* Copyright (c) 2015-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_STRING_HPP
#define SIMGRID_XBT_STRING_HPP

#include "xbt/base.h"

#include <cstdarg>
#include <cstdlib>
#include <string>

namespace simgrid::xbt {

/** Create a C++ string from a C-style format
 *
 * @ingroup XBT_str
 */
XBT_PUBLIC std::string string_printf(const char* fmt, ...) XBT_ATTRIB_PRINTF(1, 2);

/** Create a C++ string from a C-style format
 *
 * @ingroup XBT_str
 */
XBT_PUBLIC std::string string_vprintf(const char* fmt, va_list ap) XBT_ATTRIB_PRINTF(1, 0);

} // namespace simgrid::xbt
#endif
