/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_SURFXML_PARSE_HPP
#define SURF_SURFXML_PARSE_HPP

#include <xbt/base.h>
#include <xbt/signal.hpp>
#include <vector>

/* Module management functions */
XBT_PUBLIC void sg_platf_init();
XBT_PUBLIC void sg_platf_exit();

XBT_PUBLIC void surf_parse_open(const std::string& file);
XBT_PUBLIC void surf_parse_close();
XBT_PUBLIC void surf_parse_assert(bool cond, const std::string& msg);
XBT_ATTRIB_NORETURN XBT_PUBLIC void surf_parse_error(const std::string& msg);
XBT_PUBLIC void surf_parse_assert_netpoint(const std::string& hostname, const std::string& pre,
                                           const std::string& post);

XBT_PUBLIC double surf_parse_get_double(const std::string& s);
XBT_PUBLIC int surf_parse_get_int(const std::string& s);

XBT_PUBLIC void surf_parse(); /* Entry-point to the parser */

#endif
