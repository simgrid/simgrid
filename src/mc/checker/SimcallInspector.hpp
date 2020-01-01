/* Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_SIMCALL_INSPECTOR_HPP
#define SIMGRID_MC_SIMCALL_INSPECTOR_HPP

#include <string>

namespace simgrid {
namespace mc {

class SimcallInspector {
public:
  /** Whether this transition can currently be taken without blocking.
   *
   * For example, a mutex_lock is not enabled when the mutex is not free.
   * A comm_receive is not enabled before the corresponding send has been issued.
   */
  virtual bool is_enabled() { return true; }

  /** Prepare the simcall to be executed
   *
   * Do the choices that the platform would have done in non-MC settings.
   * For example if it's a waitany, pick the communication that should finish first.
   * If it's a random(), choose the next value to explore.
   */
  virtual void arm() {}

  /** Some simcalls may only be observable under some circumstances.
   * Most simcalls are not visible from the MC because they don't have an inspector at all. */
  virtual bool is_visible() { return true; }
  virtual std::string to_string() = 0;
  virtual std::string dot_label() = 0;
};
} // namespace mc
} // namespace simgrid

#endif
