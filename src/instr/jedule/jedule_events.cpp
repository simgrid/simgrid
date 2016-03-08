/* Copyright (c) 2010-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbt/dict.h"
#include "xbt/dynar.h"
#include "xbt/asserts.h"

#include "simgrid/jedule/jedule_events.h"
#include "simgrid/jedule/jedule_platform.h"

#if HAVE_JEDULE

void jed_event_add_resources(jed_event_t event, xbt_dynar_t host_selection) {
  xbt_dynar_t resource_subset_list;
  jed_res_subset_t res_set;
  unsigned int i;

  resource_subset_list = xbt_dynar_new(sizeof(jed_res_subset_t), NULL);

  jed_simgrid_get_resource_selection_by_hosts(resource_subset_list, host_selection);
  xbt_dynar_foreach(resource_subset_list, i, res_set)  {
    xbt_dynar_push(event->resource_subsets, &res_set);
  }

  xbt_dynar_free(&resource_subset_list);
}

void jed_event_add_characteristic(jed_event_t event, char *characteristic) {
  xbt_assert( characteristic != NULL );
  xbt_dynar_push(event->characteristics_list, &characteristic);
}

void jed_event_add_info(jed_event_t event, char *key, char *value) {
  char *val_cp;

  xbt_assert(key != NULL);
  xbt_assert(value != NULL);

  val_cp = strdup(value);
  xbt_dict_set(event->info_hash, key, val_cp, NULL);
}

void create_jed_event(jed_event_t *event, char *name, double start_time,
    double end_time, const char *type) {

  *event = xbt_new0(s_jed_event_t,1);
  (*event)->name = xbt_strdup(name);

  (*event)->start_time = start_time;
  (*event)->end_time = end_time;

  (*event)->type = xbt_strdup(type);

  (*event)->resource_subsets = xbt_dynar_new(sizeof(jed_res_subset_t), xbt_free_ref);
  (*event)->characteristics_list = xbt_dynar_new(sizeof(char*), NULL);
  (*event)->info_hash = xbt_dict_new_homogeneous(NULL);
}

void jed_event_free(jed_event_t event) {
  free(event->name);
  free(event->type);

  xbt_dynar_free(&event->resource_subsets);

  xbt_dynar_free(&event->characteristics_list);
  xbt_dict_free(&event->info_hash);

  free(event);
}
#endif
