/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */
/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* PythonContextHooks.cpp — Implementation of Python context hooks for SimGrid.
 *
 * Handles Python interpreter state save/restore for raw/boost factories and
 * GIL management for all factories.
 */

#include "src/bindings/python/PythonContextHooks.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/context/ContextSwapped.hpp"
#include <pybind11/detail/type_caster_base.h>
#include <pybind11/pybind11.h>

namespace simgrid::python {

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

// Captured at registration time (GIL held); stable for the lifetime of the
// serial-mode simulation, so safe to dereference even when GIL is released.
PyThreadState* s_tstate = nullptr;
} // namespace

// Save/restore all per-actor Python interpreter state around each context switch.
// Mirrors what the greenlet library does for Python 3.11+.
void py_switch_state(PythonActorState* from, PythonActorState* to)
{
  PyThreadState* tstate = s_tstate;

  from->lls = lls_get();
  // current_frame: promoted to direct PyThreadState field in 3.13.
  // Before 3.13 it lived in a _PyCFrame allocated on the C stack; save the cframe
  // pointer too so we restore tstate->cframe before writing current_frame into it.
#if PY_VERSION_HEX >= 0x030D0000
  from->current_frame = tstate->current_frame;
#else
  from->cframe_p      = tstate->cframe;
  from->current_frame = tstate->cframe->current_frame;
#endif
  // current_exception: added to PyThreadState in 3.12
#if PY_VERSION_HEX >= 0x030C0000
  from->current_exception = tstate->current_exception;
#else
  from->current_exception = nullptr;
#endif
  from->exc_info               = tstate->exc_info;
  from->datastack_chunk        = tstate->datastack_chunk;
  from->datastack_top          = tstate->datastack_top;
  from->datastack_limit        = tstate->datastack_limit;
  from->py_recursion_remaining = tstate->py_recursion_remaining;
  // c_recursion_remaining: removed in Python 3.14
#if PY_VERSION_HEX < 0x030E0000
  from->c_recursion_remaining = tstate->c_recursion_remaining;
#endif
  from->tls_attached = (PyGILState_Check() != 0);

  lls_set(static_cast<pybind11::detail::loader_life_support*>(to->lls));

  if (to->datastack_chunk != nullptr) {
    // Resumed actor: full restore.
#if PY_VERSION_HEX >= 0x030D0000
    tstate->current_frame = static_cast<_PyInterpreterFrame*>(to->current_frame);
#else
    // Restore cframe pointer first so the current_frame write lands in the right struct
    // (not the from-actor's cframe which tstate->cframe still points to at this moment).
    tstate->cframe                = static_cast<_PyCFrame*>(to->cframe_p);
    tstate->cframe->current_frame = static_cast<_PyInterpreterFrame*>(to->current_frame);
#endif
#if PY_VERSION_HEX >= 0x030C0000
    tstate->current_exception = static_cast<PyObject*>(to->current_exception);
#endif
    tstate->exc_info               = static_cast<_PyErr_StackItem*>(to->exc_info);
    tstate->datastack_chunk        = static_cast<_PyStackChunk*>(to->datastack_chunk);
    tstate->datastack_top          = static_cast<PyObject**>(to->datastack_top);
    tstate->datastack_limit        = static_cast<PyObject**>(to->datastack_limit);
    tstate->py_recursion_remaining = to->py_recursion_remaining;
#if PY_VERSION_HEX < 0x030E0000
    tstate->c_recursion_remaining = to->c_recursion_remaining;
#endif
  } else {
    // New actor (first run): fresh interpreter context with full recursion budget.
    // exc_info is left as-is so the first frame chains onto the existing stack.
    // For Python < 3.13, do NOT touch tstate->cframe: Python's eval loop will install
    // a fresh _PyCFrame on the actor's C stack when its first function is called.
#if PY_VERSION_HEX >= 0x030D0000
    tstate->current_frame = nullptr;
#endif
#if PY_VERSION_HEX >= 0x030C0000
    tstate->current_exception = nullptr;
#endif
    tstate->datastack_chunk        = nullptr;
    tstate->datastack_top          = nullptr;
    tstate->datastack_limit        = nullptr;
    tstate->py_recursion_remaining = tstate->py_recursion_limit;
    // Recursion limit constant renamed Py_C_RECURSION_LIMIT in 3.13; field gone in 3.14
#if PY_VERSION_HEX >= 0x030D0000 && PY_VERSION_HEX < 0x030E0000
    tstate->c_recursion_remaining = Py_C_RECURSION_LIMIT;
#elif PY_VERSION_HEX < 0x030D0000
    tstate->c_recursion_remaining = C_RECURSION_LIMIT;
#endif
  }

  // Restore GIL/TLS state to match what the actor had when it last suspended.
  // tls_attached=true (the default) means the actor holds the GIL; TLS must be
  // attached so that PyErr_* and PyThreadState_GET() work correctly.
  // tls_attached=false means the actor suspended with GIL released (e.g. inside
  // a SimGridGilGuard that has not yet been destroyed); TLS must be NULL
  // so that PyEval_RestoreThread in the destructor can reattach without hitting
  // Python 3.13's "_PyThreadState_Attach: non-NULL old thread state" assertion.
  bool current_attached = (PyGILState_Check() != 0);
  if (to->tls_attached && !current_attached)
    PyEval_RestoreThread(tstate);
  else if (!to->tls_attached && current_attached)
    PyEval_SaveThread();
}

// Initialize the PyThreadState pointer (called from simgrid_python.cpp module init)
void simgrid_initialize_python_context_state()
{
  if (s_tstate == nullptr) {
    // Capture the current thread state while GIL is held (at module import time)
    s_tstate = PyThreadState_GET();
  }
}

} // namespace simgrid::python
