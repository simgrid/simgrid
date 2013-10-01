/* Copyright (c) 2012-2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/config.h"

/*******************************************/
/*** Config Globals **************************/
/*******************************************/
XBT_PUBLIC_DATA(xbt_cfg_t) _sg_cfg_set;
XBT_PUBLIC_DATA(int) _sg_cfg_init_status;
XBT_PUBLIC(int) sg_cfg_get_int(const char* name);
XBT_PUBLIC(double) sg_cfg_get_double(const char* name);
XBT_PUBLIC(char*) sg_cfg_get_string(const char* name);
XBT_PUBLIC(int) sg_cfg_get_boolean(const char* name);
XBT_PUBLIC(void) sg_cfg_get_peer(const char *name, char **peer, int *port);
XBT_PUBLIC(xbt_dynar_t) sg_cfg_get_dynar(const char* name);

void sg_config_init(int *argc, char **argv);
void sg_config_finalize(void);
