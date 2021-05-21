/* Copyright (c) 2016-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ENGINEIMPL_HPP
#define SIMGRID_KERNEL_ENGINEIMPL_HPP

#include <simgrid/kernel/resource/Model.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/NetZone.hpp>
#include <simgrid/simix.hpp>
#include <xbt/functional.hpp>

#include <map>
#include <set>
#include <string>
#include <unordered_map>

namespace simgrid {
namespace kernel {

class EngineImpl {
  std::map<std::string, s4u::Host*, std::less<>> hosts_;
  std::map<std::string, resource::LinkImpl*, std::less<>> links_;
  std::unordered_map<std::string, routing::NetPoint*> netpoints_;
  std::unordered_map<std::string, actor::ActorCodeFactory> registered_functions; // Maps function names to actor code
  actor::ActorCodeFactory default_function; // Function to use as a fallback when the provided name matches nothing
  std::vector<resource::Model*> models_;
  std::unordered_map<std::string, std::shared_ptr<resource::Model>> models_prio_;
  routing::NetZoneImpl* netzone_root_ = nullptr;
  std::set<actor::ActorImpl*> daemons_;
  std::vector<actor::ActorImpl*> actors_to_run_;
  std::vector<actor::ActorImpl*> actors_that_ran_;

  std::vector<xbt::Task<void()>> tasks;
  std::vector<xbt::Task<void()>> tasksTemp;

  friend s4u::Engine;

public:
  EngineImpl() = default;

  EngineImpl(const EngineImpl&) = delete;
  EngineImpl& operator=(const EngineImpl&) = delete;
  virtual ~EngineImpl();

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
  void rm_daemon(actor::ActorImpl* d);
  void add_actor_to_run_list(actor::ActorImpl* a);
  void add_actor_to_run_list_no_check(actor::ActorImpl* a);
  bool has_actors_to_run() { return not actors_to_run_.empty(); }
  const actor::ActorImpl* get_first_actor_to_run() const { return actors_to_run_.front(); }
  const actor::ActorImpl* get_actor_to_run_at(unsigned long int i) const { return actors_to_run_[i]; }
  unsigned long int get_actor_to_run_count() { return actors_to_run_.size(); }

  const std::vector<actor::ActorImpl*>& get_actors_to_run() const { return actors_to_run_; }
  const std::vector<actor::ActorImpl*>& get_actors_that_ran() const { return actors_that_ran_; }

  bool execute_tasks();
  void add_task(xbt::Task<void()>&& t) { tasks.push_back(std::move(t)); }
  void wake_all_waiting_actors() const;
  void display_all_actor_status() const;
  void run_all_actors();

  /** @brief Run the main simulation loop. */
  void run();
};

} // namespace kernel
} // namespace simgrid

#endif
