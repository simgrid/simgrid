/* Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_XML_PARSE_HPP
#define SIMGRID_KERNEL_XML_PARSE_HPP

#include <xbt/base.h>
#include <xbt/signal.hpp>
#include <vector>

/* Module management functions */
XBT_PUBLIC void sg_platf_parser_finalize();

XBT_PUBLIC void simgrid_parse_open(const std::string& file);
XBT_PUBLIC void simgrid_parse_close();
XBT_PUBLIC void simgrid_parse_assert(bool cond, const std::string& msg);
XBT_ATTRIB_NORETURN XBT_PUBLIC void simgrid_parse_error(const std::string& msg);
XBT_PUBLIC void simgrid_parse_assert_netpoint(const std::string& hostname, const std::string& pre,
                                              const std::string& post);

XBT_PUBLIC double simgrid_parse_get_double(const std::string& s);
XBT_PUBLIC int simgrid_parse_get_int(const std::string& s);

XBT_PUBLIC void simgrid_parse(bool fire_on_platform_created_callback); /* Entry-point to the parser */
XBT_PUBLIC void parse_platform_file(const std::string& file);

#endif
