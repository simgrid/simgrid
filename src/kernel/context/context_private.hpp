/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_CONTEXT_PRIVATE_HPP
#define SIMGRID_KERNEL_CONTEXT_PRIVATE_HPP

#include "src/internal_config.h"

#if HAVE_SANITIZER_ADDRESS_FIBER_SUPPORT
#include <sanitizer/asan_interface.h>
#define ASAN_ONLY(expr) expr
#define ASAN_START_SWITCH(fake_stack_save, bottom, size) __sanitizer_start_switch_fiber(fake_stack_save, bottom, size)
#define ASAN_FINISH_SWITCH(fake_stack_save, bottom_old, size_old)                                                      \
  __sanitizer_finish_switch_fiber(fake_stack_save, bottom_old, size_old)
#else
#define ASAN_ONLY(expr) (void)0
#define ASAN_START_SWITCH(fake_stack_save, bottom, size) (void)0
#define ASAN_FINISH_SWITCH(fake_stack_save, bottom_old, size_old) (void)0
#endif

#endif
