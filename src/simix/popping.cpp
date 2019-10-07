/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "smx_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(simix_popping, simix,
                                "Popping part of SIMIX (transmuting from user request into kernel handlers)");

void SIMIX_run_kernel(std::function<void()> const* code)
{
  (*code)();
}

/** Kernel code for run_blocking
 *
 * The implementation looks a lot like SIMIX_run_kernel ^^
 *
 * However, this `run_blocking` is blocking so the process will not be woken
 * up until `ActorImpl::simcall_answer()`` is called by the kernel.
 * This means that `code` is responsible for doing this.
 */
void SIMIX_run_blocking(std::function<void()> const* code)
{
  (*code)();
}
