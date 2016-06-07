/* Copyright (c) 2006-2015. The SimGrid Team. All rights reserved.          */

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

/** @brief Simulation Agent
 *
 * An actor may be defined as a code executing in a location (host).
 *
 * All actors should be started from the XML deployment file (using the @link{s4u::Engine::loadDeployment()}),
 * even if you can also start new actors directly.
 * Separating the deployment in the XML from the logic in the code is a good habit as it makes your simulation easier
 * to adapt to new settings.
 *
 * The code that you define for a given actor should be placed in the main method that is virtual.
 * For example, a Worker actor should be declared as follows:
 *
 * \verbatim
 * #include "s4u/actor.hpp"
 * 
 * class Worker {
 *   void operator()() {
 *     printf("Hello s4u");
 *     return 0;
 *   }
 * };
 *
 * new Actor("worker", host, Worker());
 * \endverbatim
 *
 */
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

namespace this_actor {

  // Static methods working on the current actor:

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

}} // namespace simgrid::s4u

#endif /* SIMGRID_S4U_ACTOR_HPP */

#if 0

public final class Actor {
  
  public Actor(String name, Host host, double killTime, Runnable code);
  // ....

}
#endif
