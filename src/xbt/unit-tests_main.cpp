/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define CATCH_CONFIG_RUNNER // we supply our own main()

#include "catch.hpp"

#include "xbt/log.h"

int main(int argc, char* argv[])
{
  xbt_log_init(&argc, argv);
  return Catch::Session().run(argc, argv);
}
