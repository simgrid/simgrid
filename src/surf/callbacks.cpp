/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xbt/asserts.h>

#include "src/surf/callbacks.h"
#include "src/surf/surf_interface.hpp"
#include "src/surf/host_interface.hpp"

void surf_on_host_created(void (*callback)(sg_host_t))
{
  simgrid::surf::Host::onCreation.connect([callback](simgrid::surf::Host* host) {
    callback(host->p_host);
  });
}

void surf_on_storage_created(void (*callback)(sg_storage_t))
{
  simgrid::surf::storageCreatedCallbacks.connect([callback](simgrid::surf::Storage* storage) {
    const char* id = storage->getName();
    // TODO, create sg_storage_by_name
    sg_storage_t s = xbt_lib_get_elm_or_null(storage_lib, id);
    xbt_assert(s != NULL, "Storage not found for name %s", id);
    callback(s);
  });
}
