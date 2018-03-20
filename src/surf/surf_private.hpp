/* Copyright (c) 2004-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_SURF_PRIVATE_HPP
#define SURF_SURF_PRIVATE_HPP

#include "src/surf/trace_mgr.hpp"
#include "surf/surf.hpp"

extern "C" {

/* Generic functions common to all models */

XBT_PRIVATE FILE* surf_fopen(const char* name, const char* mode);
XBT_PRIVATE std::ifstream* surf_ifsopen(std::string name);

/* The __surf_is_absolute_file_path() returns 1 if
 * file_path is a absolute file path, in the other
 * case the function returns 0.
 */
XBT_PRIVATE int __surf_is_absolute_file_path(const char* file_path);

extern XBT_PRIVATE simgrid::trace_mgr::future_evt_set* future_evt_set;

XBT_PUBLIC void storage_register_callbacks();

XBT_PRIVATE void parse_after_config();

/********** Tracing **********/
/* from surf_instr.c */
void TRACE_surf_host_set_speed(double date, const char* resource, double power);
void TRACE_surf_link_set_bandwidth(double date, const char* resource, double bandwidth);
}

#endif
