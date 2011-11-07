/* Copyright (c) 2006, 2007, 2008, 2009, 2010, 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/misc.h"
#include "xbt/log.h"
#include "xbt/str.h"
#include "xbt/dict.h"
#include "simgrid/platf_interface.h"
#include "surf/surf_private.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_parse);
xbt_dynar_t surf_parse_host_cb_list = NULL; // of functions of type: surf_parsing_host_arg_t -> void

/** Module management function: creates all internal data structures */
void sg_platf_init(void) {
  surf_parse_host_cb_list = xbt_dynar_new(sizeof(surf_parse_host_fct_t), NULL);
}
/** Module management function: frees all internal data structures */
void sg_platf_exit(void) {
  xbt_dynar_free(&surf_parse_host_cb_list);
}

void surf_parse_host(surf_parsing_host_arg_t h){
  unsigned int iterator;
  surf_parse_host_fct_t fun;
  xbt_dynar_foreach(surf_parse_host_cb_list, iterator, fun) {
    if (fun) (*fun) (h);
  }
}
void surf_parse_host_add_cb(surf_parse_host_fct_t fct) {
  xbt_dynar_push(surf_parse_host_cb_list, &fct);
}
