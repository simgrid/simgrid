/* Copyright (c) 2016-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_XBT_LOG_HPP
#define SIMGRID_XBT_LOG_HPP

#include <simgrid/Exception.hpp>
#include <xbt/log.h>

namespace simgrid {
namespace xbt {

/** Display information about an exception
 *
 *  We display: the exception type, name, attached backtraces (if any) and
 *  the nested exception (if any).
 *
 *  @ingroup XBT_ex
 */
XBT_PUBLIC void log_exception(e_xbt_log_priority_t priority, const char* context, std::exception const& exception);

XBT_PUBLIC void install_exception_handler();

} // namespace xbt
} // namespace simgrid

#endif