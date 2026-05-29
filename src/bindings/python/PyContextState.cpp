/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* PyContextState.cpp — Python interpreter state save/restore for the Python context factory.
 *
 * Lives in src/bindings/python/ rather than src/kernel/context/ because it depends
 * on pybind11 internals (loader_life_support) that the core library must not see.
 * The core only knows about PythonContext::PyState (all void*) and the
 * register_py_switch hook.
 */

#include <pybind11/pybind11.h> // Must come before our own stuff
#include <pybind11/detail/type_caster_base.h> // loader_life_support
#include "src/bindings/python/PyContextState.hpp"
#include "src/kernel/context/ContextPython.hpp"
// _PyInterpreterFrame, _PyErr_StackItem, _PyStackChunk, PyGILState_Check, etc.
// come in via pybind11/pybind11.h → Python.h.

// Access pybind11::detail::loader_life_support TLS frame (private API).
// "Explicit template specialisation bypasses access control" (Rob Meyer / CWG DR 372).
namespace {
#if PYBIND11_VERSION_MAJOR >= 3
// pybind11 3.x: tls_current_frame() returns a reference — one call for get and set.
using LlsFrameFn = pybind11::detail::loader_life_support*& (*)();
struct LlsFrameTag {
  using type = LlsFrameFn;
  friend LlsFrameFn simgrid_lls_frame(LlsFrameTag);
};
template <typename Tag, typename Tag::type Fn> struct AccessPrivate {
  friend typename Tag::type simgrid_lls_frame(Tag) { return Fn; }
};
template struct AccessPrivate<LlsFrameTag, &pybind11::detail::loader_life_support::tls_current_frame>;
inline pybind11::detail::loader_life_support* lls_get()
{
  return simgrid_lls_frame(LlsFrameTag{})();
}
inline void lls_set(pybind11::detail::loader_life_support* v)
{
  simgrid_lls_frame(LlsFrameTag{})() = v;
}
#else
// pybind11 2.x: private get_stack_top() / set_stack_top()
using LlsGetFn = pybind11::detail::loader_life_support* (*)();
struct LlsGetTag {
  using type = LlsGetFn;
  friend LlsGetFn simgrid_lls_get(LlsGetTag);
};
template <typename Tag, typename Tag::type Fn> struct AccessPrivateGet {
  friend typename Tag::type simgrid_lls_get(Tag) { return Fn; }
};
template struct AccessPrivateGet<LlsGetTag, &pybind11::detail::loader_life_support::get_stack_top>;
using LlsSetFn = void (*)(pybind11::detail::loader_life_support*);
struct LlsSetTag {
  using type = LlsSetFn;
  friend LlsSetFn simgrid_lls_set(LlsSetTag);
};
template <typename Tag, typename Tag::type Fn> struct AccessPrivateSet {
  friend typename Tag::type simgrid_lls_set(Tag) { return Fn; }
};
template struct AccessPrivateSet<LlsSetTag, &pybind11::detail::loader_life_support::set_stack_top>;
inline pybind11::detail::loader_life_support* lls_get()
{
  return simgrid_lls_get(LlsGetTag{})();
}
inline void lls_set(pybind11::detail::loader_life_support* v)
{
  simgrid_lls_set(LlsSetTag{})(v);
}
#endif

using PyState = simgrid::kernel::context::PythonContext::PyState;

// Captured at registration time (GIL held); stable for the lifetime of the
// serial-mode simulation, so safe to dereference even when GIL is released.
PyThreadState* s_tstate = nullptr;

// Save/restore all per-actor Python interpreter state around each context switch.
// Mirrors what the greenlet library does for Python 3.11+.
void py_switch_state(PyState* from, PyState* to)
{
  PyThreadState* tstate = s_tstate;

  from->lls                    = lls_get();
  from->current_frame          = tstate->current_frame;
  from->current_exception      = tstate->current_exception;
  from->exc_info               = tstate->exc_info;
  from->datastack_chunk        = tstate->datastack_chunk;
  from->datastack_top          = tstate->datastack_top;
  from->datastack_limit        = tstate->datastack_limit;
  from->py_recursion_remaining = tstate->py_recursion_remaining;
  from->c_recursion_remaining  = tstate->c_recursion_remaining;
  from->tls_attached           = (PyGILState_Check() != 0);

  lls_set(static_cast<pybind11::detail::loader_life_support*>(to->lls));

  if (to->datastack_chunk != nullptr) {
    // Resumed actor: full restore.
    tstate->current_frame          = static_cast<_PyInterpreterFrame*>(to->current_frame);
    tstate->current_exception      = static_cast<PyObject*>(to->current_exception);
    tstate->exc_info               = static_cast<_PyErr_StackItem*>(to->exc_info);
    tstate->datastack_chunk        = static_cast<_PyStackChunk*>(to->datastack_chunk);
    tstate->datastack_top          = static_cast<PyObject**>(to->datastack_top);
    tstate->datastack_limit        = static_cast<PyObject**>(to->datastack_limit);
    tstate->py_recursion_remaining = to->py_recursion_remaining;
    tstate->c_recursion_remaining  = to->c_recursion_remaining;
  } else {
    // New actor (first run): fresh interpreter context with full recursion budget.
    // exc_info is left as-is so the first frame chains onto the existing stack.
    tstate->current_frame          = nullptr;
    tstate->current_exception      = nullptr;
    tstate->datastack_chunk        = nullptr;
    tstate->datastack_top          = nullptr;
    tstate->datastack_limit        = nullptr;
    tstate->py_recursion_remaining = tstate->py_recursion_limit;
    tstate->c_recursion_remaining  = Py_C_RECURSION_LIMIT;
  }

  // Restore GIL/TLS state to match what the actor had when it last suspended.
  // tls_attached=true (the default) means the actor holds the GIL; TLS must be
  // attached so that PyErr_* and PyThreadState_GET() work correctly.
  // tls_attached=false means the actor suspended with GIL released (e.g. inside
  // a py::gil_scoped_release that has not yet been destroyed); TLS must be NULL
  // so that PyEval_RestoreThread in the destructor can reattach without hitting
  // Python 3.13's "_PyThreadState_Attach: non-NULL old thread state" assertion.
  bool current_attached = (PyGILState_Check() != 0);
  if (to->tls_attached && !current_attached)
    PyEval_RestoreThread(tstate);
  else if (!to->tls_attached && current_attached)
    PyEval_SaveThread();
}
} // namespace

void simgrid_register_python_context()
{
  s_tstate = PyThreadState_GET(); // capture while GIL is held; pointer is stable
  simgrid::kernel::context::PythonContext::register_py_switch(py_switch_state);
}
