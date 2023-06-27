/* Copyright (c) 2018-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/engine.h"
#include <xbt.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Logging specific to this test");

int main(int argc, char** argv)
{
  simgrid_init(&argc, argv);

  for (int i = 1; i < argc; i++)
    XBT_INFO("argv[%d]=%s", i, argv[i]);

  return 0;
}
