/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */
/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* ContextPython: SwappedContext subclass for Python bindings.
 * Uses the same fast assembly stack-switch as ContextRaw, and saves/restores
 * per-actor Python interpreter state (PyThreadState fields + pybind11
 * loader_life_support TLS) in swap_into_for_real() so that all actors can
 * share the same OS thread without corrupting each other's interpreter state.
 *
 * The greenlet library solves the same problem for user-land coroutines:
 * we use the same set of fields it saves (current_frame, exc_info,
 * current_exception, datastack_chunk/top/limit) plus the pybind11 lls chain.
 */

#ifndef SIMGRID_KERNEL_CONTEXT_PYTHON_HPP
#define SIMGRID_KERNEL_CONTEXT_PYTHON_HPP

#include "src/kernel/context/ContextSwapped.hpp"

namespace simgrid::kernel::context {

class PythonContext final : public SwappedContext {
public:
  PythonContext(std::function<void()>&& code, actor::ActorImpl* actor,
                SwappedContextFactory* factory);
  // Opaque save of per-actor Python interpreter state.
  // All fields are typed void* so this header needs no Python or pybind11 includes.
  // datastack_chunk == nullptr serves as "actor has never been suspended" sentinel.
  struct PyState {
    void* lls                    = nullptr; // pybind11 loader_life_support TLS frame
    void* current_frame          = nullptr; // PyThreadState::current_frame
    void* current_exception      = nullptr; // PyThreadState::current_exception (3.12+)
    void* exc_info               = nullptr; // PyThreadState::exc_info
    void* datastack_chunk        = nullptr; // PyThreadState::datastack_chunk (nullptr = first-run sentinel)
    void* datastack_top          = nullptr; // PyThreadState::datastack_top   (PyObject**)
    void* datastack_limit        = nullptr; // PyThreadState::datastack_limit (PyObject**)
    int   py_recursion_remaining = 0;       // PyThreadState::py_recursion_remaining
    int   c_recursion_remaining  = 0;       // PyThreadState::c_recursion_remaining
    bool  tls_attached           = true;    // whether tstate is attached (GIL held) at suspension point
  };

  // Registered from PyContextState.cpp (which has Python + pybind11 in scope).
  // fn(from, to): saves current interpreter state into *from and restores *to.
  static void register_py_switch(void (*fn)(PyState* from, PyState* to));

private:
  void swap_into_for_real(SwappedContext* to) override;

  void*   stack_top_ = nullptr; // raw assembly stack pointer
  PyState py_state_;            // saved interpreter state for this actor

  static void (*py_switch_)(PyState* from, PyState* to);
};

class PythonContextFactory final : public SwappedContextFactory {
public:
  const char* get_name() const override { return "python"; }
  PythonContext* create_context(std::function<void()>&& code,
                                actor::ActorImpl* actor) override;
};

XBT_PRIVATE ContextFactory* python_factory();

} // namespace simgrid::kernel::context

#endif // SIMGRID_KERNEL_CONTEXT_PYTHON_HPP
