/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"

#ifdef HAVE_LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif

/********************************* MSG **************************************/

/** \ingroup msg_simulation
 * \brief A platform constructor.
 *
 * Creates a new platform, including hosts, links and the
 * routing_table. 
 * \param file a filename of a xml description of a platform. This file 
 * follows this DTD :
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

void MSG_post_create_environment(void) {
  xbt_lib_cursor_t cursor;
  void **data;
  char *name;

  /* Initialize MSG hosts */
  xbt_lib_foreach(host_lib, cursor, name, data) {
    if(data[SIMIX_HOST_LEVEL])
      __MSG_host_create(xbt_dict_cursor_get_elm(cursor));
  }
}

msg_as_t MSG_environment_get_routing_root() {
  return surf_AS_get_routing_root();
}

const char *MSG_environment_as_get_name(msg_as_t as) {
  return surf_AS_get_name(as);
}

xbt_dict_t MSG_environment_as_get_routing_sons(msg_as_t as) {
  xbt_dict_t res = surf_AS_get_routing_sons(as);
  return res;
}

const char *MSG_environment_as_get_property_value(msg_as_t as, const char *name)
{
  xbt_dict_t dict = xbt_lib_get_or_null(as_router_lib, MSG_environment_as_get_name(as), ROUTING_PROP_ASR_LEVEL);
  return xbt_dict_get_or_null(dict, name);
}
