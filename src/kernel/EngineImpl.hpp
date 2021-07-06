/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ENGINEIMPL_HPP
#define SIMGRID_KERNEL_ENGINEIMPL_HPP

#include <simgrid/kernel/resource/Model.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/simix.hpp>
#include <xbt/dynar.h>
#include <xbt/functional.hpp>

#include "src/kernel/activity/ExecImpl.hpp"
#include "src/kernel/activity/IoImpl.hpp"
#include "src/kernel/activity/MailboxImpl.hpp"
#include "src/kernel/activity/SleepImpl.hpp"
#include "src/kernel/activity/SynchroRaw.hpp"
#include "src/kernel/actor/ActorImpl.hpp"
#include "src/surf/SplitDuplexLinkImpl.hpp"

#include <boost/intrusive/list.hpp>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace simgrid {
namespace kernel {

class EngineImpl {
  std::map<std::string, s4u::Host*, std::less<>> hosts_;
  std::map<std::string, resource::LinkImpl*, std::less<>> links_;
  /* save split-duplex links separately, keep links_ with only LinkImpl* seen by the user
   * members of a split-duplex are saved in the links_ */
  std::map<std::string, std::unique_ptr<resource::SplitDuplexLinkImpl>, std::less<>> split_duplex_links_;
  std::unordered_map<std::string, routing::NetPoint*> netpoints_;
  std::unordered_map<std::string, activity::MailboxImpl*> mailboxes_;

  std::unordered_map<std::string, actor::ActorCodeFactory> registered_functions; // Maps function names to actor code
  actor::ActorCodeFactory default_function; // Function to use as a fallback when the provided name matches nothing
  std::vector<resource::Model*> models_;
  std::unordered_map<std::string, std::shared_ptr<resource::Model>> models_prio_;
  routing::NetZoneImpl* netzone_root_ = nullptr;
  std::set<actor::ActorImpl*> daemons_;
  std::vector<actor::ActorImpl*> actors_to_run_;
  std::vector<actor::ActorImpl*> actors_that_ran_;
  std::map<aid_t, actor::ActorImpl*> actor_list_;
  boost::intrusive::list<actor::ActorImpl,
                         boost::intrusive::member_hook<actor::ActorImpl, boost::intrusive::list_member_hook<>,
                                                       &actor::ActorImpl::kernel_destroy_list_hook>>
      actors_to_destroy_;
#if SIMGRID_HAVE_MC
  /* MCer cannot read members actor_list_ and actors_to_destroy_ above in the remote process, so we copy the info it
   * needs in a dynar.
   * FIXME: This is supposed to be a temporary hack.
   * A better solution would be to change the split between MCer and MCed, where the responsibility
   *   to compute the list of the enabled transitions goes to the MCed.
   * That way, the MCer would not need to have the list of actors on its side.
   * These info could be published by the MCed to the MCer in a way inspired of vd.so
   */
  xbt_dynar_t actors_vector_      = xbt_dynar_new(sizeof(actor::ActorImpl*), nullptr);
  xbt_dynar_t dead_actors_vector_ = xbt_dynar_new(sizeof(actor::ActorImpl*), nullptr);
#endif

  std::vector<xbt::Task<void()>> tasks;

  std::mutex mutex_;
  std::unique_ptr<void, std::function<int(void*)>> platf_handle_; //!< handle for platform library
  friend s4u::Engine;

public:
  EngineImpl() = default;

  EngineImpl(const EngineImpl&) = delete;
  EngineImpl& operator=(const EngineImpl&) = delete;
  virtual ~EngineImpl();

  void load_platform(const std::string& platf);
  void load_deployment(const std::string& file) const;
  void register_function(const std::string& name, const actor::ActorCodeFactory& code);
  void register_default(const actor::ActorCodeFactory& code);

  /**
   * @brief Add a model to engine list
   *
   * @param model Pointer to model
   * @param list  List of dependencies for this model
   */
  void add_model(std::shared_ptr<simgrid::kernel::resource::Model> model,
                 const std::vector<resource::Model*>& dep_models = {});

  /** @brief Get list of all models managed by this engine */
  const std::vector<resource::Model*>& get_all_models() const { return models_; }

  static EngineImpl* get_instance() { return simgrid::s4u::Engine::get_instance()->pimpl; }
  actor::ActorCodeFactory get_function(const std::string& name)
  {
    auto res = registered_functions.find(name);
    if (res == registered_functions.end())
      return default_function;
    else
      return res->second;
  }
  void add_daemon(actor::ActorImpl* d) { daemons_.insert(d); }
  void remove_daemon(actor::ActorImpl* d);
  void add_actor_to_run_list(actor::ActorImpl* actor);
  void add_actor_to_run_list_no_check(actor::ActorImpl* actor);
  void add_actor_to_destroy_list(actor::ActorImpl& actor) { actors_to_destroy_.push_back(actor); }

  bool has_actors_to_run() const { return not actors_to_run_.empty(); }
  const actor::ActorImpl* get_first_actor_to_run() const { return actors_to_run_.front(); }
  const actor::ActorImpl* get_actor_to_run_at(unsigned long int i) const { return actors_to_run_[i]; }
  unsigned long int get_actor_to_run_count() const { return actors_to_run_.size(); }
  size_t get_actor_count() const { return actor_list_.size(); }
  actor::ActorImpl* get_actor_by_pid(aid_t pid);
  void add_actor(aid_t pid, actor::ActorImpl* actor) { actor_list_[pid] = actor; }
  void remove_actor(aid_t pid) { actor_list_.erase(pid); }
  void add_split_duplex_link(const std::string& name, std::unique_ptr<resource::SplitDuplexLinkImpl> link);

#if SIMGRID_HAVE_MC
  xbt_dynar_t get_actors_vector() const { return actors_vector_; }
  xbt_dynar_t get_dead_actors_vector() const { return dead_actors_vector_; }
  void reset_actor_dynar() { xbt_dynar_reset(actors_vector_); }
  void add_actor_to_dynar(actor::ActorImpl* actor) { xbt_dynar_push_as(actors_vector_, actor::ActorImpl*, actor); }
  void add_dead_actor_to_dynar(actor::ActorImpl* actor)
  {
    xbt_dynar_push_as(dead_actors_vector_, actor::ActorImpl*, actor);
  }
#endif

  const std::map<aid_t, actor::ActorImpl*>& get_actor_list() const { return actor_list_; }
  const std::vector<actor::ActorImpl*>& get_actors_to_run() const { return actors_to_run_; }
  const std::vector<actor::ActorImpl*>& get_actors_that_ran() const { return actors_that_ran_; }

  std::mutex& get_mutex() { return mutex_; }
  bool execute_tasks();
  void add_task(xbt::Task<void()>&& t) { tasks.push_back(std::move(t)); }
  void wake_all_waiting_actors() const;
  /**
   * Garbage collection
   *
   * Should be called some time to time to free the memory allocated for actors that have finished (or killed).
   */
  void empty_trash();
  void display_all_actor_status() const;
  void run_all_actors();

  /** @brief Run the main simulation loop. */
  void run();
};

} // namespace kernel
} // namespace simgrid

#endif
