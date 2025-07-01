/* Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/host.h>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/Host.hpp>
#include <simgrid/s4u/VirtualMachine.hpp>
#include <xbt/parse_units.hpp>

#include "simgrid/s4u/Actor.hpp"
#include "simgrid/simcall.hpp"
#include "src/kernel/resource/HostImpl.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"
#include "src/kernel/resource/VirtualMachineImpl.hpp"

#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_host, s4u, "Logging specific to the S4U hosts");
XBT_LOG_EXTERNAL_CATEGORY(ker_platform);

namespace simgrid {

template class xbt::Extendable<s4u::Host>;

namespace s4u {

#ifndef DOXYGEN
xbt::signal<void(Host&)> Host::on_creation;
xbt::signal<void(Host const&)> Host::on_destruction;
xbt::signal<void(Host const&)> Host::on_onoff;
xbt::signal<void(Host const&)> Host::on_speed_change;
xbt::signal<void(kernel::resource::CpuAction&, kernel::resource::Action::State)> Host::on_exec_state_change;
#endif

Host* Host::set_cpu(kernel::resource::CpuImpl* cpu)
{
  pimpl_cpu_ = cpu;
  return this;
}

#ifndef DOXYGEN
Host* Host::set_netpoint(kernel::routing::NetPoint* netpoint)
{
  pimpl_netpoint_ = netpoint;
  return this;
}

Host::~Host()
{
  if (pimpl_netpoint_ != nullptr) // not removed yet by a children class
    Engine::get_instance()->netpoint_unregister(pimpl_netpoint_);
  delete pimpl_cpu_;
}
#endif

/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly a host, call h->destroy() instead.
 *
 * This is cumbersome but this is the simplest solution to ensure that the on_destruction() callback receives a valid
 * object (because of the destructor order in a class hierarchy).
 */
void Host::destroy()
{
  kernel::actor::simcall_answered([this] { this->pimpl_->destroy(); });
}

Host* Host::by_name(const std::string& name)
{
  return Engine::get_instance()->host_by_name(name);
}
Host* Host::by_name_or_null(const std::string& name)
{
  return Engine::get_instance()->host_by_name_or_null(name);
}

Host* Host::current()
{
  const kernel::actor::ActorImpl* self = kernel::actor::ActorImpl::self();
  xbt_assert(self != nullptr, "Cannot call Host::current() from the maestro context");
  return self->get_host();
}

std::string const& Host::get_name() const
{
  return this->pimpl_->get_name();
}

const char* Host::get_cname() const
{
  return this->pimpl_->get_cname();
}

void Host::turn_on()
{
  if (not is_on()) {
    kernel::actor::simcall_answered([this] {
      this->pimpl_cpu_->turn_on();
      this->pimpl_->turn_on();
      on_onoff(*this);
      on_this_onoff(*this);
    });
  }
}

/** @brief Stop the host if it is on */
void Host::turn_off()
{
  if (is_on()) {
    const kernel::actor::ActorImpl* self = kernel::actor::ActorImpl::self();
    kernel::actor::simcall_answered([this, self] {
      this->pimpl_cpu_->turn_off();
      this->pimpl_->turn_off(self);

      on_onoff(*this);
      on_this_onoff(*this);
    });
  }
}

bool Host::is_on() const
{
  return this->pimpl_cpu_->is_on();
}

unsigned long Host::get_pstate_count() const
{
  return this->pimpl_cpu_->get_pstate_count();
}

ActorPtr Host::add_actor(const std::string& name, const std::function<void()>& code)
{
  kernel::actor::ActorImpl* self = kernel::actor::ActorImpl::self();
  kernel::actor::ActorCreateSimcall observer{self};
  kernel::actor::ActorImpl* actor = kernel::actor::simcall_answered(
      [self, &name, this, &code, &observer] {
        auto child = self->init(name, this)->start(code);
        observer.set_child(child->get_pid());
        return child;
      },
      &observer);

  return actor->get_iface();
}

ActorPtr Host::add_actor(const std::string& name, const std::string& function,
                           std::vector<std::string> args)
{
  const simgrid::kernel::actor::ActorCodeFactory& factory = Engine::get_instance()->get_impl()->get_function(function);
  return add_actor(name, factory(std::move(args)));
}

/**
 * @brief Return a copy of the list of actors that are executing on this host.
 *
 * Daemons and regular actors are all mixed in this list.
 */
std::vector<ActorPtr> Host::get_all_actors() const
{
  return pimpl_->get_all_actors();
}

/** @brief Returns how many actors (daemonized or not) have been launched on this host */
size_t Host::get_actor_count() const
{
  return pimpl_->get_actor_count();
}

/**
 * @brief Find a route toward another host
 *
 * @param dest [IN] where to
 * @param links [OUT] where to store the list of links (must exist, cannot be nullptr).
 * @param latency [OUT] where to store the latency experienced on the path (or nullptr if not interested)
 *                It is the caller responsibility to initialize latency to 0 (we add to provided route)
 *
 * walk through the routing components tree and find a route between hosts
 * by calling each "get_route" function in each routing component.
 */
void Host::route_to(const Host* dest, std::vector<Link*>& links, double* latency) const
{
  std::vector<kernel::resource::StandardLinkImpl*> linkImpls;
  this->route_to(dest, linkImpls, latency);
  for (auto* l : linkImpls)
    links.push_back(l->get_iface());
}
std::pair<std::vector<Link*>, double> Host::route_to(const Host* dest) const
{
  std::vector<kernel::resource::StandardLinkImpl*> linkImpls;
  std::vector<Link*> links;
  double latency = 0;
  this->route_to(dest, linkImpls, &latency);
  for (auto* l : linkImpls)
    links.push_back(l->get_iface());
  return std::make_pair(links, latency);
}

/** @brief Just like Host::routeTo, but filling an array of link implementations */
void Host::route_to(const Host* dest, std::vector<kernel::resource::StandardLinkImpl*>& links, double* latency) const
{
  kernel::routing::NetZoneImpl::get_global_route(pimpl_netpoint_, dest->get_netpoint(), links, latency);
  if (XBT_LOG_ISENABLED(ker_platform, xbt_log_priority_debug)) {
    XBT_CDEBUG(ker_platform, "Route from '%s' to '%s' (latency: %f):", get_cname(), dest->get_cname(),
               (latency == nullptr ? -1 : *latency));
    for (auto const* link : links)
      XBT_CDEBUG(ker_platform, "  Link '%s'", link->get_cname());
  }
}

/** @brief Returns the networking zone englobing that host */
NetZone* Host::get_englobing_zone() const
{
  return pimpl_netpoint_->get_englobing_zone()->get_iface();
}

/** Get the properties assigned to a host */
const std::unordered_map<std::string, std::string>* Host::get_properties() const
{
  return this->pimpl_->get_properties();
}

/** Retrieve the property value (or nullptr if not set) */
const char* Host::get_property(const std::string& key) const
{
  return this->pimpl_->get_property(key);
}

Host* Host::set_property(const std::string& key, const std::string& value)
{
  kernel::actor::simcall_object_access(pimpl_, [this, &key, &value] { this->pimpl_->set_property(key, value); });
  return this;
}

Host* Host::set_properties(const std::unordered_map<std::string, std::string>& properties)
{
  kernel::actor::simcall_object_access(pimpl_, [this, &properties] { this->pimpl_->set_properties(properties); });
  return this;
}

int Host::get_concurrency_limit() const
{
  return pimpl_cpu_->get_concurrency_limit();
}

Host* Host::set_concurrency_limit(int limit)
{
  kernel::actor::simcall_object_access(pimpl_cpu_, [this, limit] { pimpl_cpu_->set_concurrency_limit(limit); });
  return this;
}

/** Specify a profile turning the host on and off according to an exhaustive list or a stochastic law.
 * The profile must contain boolean values. */
Host* Host::set_state_profile(kernel::profile::Profile* p)
{
  kernel::actor::simcall_object_access(pimpl_, [this, p] { pimpl_cpu_->set_state_profile(p); });
  return this;
}
/** Specify a profile modeling the external load according to an exhaustive list or a stochastic law.
 *
 * Each event of the profile represent a peak speed change that is due to external load. The values are given as a rate
 * of the initial value. This means that the actual value is obtained by multiplying the initial value (the peek speed
 * at this pstate level) by the rate coming from the profile.
 */
Host* Host::set_speed_profile(kernel::profile::Profile* p)
{
  kernel::actor::simcall_object_access(pimpl_, [this, p] { pimpl_cpu_->set_speed_profile(p); });
  return this;
}

/** @brief Get the peak processor speed (in flops/s), at the specified pstate  */
double Host::get_pstate_speed(unsigned long pstate_index) const
{
  return this->pimpl_cpu_->get_pstate_peak_speed(pstate_index);
}

double Host::get_speed() const
{
  return this->pimpl_cpu_->get_speed(1.0);
}
double Host::get_load() const
{
  return this->pimpl_cpu_->get_load();
}
double Host::get_available_speed() const
{
  return this->pimpl_cpu_->get_speed_ratio();
}

Host* Host::set_sharing_policy(SharingPolicy policy, const s4u::NonLinearResourceCb& cb)
{
  kernel::actor::simcall_object_access(pimpl_, [this, policy, &cb] { pimpl_cpu_->set_sharing_policy(policy, cb); });
  return this;
}

Host::SharingPolicy Host::get_sharing_policy() const
{
  return this->pimpl_cpu_->get_sharing_policy();
}

int Host::get_core_count() const
{
  return this->pimpl_cpu_->get_core_count();
}

Host* Host::set_core_count(int core_count)
{
  kernel::actor::simcall_object_access(pimpl_, [this, core_count] { this->pimpl_cpu_->set_core_count(core_count); });
  return this;
}

Host* Host::set_pstate_speed(const std::vector<double>& speed_per_state)
{
  kernel::actor::simcall_object_access(pimpl_,
                                       [this, &speed_per_state] { pimpl_cpu_->set_pstate_speed(speed_per_state); });
  return this;
}

std::vector<double> Host::convert_pstate_speed_vector(const std::vector<std::string>& speed_per_state)
{
  std::vector<double> speed_list;
  speed_list.reserve(speed_per_state.size());
  for (const auto& speed_str : speed_per_state) {
    try {
      double speed = xbt_parse_get_speed("", 0, speed_str, "");
      speed_list.push_back(speed);
    } catch (const simgrid::ParseError&) {
      throw std::invalid_argument("Invalid speed value: " + speed_str);
    }
  }
  return speed_list;
}

Host* Host::set_pstate_speed(const std::vector<std::string>& speed_per_state)
{
  set_pstate_speed(Host::convert_pstate_speed_vector(speed_per_state));
  return this;
}

/** @brief Set the pstate at which the host should run */
Host* Host::set_pstate(unsigned long pstate_index)
{
  kernel::actor::simcall_object_access(pimpl_, [this, pstate_index] { this->pimpl_cpu_->set_pstate(pstate_index); });
  return this;
}

/** @brief Retrieve the pstate at which the host is currently running */
unsigned long Host::get_pstate() const
{
  return this->pimpl_cpu_->get_pstate();
}

Host* Host::set_cpu_factor_cb(const std::function<double(Host&, double)>& cb)
{
  kernel::actor::simcall_object_access(pimpl_, [this, &cb] { pimpl_cpu_->set_cpu_factor_cb(cb); });
  return this;
}

Host* Host::set_coordinates(const std::string& coords)
{
  if (not coords.empty())
    kernel::actor::simcall_object_access(pimpl_, [this, coords] { this->pimpl_netpoint_->set_coordinates(coords); });
  return this;
}
std::vector<Disk*> Host::get_disks() const
{
  return this->pimpl_->get_disks();
}

Disk* Host::get_disk_by_name(const std::string& name) const
{
  return this->pimpl_->get_disk_by_name(name);
}

Disk* Host::add_disk(const std::string& name, double read_bandwidth, double write_bandwidth)
{
  return kernel::actor::simcall_answered([this, &name, read_bandwidth, write_bandwidth] {
    auto* disk = pimpl_->add_disk(name, read_bandwidth, write_bandwidth);
    pimpl_->register_disk(disk);
    return disk;
  });
}

Disk* Host::add_disk(const std::string& name, const std::string& read_bandwidth, const std::string& write_bandwidth)
{
  double d_read;
  try {
    d_read = xbt_parse_get_bandwidth("", 0, read_bandwidth, "");
  } catch (const simgrid::ParseError&) {
    throw std::invalid_argument("Impossible to create disk: " + name + ". Invalid read bandwidth: " + read_bandwidth);
  }
  double d_write;
  try {
    d_write = xbt_parse_get_bandwidth("", 0, write_bandwidth, "");
  } catch (const simgrid::ParseError&) {
    throw std::invalid_argument("Impossible to create disk: " + name + ". Invalid write bandwidth: " + write_bandwidth);
  }
  return add_disk(name, d_read, d_write);
}
void Host::register_disk(const Disk* disk)
{
  kernel::actor::simcall_answered([this, disk] { this->pimpl_->register_disk(disk); });
}

void Host::remove_disk(const std::string& disk_name)
{
  kernel::actor::simcall_answered([this, disk_name] { this->pimpl_->remove_disk(disk_name); });
}

VirtualMachine* Host::create_vm(const std::string& name, int core_amount)
{
  return kernel::actor::simcall_answered(
      [this, &name, core_amount] { return this->pimpl_->create_vm(name, core_amount); });
}

VirtualMachine* Host::create_vm(const std::string& name, int core_amount, size_t ramsize)
{
  return kernel::actor::simcall_answered(
      [this, &name, core_amount, ramsize] { return this->pimpl_->create_vm(name, core_amount, ramsize); });
}

VirtualMachine* Host::vm_by_name_or_null(const std::string& name)
{
  simgrid::kernel::resource::VirtualMachineImpl* vm = this->pimpl_->get_vm_by_name_or_null(name);
  return vm ? vm->get_iface() : nullptr;
}

ExecPtr Host::exec_init(double flops) const
{
  return this_actor::exec_init(flops);
}

ExecPtr Host::exec_async(double flops) const
{
  return this_actor::exec_async(flops);
}

void Host::execute(double flops) const
{
  execute(flops, 1.0 /* priority */);
}

void Host::execute(double flops, double priority) const
{
  Exec::init()->set_flops_amount(flops)->set_host(const_cast<Host*>(this))->set_priority(1 / priority)->wait();
}

Host* Host::seal()
{
  kernel::actor::simcall_answered([this]() { this->pimpl_->seal(); });
  simgrid::s4u::Host::on_creation(*this); // notify the signal
  return this;
}

} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */
size_t sg_host_count()
{
  return simgrid::s4u::Engine::get_instance()->get_host_count();
}
sg_host_t* sg_host_list()
{
  const simgrid::s4u::Engine* e = simgrid::s4u::Engine::get_instance();
  size_t host_count             = e->get_host_count();

  xbt_assert(host_count > 0, "There is no host!");
  std::vector<simgrid::s4u::Host*> hosts = e->get_all_hosts();

  auto* res = xbt_new(sg_host_t, hosts.size());
  std::copy(begin(hosts), end(hosts), res);

  return res;
}

const char* sg_host_get_name(const_sg_host_t host)
{
  return host->get_cname();
}

void* sg_host_extension_get(const_sg_host_t host, size_t ext)
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

/** @brief Retrieve a VM running on a given host from its name, or return NULL if no VM matches*/
sg_vm_t sg_vm_by_name(sg_host_t host, const char* name)
{
  return host->vm_by_name_or_null(name);
}

// ========= Layering madness ==============*

// ========== User data Layer ==========
void* sg_host_get_data(const_sg_host_t host)
{
  return host->get_data<void>();
}
void sg_host_set_data(sg_host_t host, void* userdata)
{
  host->set_data(userdata);
}

// ========= Disk related functions ============
void sg_host_get_disks(const_sg_host_t host, unsigned int* disk_count, sg_disk_t** disks)
{
  std::vector<sg_disk_t> list = host->get_disks();
  *disk_count                 = list.size();
  *disks                      = xbt_new(sg_disk_t, list.size());
  std::copy(begin(list), end(list), *disks);
}
sg_disk_t sg_host_get_disk_by_name(const_sg_host_t host, const char* name)
{
  return host->get_disk_by_name(name);
}
// =========== user-level functions ===============
// ================================================
/** @brief Returns the total speed of a host */
double sg_host_get_speed(const_sg_host_t host)
{
  return host->get_speed();
}

/** @brief Return the speed of the processor (in flop/s) at a given pstate. See also @ref plugin_host_energy.
 *
 * @param  host host to test
 * @param pstate_index pstate to test
 * @return Returns the processor speed associated with pstate_index
 */
double sg_host_get_pstate_speed(const_sg_host_t host, unsigned long pstate_index)
{
  return host->get_pstate_speed(pstate_index);
}

/** @ingroup m_host_management
 * @brief Return the number of cores.
 *
 * @param host a host
 * @return the number of cores
 */
int sg_host_core_count(const_sg_host_t host)
{
  return host->get_core_count();
}

double sg_host_get_available_speed(const_sg_host_t host)
{
  return host->get_available_speed();
}

/** @brief Returns the number of power states for a host.
 *
 *  See also @ref plugin_host_energy.
 */
unsigned long sg_host_get_nb_pstates(const_sg_host_t host)
{
  return host->get_pstate_count();
}

/** @brief Gets the pstate at which that host currently runs.
 *
 *  See also @ref plugin_host_energy.
 */
unsigned long sg_host_get_pstate(const_sg_host_t host)
{
  return host->get_pstate();
}
/** @brief Sets the pstate at which that host should run.
 *
 *  See also @ref plugin_host_energy.
 */
void sg_host_set_pstate(sg_host_t host, unsigned long pstate)
{
  host->set_pstate(pstate);
}

/** @ingroup m_host_management
 *
 * @brief Start the host if it is off
 *
 * See also #sg_host_is_on() to test the current state of the host and @ref plugin_host_energy
 * for more info on DVFS.
 */
void sg_host_turn_on(sg_host_t host)
{
  host->turn_on();
}

/** @ingroup m_host_management
 *
 * @brief Stop the host if it is on
 *
 * See also #sg_host_is_on() to test the current state of the host and @ref plugin_host_energy
 * for more info on DVFS.
 */
void sg_host_turn_off(sg_host_t host)
{
  host->turn_off();
}

/** @ingroup m_host_management
 * @brief Determine if a host is up and running.
 *
 * See also #sg_host_turn_on() and #sg_host_turn_off() to switch the host ON and OFF and @ref plugin_host_energy for
 * more info on DVFS.
 *
 * @param host host to test
 * @return Returns true if the host is up and running, and false if it's currently down
 */
int sg_host_is_on(const_sg_host_t host)
{
  return host->is_on();
}

/** @brief Get the properties of a host */
xbt_dict_t sg_host_get_properties(const_sg_host_t host)
{
  const std::unordered_map<std::string, std::string>* props = host->get_properties();
  xbt_dict_t as_dict                                        = xbt_dict_new_homogeneous(xbt_free_f);

  if (props == nullptr)
    return nullptr;
  for (auto const& [key, value] : *props) {
    xbt_dict_set(as_dict, key.c_str(), xbt_strdup(value.c_str()));
  }
  return as_dict;
}
const char** sg_host_get_property_names(const_sg_host_t host, int* size)
{
  const std::unordered_map<std::string, std::string>* props = host->get_properties();

  if (props == nullptr) {
    if (size)
      *size = 0;
    return nullptr;
  }

  const char** res = (const char**)xbt_malloc(sizeof(char*) * (props->size() + 1));
  if (size)
    *size = props->size();
  int i = 0;
  for (auto const& [key, _] : *props)
    res[i++] = key.c_str();
  res[i] = nullptr;

  return res;
}

/** @ingroup m_host_management
 * @brief Returns the value of a given host property
 *
 * @param host a host
 * @param name a property name
 * @return value of a property (or nullptr if property not set)
 */
const char* sg_host_get_property_value(const_sg_host_t host, const char* name)
{
  return host->get_property(name);
}

void sg_host_set_property_value(sg_host_t host, const char* name, const char* value)
{
  host->set_property(name, value);
}

/**
 * @brief Find a route between two hosts
 *
 * @param from where from
 * @param to where to
 * @param links [OUT] where to store the list of links (must exist, cannot be nullptr).
 */
void sg_host_get_route(const_sg_host_t from, const_sg_host_t to, xbt_dynar_t links)
{
  std::vector<simgrid::s4u::Link*> vlinks;
  from->route_to(to, vlinks, nullptr);
  for (auto const& link : vlinks)
    xbt_dynar_push(links, &link);
}
const_sg_link_t* sg_host_get_route_links(const_sg_host_t from, const_sg_host_t to, int* size)
{
  std::vector<simgrid::s4u::Link*> vlinks;
  from->route_to(to, vlinks, nullptr);

  const_sg_link_t* res = (const_sg_link_t*)xbt_malloc(sizeof(const_sg_link_t) * (vlinks.size() + 1));
  if (size)
    *size = vlinks.size();
  int i = 0;
  for (auto const& link : vlinks)
    res[i++] = link;
  res[i] = nullptr;

  return res;
}

/**
 * @brief Find the latency of the route between two hosts
 *
 * @param from where from
 * @param to where to
 */
double sg_host_get_route_latency(const_sg_host_t from, const_sg_host_t to)
{
  std::vector<simgrid::s4u::Link*> vlinks;
  double res = 0;
  from->route_to(to, vlinks, &res);
  return res;
}
/**
 * @brief Find the bandwidth of the route between two hosts
 *
 * @param from where from
 * @param to where to
 */
double sg_host_get_route_bandwidth(const_sg_host_t from, const_sg_host_t to)
{
  double min_bandwidth = -1.0;

  std::vector<simgrid::s4u::Link*> vlinks;
  from->route_to(to, vlinks, nullptr);
  for (auto const& link : vlinks) {
    double bandwidth = link->get_bandwidth();
    if (bandwidth < min_bandwidth || min_bandwidth < 0.0)
      min_bandwidth = bandwidth;
  }
  return min_bandwidth;
}

void sg_host_sendto(sg_host_t from, sg_host_t to, double byte_amount)
{
  simgrid::s4u::Comm::sendto(from, to, byte_amount);
}

void sg_host_get_actor_list(const_sg_host_t host, xbt_dynar_t whereto)
{
  auto const actors = host->get_all_actors();
  for (auto const& actor : actors)
    xbt_dynar_push(whereto, &actor);
}
const_sg_actor_t* sg_host_get_actors(const_sg_host_t host, int* size)
{
  std::vector<simgrid::s4u::ActorPtr> actors = host->get_all_actors();

  const_sg_actor_t* res = (const_sg_actor_t*)xbt_malloc(sizeof(const_sg_link_t) * (actors.size() + 1));
  if (size)
    *size = actors.size();
  int i = 0;
  for (auto actor : actors)
    res[i++] = actor.get();
  res[i] = nullptr;

  return res;
}

sg_host_t sg_host_self()
{
  return simgrid::s4u::Actor::is_maestro() ? nullptr : simgrid::kernel::actor::ActorImpl::self()->get_host();
}

/* needs to be public and without simcall for exceptions and logging events */
const char* sg_host_self_get_name()
{
  const char* res = "";
  if (not simgrid::s4u::Actor::is_maestro()) {
    const simgrid::s4u::Host* host = simgrid::kernel::actor::ActorImpl::self()->get_host();
    if (host != nullptr)
      res = host->get_cname();
  }
  return res;
}

double sg_host_get_load(const_sg_host_t host)
{
  return host->get_load();
}
