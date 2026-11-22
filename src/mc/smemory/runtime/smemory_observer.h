/* Copyright (c) 2025-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>

extern "C" {
// Entry points of the code instrumented by smemory_pass. The size is in bytes.
void __mcsimgrid_write(uintptr_t where, uint64_t size);
void __mcsimgrid_read(uintptr_t where, uint64_t size);
};