/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/jedule/jedule.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "src/simdag/simdag_private.hpp"
#include "xbt/virtu.h"

#if SIMGRID_HAVE_JEDULE

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(jed_sd, sd, "Jedule SimDag binding");

jedule_t my_jedule;

void jedule_log_sd_event(const_SD_task_t task)
{
  xbt_assert(task != nullptr);

  simgrid::jedule::Event event(std::string(SD_task_get_name(task)), SD_task_get_start_time(task),
                               SD_task_get_finish_time(task), "SD");
  event.add_resources(*task->allocation);
  my_jedule->add_event(event);
}

void jedule_sd_init()
{
  const_sg_netzone_t root_comp = simgrid::s4u::Engine::get_instance()->get_netzone_root();
  XBT_DEBUG("root name %s\n", root_comp->get_cname());

  my_jedule = new simgrid::jedule::Jedule(root_comp->get_name());
}

void jedule_sd_exit()
{
  delete my_jedule;
}

void jedule_sd_dump(const char * filename)
{
  if (my_jedule) {
    std::string fname;
    if (not filename) {
      fname = simgrid::xbt::binary_name + ".jed";
    } else {
      fname = filename;
    }

    FILE* fh = fopen(fname.c_str(), "w");
    xbt_assert(fh != nullptr, "Failed to open file: %s", fname.c_str());

    my_jedule->write_output(fh);

    fclose(fh);
  }
}
#endif
