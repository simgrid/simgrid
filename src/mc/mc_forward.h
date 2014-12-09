/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_FORWARD_H
#define MC_FORWARD_H

#include <mc/datatypes.h>
#include "mc_interface.h"

typedef struct s_mc_object_info s_mc_object_info_t, *mc_object_info_t;
typedef struct s_dw_type s_dw_type_t, *dw_type_t;
typedef struct s_memory_map s_memory_map_t, *memory_map_t;
typedef struct s_dw_variable s_dw_variable_t, *dw_variable_t;
typedef struct s_dw_frame s_dw_frame_t, *dw_frame_t;
typedef struct s_mc_pages_store s_mc_pages_store_t, *mc_pages_store_t;
typedef struct s_mc_model_checker s_mc_model_checker_t, *mc_model_checker_t;
extern mc_model_checker_t mc_model_checker;

#endif
