/* Copyright (c) 2010-2011, 2013-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JEDULE_SD_BINDING_H_
#define JEDULE_SD_BINDING_H_

#include "simgrid_config.h"

#include "simdag/private.h"
#include "simdag/datatypes.h"
#include "simdag/simdag.h"

#ifdef HAVE_JEDULE

void jedule_log_sd_event(SD_task_t task);

void jedule_setup_platform(void);

void jedule_sd_init(void);

void jedule_sd_cleanup(void);

void jedule_sd_exit(void);

void jedule_sd_dump(void);

#endif /* JEDULE_SD_BINDING_H_ */


#endif
