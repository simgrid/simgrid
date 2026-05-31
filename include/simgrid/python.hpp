/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */
/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Utilities for SimGrid Python extension modules (DTLMod, etc.).
 *
 * Include this header instead of using py::call_guard<py::gil_scoped_release>()
 * directly, so that GIL management is a no-op when using a cooperative context
 * factory (raw, boost) and only active when the thread factory is in use.
 */

#ifndef SIMGRID_PYTHON_HPP
#define SIMGRID_PYTHON_HPP

#include <Python.h>
#include <simgrid/s4u/Engine.hpp>
#include <string>

namespace simgrid {

// RAII guard for GIL management in SimGrid Python extension modules.
// Drop-in replacement for py::call_guard<py::gil_scoped_release>().
//
// With raw/boost context factories (single OS thread): no-op — zero overhead.
// With thread context factory: releases GIL on construction and reacquires on
// destruction, preventing deadlock between actor threads competing for the GIL.
//
// The factory type is checked once and cached; all subsequent calls pay only a
// single comparison against a static integer.
struct SimGridGilGuard {
  PyThreadState* saved_ = nullptr;

  SimGridGilGuard()
  {
    static int s_mode = 0; // 0=unchecked, 1=no-GIL needed, 2=GIL needed
    if (s_mode == 0)
      s_mode = (std::string(s4u::Engine::get_instance()->get_context_factory_name()) == "thread") ? 2 : 1;
    if (s_mode == 2)
      saved_ = PyEval_SaveThread();
  }

  ~SimGridGilGuard()
  {
    if (saved_)
      PyEval_RestoreThread(saved_);
  }
};

} // namespace simgrid

#endif // SIMGRID_PYTHON_HPP
