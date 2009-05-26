/* 	$Id$	 */

/* Copyright (c) 2005 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _SURF_TIMER_PRIVATE_H
#define _SURF_TIMER_PRIVATE_H

#include "surf_private.h"
#include "xbt/dict.h"

/* typedef struct surf_action_timer_Cas01 { */
/*   s_surf_action_t generic_action; */
/*   lmm_variable_t variable; */
/* } s_surf_action_timer_t, *surf_action_timer_t; */

typedef struct command {
  surf_model_t model;           /* Any such object, added in a trace
                                   should start by this field!!! */
  void *function;
  void *args;
  s_xbt_swag_hookup_t command_set_hookup;
} s_command_t, *command_t;

extern xbt_dict_t command_set;

#endif /* _SURF_TIMER_PRIVATE_H */
