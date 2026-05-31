/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */
/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* PythonContextHooks: Language-specific hooks for Python actors in SimGrid.
 *
 * When Python bindings are used, each actor has per-actor Python interpreter
 * state that must be saved/restored around context switches (for raw/boost
 * factories) or GIL-guarded (for thread factory).
 */

#ifndef SIMGRID_PYTHON_CONTEXT_HOOKS_HPP
#define SIMGRID_PYTHON_CONTEXT_HOOKS_HPP

#include <pybind11/pybind11.h>
#include <pybind11/detail/type_caster_base.h>

namespace simgrid::python {

// Per-actor Python interpreter state (mirrors what greenlet saves for Python 3.11+)
struct PythonActorState {
  void* lls                    = nullptr; // pybind11 loader_life_support TLS frame
  void* current_frame          = nullptr; // PyThreadState::current_frame
  void* current_exception      = nullptr; // PyThreadState::current_exception (3.12+)
  void* exc_info               = nullptr; // PyThreadState::exc_info
  void* datastack_chunk        = nullptr; // PyThreadState::datastack_chunk (nullptr = first-run sentinel)
  void* datastack_top          = nullptr; // PyThreadState::datastack_top (PyObject**)
  void* datastack_limit        = nullptr; // PyThreadState::datastack_limit (PyObject**)
  int   py_recursion_remaining = 0;       // PyThreadState::py_recursion_remaining
  int   c_recursion_remaining  = 0;       // PyThreadState::c_recursion_remaining
  bool  tls_attached           = true;    // whether tstate is attached (GIL held) at suspension point
};

// Save/restore Python interpreter state around context switches (implemented in PythonContextHooks.cpp)
void py_switch_state(PythonActorState* from, PythonActorState* to);

// RAII guard for GIL release/acquire (used by thread factory and binding functions)
// For raw/boost factories, this is a no-op (no GIL management needed).
// For thread factory, this releases the GIL on construction and reacquires on destruction.
struct SimGridGilGuard {
  PyThreadState* saved_ = nullptr;

  SimGridGilGuard();
  ~SimGridGilGuard();
};

// Initialize Python context state (call from module init, before anything else uses py_switch_state)
void simgrid_initialize_python_context_state();

} // namespace simgrid::python

#endif // SIMGRID_PYTHON_CONTEXT_HOOKS_HPP
