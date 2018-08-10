/* s4u::Engine Simulation Engine and global functions. */

/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.h"
#include "src/simix/smx_private.hpp" // For access to simix_global->process_list
#include "src/instr/instr_private.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/surf/network_interface.hpp"
#include "surf/surf.hpp" // routing_platf. FIXME:KILLME. SOON

#include <string>

XBT_LOG_NEW_CATEGORY(s4u, "Log channels of the S4U (Simgrid for you) interface");

namespace simgrid {
namespace s4u {
xbt::signal<void()> on_platform_creation;
xbt::signal<void()> on_platform_created;
xbt::signal<void()> on_simulation_end;
xbt::signal<void(double)> on_time_advance;
xbt::signal<void(void)> on_deadlock;

Engine* Engine::instance_ = nullptr; /* That singleton is awful, but I don't see no other solution right now. */

Engine::Engine(int* argc, char** argv)
{
  xbt_assert(s4u::Engine::instance_ == nullptr,
             "It is currently forbidden to create more than one instance of s4u::Engine");
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

/** @brief Retrieve the engine singleton */
Engine* Engine::get_instance()
{
  if (s4u::Engine::instance_ == nullptr)
    return new Engine(0, nullptr);
  else
    return s4u::Engine::instance_;
}

void Engine::shutdown()
{
  delete s4u::Engine::instance_;
  s4u::Engine::instance_ = nullptr;
}

double Engine::get_clock()
{
  return SIMIX_get_clock();
}

void Engine::load_platform(std::string platf)
{
  SIMIX_create_environment(platf);
}

void Engine::register_function(std::string name, int (*code)(int, char**))
{
  SIMIX_function_register(name, code);
}
void Engine::register_function(std::string name, void (*code)(std::vector<std::string>))
{
  SIMIX_function_register(name, code);
}
void Engine::register_default(int (*code)(int, char**))
{
  SIMIX_function_register_default(code);
}
void Engine::load_deployment(std::string deploy)
{
  SIMIX_launch_application(deploy);
}
/** @brief Returns the amount of hosts in the platform */
size_t Engine::get_host_count()
{
  return pimpl->hosts_.size();
}
/** @brief Fills the passed list with all hosts found in the platform
 *  @deprecated Please prefer Engine::getAllHosts()
 */
void Engine::getHostList(std::vector<Host*>* list)
{
  for (auto const& kv : pimpl->hosts_)
    list->push_back(kv.second);
}

std::vector<Host*> Engine::get_all_hosts()
{
  std::vector<Host*> res;
  for (auto const& kv : pimpl->hosts_)
    res.push_back(kv.second);
  return res;
}

std::vector<Host*> Engine::get_filtered_hosts(std::function<bool(Host*)> filter)
{
  std::vector<Host*> hosts;
  for (auto const& kv : pimpl->hosts_) {
    if (filter(kv.second))
      hosts.push_back(kv.second);
  }

  return hosts;
}

void Engine::host_register(std::string name, simgrid::s4u::Host* host)
{
  pimpl->hosts_[name] = host;
}

void Engine::host_unregister(std::string name)
{
  pimpl->hosts_.erase(name);
}

/** @brief Find an host from its name.
 *
 *  @throw std::invalid_argument if the searched host does not exist.
 */
simgrid::s4u::Host* Engine::host_by_name(std::string name)
{
  if (pimpl->hosts_.find(name) == pimpl->hosts_.end())
    throw std::invalid_argument(std::string("Host not found: '") + name + std::string("'"));
  return pimpl->hosts_.at(name);
}

/** @brief Find an host from its name (or nullptr if that host does not exist) */
simgrid::s4u::Host* Engine::host_by_name_or_null(std::string name)
{
  auto host = pimpl->hosts_.find(name);
  return host == pimpl->hosts_.end() ? nullptr : host->second;
}

/** @brief Find a link from its name.
 *
 *  @throw std::invalid_argument if the searched link does not exist.
 */
simgrid::s4u::Link* Engine::link_by_name(std::string name)
{
  if (pimpl->links_.find(name) == pimpl->links_.end())
    throw std::invalid_argument(std::string("Link not found: ") + name);

  return pimpl->links_.at(name);
}

/** @brief Find an link from its name (or nullptr if that link does not exist) */
simgrid::s4u::Link* Engine::link_by_name_or_null(std::string name)
{
  auto link = pimpl->links_.find(name);
  return link == pimpl->links_.end() ? nullptr : link->second;
}

void Engine::link_register(std::string name, simgrid::s4u::Link* link)
{
  pimpl->links_[name] = link;
}

void Engine::link_unregister(std::string name)
{
  pimpl->links_.erase(name);
}

/** @brief Returns the amount of storages in the platform */
size_t Engine::get_storage_count()
{
  return pimpl->storages_.size();
}

/** @brief Returns the list of all storages found in the platform */
std::vector<Storage*> Engine::get_all_storages()
{
  std::vector<Storage*> res;
  for (auto const& kv : pimpl->storages_)
    res.push_back(kv.second);
  return res;
}

/** @brief Find a storage from its name.
 *
 *  @throw std::invalid_argument if the searched storage does not exist.
 */
simgrid::s4u::Storage* Engine::storage_by_name(std::string name)
{
  if (pimpl->storages_.find(name) == pimpl->storages_.end())
    throw std::invalid_argument(std::string("Storage not found: ") + name);

  return pimpl->storages_.at(name);
}

/** @brief Find a storage from its name (or nullptr if that storage does not exist) */
simgrid::s4u::Storage* Engine::storage_by_name_or_null(std::string name)
{
  auto storage = pimpl->storages_.find(name);
  return storage == pimpl->storages_.end() ? nullptr : storage->second;
}

void Engine::storage_register(std::string name, simgrid::s4u::Storage* storage)
{
  pimpl->storages_[name] = storage;
}

void Engine::storage_unregister(std::string name)
{
  pimpl->storages_.erase(name);
}

/** @brief Returns the amount of links in the platform */
size_t Engine::get_link_count()
{
  return pimpl->links_.size();
}

/** @brief Returns the list of all links found in the platform */
std::vector<Link*> Engine::get_all_links()
{
  std::vector<Link*> res;
  for (auto const& kv : pimpl->links_)
    res.push_back(kv.second);
  return res;
}

std::vector<Link*> Engine::get_filtered_links(std::function<bool(Link*)> filter)
{
  std::vector<Link*> filtered_list;
  for (auto const& kv : pimpl->links_)
    if (filter(kv.second))
      filtered_list.push_back(kv.second);
  return filtered_list;
}

size_t Engine::get_actor_count()
{
  return simix_global->process_list.size();
}

std::vector<ActorPtr> Engine::get_all_actors()
{
  std::vector<ActorPtr> actor_list;
  actor_list.push_back(simgrid::s4u::Actor::self());
  for (auto& kv : simix_global->process_list) {
    actor_list.push_back(kv.second->iface());
  }
  return actor_list;
}

std::vector<ActorPtr> Engine::get_filtered_actors(std::function<bool(ActorPtr)> filter)
{
  std::vector<ActorPtr> actor_list;
  for (auto& kv : simix_global->process_list) {
    if (filter(kv.second->iface()))
      actor_list.push_back(kv.second->iface());
  }
  return actor_list;
}

void Engine::run()
{
  /* Clean IO before the run */
  fflush(stdout);
  fflush(stderr);

  if (MC_is_active()) {
    MC_run();
  } else {
    SIMIX_run();
  }
}

/** @brief Retrieve the root netzone, containing all others */
s4u::NetZone* Engine::get_netzone_root()
{
  return pimpl->netzone_root_->get_iface();
}
/** @brief Set the root netzone, containing all others. Once set, it cannot be changed. */
void Engine::set_netzone_root(s4u::NetZone* netzone)
{
  xbt_assert(pimpl->netzone_root_ == nullptr, "The root NetZone cannot be changed once set");
  pimpl->netzone_root_ = netzone->get_impl();
}

static s4u::NetZone* netzone_by_name_recursive(s4u::NetZone* current, std::string name)
{
  if (current->get_name() == name)
    return current;

  for (auto const& elem : current->get_children()) {
    simgrid::s4u::NetZone* tmp = netzone_by_name_recursive(elem, name);
    if (tmp != nullptr) {
      return tmp;
    }
  }
  return nullptr;
}

/** @brief Retrieve the NetZone of the given name (or nullptr if not found) */
NetZone* Engine::netzone_by_name_or_null(std::string name)
{
  return netzone_by_name_recursive(get_netzone_root(), name);
}

/** @brief Retrieve the netpoint of the given name (or nullptr if not found) */
simgrid::kernel::routing::NetPoint* Engine::netpoint_by_name_or_null(std::string name)
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
std::vector<simgrid::kernel::routing::NetPoint*> Engine::get_all_netpoints()
{
  std::vector<simgrid::kernel::routing::NetPoint*> res;
  for (auto const& kv : pimpl->netpoints_)
    res.push_back(kv.second);
  return res;
}

/** @brief Register a new netpoint to the system */
void Engine::netpoint_register(simgrid::kernel::routing::NetPoint* point)
{
  // simgrid::simix::simcall([&]{ FIXME: this segfaults in set_thread
  pimpl->netpoints_[point->get_name()] = point;
  // });
}
/** @brief Unregister a given netpoint */
void Engine::netpoint_unregister(simgrid::kernel::routing::NetPoint* point)
{
  simgrid::simix::simcall([this, point] {
    pimpl->netpoints_.erase(point->get_name());
    delete point;
  });
}

bool Engine::is_initialized()
{
  return Engine::instance_ != nullptr;
}
void Engine::set_config(std::string str)
{
  simgrid::config::set_parse(std::move(str));
}
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */
void simgrid_init(int* argc, char** argv)
{
  simgrid::s4u::Engine e(argc, argv);
}
void simgrid_load_platform(const char* file)
{
  simgrid::s4u::Engine::get_instance()->load_platform(file);
}

void simgrid_load_deployment(const char* file)
{
  simgrid::s4u::Engine::get_instance()->load_deployment(file);
}
void simgrid_run()
{
  simgrid::s4u::Engine::get_instance()->run();
}
void simgrid_register_function(const char* name, int (*code)(int, char**))
{
  simgrid::s4u::Engine::get_instance()->register_function(name, code);
}
void simgrid_register_default(int (*code)(int, char**))
{
  simgrid::s4u::Engine::get_instance()->register_default(code);
}
double simgrid_get_clock()
{
  return simgrid::s4u::Engine::get_clock();
}
