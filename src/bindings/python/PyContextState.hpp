/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_BINDINGS_PYTHON_PYCONTEXTSTATE_HPP
#define SIMGRID_BINDINGS_PYTHON_PYCONTEXTSTATE_HPP

// Register py_switch_state with PythonContext and capture the current PyThreadState.
// Must be called from PYBIND11_MODULE init while the GIL is held.
void simgrid_register_python_context();

#endif // SIMGRID_BINDINGS_PYTHON_PYCONTEXTSTATE_HPP
