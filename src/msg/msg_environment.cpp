/* Copyright (c) 2004-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

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

}
