/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>

extern "C" {
void __mcsimgrid_write(uintptr_t where, unsigned char size);
void __mcsimgrid_read(uintptr_t where, unsigned char size);
void __mcsimgrid_copy(uintptr_t from, uintptr_t to, unsigned char size);
void __mcsimgrid_deref(uintptr_t where, unsigned char size);
bool smemory_is_on_stack(uintptr_t ptr);
void smemory_add_stack(uintptr_t begin, uintptr_t end);
void smemory_remove_stack(uintptr_t begin, uintptr_t end);
bool smemory_is_rw_segment(uintptr_t location);
};