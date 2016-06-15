/* Copyright (c) 2006-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTOR_HPP
#define SIMGRID_S4U_ACTOR_HPP

#include <stdexcept>
#include <xbt/base.h>
#include <simgrid/simix.h>
#include <simgrid/s4u/forward.hpp>

namespace simgrid {
namespace s4u {

/** @defgroup  s4u_actor Actors: simulation agents
 *  @addtogroup S4U_API
 */
  
/** @addtogroup s4u_actor
 * 
 * @tableofcontents
 * 
 * An actor is an independent stream of execution in your distributed application.
 *
 * You can think of an actor as a process in your distributed application, or as a thread in a multithreaded program.
 * This is the only component in SimGrid that actually does something on its own, executing its own code. 
 * A resource will not get used if you don't schedule activities on them. This is the code of Actors that create and schedule these activities.
 * 
 * An actor is located on a (simulated) host, but it can interact
 * with the whole simulated platform.
 * 
 * (back to the @ref s4u_api "S4U documentation")
 * 
 * @section s4u_actor_def Defining an Actor
 * 
 * The code of an actor (ie, the code that this actor will run when starting) the () operator.
 * In this code, your actor can use the functions of the simgrid::s4u::this_actor namespace to interact with the world.
 * 
 * For example, a Worker actor should be declared as follows:
 *
 * \code{.cpp}
 * #include "s4u/actor.hpp"
 * 
 * class Worker {
 *   void operator()() { // Two pairs of () because this defines the method called ()
 *     printf("Hello s4u");
 *     simgrid::s4u::this_actor::execute(5*1024*1024); // Get the worker executing a task of 5 MFlops
 *   }
 * };
 * \endcode
 * 
 * @section s4u_actor_new Creating a new instance of your Actor
 * 
 * // Then later in your main() function or so:
 *   ...
 *   new Actor("worker", host, Worker());
 *   ...
 *
 * You can start your actors with simple @c new, for example from the @c main function, 
 * but this is usually considered as a bad habit as it makes it harder to test your application
 * in differing settings. Instead, you are advised to use an XML deployment file using 
 * s4u::Engine::loadDeployment() to start your actors.
 * 
 *  @{
 */
   
/** @brief Simulation Agent (see \ref s4u_actor)*/
XBT_PUBLIC_CLASS Actor {
  explicit Actor(smx_process_t smx_proc);
public:
  Actor(const char* name, s4u::Host *host, double killTime, std::function<void()> code);
  Actor(const char* name, s4u::Host *host, std::function<void()> code)
    : Actor(name, host, -1, std::move(code)) {};
  template<class C>
  Actor(const char* name, s4u::Host *host, C code)
    : Actor(name, host, -1, std::function<void()>(std::move(code))) {}
  ~Actor();

  /** Retrieves the actor that have the given PID (or NULL if not existing) */
  //static Actor *byPid(int pid); not implemented

  /** Retrieves the name of that actor */
  const char* getName();
  /** Retrieves the host on which that actor is running */
  s4u::Host *getHost();
  /** Retrieves the PID of that actor */
  int getPid();

  /** If set to true, the actor will automatically restart when its host reboots */
  void setAutoRestart(bool autorestart);
  /** Sets the time at which that actor should be killed */
  void setKillTime(double time);
  /** Retrieves the time at which that actor will be killed (or -1 if not set) */
  double getKillTime();

  /** Ask the actor to die.
   *
   * It will only notice your request when doing a simcall next time (a communication or similar).
   * SimGrid sometimes have issues when you kill actors that are currently communicating and such. We are working on it to fix the issues.
   */
  void kill();

  static void kill(int pid);
  
  /**
   * Wait for the actor to finish.
   */ 
  void join();
  
  // Static methods on all actors:

  /** Ask kindly to all actors to die. Only the issuer will survive. */
  static void killAll();

protected:
  smx_process_t getInferior() {return pimpl_;}
private:
  smx_process_t pimpl_ = nullptr;
};

/** @brief Static methods working on the current actor (see @ref s4u_actor) */
namespace this_actor {

  /** Block the actor sleeping for that amount of seconds (may throws hostFailure) */
  XBT_PUBLIC(void) sleep(double duration);

  /** Block the actor, computing the given amount of flops */
  XBT_PUBLIC(e_smx_state_t) execute(double flop);

  /** Block the actor until it gets a message from the given mailbox.
   *
   * See \ref Comm for the full communication API (including non blocking communications).
   */
  XBT_PUBLIC(void*) recv(Mailbox &chan);

  /** Block the actor until it delivers a message of the given simulated size to the given mailbox
   *
   * See \ref Comm for the full communication API (including non blocking communications).
  */
  XBT_PUBLIC(void) send(Mailbox &chan, void*payload, size_t simulatedSize);

};

/** @} */

}} // namespace simgrid::s4u


#endif /* SIMGRID_S4U_ACTOR_HPP */
