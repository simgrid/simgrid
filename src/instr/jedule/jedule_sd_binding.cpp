/* Copyright (c) 2010-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/asserts.h"
#include "simgrid/jedule/jedule_sd_binding.h"

#include "simgrid/forward.h"

#include "../../simdag/simdag_private.hpp"
#include "simgrid/jedule/jedule.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/s4u/engine.hpp"

#if HAVE_JEDULE

XBT_LOG_NEW_CATEGORY(jedule, "Logging specific to Jedule");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(jed_sd, jedule, "Logging specific to Jedule SD binding");

jedule_t my_jedule;

void jedule_log_sd_event(SD_task_t task)
{
  xbt_assert(task != nullptr);

  jed_event_t event = new simgrid::jedule::Event(std::string(SD_task_get_name(task)),
                                                 SD_task_get_start_time(task), SD_task_get_finish_time(task), "SD");
  event->addResources(task->allocation);
  my_jedule->event_set.push_back(event);
}

void jedule_sd_init()
{
  AS_t root_comp = simgrid::s4u::Engine::instance()->rootAs();
  XBT_DEBUG("root name %s\n", root_comp->name());

  my_jedule = new simgrid::jedule::Jedule();

  jed_container_t root_container = new simgrid::jedule::Container(std::string(root_comp->name()));
  root_container->createHierarchy(root_comp);
  my_jedule->root_container = root_container;
}

void jedule_sd_exit(void)
{
  delete my_jedule;
}

void jedule_sd_dump(const char * filename)
{
  if (my_jedule) {
    char *fname;
    if (!filename) {
      fname = bprintf("%s.jed", xbt_binary_name);
    } else {
      fname = xbt_strdup(filename);
    }

    FILE *fh = fopen(fname, "w");

    my_jedule->writeOutput(fh);

    fclose(fh);
    xbt_free(fname);
  }
}
#endif
