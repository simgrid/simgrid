/* Copyright (c) 2010-2011, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JEDULE_SD_BINDING_H_
#define JEDULE_SD_BINDING_H_

#include "simgrid_config.h"
#include "simgrid/simdag.h"

#if HAVE_JEDULE
SG_BEGIN_DECL()
XBT_PUBLIC(void) jedule_log_sd_event(SD_task_t task);
XBT_PUBLIC(void) jedule_setup_platform(void);
XBT_PUBLIC(void) jedule_sd_init(void);
XBT_PUBLIC(void) jedule_sd_cleanup(void);
XBT_PUBLIC(void) jedule_sd_exit(void);
XBT_PUBLIC(void) jedule_sd_dump(const char* filename);
SG_END_DECL()
#endif /* JEDULE_SD_BINDING_H_ */

#endif
