/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "src/msg/msg_private.hpp"

#if SIMGRID_HAVE_LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif

extern "C" {

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

msg_netzone_t MSG_zone_get_root()
{
  return simgrid::s4u::Engine::getInstance()->getNetRoot();
}

const char* MSG_zone_get_name(msg_netzone_t netzone)
{
  return netzone->getCname();
}

msg_netzone_t MSG_zone_get_by_name(const char* name)
{
  return simgrid::s4u::Engine::getInstance()->getNetzoneByNameOrNull(name);
}

void MSG_zone_get_sons(msg_netzone_t netzone, xbt_dict_t whereto)
{
  for (auto const& elem : *netzone->getChildren()) {
    xbt_dict_set(whereto, elem->getCname(), static_cast<void*>(elem), nullptr);
  }
}

const char* MSG_zone_get_property_value(msg_netzone_t netzone, const char* name)
{
  return netzone->getProperty(name);
}

void MSG_zone_set_property_value(msg_netzone_t netzone, const char* name, char* value)
{
  netzone->setProperty(name, value);
}

void MSG_zone_get_hosts(msg_netzone_t netzone, xbt_dynar_t whereto)
{
  /* converts vector to dynar */
  std::vector<simgrid::s4u::Host*> hosts;
  netzone->getHosts(&hosts);
  for (auto const& host : hosts)
    xbt_dynar_push(whereto, &host);
}
}
