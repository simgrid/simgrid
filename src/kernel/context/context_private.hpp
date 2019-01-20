/* Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.          */

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

/* We are using the bottom of the stack to save some information, like the
 * valgrind_stack_id. Define smx_context_usable_stack_size to give the remaining
 * size for the stack. Round its value to a multiple of 16 (asan wants the stacks to be aligned this way). */
#if HAVE_VALGRIND_H
#define smx_context_usable_stack_size                                                                                  \
  ((smx_context_stack_size - sizeof(unsigned int)) & ~0xf) /* for valgrind_stack_id */
#else
#define smx_context_usable_stack_size (smx_context_stack_size & ~0xf)
#endif

#endif
