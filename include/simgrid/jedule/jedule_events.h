/* Copyright (c) 2010-2012, 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef JEDULE_EVENTS_H_
#define JEDULE_EVENTS_H_

#include "simgrid_config.h"
#include "xbt/dynar.h"
#include "xbt/dict.h"
#include "simgrid/jedule/jedule_platform.h"

#if HAVE_JEDULE
SG_BEGIN_DECL()

struct jed_event {
  int event_id;
  char *name;
  double start_time;
  double end_time;
  char *type;
  xbt_dynar_t resource_subsets;
  xbt_dynar_t characteristics_list; /* just a list of names (strings) */
  xbt_dict_t info_hash;     /* key/value pairs */
};

typedef struct jed_event s_jed_event_t, *jed_event_t;

void create_jed_event(jed_event_t *event, char *name, double start_time, double end_time, const char *type);
void jed_event_free(jed_event_t event);
void jed_event_add_resources(jed_event_t event, xbt_dynar_t host_selection);
void jed_event_add_characteristic(jed_event_t event, char *characteristic);
void jed_event_add_info(jed_event_t event, char *key, char *value);

SG_END_DECL()
#endif

#endif /* JEDULE_EVENTS_H_ */
