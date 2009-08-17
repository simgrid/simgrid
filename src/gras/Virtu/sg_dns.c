/* $Id$ */

/* sg_dns - name resolution (simulator)                                     */

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras/Virtu/virtu_sg.h"

const char *gras_os_myname(void)
{
  smx_host_t host;
  smx_process_t process = SIMIX_process_self();

  /*HACK: maestro used not have a simix process, now it does so 
    SIMIX_process_self will return something different to NULL. This breaks
    the old xbt_log logic that assumed that NULL was equivalent to maestro,
    thus when printing it searches for maestro host name (which doesn't exists)
    and breaks the logging.
    As a hack we check for maestro by looking to the assigned host, if it is
    NULL then we are sure is maestro
  */
  if (process != NULL && (host = SIMIX_host_self()) != NULL)
      return SIMIX_host_get_name(host);
 
  return ""; 
}
