/* $Id: gras.c 3859 2007-07-18 12:29:51Z donassbr $ */

/* small_sleep.c -- check what happens when the processes do sleeps very shortly*/

/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "gras.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Small sleep test");

int function(int argc, char *argv[]);


int function(int argc, char *argv[])
{
  gras_init(&argc, argv);
  gras_msg_handleall(100);
  INFO0("Let's go 1E-5");
  gras_msg_handleall(1E-5);
  INFO0("Let's go 1E-10");
  gras_msg_handleall(1E-10);
  INFO0("Let's go 1E-15");
  gras_msg_handleall(1E-15);
  INFO0("Let's go 1E-20");
  gras_msg_handleall(1E-20);
  INFO0("done");
  gras_exit();
  return 0;
}
