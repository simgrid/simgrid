/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ENGINE_HPP
#define SIMGRID_S4U_ENGINE_HPP

#include <string>
#include <utility>
#include <vector>

#include <xbt/base.h>
#include <xbt/functional.hpp>

#include <simgrid/simix.hpp>

#include <simgrid/s4u/forward.hpp>

namespace simgrid {
namespace s4u {
/** @brief Simulation engine
 *
 * This class is an interface to the simulation engine.
 */
XBT_PUBLIC_CLASS Engine {
public:
  /** Constructor, taking the command line parameters of your main function */
  Engine(int *argc, char **argv);

  /** Finalize the default engine and all its dependencies */
  static void shutdown();

  /** @brief Load a platform file describing the environment
   *
   * The environment is either a XML file following the simgrid.dtd formalism, or a lua file.
   * Some examples can be found in the directory examples/platforms.
   */
  void loadPlatform(const char *platf);

  /** Registers the main function of an actor that will be launched from the deployment file */
  void registerFunction(const char*name, int (*code)(int,char**));

  /** Registers a function as the default main function of actors
   *
   * It will be used as fallback when the function requested from the deployment file was not registered.
   * It is used for trace-based simulations (see examples/msg/actions).
   */
  void registerDefault(int (*code)(int,char**));

  /** @brief Load a deployment file and launch the actors that it contains */
  void loadDeployment(const char *deploy);

  /** @brief Run the simulation */
  void run();

  /** @brief Retrieve the simulation time */
  static double getClock();
  
  /** @brief Retrieve the engine singleton */
  static s4u::Engine *instance();

  /** @brief Retrieve the root AS, containing all others */
  simgrid::s4u::As *rootAs();
  /** @brief Retrieve the AS of the given name (or nullptr if not found) */
  simgrid::s4u::As *asByNameOrNull(const char *name);

  template<class F>
  void registerFunction(const char* name)
  {
    simgrid::simix::registerFunction(name, [](std::vector<std::string> args){
      return simgrid::simix::ActorCode([args] {
        F code(std::move(args));
        code();
      });
    });
  }

  template<class F>
  void registerFunction(const char* name, F code)
  {
    simgrid::simix::registerFunction(name, [code](std::vector<std::string> args){
      return simgrid::simix::ActorCode([code,args] {
        code(std::move(args));
      });
    });
  }

private:
  static s4u::Engine *instance_;
};
}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_ENGINE_HPP */
