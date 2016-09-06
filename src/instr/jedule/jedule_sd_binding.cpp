/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/asserts.h"
#include "xbt/dynar.h"

#include "src/surf/surf_private.h"
#include "surf/surf.h"

#include "simgrid/jedule/jedule_sd_binding.h"
#include "simgrid/simdag.h"
#include "simgrid/s4u/As.hpp"
#include "simgrid/s4u/engine.hpp"

#include <stdio.h>
#include "simgrid/forward.h"

#include "simgrid/jedule/jedule_events.hpp"
#include "simgrid/jedule/jedule_output.hpp"
#include "simgrid/jedule/jedule_platform.hpp"
#include "../../simdag/simdag_private.hpp"

#if HAVE_JEDULE

XBT_LOG_NEW_CATEGORY(jedule, "Logging specific to Jedule");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(jed_sd, jedule, "Logging specific to Jedule SD binding");

jedule_t my_jedule;

void jedule_log_sd_event(SD_task_t task)
{
  xbt_assert(task != nullptr);

  jed_event_t event =
      new simgrid::jedule::Event(std::string(SD_task_get_name(task)), task->start_time, task->finish_time,"SD");
  event->addResources(task->allocation);
  jedule_store_event(event);
}

void jedule_setup_platform()
{
  jed_create_jedule(&my_jedule);

  AS_t root_comp = simgrid::s4u::Engine::instance()->rootAs();
  XBT_DEBUG("root name %s\n", root_comp->name());

  jed_container_t root_container = new simgrid::jedule::Container(std::string(root_comp->name()));
  my_jedule->root_container = root_container;

  root_container->createHierarchy(root_comp);
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
  if (my_jedule) {
    jed_free_jedule(my_jedule);
    my_jedule = nullptr;
  }
}

void jedule_sd_dump(const char * filename)
{
  if (my_jedule) {
    char *fname;
    FILE *fh;
    if (!filename) {
      fname = bprintf("%s.jed", xbt_binary_name);
    } else {
      fname = xbt_strdup(filename);
    }

    fh = fopen(fname, "w");

    write_jedule_output(fh, my_jedule, jedule_event_list, nullptr);

    fclose(fh);
    free(fname);
  }
}
#endif
