/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_SURFXML_PARSE_HPP
#define SURF_SURFXML_PARSE_HPP

#include <xbt/signal.hpp>

extern "C" {

/* Module management functions */
XBT_PUBLIC(void) sg_platf_init();;
XBT_PUBLIC(void) sg_platf_exit();

XBT_PUBLIC(void) surf_parse_open(const char *file);
XBT_PUBLIC(void) surf_parse_close();
XBT_PUBLIC(void) surf_parse_assert(bool cond, std::string msg);
XBT_ATTRIB_NORETURN XBT_PUBLIC(void) surf_parse_error(std::string msg);
XBT_PUBLIC(void) surf_parse_assert_netpoint(std::string hostname, std::string pre, std::string post);
XBT_PUBLIC(void) surf_parse_warn(std::string msg);

XBT_PUBLIC(double) surf_parse_get_double(std::string s);
XBT_PUBLIC(int) surf_parse_get_int(std::string s);
XBT_PUBLIC(double) surf_parse_get_time(const char* string, const char* entity_kind, std::string name);
XBT_PUBLIC(double) surf_parse_get_size(const char* string, const char* entity_kind, std::string name);
XBT_PUBLIC(double) surf_parse_get_bandwidth(const char* string, const char* entity_kind, std::string name);
XBT_PUBLIC(double) surf_parse_get_speed(const char* string, const char* entity_kind, std::string name);

XBT_PUBLIC(int) surf_parse(); /* Entry-point to the parser */
}

#endif
