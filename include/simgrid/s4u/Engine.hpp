/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ENGINE_HPP
#define SIMGRID_S4U_ENGINE_HPP

#include <string>
#include <utility>
#include <vector>

#include <xbt/base.h>
#include <xbt/functional.hpp>

#include <simgrid/forward.h>
#include <simgrid/simix.hpp>

#include <simgrid/s4u/NetZone.hpp>

namespace simgrid {
namespace s4u {
/** @brief Simulation engine
 *
 * This class is an interface to the simulation engine.
 */
class XBT_PUBLIC Engine {
public:
  /** Constructor, taking the command line parameters of your main function */
  Engine(int* argc, char** argv);

  ~Engine();
  /** Finalize the default engine and all its dependencies */
  static void shutdown();

  /** @brief Run the simulation */
  void run();

  /** @brief Retrieve the simulation time */
  static double get_clock();
  /** @brief Retrieve the engine singleton */
  static s4u::Engine* get_instance();

  /** @brief Load a platform file describing the environment
   *
   * The environment is either a XML file following the simgrid.dtd formalism, or a lua file.
   * Some examples can be found in the directory examples/platforms.
   */
  void load_platform(const char* platf);

  /** Registers the main function of an actor that will be launched from the deployment file */
  void register_function(const char* name, int (*code)(int, char**));
  // FIXME: provide a register_function(std::string, void (*code)(int, char**)) and deprecate the int returning one
  // FIXME: provide a register_function(std::string, std::vector<std::string>)

  /** Registers a function as the default main function of actors
   *
   * It will be used as fallback when the function requested from the deployment file was not registered.
   * It is used for trace-based simulations (see examples/s4u/replay-comms and similar).
   */
  void register_default(int (*code)(int, char**));

  template <class F> void register_actor(const char* name)
  {
    simgrid::simix::register_function(name, [](std::vector<std::string> args) {
      return simgrid::simix::ActorCode([args] {
        F code(std::move(args));
        code();
      });
    });
  }

  template <class F> void register_actor(const char* name, F code)
  {
    simgrid::simix::register_function(name, [code](std::vector<std::string> args) {
      return simgrid::simix::ActorCode([code, args] { code(std::move(args)); });
    });
  }

  /** @brief Load a deployment file and launch the actors that it contains */
  void load_deployment(const char* deploy);

protected:
  friend s4u::Host;
  friend s4u::Storage;
  friend kernel::routing::NetPoint;
  friend kernel::routing::NetZoneImpl;
  void host_register(std::string name, simgrid::s4u::Host* host);
  void host_unregister(std::string name);
  void storage_register(std::string name, simgrid::s4u::Storage* storage);
  void storage_unregister(std::string name);
  void netpoint_register(simgrid::kernel::routing::NetPoint* card);
  void netpoint_unregister(simgrid::kernel::routing::NetPoint* card);

public:
  size_t get_host_count();
  std::vector<Host*> get_all_hosts();
  simgrid::s4u::Host* host_by_name(std::string name);
  simgrid::s4u::Host* host_by_name_or_null(std::string name);

  size_t get_link_count();
  std::vector<Link*> get_all_links();

  size_t get_storage_count();
  std::vector<Storage*> get_all_storages();
  simgrid::s4u::Storage* storage_by_name(std::string name);
  simgrid::s4u::Storage* storage_by_name_or_null(std::string name);

  std::vector<simgrid::kernel::routing::NetPoint*> get_all_netpoints();
  simgrid::kernel::routing::NetPoint* netpoint_by_name_or_null(std::string name);

  simgrid::s4u::NetZone* get_netzone_root();
  void set_netzone_root(s4u::NetZone* netzone);

  simgrid::s4u::NetZone* netzone_by_name_or_null(const char* name);

  /** @brief Retrieves all netzones of the type indicated by the template argument */
  template <class T> std::vector<T*> filter_netzones_by_type()
  {
    std::vector<T*> res;
    filter_netzones_by_type_recursive(get_netzone_root(), &res);
    return res;
  }

  /** Returns whether SimGrid was initialized yet -- mostly for internal use */
  static bool is_initialized();
  /** @brief set a configuration variable
   *
   * Do --help on any simgrid binary to see the list of currently existing configuration variables (see also @ref
   * options).
   *
   * Example:
   * e->set_config("host/model:ptask_L07");
   */
  void set_config(std::string str);

private:
  simgrid::kernel::EngineImpl* pimpl;
  static s4u::Engine* instance_;

  //////////////// Deprecated functions
public:
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::load_platform()") void loadPlatform(const char* platf)
  {
    load_platform(platf);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::register_function()") void registerFunction(const char* name,
                                                                                             int (*code)(int, char**))
  {
    register_function(name, code);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::register_default()") void registerDefault(int (*code)(int, char**))
  {
    register_default(code);
  }
  template <class F>
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::register_actor()") void registerFunction(const char* name)
  {
    register_actor<F>(name);
  }
  template <class F>
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::register_actor()") void registerFunction(const char* name, F code)
  {
    register_actor<F>(name, code);
  }

  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::load_deployment()") void loadDeployment(const char* deploy)
  {
    load_deployment(deploy);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::host_by_name()") simgrid::s4u::Host* hostByName(std::string name)
  {
    return host_by_name(name);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::host_by_name_or_null()") simgrid::s4u::Host* hostByNameOrNull(
      std::string name)
  {
    return host_by_name_or_null(name);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::storage_by_name()") simgrid::s4u::Storage* storageByName(
      std::string name)
  {
    return storage_by_name(name);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::storage_by_name_or_null()") simgrid::s4u::Storage* storageByNameOrNull(
      std::string name)
  {
    return storage_by_name_or_null(name);
  }

  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::get_host_count()") size_t getHostCount() { return get_host_count(); }
  XBT_ATTRIB_DEPRECATED_v322("Engine::getHostList() is deprecated in favor of Engine::get_all_hosts(). Please switch "
                             "before v3.22") void getHostList(std::vector<Host*>* whereTo);
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::get_all_hosts()") std::vector<Host*> getAllHosts()
  {
    return get_all_hosts();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::get_link_count()") size_t getLinkCount() { return get_link_count(); }
  XBT_ATTRIB_DEPRECATED_v322("Engine::getLinkList() is deprecated in favor of Engine::get_all_links(). Please "
                             "switch before v3.22") void getLinkList(std::vector<Link*>* list);
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::get_link_list()") std::vector<Link*> getAllLinks()
  {
    return get_all_links();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::get_all_storages()") std::vector<Storage*> getAllStorages()
  {
    return get_all_storages();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::get_clock()") static double getClock() { return get_clock(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::get_all_netpoints()") void getNetpointList(
      std::vector<simgrid::kernel::routing::NetPoint*>* list);
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::netpoint_by_name_or_null()")
      simgrid::kernel::routing::NetPoint* getNetpointByNameOrNull(std::string name)
  {
    return netpoint_by_name_or_null(name);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::get_netzone_root()") simgrid::s4u::NetZone* getNetRoot()
  {
    return get_netzone_root();
  }
  XBT_ATTRIB_DEPRECATED_v323(
      "Please use Engine::netzone_by_name_or_null()") simgrid::s4u::NetZone* getNetzoneByNameOrNull(const char* name)
  {
    return netzone_by_name_or_null(name);
  }
  template <class T>
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::filter_netzones_by_type()") void getNetzoneByType(
      std::vector<T*>* whereto)
  {
    filter_netzones_by_type_recursive(get_netzone_root(), whereto);
  }

  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::get_instance()") static s4u::Engine* getInstance()
  {
    return get_instance();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::is_initialized()") static bool isInitialized()
  {
    return is_initialized();
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Engine::set_config()") void setConfig(std::string str) { set_config(str); }
};

/** Callback fired when the platform is created (ie, the xml file parsed),
 * right before the actual simulation starts. */
extern XBT_PUBLIC xbt::signal<void()> on_platform_created;

/** Callback fired when the platform is about to be created
 * (ie, after any configuration change and just before the resource creation) */
extern XBT_PUBLIC xbt::signal<void()> on_platform_creation;

/** Callback fired when the main simulation loop ends, just before the end of Engine::run() */
extern XBT_PUBLIC xbt::signal<void()> on_simulation_end;

/** Callback fired when the time jumps into the future */
extern XBT_PUBLIC xbt::signal<void(double)> on_time_advance;

/** Callback fired when the time cannot jump because of inter-actors deadlock */
extern XBT_PUBLIC xbt::signal<void(void)> on_deadlock;

template <class T> XBT_PRIVATE void filter_netzones_by_type_recursive(s4u::NetZone* current, std::vector<T*>* whereto)
{
  for (auto const& elem : *(current->getChildren())) {
    filter_netzones_by_type_recursive(elem, whereto);
    if (elem == dynamic_cast<T*>(elem))
      whereto->push_back(dynamic_cast<T*>(elem));
  }
}
}
} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_ENGINE_HPP */
