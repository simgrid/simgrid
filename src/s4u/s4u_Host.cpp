/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "src/simix/smx_host_private.hpp"
#include "src/surf/HostImpl.hpp"

#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_host, s4u, "Logging specific to the S4U hosts");
XBT_LOG_EXTERNAL_CATEGORY(surf_route);

int USER_HOST_LEVEL = -1;

namespace simgrid {

namespace xbt {
template class Extendable<simgrid::s4u::Host>;
}

namespace s4u {

simgrid::xbt::signal<void(Host&)> Host::onCreation;
simgrid::xbt::signal<void(Host&)> Host::onDestruction;
simgrid::xbt::signal<void(Host&)> Host::onStateChange;
simgrid::xbt::signal<void(Host&)> Host::onSpeedChange;

Host::Host(const char* name) : name_(name)
{
  xbt_assert(Host::by_name_or_null(name) == nullptr, "Refusing to create a second host named '%s'.", name);
  Engine::get_instance()->host_register(std::string(name_), this);
  new simgrid::surf::HostImpl(this);
}

Host::~Host()
{
  xbt_assert(currentlyDestroying_, "Please call h->destroy() instead of manually deleting it.");

  delete pimpl_;
  if (pimpl_netpoint != nullptr) // not removed yet by a children class
    simgrid::s4u::Engine::get_instance()->netpoint_unregister(pimpl_netpoint);
  delete pimpl_cpu;
  delete mounts;
}

/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly an Host, call h->destroy() instead.
 *
 * This is cumbersome but this is the simplest solution to ensure that the
 * onDestruction() callback receives a valid object (because of the destructor
 * order in a class hierarchy).
 */
void Host::destroy()
{
  if (not currentlyDestroying_) {
    currentlyDestroying_ = true;
    onDestruction(*this);
    Engine::get_instance()->host_unregister(std::string(name_));
    delete this;
  }
}

Host* Host::by_name(std::string name)
{
  return Engine::get_instance()->host_by_name(name);
}
Host* Host::by_name(const char* name)
{
  return Engine::get_instance()->host_by_name(std::string(name));
}
Host* Host::by_name_or_null(const char* name)
{
  return Engine::get_instance()->host_by_name_or_null(std::string(name));
}
Host* Host::by_name_or_null(std::string name)
{
  return Engine::get_instance()->host_by_name_or_null(name);
}

Host* Host::current()
{
  smx_actor_t smx_proc = SIMIX_process_self();
  if (smx_proc == nullptr)
    xbt_die("Cannot call Host::current() from the maestro context");
  return smx_proc->host;
}

void Host::turnOn()
{
  if (isOff()) {
    simgrid::simix::simcall([this] {
      this->extension<simgrid::simix::Host>()->turnOn();
      this->pimpl_cpu->turn_on();
      onStateChange(*this);
    });
  }
}

void Host::turnOff()
{
  if (isOn()) {
    smx_actor_t self = SIMIX_process_self();
    simgrid::simix::simcall([this, self] {
      SIMIX_host_off(this, self);
      onStateChange(*this);
    });
  }
}

bool Host::isOn()
{
  return this->pimpl_cpu->is_on();
}

int Host::getPstatesCount() const
{
  return this->pimpl_cpu->getNbPStates();
}

/**
 * \brief Return the list of actors attached to an host.
 *
 * \param whereto a vector in which we should push actors living on that host
 */
void Host::actorList(std::vector<ActorPtr>* whereto)
{
  for (auto& actor : this->extension<simgrid::simix::Host>()->process_list) {
    whereto->push_back(actor.ciface());
  }
}

/**
 * \brief Find a route toward another host
 *
 * \param dest [IN] where to
 * \param links [OUT] where to store the list of links (must exist, cannot be nullptr).
 * \param latency [OUT] where to store the latency experienced on the path (or nullptr if not interested)
 *                It is the caller responsibility to initialize latency to 0 (we add to provided route)
 * \pre links!=nullptr
 *
 * walk through the routing components tree and find a route between hosts
 * by calling each "get_route" function in each routing component.
 */
void Host::routeTo(Host* dest, std::vector<Link*>& links, double* latency)
{
  std::vector<kernel::resource::LinkImpl*> linkImpls;
  this->routeTo(dest, linkImpls, latency);
  for (kernel::resource::LinkImpl* const& l : linkImpls)
    links.push_back(&l->piface_);
}

/** @brief Just like Host::routeTo, but filling an array of link implementations */
void Host::routeTo(Host* dest, std::vector<kernel::resource::LinkImpl*>& links, double* latency)
{
  simgrid::kernel::routing::NetZoneImpl::get_global_route(pimpl_netpoint, dest->pimpl_netpoint, links, latency);
  if (XBT_LOG_ISENABLED(surf_route, xbt_log_priority_debug)) {
    XBT_CDEBUG(surf_route, "Route from '%s' to '%s' (latency: %f):", get_cname(), dest->get_cname(),
               (latency == nullptr ? -1 : *latency));
    for (auto const& link : links)
      XBT_CDEBUG(surf_route, "Link %s", link->get_cname());
  }
}

/** Get the properties assigned to a host */
std::map<std::string, std::string>* Host::getProperties()
{
  return simgrid::simix::simcall([this] { return this->pimpl_->getProperties(); });
}

/** Retrieve the property value (or nullptr if not set) */
const char* Host::getProperty(const char* key)
{
  return this->pimpl_->getProperty(key);
}

void Host::setProperty(std::string key, std::string value)
{
  simgrid::simix::simcall([this, key, value] { this->pimpl_->setProperty(key, value); });
}

/** Get the processes attached to the host */
void Host::getProcesses(std::vector<ActorPtr>* list)
{
  for (auto& actor : this->extension<simgrid::simix::Host>()->process_list) {
    list->push_back(actor.iface());
  }
}

/** @brief Returns how many actors (daemonized or not) have been launched on this host */
int Host::get_actor_count()
{
  return this->extension<simgrid::simix::Host>()->process_list.size();
}

/** @brief Get the peak processor speed (in flops/s), at the specified pstate  */
double Host::getPstateSpeed(int pstate_index)
{
  return simgrid::simix::simcall([this, pstate_index] { return this->pimpl_cpu->getPstateSpeed(pstate_index); });
}

/** @brief Get the peak processor speed (under full load (=1.0), in flops/s), at the current pstate */
double Host::getSpeed()
{
  return pimpl_cpu->getSpeed(1.0);
}

/** @brief Returns the number of core of the processor. */
int Host::getCoreCount()
{
  return pimpl_cpu->coreCount();
}

/** @brief Set the pstate at which the host should run */
void Host::setPstate(int pstate_index)
{
  simgrid::simix::simcall([this, pstate_index] { this->pimpl_cpu->setPState(pstate_index); });
}
/** @brief Retrieve the pstate at which the host is currently running */
int Host::getPstate()
{
  return this->pimpl_cpu->getPState();
}

/**
 * \ingroup simix_storage_management
 * \brief Returns the list of storages attached to an host.
 * \return a vector containing all storages attached to the host
 */
std::vector<const char*> Host::get_attached_storages()
{
  return simgrid::simix::simcall([this] { return this->pimpl_->get_attached_storages(); });
}

void Host::getAttachedStorages(std::vector<const char*>* storages)
{
  std::vector<const char*> local_storages =
      simgrid::simix::simcall([this] { return this->pimpl_->get_attached_storages(); });
  for (auto elm : local_storages)
    storages->push_back(elm);
}

std::unordered_map<std::string, Storage*> const& Host::getMountedStorages()
{
  if (mounts == nullptr) {
    mounts = new std::unordered_map<std::string, Storage*>();
    for (auto const& m : this->pimpl_->storage_) {
      mounts->insert({m.first, &m.second->piface_});
    }
  }
  return *mounts;
}

void Host::execute(double flops)
{
  execute(flops, 1.0 /* priority */);
}
void Host::execute(double flops, double priority)
{
  smx_activity_t s = simcall_execution_start(nullptr, flops, 1 / priority /*priority*/, 0. /*bound*/, this);
  simcall_execution_wait(s);
}

double Host::getLoad()
{
  return this->pimpl_cpu->get_load();
}

} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */
size_t sg_host_count()
{
  return simgrid::s4u::Engine::get_instance()->get_host_count();
}
/** @brief Returns the host list
 *
 * Uses sg_host_count() to know the array size.
 *
 * \return an array of \ref sg_host_t containing all the hosts in the platform.
 * \remark The host order in this array is generally different from the
 * creation/declaration order in the XML platform (we use a hash table
 * internally).
 * \see sg_host_count()
 */
sg_host_t* sg_host_list()
{
  xbt_assert(sg_host_count() > 0, "There is no host!");
  std::vector<simgrid::s4u::Host*> hosts = simgrid::s4u::Engine::get_instance()->get_all_hosts();

  sg_host_t* res = (sg_host_t*)malloc(sizeof(sg_host_t) * hosts.size());
  memcpy(res, hosts.data(), sizeof(sg_host_t) * hosts.size());

  return res;
}

const char* sg_host_get_name(sg_host_t host)
{
  return host->get_cname();
}

void* sg_host_extension_get(sg_host_t host, size_t ext)
{
  return host->extension(ext);
}

size_t sg_host_extension_create(void (*deleter)(void*))
{
  return simgrid::s4u::Host::extension_create(deleter);
}

sg_host_t sg_host_by_name(const char* name)
{
  return simgrid::s4u::Host::by_name_or_null(name);
}

static int hostcmp_voidp(const void* pa, const void* pb)
{
  return strcmp((*static_cast<simgrid::s4u::Host* const*>(pa))->get_cname(),
                (*static_cast<simgrid::s4u::Host* const*>(pb))->get_cname());
}

xbt_dynar_t sg_hosts_as_dynar()
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(sg_host_t), nullptr);

  std::vector<simgrid::s4u::Host*> list = simgrid::s4u::Engine::get_instance()->get_all_hosts();

  for (auto const& host : list) {
    if (host && host->pimpl_netpoint && host->pimpl_netpoint->is_host())
      xbt_dynar_push(res, &host);
  }
  xbt_dynar_sort(res, hostcmp_voidp);
  return res;
}

// ========= Layering madness ==============*

// ========== User data Layer ==========
void* sg_host_user(sg_host_t host)
{
  return host->extension(USER_HOST_LEVEL);
}
void sg_host_user_set(sg_host_t host, void* userdata)
{
  host->extension_set(USER_HOST_LEVEL, userdata);
}
void sg_host_user_destroy(sg_host_t host)
{
  host->extension_set(USER_HOST_LEVEL, nullptr);
}

// ========= storage related functions ============
xbt_dict_t sg_host_get_mounted_storage_list(sg_host_t host)
{
  xbt_assert((host != nullptr), "Invalid parameters");
  xbt_dict_t res = xbt_dict_new_homogeneous(nullptr);
  for (auto const& elm : host->getMountedStorages()) {
    const char* mount_name = elm.first.c_str();
    sg_storage_t storage   = elm.second;
    xbt_dict_set(res, mount_name, (void*)storage->get_cname(), nullptr);
  }

  return res;
}

xbt_dynar_t sg_host_get_attached_storage_list(sg_host_t host)
{
  xbt_dynar_t storage_dynar               = xbt_dynar_new(sizeof(const char*), nullptr);
  std::vector<const char*> storage_vector = host->get_attached_storages();
  for (auto const& name : storage_vector)
    xbt_dynar_push(storage_dynar, &name);
  return storage_dynar;
}

// =========== user-level functions ===============
// ================================================
/** @brief Returns the total speed of a host */
double sg_host_speed(sg_host_t host)
{
  return host->getSpeed();
}

/** \brief Return the speed of the processor (in flop/s) at a given pstate. See also @ref plugin_energy.
 *
 * \param  host host to test
 * \param pstate_index pstate to test
 * \return Returns the processor speed associated with pstate_index
 */
double sg_host_get_pstate_speed(sg_host_t host, int pstate_index)
{
  return host->getPstateSpeed(pstate_index);
}

/** \ingroup m_host_management
 * \brief Return the number of cores.
 *
 * \param host a host
 * \return the number of cores
 */
int sg_host_core_count(sg_host_t host)
{
  return host->getCoreCount();
}

double sg_host_get_available_speed(sg_host_t host)
{
  return host->pimpl_cpu->get_available_speed();
}

/** @brief Returns the number of power states for a host.
 *
 *  See also @ref plugin_energy.
 */
int sg_host_get_nb_pstates(sg_host_t host)
{
  return host->getPstatesCount();
}

/** @brief Gets the pstate at which that host currently runs.
 *
 *  See also @ref plugin_energy.
 */
int sg_host_get_pstate(sg_host_t host)
{
  return host->getPstate();
}
/** @brief Sets the pstate at which that host should run.
 *
 *  See also @ref plugin_energy.
 */
void sg_host_set_pstate(sg_host_t host, int pstate)
{
  host->setPstate(pstate);
}

/** \ingroup m_host_management
 *
 * \brief Start the host if it is off
 *
 * See also #sg_host_is_on() and #sg_host_is_off() to test the current state of the host and @ref plugin_energy
 * for more info on DVFS.
 */
void sg_host_turn_on(sg_host_t host)
{
  host->turnOn();
}

/** \ingroup m_host_management
 *
 * \brief Stop the host if it is on
 *
 * See also #MSG_host_is_on() and #MSG_host_is_off() to test the current state of the host and @ref plugin_energy
 * for more info on DVFS.
 */
void sg_host_turn_off(sg_host_t host)
{
  host->turnOff();
}

/** @ingroup m_host_management
 * @brief Determine if a host is up and running.
 *
 * See also #sg_host_turn_on() and #sg_host_turn_off() to switch the host ON and OFF and @ref plugin_energy for more
 * info on DVFS.
 *
 * @param host host to test
 * @return Returns true if the host is up and running, and false if it's currently down
 */
int sg_host_is_on(sg_host_t host)
{
  return host->isOn();
}

/** @ingroup m_host_management
 * @brief Determine if a host is currently off.
 *
 * See also #sg_host_turn_on() and #sg_host_turn_off() to switch the host ON and OFF and @ref plugin_energy for more
 * info on DVFS.
 */
int sg_host_is_off(sg_host_t host)
{
  return host->isOff();
}

/** @brief Get the properties of an host */
xbt_dict_t sg_host_get_properties(sg_host_t host)
{
  xbt_dict_t as_dict = xbt_dict_new_homogeneous(xbt_free_f);
  std::map<std::string, std::string>* props = host->getProperties();
  if (props == nullptr)
    return nullptr;
  for (auto const& elm : *props) {
    xbt_dict_set(as_dict, elm.first.c_str(), xbt_strdup(elm.second.c_str()), nullptr);
  }
  return as_dict;
}

/** \ingroup m_host_management
 * \brief Returns the value of a given host property
 *
 * \param host a host
 * \param name a property name
 * \return value of a property (or nullptr if property not set)
*/
const char* sg_host_get_property_value(sg_host_t host, const char* name)
{
  return host->getProperty(name);
}

void sg_host_set_property_value(sg_host_t host, const char* name, const char* value)
{
  host->setProperty(name, value);
}

/**
 * \brief Find a route between two hosts
 *
 * \param from where from
 * \param to where to
 * \param links [OUT] where to store the list of links (must exist, cannot be nullptr).
 */
void sg_host_route(sg_host_t from, sg_host_t to, xbt_dynar_t links)
{
  std::vector<simgrid::s4u::Link*> vlinks;
  from->routeTo(to, vlinks, nullptr);
  for (auto const& link : vlinks)
    xbt_dynar_push(links, &link);
}
/**
 * \brief Find the latency of the route between two hosts
 *
 * \param from where from
 * \param to where to
 */
double sg_host_route_latency(sg_host_t from, sg_host_t to)
{
  std::vector<simgrid::s4u::Link*> vlinks;
  double res = 0;
  from->routeTo(to, vlinks, &res);
  return res;
}
/**
 * \brief Find the bandwitdh of the route between two hosts
 *
 * \param from where from
 * \param to where to
 */
double sg_host_route_bandwidth(sg_host_t from, sg_host_t to)
{
  double min_bandwidth = -1.0;

  std::vector<simgrid::s4u::Link*> vlinks;
  from->routeTo(to, vlinks, nullptr);
  for (auto const& link : vlinks) {
    double bandwidth = link->bandwidth();
    if (bandwidth < min_bandwidth || min_bandwidth < 0.0)
      min_bandwidth = bandwidth;
  }
  return min_bandwidth;
}

/** @brief Displays debugging information about a host */
void sg_host_dump(sg_host_t host)
{
  XBT_INFO("Displaying host %s", host->get_cname());
  XBT_INFO("  - speed: %.0f", host->getSpeed());
  XBT_INFO("  - available speed: %.2f", sg_host_get_available_speed(host));
  std::map<std::string, std::string>* props = host->getProperties();

  if (not props->empty()) {
    XBT_INFO("  - properties:");
    for (auto const& elm : *props) {
      XBT_INFO("    %s->%s", elm.first.c_str(), elm.second.c_str());
    }
  }
}

/** \brief Return the list of actors attached to an host.
 *
 * \param host a host
 * \param whereto a dynar in which we should push actors living on that host
 */
void sg_host_get_actor_list(sg_host_t host, xbt_dynar_t whereto)
{
  for (auto& actor : host->extension<simgrid::simix::Host>()->process_list) {
    s4u_Actor* p = actor.ciface();
    xbt_dynar_push(whereto, &p);
  }
}

sg_host_t sg_host_self()
{
  smx_actor_t process = SIMIX_process_self();
  return (process == nullptr) ? nullptr : process->host;
}
