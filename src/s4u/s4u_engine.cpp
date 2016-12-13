/* s4u::Engine Simulation Engine and global functions. */

/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_interface.h"
#include "mc/mc.h"
#include "simgrid/s4u/Mailbox.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/s4u/engine.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/s4u/storage.hpp"
#include "simgrid/simix.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/routing/NetZoneImpl.hpp"

#include "src/surf/network_interface.hpp"
#include "src/surf/surf_routing.hpp" // routing_platf. FIXME:KILLME. SOON
#include "surf/surf.h"               // routing_platf. FIXME:KILLME. SOON

XBT_LOG_NEW_CATEGORY(s4u,"Log channels of the S4U (Simgrid for you) interface");

namespace simgrid {
namespace s4u {

Engine *Engine::instance_ = nullptr; /* That singleton is awful, but I don't see no other solution right now. */


Engine::Engine(int *argc, char **argv) {
  xbt_assert(s4u::Engine::instance_ == nullptr, "It is currently forbidden to create more than one instance of s4u::Engine");
  s4u::Engine::instance_ = this;
  pimpl                  = new kernel::EngineImpl();

  TRACE_global_init(argc, argv);
  SIMIX_global_init(argc, argv);
}

Engine::~Engine()
{
  delete pimpl;
}

Engine *Engine::instance() {
  if (s4u::Engine::instance_ == nullptr)
    new Engine(0,nullptr);
  return s4u::Engine::instance_;
}

void Engine::shutdown() {
  delete s4u::Engine::instance_;
  s4u::Engine::instance_ = nullptr;
  delete s4u::Storage::storages_;
}

double Engine::getClock()
{
  return SIMIX_get_clock();
}

void Engine::loadPlatform(const char *platf)
{
  SIMIX_create_environment(platf);
}

void Engine::registerFunction(const char*name, int (*code)(int,char**))
{
  SIMIX_function_register(name,code);
}
void Engine::registerDefault(int (*code)(int,char**))
{
  SIMIX_function_register_default(code);
}
void Engine::loadDeployment(const char *deploy)
{
  SIMIX_launch_application(deploy);
}

void Engine::run() {
  if (MC_is_active()) {
    MC_run();
  } else {
    SIMIX_run();
  }
}

s4u::NetZone* Engine::netRoot()
{
  return pimpl->netRoot_;
}

static s4u::NetZone* netzoneByNameRecursive(s4u::NetZone* current, const char* name)
{
  if(!strcmp(current->name(), name))
    return current;

  xbt_dict_cursor_t cursor = nullptr;
  char *key;
  NetZone_t elem;
  xbt_dict_foreach(current->children(), cursor, key, elem) {
    simgrid::s4u::NetZone* tmp = netzoneByNameRecursive(elem, name);
    if (tmp != nullptr )
        return tmp;
  }
  return nullptr;
}

/** @brief Retrieve the NetZone of the given name (or nullptr if not found) */
NetZone* Engine::netzoneByNameOrNull(const char* name)
{
  return netzoneByNameRecursive(netRoot(), name);
}

}
}
