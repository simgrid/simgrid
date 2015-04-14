/* Copyright (c) 2008-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_ADDRESS_SPACE_H
#define MC_ADDRESS_SPACE_H

#include <xbt/misc.h>

#include <stdint.h>

#include "mc_forward.h"

SG_BEGIN_DECL()

// ***** Data types

/** Options for the read() operation
 *
 *  - MC_ADDRESS_SPACE_READ_FLAGS_LAZY, avoid a copy when the data is
 *    available in the current process. In this case, the return value
 *    of MC_address_space_read might be different from the provided one.
 */
typedef int adress_space_read_flags_t;
#define MC_ADDRESS_SPACE_READ_FLAGS_NONE 0
#define MC_ADDRESS_SPACE_READ_FLAGS_LAZY 1

/** Process index used when no process is available
 *
 *  The expected behaviour is that if a process index is needed it will fail.
 * */
#define MC_PROCESS_INDEX_MISSING -1

#define MC_PROCESS_INDEX_DISABLED -2

/** Process index when any process is suitable
 *
 * We could use a special negative value in the future.
 */
#define MC_PROCESS_INDEX_ANY 0

// ***** Class definition

typedef struct s_mc_address_space s_mc_address_space_t, *mc_address_space_t;
typedef struct s_mc_address_space_class s_mc_address_space_class_t, *mc_address_space_class_t;

/** Abstract base class for an address space
 *
 *  This is the base class for all virtual address spaces (process, snapshot).
 *  It uses dynamic dispatch based on a vtable (`address_space_class`).
 */
struct s_mc_address_space {
  const s_mc_address_space_class_t* address_space_class;
};

/** Class object (vtable) for the virtual address spaces
 */
struct s_mc_address_space_class {
  const void* (*read)(
    mc_address_space_t address_space, adress_space_read_flags_t flags,
    void* target, const void* addr, size_t size,
    int process_index);
  mc_process_t (*get_process)(mc_address_space_t address_space);
};

typedef const void* (*mc_address_space_class_read_callback_t)(
  mc_address_space_t address_space, adress_space_read_flags_t flags,
  void* target, const void* addr, size_t size,
  int process_index);
typedef mc_process_t (*mc_address_space_class_get_process_callback_t)(mc_address_space_t address_space);

// ***** Virtual/non-final methods

/** Read data from the given address space
 *
 *  Dysnamic dispatch.
 */
static inline __attribute__((always_inline))
const void* MC_address_space_read(
  mc_address_space_t address_space, adress_space_read_flags_t flags,
  void* target, const void* addr, size_t size,
  int process_index)
{
  return address_space->address_space_class->read(
    address_space, flags, target, addr, size,
    process_index);
}

static inline __attribute__((always_inline))
const void* MC_address_space_get_process(mc_address_space_t address_space)
{
  if (address_space->address_space_class->get_process)
    return address_space->address_space_class->get_process(address_space);
  else
    return NULL;
}

SG_END_DECL()

#endif
