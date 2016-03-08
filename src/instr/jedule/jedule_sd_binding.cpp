/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/asserts.h"
#include "xbt/dynar.h"

#include "src/surf/surf_private.h"
#include "surf/surf.h"

#include "simgrid/jedule/jedule_sd_binding.h"
#include "simgrid/jedule/jedule_events.h"
#include "simgrid/jedule/jedule_platform.h"
#include "simgrid/jedule/jedule_output.h"

#include "simgrid/simdag.h"
#include "src/simdag/simdag_private.h"

#include "simgrid/s4u/As.hpp"
#include "simgrid/s4u/engine.hpp"

#include <stdio.h>

#if HAVE_JEDULE

XBT_LOG_NEW_CATEGORY(jedule, "Logging specific to Jedule");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(jed_sd, jedule, "Logging specific to Jedule SD binding");

jedule_t jedule;

void jedule_log_sd_event(SD_task_t task)
{
  jed_event_t event;

  xbt_assert(task != NULL);

  xbt_dynar_t host_list = xbt_dynar_new(sizeof(char*), NULL);

  for(int i=0; i<task->host_count; i++) {
    const char *hostname = sg_host_get_name(task->host_list[i]);
    xbt_dynar_push(host_list, &hostname);
  }

  create_jed_event(&event, (char*)SD_task_get_name(task), task->start_time, task->finish_time,"SD");

  jed_event_add_resources(event, host_list);
  jedule_store_event(event);

  xbt_dynar_free(&host_list);
}

static void create_hierarchy(AS_t current_comp, jed_simgrid_container_t current_container)
{
  xbt_dict_cursor_t cursor = NULL;
  char *key;
  AS_t elem;
  xbt_dict_t routing_sons = current_comp->children();

  if (xbt_dict_is_empty(routing_sons)) {
    // I am no AS
    // add hosts to jedule platform
    xbt_dynar_t table = current_comp->hosts();
    xbt_dynar_t hosts;
    unsigned int dynar_cursor;
    sg_host_t host_elem;

    hosts = xbt_dynar_new(sizeof(char*), NULL);

    xbt_dynar_foreach(table, dynar_cursor, host_elem) {
      xbt_dynar_push_as(hosts, const char*, sg_host_get_name(host_elem));
    }

    jed_simgrid_add_resources(current_container, hosts);
    xbt_dynar_free(&hosts);
    xbt_dynar_free(&table);
  } else {
    xbt_dict_foreach(routing_sons, cursor, key, elem) {
      jed_simgrid_container_t child_container;
      jed_simgrid_create_container(&child_container, elem->name());
      jed_simgrid_add_container(current_container, child_container);
      XBT_DEBUG("name : %s\n", elem->name());
      create_hierarchy(elem, child_container);
    }
  }
}

void jedule_setup_platform()
{
  jed_create_jedule(&jedule);

  AS_t root_comp = simgrid::s4u::Engine::instance()->rootAs();
  XBT_DEBUG("root name %s\n", root_comp->name());

  jed_simgrid_container_t root_container;
  jed_simgrid_create_container(&root_container, root_comp->name());
  jedule->root_container = root_container;

  create_hierarchy(root_comp, root_container);
}

void jedule_sd_cleanup()
{
  jedule_cleanup_output();
}

void jedule_sd_init()
{
  jedule_init_output();
}

void jedule_sd_exit(void)
{
  if (jedule) {
    jed_free_jedule(jedule);
    jedule = NULL;
  }
}

void jedule_sd_dump(const char * filename)
{
  if (jedule) {
    char *fname;
    FILE *fh;
    if (!filename) {
      fname = bprintf("%s.jed", xbt_binary_name);
    } else {
      fname = xbt_strdup(filename);
    }

    fh = fopen(fname, "w");

    write_jedule_output(fh, jedule, jedule_event_list, NULL);

    fclose(fh);
    free(fname);
  }
}
#endif
