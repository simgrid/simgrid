/* Copyright (c) 2004-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/s4u/engine.hpp"
#include "src/msg/msg_private.h"

#if HAVE_LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif

SG_BEGIN_DECL()

/********************************* MSG **************************************/

/** \ingroup msg_simulation
 * \brief A platform constructor.
 *
 * Creates a new platform, including hosts, links and the routing_table.
 * \param file a filename of a xml description of a platform. This file follows this DTD :
 *
 *     \include simgrid.dtd
 *
 * Here is a small example of such a platform 
 *
 *     \include small_platform.xml
 *
 * Have a look in the directory examples/msg/ to have a big example.
 */
void MSG_create_environment(const char *file)
{
  SIMIX_create_environment(file);
}

void MSG_post_create_environment() {
  xbt_lib_cursor_t cursor;
  void **data;
  char *name;

  /* Initialize MSG storages */
  xbt_lib_foreach(storage_lib, cursor, name, data) {
    if(data[SIMIX_STORAGE_LEVEL])
      __MSG_storage_create(xbt_dict_cursor_get_elm(cursor));
  }
}

msg_netzone_t MSG_environment_get_routing_root()
{
  return simgrid::s4u::Engine::instance()->netRoot();
}

const char* MSG_environment_as_get_name(msg_netzone_t netzone)
{
  return netzone->name();
}

msg_netzone_t MSG_environment_as_get_by_name(const char* name)
{
  return simgrid::s4u::Engine::instance()->netzoneByNameOrNull(name);
}

xbt_dict_t MSG_environment_as_get_routing_sons(msg_netzone_t netzone)
{
  return netzone->children();
}

const char* MSG_environment_as_get_property_value(msg_netzone_t netzone, const char* name)
{
  return netzone->property(name);
}

void MSG_environment_as_set_property_value(msg_netzone_t netzone, const char* name, char* value)
{
  netzone->setProperty(name, value);
}

xbt_dynar_t MSG_environment_as_get_hosts(msg_netzone_t netzone)
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(sg_host_t), nullptr);

  for (auto host : netzone->hosts()) {
    xbt_dynar_push(res, &host);
  }

  return res;
}

SG_END_DECL()
