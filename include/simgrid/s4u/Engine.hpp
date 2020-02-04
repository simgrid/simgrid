/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ENGINE_HPP
#define SIMGRID_S4U_ENGINE_HPP

#include <xbt/base.h>
#include <xbt/functional.hpp>

#include <simgrid/forward.h>
#include <simgrid/simix.hpp>

#include <simgrid/s4u/NetZone.hpp>

#include <string>
#include <utility>
#include <vector>

namespace simgrid {
namespace s4u {
/** @brief Simulation engine
 *
 * This is a singleton containing all the main functions of the simulation.
 */
class XBT_PUBLIC Engine {
  friend simgrid::kernel::EngineImpl;

public:
  /** Constructor, taking the command line parameters of your main function */
  explicit Engine(int* argc, char** argv);
  /* Currently, only one instance is allowed to exist. This is why you can't copy or move it */
#ifndef DOXYGEN
  Engine(const Engine&) = delete;
  Engine(Engine&&)      = delete;
  ~Engine();
#endif

  /** Finalize the default engine and all its dependencies */
  static void shutdown();

  /** Run the simulation after initialization */
  void run();

  /** @brief Retrieve the simulation time (in seconds) */
  static double get_clock();
  /** @brief Retrieve the engine singleton */
  static s4u::Engine* get_instance();

  void load_platform(const std::string& platf);

  XBT_ATTRIB_DEPRECATED_v330("Please change the return code of your actors to void") void register_function(
      const std::string& name, int (*code)(int, char**));

  void register_function(const std::string& name, void (*code)(int, char**));
  void register_function(const std::string& name, void (*code)(std::vector<std::string>));

  XBT_ATTRIB_DEPRECATED_v330("Please change the return code of your actors to void") void register_default(
      int (*code)(int, char**));
  void register_default(void (*code)(int, char**));
  void register_default(kernel::actor::ActorCodeFactory factory);

  void register_function(const std::string& name, kernel::actor::ActorCodeFactory factory);
  template <class F> void register_actor(const std::string& name)
  {
    register_function(name, [](std::vector<std::string> args) {
      return kernel::actor::ActorCode([args] {
        F code(std::move(args));
        code();
      });
    });
  }
  template <class F> void register_actor(const std::string& name, F code)
  {
    register_function(name, [code](std::vector<std::string> args) {
      return kernel::actor::ActorCode([code, args] { code(std::move(args)); });
    });
  }

  void load_deployment(const std::string& deploy);

protected:
#ifndef DOXYGEN
  friend Host;
  friend Link;
  friend Disk;
  friend Storage;
  friend kernel::routing::NetPoint;
  friend kernel::routing::NetZoneImpl;
  friend kernel::resource::LinkImpl;
  void host_register(const std::string& name, Host* host);
  void host_unregister(const std::string& name);
  void link_register(const std::string& name, const Link* link);
  void link_unregister(const std::string& name);
  void storage_register(const std::string& name, const Storage* storage);
  void storage_unregister(const std::string& name);
  void netpoint_register(simgrid::kernel::routing::NetPoint* card);
  void netpoint_unregister(simgrid::kernel::routing::NetPoint* card);
#endif /*DOXYGEN*/

public:
  /** Returns the amount of hosts existing in the platform. */
  size_t get_host_count();
  /** Returns a vector of all hosts found in the platform.
   *
   * The order is generally different from the creation/declaration order in the XML platform because we use a hash
   * table internally.
   */
  std::vector<Host*> get_all_hosts();
  std::vector<Host*> get_filtered_hosts(const std::function<bool(Host*)>& filter);
  Host* host_by_name(const std::string& name);
  Host* host_by_name_or_null(const std::string& name);

  size_t get_link_count();
  std::vector<Link*> get_all_links();
  std::vector<Link*> get_filtered_links(const std::function<bool(Link*)>& filter);
  Link* link_by_name(const std::string& name);
  Link* link_by_name_or_null(const std::string& name);

  size_t get_actor_count();
  std::vector<ActorPtr> get_all_actors();
  std::vector<ActorPtr> get_filtered_actors(const std::function<bool(ActorPtr)>& filter);

#ifndef DOXYGEN
  size_t get_storage_count();
  std::vector<Storage*> get_all_storages();
  Storage* storage_by_name(const std::string& name);
  Storage* storage_by_name_or_null(const std::string& name);
#endif

  std::vector<kernel::routing::NetPoint*> get_all_netpoints();
  kernel::routing::NetPoint* netpoint_by_name_or_null(const std::string& name);

  NetZone* get_netzone_root();
  void set_netzone_root(const NetZone* netzone);

  NetZone* netzone_by_name_or_null(const std::string& name);

  /** @brief Retrieves all netzones of the type indicated by the template argument */
  template <class T> std::vector<T*> get_filtered_netzones()
  {
    static_assert(std::is_base_of<kernel::routing::NetZoneImpl, T>::value,
                  "Filtering netzones is only possible for subclasses of kernel::routing::NetZoneImpl");
    std::vector<T*> res;
    get_filtered_netzones_recursive(get_netzone_root(), &res);
    return res;
  }

  /** Returns whether SimGrid was initialized yet -- mostly for internal use */
  static bool is_initialized();
  /** @brief set a configuration variable
   *
   * @beginrst
   * Do --help on any simgrid binary to see the list of currently existing configuration variables
   * (see also :ref:`options`).
   * @endrst
   *
   * Example:
   * e->set_config("host/model:ptask_L07");
   */
  void set_config(const std::string& str);

  /** Callback fired when the platform is created (ie, the xml file parsed),
   * right before the actual simulation starts. */
  static xbt::signal<void()> on_platform_created;

  /** Callback fired when the platform is about to be created
   * (ie, after any configuration change and just before the resource creation) */
  static xbt::signal<void()> on_platform_creation;

  /** Callback fired when the main simulation loop ends, just before the end of Engine::run() */
  static xbt::signal<void()> on_simulation_end;

  /** Callback fired when the time jumps into the future */
  static xbt::signal<void(double)> on_time_advance;

  /** Callback fired when the time cannot advance because of inter-actors deadlock */
  static xbt::signal<void(void)> on_deadlock;

private:
  kernel::EngineImpl* const pimpl;
  static Engine* instance_;
};

#ifndef DOXYGEN /* Internal use only, no need to expose it */
template <class T>
XBT_PRIVATE void get_filtered_netzones_recursive(const s4u::NetZone* current, std::vector<T*>* whereto)
{
  static_assert(std::is_base_of<kernel::routing::NetZoneImpl, T>::value,
                "Filtering netzones is only possible for subclasses of kernel::routing::NetZoneImpl");
  for (auto const& elem : current->get_children()) {
    get_filtered_netzones_recursive(elem, whereto);
    T* elem_impl = dynamic_cast<T*>(elem->get_impl());
    if (elem_impl != nullptr)
      whereto->push_back(elem_impl);
  }
}
#endif
} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_ENGINE_HPP */
