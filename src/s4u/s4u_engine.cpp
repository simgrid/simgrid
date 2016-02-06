/* s4u::Engine Simulation Engine and global functions. */

/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simix.h"
#include "mc/mc.h"
#include "simgrid/s4u/engine.hpp"

XBT_LOG_NEW_CATEGORY(s4u,"Log channels of the S4U (Simgrid for you) interface");
using namespace simgrid;

double s4u::Engine::getClock() {
  return SIMIX_get_clock();
}

s4u::Engine::Engine(int *argc, char **argv) {
  SIMIX_global_init(argc, argv);
}

void s4u::Engine::loadPlatform(const char *platf) {
  SIMIX_create_environment(platf);
}

void s4u::Engine::register_function(const char*name, int (*code)(int,char**)) {
  SIMIX_function_register(name,code);
}
void s4u::Engine::register_default(int (*code)(int,char**)) {
  SIMIX_function_register_default(code);
}
void s4u::Engine::loadDeployment(const char *deploy) {
  SIMIX_launch_application(deploy);
}

void s4u::Engine::run() {
  if (MC_is_active()) {
    MC_run();
  } else {
    SIMIX_run();
  }
}
