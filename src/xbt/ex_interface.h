/* ex -- exception handling                                                 */
/* This file is to loaded in any location defining exception handlers       */
/* (such as context.c), to exchange them.        */

/* Copyright (c) 2006-2007, 2009-2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_EX_INTERFACE_H_
#define _XBT_EX_INTERFACE_H_

#include <cstddef>

#include <vector>
#include <string>

#include "xbt/base.h"
#include "xbt/backtrace.h"

namespace simgrid {
namespace xbt {

/* Change raw libc symbols to file names and line numbers */
XBT_PUBLIC(std::vector<std::string>) resolveBacktrace(
  xbt_backtrace_location_t* loc, std::size_t count);

}
}

#endif                          /* _XBT_EX_INTERFACE_H_ */
