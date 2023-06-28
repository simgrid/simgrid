/* module - modularize the code                                             */

/* Copyright (c) 2004-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef XBT_MODULE_H
#define XBT_MODULE_H

// Deprecation warning on include (remove entire file with XBT_ATTRIB_DEPRECATED_v338)
#warning xbt/module.h is deprecated and will be removed after v3.37.

#include <simgrid/engine.h>
#include <xbt/base.h>

SG_BEGIN_DECL

XBT_ATTRIB_DEPRECATED_v338("Please use simgrid_init(&argc, argv) instead") static void xbt_init(int* argc, char** argv)
{
  simgrid_init(argc, argv);
}

SG_END_DECL

#endif /* XBT_MODULE_H */
