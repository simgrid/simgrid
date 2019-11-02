/* s4u::Engine Simulation Engine and global functions. */

/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Disk.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Mailbox.hpp"
#include "simgrid/s4u/NetZone.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.h"
#include "src/instr/instr_private.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/simix/smx_private.hpp" // For access to simix_global->process_list
#include "src/surf/network_interface.hpp"
#include "surf/surf.hpp" // routing_platf. FIXME:KILLME. SOON
#include <simgrid/Exception.hpp>

#include <string>

XBT_LOG_NEW_CATEGORY(s4u, "Log channels of the S4U (Simgrid for you) interface");
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_engine, s4u, "Logging specific to S4U (engine)");

namespace simgrid {
namespace s4u {
xbt::signal<void()> Engine::on_platform_creation;
xbt::signal<void()> Engine::on_platform_created;
xbt::signal<void()> Engine::on_simulation_end;
xbt::signal<void(double)> Engine::on_time_advance;
xbt::signal<void(void)> Engine::on_deadlock;

Engine* Engine::instance_ = nullptr; /* That singleton is awful, but I don't see no other solution right now. */

Engine::Engine(int* argc, char** argv) : pimpl(new kernel::EngineImpl())
{
  xbt_assert(Engine::instance_ == nullptr, "It is currently forbidden to create more than one instance of s4u::Engine");
  TRACE_global_init();
  SIMIX_global_init(argc, argv);

  Engine::instance_ = this;
}

Engine::~Engine()
{
  delete pimpl;
  Engine::instance_ = nullptr;
}

/** @brief Retrieve the engine singleton */
Engine* Engine::get_instance()
{
  if (Engine::instance_ == nullptr) {
    auto e = new Engine(0, nullptr);
    xbt_assert(Engine::instance_ == e);
  }
  return Engine::instance_;
}

void Engine::shutdown()
{
  delete Engine::instance_;
  Engine::instance_ = nullptr;
}

double Engine::get_clock()
{
  return SIMIX_get_clock();
}

/**
 * Creates a new platform, including hosts, links, and the routing table.
 *
 * \rst
 * See also: :ref:`platform`.
 * \endrst
 */
void Engine::load_platform(const std::string& platf)
{
  double start = xbt_os_time();
  try {
    parse_platform_file(platf);
  } catch (const Exception& e) {
    xbt_die("Error while loading %s: %s", platf.c_str(), e.what());
  }

  double end = xbt_os_time();
  XBT_DEBUG("PARSE TIME: %g", (end - start));
}

/** Registers the main function of an actor that will be launched from the deployment file */
void Engine::register_function(const std::string& name, int (*code)(int, char**))
{
  SIMIX_function_register(name, code);
}
/** Registers the main function of an actor that will be launched from the deployment file */
void Engine::register_function(const std::string& name, void (*code)(std::vector<std::string>))
{
  SIMIX_function_register(name, code);
}
/** Registers a function as the default main function of actors
 *
 * It will be used as fallback when the function requested from the deployment file was not registered.
 * It is used for trace-based simulations (see examples/s4u/replay-comms and similar).
 */
void Engine::register_default(int (*code)(int, char**))
{
  SIMIX_function_register_default(code);
}
/** Load a deployment file and launch the actors that it contains
 *
 * \rst
 * See also: :ref:`deploy`.
 * \endrst
 */
void Engine::load_deployment(const std::string& deploy)
{
  SIMIX_launch_application(deploy);
}
/** Returns the amount of hosts in the platform */
size_t Engine::get_host_count()
{
  return pimpl->hosts_.size();
}

std::vector<Host*> Engine::get_all_hosts()
{
  std::vector<Host*> res;
  for (auto const& kv : pimpl->hosts_)
    res.push_back(kv.second);
  return res;
}

std::vector<Host*> Engine::get_filtered_hosts(const std::function<bool(Host*)>& filter)
{
  std::vector<Host*> hosts;
  for (auto const& kv : pimpl->hosts_) {
    if (filter(kv.second))
      hosts.push_back(kv.second);
  }

  return hosts;
}

void Engine::host_register(const std::string& name, Host* host)
{
  pimpl->hosts_[name] = host;
}

void Engine::host_unregister(const std::string& name)
{
  pimpl->hosts_.erase(name);
}

/** @brief Find a host from its name.
 *
 *  @throw std::invalid_argument if the searched host does not exist.
 */
Host* Engine::host_by_name(const std::string& name)
{
  if (pimpl->hosts_.find(name) == pimpl->hosts_.end())
    throw std::invalid_argument(std::string("Host not found: '") + name + std::string("'"));
  return pimpl->hosts_.at(name);
}

/** @brief Find a host from its name (or nullptr if that host does not exist) */
Host* Engine::host_by_name_or_null(const std::string& name)
{
  auto host = pimpl->hosts_.find(name);
  return host == pimpl->hosts_.end() ? nullptr : host->second;
}

/** @brief Find a link from its name.
 *
 *  @throw std::invalid_argument if the searched link does not exist.
 */
Link* Engine::link_by_name(const std::string& name)
{
  if (pimpl->links_.find(name) == pimpl->links_.end())
    throw std::invalid_argument(std::string("Link not found: ") + name);

  return pimpl->links_.at(name);
}

/** @brief Find an link from its name (or nullptr if that link does not exist) */
Link* Engine::link_by_name_or_null(const std::string& name)
{
  auto link = pimpl->links_.find(name);
  return link == pimpl->links_.end() ? nullptr : link->second;
}

void Engine::link_register(const std::string& name, Link* link)
{
  pimpl->links_[name] = link;
}

void Engine::link_unregister(const std::string& name)
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
Storage* Engine::storage_by_name(const std::string& name)
{
  if (pimpl->storages_.find(name) == pimpl->storages_.end())
    throw std::invalid_argument(std::string("Storage not found: ") + name);

  return pimpl->storages_.at(name);
}

/** @brief Find a storage from its name (or nullptr if that storage does not exist) */
Storage* Engine::storage_by_name_or_null(const std::string& name)
{
  auto storage = pimpl->storages_.find(name);
  return storage == pimpl->storages_.end() ? nullptr : storage->second;
}

void Engine::storage_register(const std::string& name, Storage* storage)
{
  pimpl->storages_[name] = storage;
}

void Engine::storage_unregister(const std::string& name)
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

std::vector<Link*> Engine::get_filtered_links(const std::function<bool(Link*)>& filter)
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

std::vector<ActorPtr> Engine::get_filtered_actors(const std::function<bool(ActorPtr)>& filter)
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

static NetZone* netzone_by_name_recursive(NetZone* current, const std::string& name)
{
  if (current->get_name() == name)
    return current;

  for (auto const& elem : current->get_children()) {
    NetZone* tmp = netzone_by_name_recursive(elem, name);
    if (tmp != nullptr) {
      return tmp;
    }
  }
  return nullptr;
}

/** @brief Retrieve the NetZone of the given name (or nullptr if not found) */
NetZone* Engine::netzone_by_name_or_null(const std::string& name)
{
  return netzone_by_name_recursive(get_netzone_root(), name);
}

/** @brief Retrieve the netpoint of the given name (or nullptr if not found) */
kernel::routing::NetPoint* Engine::netpoint_by_name_or_null(const std::string& name)
{
  auto netp = pimpl->netpoints_.find(name);
  return netp == pimpl->netpoints_.end() ? nullptr : netp->second;
}

std::vector<kernel::routing::NetPoint*> Engine::get_all_netpoints()
{
  std::vector<kernel::routing::NetPoint*> res;
  for (auto const& kv : pimpl->netpoints_)
    res.push_back(kv.second);
  return res;
}

/** @brief Register a new netpoint to the system */
void Engine::netpoint_register(kernel::routing::NetPoint* point)
{
  // simgrid::kernel::actor::simcall([&]{ FIXME: this segfaults in set_thread
  pimpl->netpoints_[point->get_name()] = point;
  // });
}

/** @brief Unregister a given netpoint */
void Engine::netpoint_unregister(kernel::routing::NetPoint* point)
{
  kernel::actor::simcall([this, point] {
    pimpl->netpoints_.erase(point->get_name());
    delete point;
  });
}

bool Engine::is_initialized()
{
  return Engine::instance_ != nullptr;
}
void Engine::set_config(const std::string& str)
{
  config::set_parse(str);
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
