/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/actor.h>
#include <simgrid/modelchecker.h>
#include <simgrid/s4u/Actor.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Exec.hpp>
#include <simgrid/s4u/VirtualMachine.hpp>

#include "src/include/mc/mc.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/kernel/context/Context.hpp"
#include "src/mc/mc_replay.hpp"
#include "src/surf/HostImpl.hpp"

#include <algorithm>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_actor, s4u, "S4U actors");

namespace simgrid {

template class xbt::Extendable<s4u::Actor>;

namespace s4u {

xbt::signal<void(Actor&)> s4u::Actor::on_creation;
xbt::signal<void(Actor const&)> s4u::Actor::on_suspend;
xbt::signal<void(Actor const&)> s4u::Actor::on_resume;
xbt::signal<void(Actor const&)> s4u::Actor::on_sleep;
xbt::signal<void(Actor const&)> s4u::Actor::on_wake_up;
xbt::signal<void(Actor const&, Host const& previous_location)> s4u::Actor::on_host_change;
xbt::signal<void(Actor const&)> s4u::Actor::on_termination;
xbt::signal<void(Actor const&)> s4u::Actor::on_destruction;

// ***** Actor creation *****
Actor* Actor::self()
{
  const kernel::context::Context* self_context = kernel::context::Context::self();
  if (self_context == nullptr)
    return nullptr;

  return self_context->get_actor()->get_ciface();
}

ActorPtr Actor::init(const std::string& name, s4u::Host* host)
{
  const kernel::actor::ActorImpl* self = kernel::actor::ActorImpl::self();
  kernel::actor::ActorImpl* actor =
      kernel::actor::simcall([self, &name, host] { return self->init(name, host).get(); });
  return actor->get_iface();
}

/** Set a non-default stack size for this context (in Kb)
 *
 * This must be done before starting the actor, and it won't work with the thread factory. */
ActorPtr Actor::set_stacksize(unsigned stacksize)
{
  pimpl_->set_stacksize(stacksize * 1024);
  return this;
}

ActorPtr Actor::start(const std::function<void()>& code)
{
  simgrid::kernel::actor::simcall([this, &code] { pimpl_->start(code); });
  return this;
}

ActorPtr Actor::create(const std::string& name, s4u::Host* host, const std::function<void()>& code)
{
  const kernel::actor::ActorImpl* self = kernel::actor::ActorImpl::self();
  kernel::actor::ActorImpl* actor =
      kernel::actor::simcall([self, &name, host, &code] { return self->init(name, host)->start(code); });

  return actor->get_iface();
}

ActorPtr Actor::create(const std::string& name, s4u::Host* host, const std::string& function,
                       std::vector<std::string> args)
{
  const simgrid::kernel::actor::ActorCodeFactory& factory =
      simgrid::kernel::EngineImpl::get_instance()->get_function(function);
  return create(name, host, factory(std::move(args)));
}

void intrusive_ptr_add_ref(const Actor* actor)
{
  intrusive_ptr_add_ref(actor->pimpl_);
}
void intrusive_ptr_release(const Actor* actor)
{
  intrusive_ptr_release(actor->pimpl_);
}
int Actor::get_refcount() const
{
  return pimpl_->get_refcount();
}

// ***** Actor methods *****

void Actor::join() const
{
  join(-1);
}

void Actor::join(double timeout) const
{
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  const kernel::actor::ActorImpl* target = pimpl_;
  kernel::actor::simcall_blocking([issuer, target, timeout] {
    if (target->finished_) {
      // The joined actor is already finished, just wake up the issuer right away
      issuer->simcall_answer();
    } else {
      kernel::activity::ActivityImplPtr sync = issuer->join(target, timeout);
      sync->register_simcall(&issuer->simcall_);
    }
  });
}

void Actor::set_auto_restart(bool autorestart)
{
  kernel::actor::simcall([this, autorestart]() {
    xbt_assert(autorestart && not pimpl_->has_to_auto_restart()); // FIXME: handle all cases
    pimpl_->set_auto_restart(autorestart);

    auto* arg = new kernel::actor::ProcessArg(pimpl_->get_host(), pimpl_);
    XBT_DEBUG("Adding %s to the actors_at_boot_ list of Host %s", arg->name.c_str(), arg->host->get_cname());
    pimpl_->get_host()->get_impl()->add_actor_at_boot(arg);
  });
}

void Actor::on_exit(const std::function<void(bool /*failed*/)>& fun) const
{
  kernel::actor::simcall([this, &fun] { pimpl_->on_exit->emplace_back(fun); });
}

void Actor::set_host(Host* new_host)
{
  const s4u::Host* previous_location = get_host();

  kernel::actor::simcall([this, new_host]() {
    for (auto const& activity : pimpl_->activities_) {
      // FIXME: implement the migration of other kinds of activities
      if (auto exec = boost::dynamic_pointer_cast<kernel::activity::ExecImpl>(activity))
        exec->migrate(new_host);
    }
    this->pimpl_->set_host(new_host);
  });

  s4u::Actor::on_host_change(*this, *previous_location);
}

s4u::Host* Actor::get_host() const
{
  return this->pimpl_->get_host();
}

void Actor::daemonize()
{
  kernel::actor::simcall([this]() { pimpl_->daemonize(); });
}

bool Actor::is_daemon() const
{
  return this->pimpl_->is_daemon();
}

const simgrid::xbt::string& Actor::get_name() const
{
  return this->pimpl_->get_name();
}

const char* Actor::get_cname() const
{
  return this->pimpl_->get_cname();
}

aid_t Actor::get_pid() const
{
  return this->pimpl_->get_pid();
}

aid_t Actor::get_ppid() const
{
  return this->pimpl_->get_ppid();
}

void Actor::suspend()
{
  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  kernel::actor::ActorImpl* target = pimpl_;
  s4u::Actor::on_suspend(*this);
  kernel::actor::simcall_blocking([issuer, target]() {
    target->suspend();
    if (target != issuer) {
      /* If we are suspending ourselves, then just do not finish the simcall now */
      issuer->simcall_answer();
    }
  });
}

void Actor::resume()
{
  kernel::actor::simcall([this] { pimpl_->resume(); });
  s4u::Actor::on_resume(*this);
}

bool Actor::is_suspended() const
{
  return pimpl_->is_suspended();
}

void Actor::set_kill_time(double kill_time)
{
  kernel::actor::simcall([this, kill_time] { pimpl_->set_kill_time(kill_time); });
}

/** @brief Get the kill time of an actor(or 0 if unset). */
double Actor::get_kill_time() const
{
  return pimpl_->get_kill_time();
}

void Actor::kill()
{
  const kernel::actor::ActorImpl* self = kernel::actor::ActorImpl::self();
  kernel::actor::simcall([this, self] { self->kill(pimpl_); });
}

// ***** Static functions *****

ActorPtr Actor::by_pid(aid_t pid)
{
  kernel::actor::ActorImpl* actor = kernel::actor::ActorImpl::by_pid(pid);
  if (actor != nullptr)
    return actor->get_iface();
  else
    return ActorPtr();
}

void Actor::kill_all()
{
  const kernel::actor::ActorImpl* self = kernel::actor::ActorImpl::self();
  kernel::actor::simcall([self] { self->kill_all(); });
}

const std::unordered_map<std::string, std::string>* Actor::get_properties() const
{
  return pimpl_->get_properties();
}

/** Retrieve the property value (or nullptr if not set) */
const char* Actor::get_property(const std::string& key) const
{
  return pimpl_->get_property(key);
}

void Actor::set_property(const std::string& key, const std::string& value)
{
  kernel::actor::simcall([this, &key, &value] { pimpl_->set_property(key, value); });
}

Actor* Actor::restart()
{
  return kernel::actor::simcall([this]() { return pimpl_->restart(); });
}

// ***** this_actor *****

namespace this_actor {

/** Returns true if run from the kernel mode, and false if run from a real actor
 *
 * Everything that is run out of any actor (simulation setup before the engine is run,
 * computing the model evolutions as a result to the actors' action, etc) is run in
 * kernel mode, just as in any operating systems.
 *
 * In SimGrid, the actor in charge of doing the stuff in kernel mode is called Maestro,
 * because it is the one scheduling when the others should move or wait.
 */
bool is_maestro()
{
  return SIMIX_is_maestro();
}

void sleep_for(double duration)
{
  xbt_assert(std::isfinite(duration), "duration is not finite!");

  if (duration <= 0) /* that's a no-op */
    return;

  if (duration < sg_surf_precision) {
    static unsigned int warned = 0; // At most 20 such warnings
    warned++;
    if (warned <= 20)
      XBT_INFO("The parameter to sleep_for() is smaller than the SimGrid numerical accuracy (%g < %g). "
               "Please refer to https://simgrid.org/doc/latest/Configuring_SimGrid.html#numerical-precision",
               duration, sg_surf_precision);
    if (warned == 20)
      XBT_VERB("(further warnings about the numerical accuracy of sleep_for() will be omitted).");
  }

  kernel::actor::ActorImpl* issuer = kernel::actor::ActorImpl::self();
  Actor::on_sleep(*issuer->get_ciface());

  kernel::actor::simcall_blocking([issuer, duration]() {
    if (MC_is_active() || MC_record_replay_is_active()) {
      MC_process_clock_add(issuer, duration);
      issuer->simcall_answer();
      return;
    }
    kernel::activity::ActivityImplPtr sync = issuer->sleep(duration);
    sync->register_simcall(&issuer->simcall_);
  });

  Actor::on_wake_up(*issuer->get_ciface());
}

void yield()
{
  kernel::actor::simcall([] { /* do nothing*/ });
}

XBT_PUBLIC void sleep_until(double wakeup_time)
{
  double now = s4u::Engine::get_clock();
  if (wakeup_time > now)
    sleep_for(wakeup_time - now);
}

void execute(double flops)
{
  execute(flops, 1.0 /* priority */);
}

void execute(double flops, double priority)
{
  exec_init(flops)->set_priority(priority)->vetoable_start()->wait();
}

void parallel_execute(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                      const std::vector<double>& bytes_amounts)
{
  exec_init(hosts, flops_amounts, bytes_amounts)->wait();
}

ExecPtr exec_init(double flops_amount)
{
  return Exec::init()->set_flops_amount(flops_amount)->set_host(get_host());
}

ExecPtr exec_init(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                  const std::vector<double>& bytes_amounts)
{
  xbt_assert(not hosts.empty(), "Your parallel executions must span over at least one host.");
  xbt_assert(hosts.size() == flops_amounts.size() || flops_amounts.empty(),
             "Host count (%zu) does not match flops_amount count (%zu).", hosts.size(), flops_amounts.size());
  xbt_assert(hosts.size() * hosts.size() == bytes_amounts.size() || bytes_amounts.empty(),
             "bytes_amounts must be a matrix of size host_count * host_count (%zu*%zu), but it's of size %zu.",
             hosts.size(), hosts.size(), bytes_amounts.size());
  /* Check that we are not mixing VMs and PMs in the parallel task */
  bool is_a_vm = (nullptr != dynamic_cast<VirtualMachine*>(hosts.front()));
  xbt_assert(std::all_of(hosts.begin(), hosts.end(),
                         [is_a_vm](s4u::Host* elm) {
                           bool tmp_is_a_vm = (nullptr != dynamic_cast<VirtualMachine*>(elm));
                           return is_a_vm == tmp_is_a_vm;
                         }),
             "parallel_execute: mixing VMs and PMs is not supported (yet).");
  /* checking for infinite values */
  xbt_assert(std::all_of(flops_amounts.begin(), flops_amounts.end(), [](double elm) { return std::isfinite(elm); }),
             "flops_amounts comprises infinite values!");
  xbt_assert(std::all_of(bytes_amounts.begin(), bytes_amounts.end(), [](double elm) { return std::isfinite(elm); }),
             "flops_amounts comprises infinite values!");

  return Exec::init()->set_flops_amounts(flops_amounts)->set_bytes_amounts(bytes_amounts)->set_hosts(hosts);
}

ExecPtr exec_async(double flops)
{
  ExecPtr res = exec_init(flops);
  res->vetoable_start();
  return res;
}

aid_t get_pid()
{
  return simgrid::kernel::actor::ActorImpl::self()->get_pid();
}

aid_t get_ppid()
{
  return simgrid::kernel::actor::ActorImpl::self()->get_ppid();
}

std::string get_name()
{
  return simgrid::kernel::actor::ActorImpl::self()->get_name();
}

const char* get_cname()
{
  return simgrid::kernel::actor::ActorImpl::self()->get_cname();
}

Host* get_host()
{
  return simgrid::kernel::actor::ActorImpl::self()->get_host();
}

void suspend()
{
  kernel::actor::ActorImpl* self = simgrid::kernel::actor::ActorImpl::self();
  s4u::Actor::on_suspend(*self->get_ciface());
  kernel::actor::simcall_blocking([self] { self->suspend(); });
}

void exit()
{
  kernel::actor::ActorImpl* self = simgrid::kernel::actor::ActorImpl::self();
  simgrid::kernel::actor::simcall([self] { self->exit(); });
  THROW_IMPOSSIBLE;
}

void on_exit(const std::function<void(bool)>& fun)
{
  simgrid::kernel::actor::ActorImpl::self()->get_iface()->on_exit(fun);
}

/** @brief Moves the current actor to another host
 *
 * @see simgrid::s4u::Actor::migrate() for more information
 */
void set_host(Host* new_host)
{
  simgrid::kernel::actor::ActorImpl::self()->get_iface()->set_host(new_host);
}

} // namespace this_actor
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */
size_t sg_actor_count()
{
  return simgrid::s4u::Engine::get_instance()->get_actor_count();
}

sg_actor_t* sg_actor_list()
{
  const simgrid::s4u::Engine* e = simgrid::s4u::Engine::get_instance();
  size_t actor_count      = e->get_actor_count();
  xbt_assert(actor_count > 0, "There is no actor!");
  std::vector<simgrid::s4u::ActorPtr> actors = e->get_all_actors();

  auto* res = xbt_new(sg_actor_t, actors.size());
  for (size_t i = 0; i < actor_count; i++)
    res[i] = actors[i].get();
  return res;
}

sg_actor_t sg_actor_init(const char* name, sg_host_t host)
{
  return simgrid::s4u::Actor::init(name, host).get();
}

void sg_actor_start_(sg_actor_t actor, xbt_main_func_t code, int argc, const char* const* argv)
{
  simgrid::kernel::actor::ActorCode function;
  if (code)
    function = simgrid::xbt::wrap_main(code, argc, argv);
  actor->start(function);
}

sg_actor_t sg_actor_create_(const char* name, sg_host_t host, xbt_main_func_t code, int argc, const char* const* argv)
{
  simgrid::kernel::actor::ActorCode function = simgrid::xbt::wrap_main(code, argc, argv);
  return simgrid::s4u::Actor::init(name, host)->start(function).get();
}

void sg_actor_set_stacksize(sg_actor_t actor, unsigned size)
{
  actor->set_stacksize(size);
}

void sg_actor_exit()
{
  simgrid::s4u::this_actor::exit();
}

/**
 * @brief Returns the process ID of @a actor.
 *
 * This function checks whether @a actor is a valid pointer and return its PID (or 0 in case of problem).
 */

aid_t sg_actor_get_pid(const_sg_actor_t actor)
{
  /* Do not raise an exception here: this function is called by the logs
   * and the exceptions, so it would be called back again and again */
  if (actor == nullptr || actor->get_impl() == nullptr)
    return 0;
  return actor->get_pid();
}

/**
 * @brief Returns the process ID of the parent of @a actor.
 *
 * This function checks whether @a actor is a valid pointer and return its parent's PID.
 * Returns -1 if the actor has not been created by any other actor.
 */
aid_t sg_actor_get_ppid(const_sg_actor_t actor)
{
  return actor->get_ppid();
}

/**
 * @brief Return a #sg_actor_t given its PID.
 *
 * This function search in the list of all the created sg_actor_t for a sg_actor_t  whose PID is equal to @a PID.
 * If none is found, @c nullptr is returned.
   Note that the PID are unique in the whole simulation, not only on a given host.
 */
sg_actor_t sg_actor_by_pid(aid_t pid)
{
  return simgrid::s4u::Actor::by_pid(pid).get();
}

aid_t sg_actor_get_PID(const_sg_actor_t actor) // XBT_ATTRIB_DEPRECATED_v331
{
  return sg_actor_get_pid(actor);
}

aid_t sg_actor_get_PPID(const_sg_actor_t actor) // XBT_ATTRIB_DEPRECATED_v331
{
  return sg_actor_get_ppid(actor);
}

sg_actor_t sg_actor_by_PID(aid_t pid) // XBT_ATTRIB_DEPRECATED_v331
{
  return sg_actor_by_pid(pid);
}

/** @brief Return the name of an actor. */
const char* sg_actor_get_name(const_sg_actor_t actor)
{
  return actor->get_cname();
}

sg_host_t sg_actor_get_host(const_sg_actor_t actor)
{
  return actor->get_host();
}

/**
 * @brief Returns the value of a given actor property
 *
 * @param actor an actor
 * @param name a property name
 * @return value of a property (or nullptr if the property is not set)
 */
const char* sg_actor_get_property_value(const_sg_actor_t actor, const char* name)
{
  return actor->get_property(name);
}

/**
 * @brief Return the list of properties
 *
 * This function returns all the parameters associated with an actor
 */
xbt_dict_t sg_actor_get_properties(const_sg_actor_t actor)
{
  xbt_assert(actor != nullptr, "Invalid parameter: First argument must not be nullptr");
  xbt_dict_t as_dict                        = xbt_dict_new_homogeneous(xbt_free_f);
  const std::unordered_map<std::string, std::string>* props = actor->get_properties();
  if (props == nullptr)
    return nullptr;
  for (auto const& kv : *props) {
    xbt_dict_set(as_dict, kv.first.c_str(), xbt_strdup(kv.second.c_str()));
  }
  return as_dict;
}

/**
 * @brief Suspend the actor.
 *
 * This function suspends the actor by suspending the task on which it was waiting for the completion.
 */
void sg_actor_suspend(sg_actor_t actor)
{
  xbt_assert(actor != nullptr, "Invalid parameter: First argument must not be nullptr");
  actor->suspend();
}

/**
 * @brief Resume a suspended actor.
 *
 * This function resumes a suspended actor by resuming the task on which it was waiting for the completion.
 */
void sg_actor_resume(sg_actor_t actor)
{
  xbt_assert(actor != nullptr, "Invalid parameter: First argument must not be nullptr");
  actor->resume();
}

/**
 * @brief Returns true if the actor is suspended .
 *
 * This checks whether an actor is suspended or not by inspecting the task on which it was waiting for the completion.
 */
int sg_actor_is_suspended(const_sg_actor_t actor)
{
  return actor->is_suspended();
}

/** @brief Restarts an actor from the beginning. */
sg_actor_t sg_actor_restart(sg_actor_t actor)
{
  return actor->restart();
}

/**
 * @brief Sets the "auto-restart" flag of the actor.
 * If the flag is set to 1, the actor will be automatically restarted when its host comes back up.
 */
void sg_actor_set_auto_restart(sg_actor_t actor, int auto_restart)
{
  actor->set_auto_restart(auto_restart);
}

/** @brief This actor will be terminated automatically when the last non-daemon actor finishes */
void sg_actor_daemonize(sg_actor_t actor)
{
  actor->daemonize();
}

/** Returns whether or not this actor has been daemonized or not */
int sg_actor_is_daemon(const_sg_actor_t actor)
{
  return actor->is_daemon();
}

/**
 * @brief Migrates an actor to another location.
 *
 * This function changes the value of the #sg_host_t on  which @a actor is running.
 */
void sg_actor_set_host(sg_actor_t actor, sg_host_t host)
{
  actor->set_host(host);
}

/**
 * @brief Wait for the completion of a #sg_actor_t.
 *
 * @param actor the actor to wait for
 * @param timeout wait until the actor is over, or the timeout expires
 */
void sg_actor_join(const_sg_actor_t actor, double timeout)
{
  actor->join(timeout);
}

void sg_actor_kill(sg_actor_t actor)
{
  actor->kill();
}

void sg_actor_kill_all()
{
  simgrid::s4u::Actor::kill_all();
}

/**
 * @brief Set the kill time of an actor.
 *
 * @param actor an actor
 * @param kill_time the time when the actor is killed.
 */
void sg_actor_set_kill_time(sg_actor_t actor, double kill_time)
{
  actor->set_kill_time(kill_time);
}

/** Yield the current actor; let the other actors execute first */
void sg_actor_yield()
{
  simgrid::s4u::this_actor::yield();
}

void sg_actor_sleep_for(double duration)
{
  simgrid::s4u::this_actor::sleep_for(duration);
}

void sg_actor_sleep_until(double wakeup_time)
{
  simgrid::s4u::this_actor::sleep_until(wakeup_time);
}

sg_actor_t sg_actor_attach(const char* name, void* data, sg_host_t host, xbt_dict_t properties)
{
  xbt_assert(host != nullptr, "Invalid parameters: host and code params must not be nullptr");
  std::unordered_map<std::string, std::string> props;
  xbt_dict_cursor_t cursor = nullptr;
  char* key;
  char* value;
  xbt_dict_foreach (properties, cursor, key, value)
    props[key] = value;
  xbt_dict_free(&properties);

  /* Let's create the actor: SIMIX may decide to start it right now, even before returning the flow control to us */
  smx_actor_t actor = nullptr;
  try {
    actor = simgrid::kernel::actor::ActorImpl::attach(name, data, host).get();
    actor->set_properties(props);
  } catch (simgrid::HostFailureException const&) {
    xbt_die("Could not attach");
  }

  simgrid::s4u::this_actor::yield();
  return actor->get_ciface();
}

void sg_actor_detach()
{
  simgrid::kernel::actor::ActorImpl::detach();
}

aid_t sg_actor_self_get_pid()
{
  return simgrid::s4u::this_actor::get_pid();
}

aid_t sg_actor_self_get_ppid()
{
  return simgrid::s4u::this_actor::get_ppid();
}

const char* sg_actor_self_get_name()
{
  return simgrid::s4u::this_actor::get_cname();
}

void* sg_actor_self_get_data()
{
  return simgrid::s4u::Actor::self()->get_data();
}

void sg_actor_self_set_data(void* userdata)
{
  simgrid::s4u::Actor::self()->set_data(userdata);
}

void* sg_actor_self_data() // XBT_ATTRIB_DEPRECATED_v330
{
  return sg_actor_self_get_data();
}

void sg_actor_self_data_set(void* userdata) // XBT_ATTRIB_DEPRECATED_v330
{
  sg_actor_self_set_data(userdata);
}

sg_actor_t sg_actor_self()
{
  return simgrid::s4u::Actor::self();
}

void sg_actor_self_execute(double flops) // XBT_ATTRIB_DEPRECATED_v330
{
  simgrid::s4u::this_actor::execute(flops);
}

void sg_actor_execute(double flops)
{
  simgrid::s4u::this_actor::execute(flops);
}
void sg_actor_execute_with_priority(double flops, double priority)
{
  simgrid::s4u::this_actor::exec_init(flops)->set_priority(priority)->wait();
}

void sg_actor_parallel_execute(int host_nb, sg_host_t* host_list, double* flops_amount, double* bytes_amount)
{
  std::vector<simgrid::s4u::Host*> hosts(host_list, host_list + host_nb);
  std::vector<double> flops;
  std::vector<double> bytes;
  if (flops_amount != nullptr)
    flops = std::vector<double>(flops_amount, flops_amount + host_nb);
  if (bytes_amount != nullptr)
    bytes = std::vector<double>(bytes_amount, bytes_amount + host_nb * host_nb);

  simgrid::s4u::this_actor::parallel_execute(hosts, flops, bytes);
}

/** @brief Take an extra reference on that actor to prevent it to be garbage-collected */
void sg_actor_ref(const_sg_actor_t actor)
{
  intrusive_ptr_add_ref(actor);
}
/** @brief Release a reference on that actor so that it can get be garbage-collected */
void sg_actor_unref(const_sg_actor_t actor)
{
  intrusive_ptr_release(actor);
}

/** @brief Return the user data of a #sg_actor_t */
void* sg_actor_get_data(const_sg_actor_t actor)
{
  return actor->get_data();
}

/** @brief Set the user data of a #sg_actor_t */
void sg_actor_set_data(sg_actor_t actor, void* userdata)
{
  actor->set_data(userdata);
}

void* sg_actor_data(const_sg_actor_t actor) // XBT_ATTRIB_DEPRECATED_v330
{
  return sg_actor_get_data(actor);
}

void sg_actor_data_set(sg_actor_t actor, void* userdata) // XBT_ATTRIB_DEPRECATED_v330
{
  sg_actor_set_data(actor, userdata);
}

/** @brief Add a function to the list of "on_exit" functions for the current actor.
 *  The on_exit functions are the functions executed when your actor is killed.
 *  You should use them to free the data used by your actor.
 */
void sg_actor_on_exit(void_f_int_pvoid_t fun, void* data)
{
  simgrid::s4u::this_actor::on_exit([fun, data](bool failed) { fun(failed ? 1 /*FAILURE*/ : 0 /*SUCCESS*/, data); });
}

sg_exec_t sg_actor_exec_init(double computation_amount)
{
  simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(computation_amount);
  exec->add_ref();
  return exec.get();
}

sg_exec_t sg_actor_parallel_exec_init(int host_nb, const sg_host_t* host_list, double* flops_amount,
                                      double* bytes_amount)
{
  std::vector<simgrid::s4u::Host*> hosts(host_list, host_list + host_nb);
  std::vector<double> flops;
  std::vector<double> bytes;
  if (flops_amount != nullptr)
    flops = std::vector<double>(flops_amount, flops_amount + host_nb);
  if (bytes_amount != nullptr)
    bytes = std::vector<double>(bytes_amount, bytes_amount + host_nb * host_nb);

  simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(hosts, flops, bytes);
  exec->add_ref();
  return exec.get();
}

sg_exec_t sg_actor_exec_async(double computation_amount)
{
  simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_async(computation_amount);
  exec->add_ref();
  return exec.get();
}
