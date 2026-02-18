/* Copyright (c) 2025-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>

/* Write operations = store */

extern "C" void __mcsimgrid_write(void* where, unsigned char size);
extern "C" void __mcsimgrid_read(void* where, unsigned char size);
extern "C" void __mcsimgrid_copy(void* from, void* to, unsigned char size);
extern "C" void __mcsimgrid_deref(void* where, unsigned char size);
