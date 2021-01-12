/* Copyright (c) 2004-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SURF_SURF_PRIVATE_HPP
#define SURF_SURF_PRIVATE_HPP

#include "simgrid/forward.h"
#include "xbt/sysdep.h"

/* Generic functions common to all models */

XBT_PRIVATE FILE* surf_fopen(const std::string& name, const char* mode);
XBT_PRIVATE std::ifstream* surf_ifsopen(const std::string& name);

XBT_PRIVATE void check_disk_attachment();

#endif
