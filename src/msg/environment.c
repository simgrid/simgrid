/*     $Id$      */

/* Copyright (c) 2002-2007 Arnaud Legrand.                                  */
/* Copyright (c) 2007 Bruno Donassolo.                                      */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/private.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/dict.h"

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
  if (simix_h == NULL) {
    return NULL;
  } else
    return (m_host_t) simix_h->data;
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

  /* Initialize MSG hosts */
  xbt_dict_foreach(SIMIX_host_get_dict(), c, name, h) {
    __MSG_host_create(h, NULL);
  }
  return;
}
