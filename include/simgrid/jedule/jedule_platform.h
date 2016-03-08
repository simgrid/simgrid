/* Copyright (c) 2010-2012, 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JED_SIMGRID_PLATFORM_H_
#define JED_SIMGRID_PLATFORM_H_

#include "simgrid_config.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"

#if HAVE_JEDULE
SG_BEGIN_DECL()

typedef struct jed_simgrid_container s_jed_simgrid_container_t, *jed_simgrid_container_t;


struct jed_simgrid_container {
  char *name;
  xbt_dynar_t container_children;
  jed_simgrid_container_t parent;
  xbt_dynar_t resource_list;
  xbt_dict_t name2id;
  int last_id;
  int is_lowest;
};

/** selection of a subset of resources from the original set */
struct jed_res_subset {
  jed_simgrid_container_t parent;
  int start_idx; // start idx in resource_list of container
  int nres;      // number of resources spanning starting at start_idx
};

typedef struct jed_res_subset s_jed_res_subset_t, *jed_res_subset_t;

struct jedule_struct {
  jed_simgrid_container_t root_container;
  xbt_dict_t jedule_meta_info;
};

typedef struct jedule_struct s_jedule_t, *jedule_t;

void jed_create_jedule(jedule_t *jedule);
void jed_free_jedule(jedule_t jedule);
void jedule_add_meta_info(jedule_t jedule, char *key, char *value);
void jed_simgrid_create_container(jed_simgrid_container_t *container, const char *name);
void jed_simgrid_add_container(jed_simgrid_container_t parent, jed_simgrid_container_t child);
void jed_simgrid_add_resources(jed_simgrid_container_t parent, xbt_dynar_t host_names);

/**
 * it is assumed that the host_names in the entire system are unique that means that we don't need parent references
 *
 * subset_list must be allocated
 * host_names is the list of host_names associated with an event
 */
void jed_simgrid_get_resource_selection_by_hosts(xbt_dynar_t subset_list, xbt_dynar_t host_names);

/*
  global:
      hash host_id -> container
  container:
      hash host_id -> jed_host_id
      list <- [ jed_host_ids ]
      list <- sort( list )
      list_chunks <- chunk( list )   -> [ 1, 3-5, 7-9 ]
*/

SG_END_DECL()

#endif


#endif /* JED_SIMGRID_PLATFORM_H_ */
