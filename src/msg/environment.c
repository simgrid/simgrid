/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#ifdef HAVE_LUA
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif
//#endif
/** \defgroup msg_easier_life      Platform and Application management
 *  \brief This section describes functions to manage the platform creation
 *  and the application deployment. You should also have a look at 
 *  \ref MSG_examples  to have an overview of their usage.
 */
/** @addtogroup msg_easier_life
 *    \htmlonly <!-- DOXYGEN_NAVBAR_LABEL="Platforms and Applications" --> \endhtmlonly
 * 
 */

/********************************* MSG **************************************/

/** \ingroup msg_easier_life
 * \brief A name directory service...
 *
 * Finds a m_host_t using its name.
 * \param name the name of an host.
 * \return the corresponding host
 */
m_host_t MSG_get_host_by_name(const char *name)
{
  smx_host_t simix_h = NULL;
  simix_h = SIMIX_host_get_by_name(name);
  
  if (simix_h == NULL)
    return NULL;

  return (m_host_t)SIMIX_host_get_data(simix_h);
}

/** \ingroup msg_easier_life
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
  xbt_dict_cursor_t c;
  smx_host_t h;
  char *name;

  SIMIX_create_environment(file);
  SIMIX_init();

  /* Initialize MSG hosts */
  xbt_dict_foreach(SIMIX_host_get_dict(), c, name, h) {
    __MSG_host_create(h, NULL);
  }
  return;
}

/**
 * \brief A platform constructor bypassing the parser.
 *
 * load lua script file to set up new platform, including hosts,links
 * and the routing table
 */

void MSG_load_platform_script(const char *script_file) {
#ifdef HAVE_LUA
    lua_State *L = lua_open();
    luaL_openlibs(L);

    if (luaL_loadfile(L, script_file) || lua_pcall(L, 0, 0, 0)) {
         printf("error: %s\n", lua_tostring(L, -1));
         return;
       }
#endif
    return;
}
