/* Copyright (c) 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_PLUGINS_FILE_SYSTEM_H_
#define SIMGRID_PLUGINS_FILE_SYSTEM_H_

#include <simgrid/forward.h>
#include <xbt/base.h>

SG_BEGIN_DECL()

XBT_PUBLIC(void) sg_storage_file_system_init();
XBT_PUBLIC(sg_size_t) sg_storage_get_size_free(sg_storage_t st);
XBT_PUBLIC(sg_size_t) sg_storage_get_size_used(sg_storage_t st);
XBT_PUBLIC(sg_size_t) sg_storage_get_size(sg_storage_t st);

#define MSG_storage_file_system_init() sg_storage_file_system_init()
#define MSG_storage_get_free_size(st) sg_storage_get_size_free(st)
#define MSG_storage_get_used_size(st) sg_storage_get_size_used(st)
#define MSG_storage_get_size(st) sg_storage_get_size(st)

SG_END_DECL()

#endif
