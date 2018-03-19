/* s4u::Engine Simulation Engine and global functions. */

/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "instr/instr_interface.h"
#include "mc/mc.h"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/surf/network_interface.hpp"
#include "surf/surf.hpp" // routing_platf. FIXME:KILLME. SOON

XBT_LOG_NEW_CATEGORY(s4u,"Log channels of the S4U (Simgrid for you) interface");

namespace simgrid {
namespace s4u {
xbt::signal<void()> onPlatformCreated;
xbt::signal<void()> onSimulationEnd;
xbt::signal<void(double)> onTimeAdvance;
xbt::signal<void(void)> onDeadlock;

Engine *Engine::instance_ = nullptr; /* That singleton is awful, but I don't see no other solution right now. */

Engine::Engine(int *argc, char **argv) {
  xbt_assert(s4u::Engine::instance_ == nullptr, "It is currently forbidden to create more than one instance of s4u::Engine");
  TRACE_global_init();
  SIMIX_global_init(argc, argv);

  pimpl                  = new kernel::EngineImpl();
  s4u::Engine::instance_ = this;
}

Engine::~Engine()
{
  delete pimpl;
  s4u::Engine::instance_ = nullptr;
}

Engine* Engine::getInstance()
{
  if (s4u::Engine::instance_ == nullptr)
    return new Engine(0, nullptr);
  else
    return s4u::Engine::instance_;
}

void Engine::shutdown() {
  delete s4u::Engine::instance_;
  s4u::Engine::instance_ = nullptr;
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
/** @brief Returns the amount of hosts in the platform */
size_t Engine::getHostCount()
{
  return pimpl->hosts_.size();
}
/** @brief Fills the passed list with all hosts found in the platform
 *  @deprecated Please prefer Engine::getAllHosts()
 */
void XBT_ATTRIB_DEPRECATED_v322("Engine::getHostList() is deprecated in favor of Engine::getAllHosts(). Please switch before v3.22")
Engine::getHostList(std::vector<Host*>* list)
{
  for (auto const& kv : pimpl->hosts_)
    list->push_back(kv.second);
}

/** @brief Returns the list of all hosts found in the platform */
std::vector<Host*> Engine::getAllHosts()
{
  std::vector<Host*> res;
  for (auto const& kv : pimpl->hosts_)
    res.push_back(kv.second);
  return res;
}

void Engine::addHost(std::string name, simgrid::s4u::Host* host)
{
  pimpl->hosts_[name] = host;
}

void Engine::delHost(std::string name)
{
  pimpl->hosts_.erase(name);
}

simgrid::s4u::Host* Engine::hostByName(std::string name)
{
  return pimpl->hosts_.at(name); // Will raise a std::out_of_range if the host does not exist
}

simgrid::s4u::Host* Engine::hostByNameOrNull(std::string name)
{
  auto host = pimpl->hosts_.find(name);
  return host == pimpl->hosts_.end() ? nullptr : host->second;
}

/** @brief Returns the list of all storages found in the platform */
std::vector<Storage*> Engine::getAllStorages()
{
  std::vector<Storage*> res;
  for (auto const& kv : pimpl->storages_)
    res.push_back(kv.second);
  return res;
}

simgrid::s4u::Storage* Engine::storageByName(std::string name)
{
  return pimpl->storages_.at(name); // Will raise a std::out_of_range if the host does not exist
}

simgrid::s4u::Storage* Engine::storageByNameOrNull(std::string name)
{
  auto storage = pimpl->storages_.find(name);
  return storage == pimpl->storages_.end() ? nullptr : storage->second;
}

void Engine::addStorage(std::string name, simgrid::s4u::Storage* storage)
{
  pimpl->storages_[name] = storage;
}

void Engine::delStorage(std::string name)
{
  pimpl->storages_.erase(name);
}

/** @brief Returns the amount of links in the platform */
size_t Engine::getLinkCount()
{
  return simgrid::surf::LinkImpl::linksCount();
}

/** @brief Fills the passed list with all links found in the platform
 *
 *  @deprecated. Prefer Engine::getAllLinks() */
void XBT_ATTRIB_DEPRECATED_v322("Engine::getLinkList() is deprecated in favor of Engine::getAllLinks(). Please switch before v3.22")
Engine::getLinkList(std::vector<Link*>* list)
{
  simgrid::surf::LinkImpl::linksList(list);
}

/** @brief Returns the list of all links found in the platform */
std::vector<Link*> Engine::getAllLinks()
{
  std::vector<Link*> res;
  simgrid::surf::LinkImpl::linksList(&res);
  return res;
}

void Engine::run() {
  if (MC_is_active()) {
    MC_run();
  } else {
    SIMIX_run();
  }
}

s4u::NetZone* Engine::getNetRoot()
{
  return pimpl->netRoot_;
}

static s4u::NetZone* netzoneByNameRecursive(s4u::NetZone* current, const char* name)
{
  if (not strcmp(current->getCname(), name))
    return current;

  for (auto const& elem : *(current->getChildren())) {
    simgrid::s4u::NetZone* tmp = netzoneByNameRecursive(elem, name);
    if (tmp != nullptr) {
      return tmp;
    }
  }
  return nullptr;
}

/** @brief Retrieve the NetZone of the given name (or nullptr if not found) */
NetZone* Engine::getNetzoneByNameOrNull(const char* name)
{
  return netzoneByNameRecursive(getNetRoot(), name);
}

/** @brief Retrieve the netpoint of the given name (or nullptr if not found) */
simgrid::kernel::routing::NetPoint* Engine::getNetpointByNameOrNull(std::string name)
{
  auto netp = pimpl->netpoints_.find(name);
  return netp == pimpl->netpoints_.end() ? nullptr : netp->second;
}

/** @brief Fill the provided vector with all existing netpoints */
void Engine::getNetpointList(std::vector<simgrid::kernel::routing::NetPoint*>* list)
{
  for (auto const& kv : pimpl->netpoints_)
    list->push_back(kv.second);
}
/** @brief Register a new netpoint to the system */
void Engine::netpointRegister(simgrid::kernel::routing::NetPoint* point)
{
  // simgrid::simix::kernelImmediate([&]{ FIXME: this segfaults in set_thread
  pimpl->netpoints_[point->getName()] = point;
  // });
}
/** @brief Unregister a given netpoint */
void Engine::netpointUnregister(simgrid::kernel::routing::NetPoint* point)
{
  simgrid::simix::kernelImmediate([this, point] {
    pimpl->netpoints_.erase(point->getName());
    delete point;
  });
}

bool Engine::isInitialized()
{
  return Engine::instance_ != nullptr;
}
void Engine::setConfig(std::string str)
{
  xbt_cfg_set_parse(str.c_str());
}
}
} // namespace
