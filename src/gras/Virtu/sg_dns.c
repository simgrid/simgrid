/* sg_dns - name resolution (simulator)                                     */

/* Copyright (c) 2005, 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras/Virtu/virtu_sg.h"

const char *gras_os_myname(void)
{
  return SIMIX_host_self_get_name();
}
