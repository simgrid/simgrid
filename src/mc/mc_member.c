/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc_private.h"

/** Resolve snapshot in the process address space
 *
 * @param object   Process address of the struct/class
 * @param type     Type of the struct/class
 * @param member   Member description
 * @param snapshot Snapshot (or NULL)
 * @return Process address of the given member of the 'object' struct/class
 */
void *mc_member_resolve(const void *base, dw_type_t type, dw_type_t member,
                        mc_snapshot_t snapshot)
{
  if (!member->location.size) {
    return ((char *) base) + member->offset;
  }

  s_mc_expression_state_t state;
  memset(&state, 0, sizeof(s_mc_expression_state_t));
  state.frame_base = NULL;
  state.cursor = NULL;
  state.snapshot = snapshot;
  state.stack_size = 1;
  state.stack[0] = (uintptr_t) base;

  if (mc_dwarf_execute_expression
      (member->location.size, member->location.ops, &state))
    xbt_die("Error evaluating DWARF expression");
  if (state.stack_size == 0)
    xbt_die("No value on the stack");
  else
    return (void *) state.stack[state.stack_size - 1];
}

