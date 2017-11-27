/* Copyright (c) 2011-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "private.hpp"
#include <stack>

struct s_smpi_static_t {
  void *ptr;
  void_f_pvoid_t free_fn;
};

/**
 * \brief Holds a reference to all static variables that were registered
 *        via smpi_register_static(). This helps to free them when
 *        SMPI shuts down.
 */
static std::stack<s_smpi_static_t> registered_static_variables_stack;

void smpi_register_static(void* arg, void_f_pvoid_t free_fn) {
  s_smpi_static_t elm { arg, free_fn };
  registered_static_variables_stack.push(elm);
}

void smpi_free_static() {
  while (not registered_static_variables_stack.empty()) {
    s_smpi_static_t elm = registered_static_variables_stack.top();
    elm.free_fn(elm.ptr);
    registered_static_variables_stack.pop();
  }
}
