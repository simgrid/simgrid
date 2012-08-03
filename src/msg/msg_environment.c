/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg_private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
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
      __MSG_host_create((smx_host_t)data[SIMIX_HOST_LEVEL]);
  }
}

/**
 * @ingroup MSG_LUA
 * \brief A platform constructor bypassing the parser.
 *
 * load lua script file to set up new platform, including hosts,links
 * and the routing table
 */

void MSG_load_platform_script(const char *script_file)
{
#ifdef HAVE_LUA
  lua_State *L = lua_open();
  luaL_openlibs(L);

  if (luaL_loadfile(L, script_file) || lua_pcall(L, 0, 0, 0)) {
    printf("error: %s\n", lua_tostring(L, -1));
    return;
  }
#else
  xbt_die
      ("Lua is not available! It is mandatory to use MSG_load_platform_script however. Bailing out...");
#endif
}
