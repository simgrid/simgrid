/* s4u::Engine Simulation Engine and global functions. */

/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simix.h"
#include "mc/mc.h"
#include "simgrid/s4u/As.hpp"
#include "simgrid/s4u/engine.hpp"

XBT_LOG_NEW_CATEGORY(s4u,"Log channels of the S4U (Simgrid for you) interface");
using namespace simgrid;

s4u::Engine *s4u::Engine::instance_ = nullptr; /* That singleton is awful, but I don't see no other solution right now. */


s4u::Engine::Engine(int *argc, char **argv) {
  xbt_assert(s4u::Engine::instance_ == nullptr, "It is currently forbidden to create more than one instance of s4u::Engine");
  s4u::Engine::instance_ = this;

  SIMIX_global_init(argc, argv);
}

s4u::Engine *s4u::Engine::instance() {
  if (s4u::Engine::instance_ == nullptr)
    new Engine(0,nullptr);
  return s4u::Engine::instance_;
}

double s4u::Engine::getClock()
{
  return SIMIX_get_clock();
}

void s4u::Engine::loadPlatform(const char *platf)
{
  SIMIX_create_environment(platf);
}

void s4u::Engine::registerFunction(const char*name, int (*code)(int,char**))
{
  SIMIX_function_register(name,code);
}
void s4u::Engine::registerDefault(int (*code)(int,char**))
{
  SIMIX_function_register_default(code);
}
void s4u::Engine::loadDeployment(const char *deploy)
{
  SIMIX_launch_application(deploy);
}

void s4u::Engine::run() {
  if (MC_is_active()) {
    MC_run();
  } else {
    SIMIX_run();
  }
}

#include "surf/surf.h"               // routing_platf. FIXME:KILLME. SOON
#include "src/surf/surf_routing.hpp" // routing_platf. FIXME:KILLME. SOON

s4u::As *s4u::Engine::rootAs()
{
  return routing_platf->root_; // FIXME: get the root into the Engine directly (and kill the platf)
}

static s4u::As *asByNameRecursive(s4u::As *current, const char *name)
{
  if(!strcmp(current->name(), name))
    return current;

  xbt_dict_cursor_t cursor = NULL;
  char *key;
  AS_t elem;
  xbt_dict_foreach(current->children(), cursor, key, elem) {
    simgrid::s4u::As *tmp = asByNameRecursive(elem, name);
    if (tmp != NULL )
        return tmp;
  }
  return nullptr;
}
/** @brief Retrieve the AS of the given name (or nullptr if not found) */
s4u::As *s4u::Engine::asByNameOrNull(const char *name) {
  return asByNameRecursive(rootAs(),name);
}

