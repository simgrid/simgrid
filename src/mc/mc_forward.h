/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_FORWARD_H
#define SIMGRID_MC_FORWARD_H

#include <xbt/misc.h>
#include <mc/datatypes.h>

#ifdef __cplusplus

#define MC_OVERRIDE

namespace simgrid {
namespace mc {

class PageStore;
class ModelChecker;
class AddressSpace;
class Process;
class Snapshot;
class ObjectInformation;
class Type;

}
}

typedef ::simgrid::mc::ModelChecker s_mc_model_checker_t;
typedef ::simgrid::mc::PageStore s_mc_pages_store_t;
typedef ::simgrid::mc::AddressSpace s_mc_address_space_t;
typedef ::simgrid::mc::Process s_mc_process_t;
typedef ::simgrid::mc::Snapshot s_mc_snapshot_t;
typedef ::simgrid::mc::ObjectInformation s_mc_object_info_t;
typedef ::simgrid::mc::Type s_mc_type_t;

#else

typedef struct _s_mc_model_checker s_mc_model_checker_t;
typedef struct _s_mc_pages_store s_mc_pages_store_t;
typedef struct _s_mc_address_space_t s_mc_address_space_t;
typedef struct _s_mc_process_t s_mc_process_t;
typedef struct _s_mc_snapshot_t s_mc_snapshot_t;
typedef struct _s_mc_object_info_t s_mc_object_info_t;
typedef struct _s_mc_type_t s_mc_type_t;

#endif

typedef struct s_memory_map s_memory_map_t, *memory_map_t;
typedef struct s_dw_variable s_dw_variable_t, *dw_variable_t;
typedef struct s_dw_frame s_dw_frame_t, *dw_frame_t;

typedef s_mc_pages_store_t *mc_pages_store_t;
typedef s_mc_model_checker_t *mc_model_checker_t;
typedef s_mc_address_space_t *mc_address_space_t;
typedef s_mc_process_t *mc_process_t;
typedef s_mc_snapshot_t *mc_snapshot_t;
typedef s_mc_object_info_t *mc_object_info_t;
typedef s_mc_type_t *mc_type_t;

SG_BEGIN_DECL()
extern mc_model_checker_t mc_model_checker;
SG_END_DECL()

#endif
