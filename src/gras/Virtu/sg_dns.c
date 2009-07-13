/* $Id$ */

/* sg_dns - name resolution (simulator)                                     */

/* Copyright (c) 2005 Martin Quinson. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras/Virtu/virtu_sg.h"

const char *gras_os_myname(void)
{
  smx_host_t host = SIMIX_host_self();
  if (host != NULL)
    return SIMIX_host_get_name(SIMIX_host_self());
  else
    return "";
}
