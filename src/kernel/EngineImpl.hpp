/* Copyright (c) 2016-2023. The SimGrid Team. All rights reserved.          */

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
#include "src/kernel/activity/MessageQueueImpl.hpp"
#include "src/kernel/activity/SleepImpl.hpp"
#include "src/kernel/activity/Synchro.hpp"
#include "src/kernel/actor/ActorImpl.hpp"

#include <boost/intrusive/list.hpp>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace simgrid::kernel {

class EngineImpl {
  std::unordered_map<std::string, routing::NetPoint*> netpoints_;
  std::unordered_map<std::string, activity::MailboxImpl*> mailboxes_;
  std::unordered_map<std::string, activity::MessageQueueImpl*> mqueues_;

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

  static double now_;
  static EngineImpl* instance_;
  actor::ActorImpl* maestro_ = nullptr;
  context::ContextFactory* context_factory_ = nullptr;

  std::unique_ptr<void, std::function<int(void*)>> platf_handle_; //!< handle for platform library
  friend s4u::Engine;

  std::vector<std::string> cmdline_; // Copy of the argv we got (including argv[0])

public:
  EngineImpl() = default;

  /* Currently, only one instance is allowed to exist. This is why you can't copy or move it */
#ifndef DOXYGEN
  EngineImpl(const EngineImpl&) = delete;
  EngineImpl& operator=(const EngineImpl&) = delete;
  virtual ~EngineImpl();
  static void shutdown();
#endif

  void initialize(int* argc, char** argv);
  const std::vector<std::string>& get_cmdline() const
  {
    return cmdline_;
  }
  void load_platform(const std::string& platf);
  void load_deployment(const std::string& file) const;
  void seal_platform() const;
  void register_function(const std::string& name, const actor::ActorCodeFactory& code);
  void register_default(const actor::ActorCodeFactory& code);

  bool is_maestro(const actor::ActorImpl* actor) const { return actor == maestro_; }
  void set_maestro(actor::ActorImpl* actor) { maestro_ = actor; }
  actor::ActorImpl* get_maestro() const { return maestro_; }

  context::ContextFactory* get_context_factory() const { return context_factory_; }
  void set_context_factory(context::ContextFactory* factory) { context_factory_ = factory; }
  bool has_context_factory() const { return context_factory_ != nullptr; }

  void context_mod_init() const;
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

  static bool has_instance() { return s4u::Engine::has_instance(); }
  static EngineImpl* get_instance()
  {
    return s4u::Engine::get_instance()->pimpl_;
  }
  static EngineImpl* get_instance(int* argc, char** argv)
  {
    return s4u::Engine::get_instance(argc, argv)->pimpl_;
  }

  actor::ActorCodeFactory get_function(const std::string& name)
  {
    auto res = registered_functions.find(name);
    if (res == registered_functions.end())
      return default_function;
    else
      return res->second;
  }

  routing::NetZoneImpl* get_netzone_root() const { return netzone_root_; }

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

  const std::map<aid_t, actor::ActorImpl*>& get_actor_list() const { return actor_list_; }
  const std::vector<actor::ActorImpl*>& get_actors_to_run() const { return actors_to_run_; }
  const std::vector<actor::ActorImpl*>& get_actors_that_ran() const { return actors_that_ran_; }

  void handle_ended_actions() const;
  /**
   * Garbage collection
   *
   * Should be called some time to time to free the memory allocated for actors that have finished (or killed).
   */
  void empty_trash();
  void display_all_actor_status() const;
  void run_all_actors();

  /** @brief Performs a part of the simulation
   *  @param max_date Maximum date to update the simulation to, or -1
   *  @return the elapsed time, or -1.0 if no event could be executed
   *
   *  This function execute all possible events, update the action states  and returns the time elapsed.
   *  When you call execute or communicate on a model, the corresponding actions are not executed immediately but only
   *  when you call solve().
   *  Note that the returned elapsed time can be zero.
   */
  double solve(double max_date) const;

  /** @brief Run the main simulation loop until the specified date (or infinitly if max_date is negative). */
  void run(double max_date);

  /** @brief Return the current time in milliseconds. */
  static double get_clock();
};

} // namespace simgrid::kernel

#endif
