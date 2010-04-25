/* $Id: gras.c 3859 2007-07-18 12:29:51Z donassbr $ */

/* empty_main.c -- check what happens when the processes do nothing         */
/* Thanks to Loris Marshal for reporting a problem in that case             */

/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras.h"

int function(int argc, char *argv[]);

int function(int argc, char *argv[])
{
  gras_init(&argc, argv);
  //  gras_os_sleep(3);
  gras_exit();
  return 0;
}
