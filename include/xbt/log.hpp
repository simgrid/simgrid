/* Copyright (c) 2016. The SimGrid Team. All rights reserved.*/

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/log.h>
#include <xbt/exception.hpp>

namespace simgrid {
namespace xbt {

/** Display informations about an exception
 *
 *  We display: the exception type, name, attached backtraces (if any) and
 *  the nested exception (if any).
 *
 *  @ingroup XBT_ex
 */
XBT_PUBLIC(void) logException(
  e_xbt_log_priority_t priority,
  const char* context, std::exception const& exception);

}
}
