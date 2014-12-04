/* Copyright (c) 2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>
#include <stdbool.h>

#include "mc_private.h"
#include "mc/datatypes.h"
#include <mc/mc.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_hash, mc, "Logging specific to mc_hash");

// This is djb2:
typedef uint64_t mc_hash_t;
#define MC_HASH_INIT ((uint64_t)5381)

// #define MC_HASH(hash, value) hash = (((hash << 5) + hash) + (uint64_t) value)
#define MC_HASH(hash, value) \
  { hash = (((hash << 5) + hash) + (uint64_t) value);\
  XBT_DEBUG("%s:%i: %"PRIx64" -> %"PRIx64, __FILE__, __LINE__, (uint64_t) value, hash); }

// ***** Hash state

typedef struct s_mc_hashing_state {
  // Set of pointers/addresses already processed (avoid loops):
  mc_address_set_t handled_addresses;
} mc_hashing_state;

void mc_hash_state_init(mc_hashing_state * state);
void mc_hash_state_destroy(mc_hashing_state * state);

void mc_hash_state_init(mc_hashing_state * state)
{
  state->handled_addresses = mc_address_set_new();
}

void mc_hash_state_destroy(mc_hashing_state * state)
{
  mc_address_set_free(&state->handled_addresses);
}

// TODO, detect and avoid loops

static bool mc_ignored(const void *address, size_t size)
{
  mc_heap_ignore_region_t region;
  unsigned int cursor = 0;
  const void *end = (char *) address + size;
  xbt_dynar_foreach(mc_heap_comparison_ignore, cursor, region) {
    void *istart = region->address;
    void *iend = (char *) region->address + region->size;

    if (address >= istart && address < iend && end >= istart && end < iend)
      return true;
  }

  return false;
}

static void mc_hash_binary(mc_hash_t * hash, const void *s, size_t len)
{
  const char *p = (const void *) s;
  int i;
  for (i = 0; i != len; ++i) {
    MC_HASH(*hash, p[i]);
  }
}

#if 0
/** \brief Compute a hash for a given value of a given type
 *
 *  We try to be very conservative (do not hash too ambiguous things).
 *
 *  \param address address of the variable
 *  \param type type of the variable
 * */
static void mc_hash_value(mc_hash_t * hash, mc_hashing_state * state,
                          mc_object_info_t info, const void *address,
                          dw_type_t type)
{
top:

  switch (type->type) {

    // Not relevant, do nothing:
  case DW_TAG_unspecified_type:
    return;

    // Simple case, hash this has binary:
  case DW_TAG_base_type:
  case DW_TAG_enumeration_type:
    {
      if (mc_ignored(address, 1))
        return;
      mc_hash_binary(hash, address, type->byte_size);
      return;
    }

  case DW_TAG_array_type:
    {
      if (mc_ignored(address, type->byte_size))
        return;

      long element_count = type->element_count;
      dw_type_t subtype = type->subtype;
      if (subtype == NULL) {
        XBT_DEBUG("Hash array without subtype");
        return;
      }
      int i;
      for (i = 0; i != element_count; ++i) {
        XBT_DEBUG("Hash array element %i", i);
        void *subaddress = ((char *) address) + i * subtype->byte_size;
        mc_hash_value(hash, state, info, subaddress, subtype);
      }
      return;
    }

    // Get the raw type:
  case DW_TAG_typedef:
  case DW_TAG_volatile_type:
  case DW_TAG_const_type:
  case DW_TAG_restrict_type:
    {
      type = type->subtype;
      if (type == NULL)
        return;
      else
        goto top;
    }

  case DW_TAG_structure_type:
  case DW_TAG_class_type:
    {
      if (mc_ignored(address, type->byte_size))
        return;

      unsigned int cursor = 0;
      dw_type_t member;
      xbt_dynar_foreach(type->members, cursor, member) {
        XBT_DEBUG("Hash struct member %s", member->name);
        if (type->subtype == NULL)
          return;
        void *member_variable = mc_member_resolve(address, type, member, NULL);
        mc_hash_value(hash, state, info, member_variable, type->subtype);
      }
      return;
    }

    // Pointer, we hash a single value but it might be an array.
  case DW_TAG_pointer_type:
  case DW_TAG_reference_type:
  case DW_TAG_rvalue_reference_type:
    {
      if (mc_ignored(address, 1))
        return;

      void *pointed = *(void **) address;
      if (pointed == NULL) {
        XBT_DEBUG("Hashed pinter is NULL");
        return;
      }
      // Avoid loops:
      if (mc_address_test(state->handled_addresses, pointed)) {
        XBT_DEBUG("Hashed pointed data %p already hashed", pointed);
        return;
      }
      mc_address_add(state->handled_addresses, pointed);

      // Anything outside the R/W segments and the heap is not hashed:
      bool valid_pointer = (pointed >= (void *) binary_info->start_rw
                            && pointed <= (void *) binary_info->end_rw)
          || (pointed >= (void *) libsimgrid_info->start_rw
              && pointed <= (void *) libsimgrid_info->end_rw)
          || (pointed >= std_heap
              && pointed < (void *) ((const char *) std_heap + STD_HEAP_SIZE));
      if (!valid_pointer) {
        XBT_DEBUG("Hashed pointed data %p is in an ignored range", pointed);
        return;
      }

      if (type->subtype == NULL) {
        XBT_DEBUG("Missing type for %p (type=%s)", pointed, type->dw_type_id);
        return;
      }

      address = pointed;
      type = type->subtype;
      goto top;
    }

    // Skip this:
  case DW_TAG_union_type:
  case DW_TAG_subroutine_type:
  default:
    return;
  }
}

static void mc_hash_object_globals(mc_hash_t * hash, mc_hashing_state * state,
                                   mc_object_info_t info)
{
  unsigned int cursor = 0;
  dw_variable_t variable;
  xbt_dynar_foreach(info->global_variables, cursor, variable) {
    XBT_DEBUG("Hash global variable %s", variable->name);

    if (variable->type_origin == NULL) {
      // Nothing
      continue;
    }

    dw_type_t type = variable->type;
    if (type == NULL) {
      // Nothing
      continue;
    }

    const char *address = variable->address;
    bool valid_pointer = (address >= binary_info->start_rw
                          && address <= binary_info->end_rw)
        || (address >= libsimgrid_info->start_rw
            && address <= libsimgrid_info->end_rw)
        || (address >= (const char *) std_heap
            && address < (const char *) std_heap + STD_HEAP_SIZE);
    if (!valid_pointer)
      continue;

    mc_hash_value(hash, state, info, variable->address, type);
  }
}

static void mc_hash_stack_frame(mc_hash_t * hash,
                                mc_object_info_t info,
                                unw_cursor_t * unw_cursor, dw_frame_t frame,
                                char *frame_pointer, mc_hashing_state * state)
{

  // return; // TEMP

  unsigned int cursor = 0;
  dw_variable_t variable;
  xbt_dynar_foreach(frame->variables, cursor, variable) {

    if (variable->type_origin == NULL) {
      XBT_DEBUG("Hash local variable %s without type", variable->name);
      continue;
    }
    if (variable->locations.size == 0) {
      XBT_DEBUG("Hash local variable %s without location", variable->name);
      continue;
    }

    XBT_DEBUG("Hash local variable %s", variable->name);

    void *variable_address =
        (void *) mc_dwarf_resolve_locations(&variable->locations,
                                            variable->object_info, unw_cursor,
                                            frame_pointer, NULL);

    dw_type_t type = variable->type;
    if (type == NULL) {
      XBT_DEBUG("Hash local variable %s without loctypeation", variable->name);
      continue;
    }

    mc_hash_value(hash, state, info, variable_address, type);
  }

  // TODO, handle nested scopes
}

static void mc_hash_stack(mc_hash_t * hash, mc_snapshot_stack_t stack,
                          mc_hashing_state * state)
{

  unsigned cursor = 0;
  mc_stack_frame_t stack_frame;

  xbt_dynar_foreach(stack->stack_frames, cursor, stack_frame) {

    MC_HASH(*hash, stack_frame->ip);

    mc_object_info_t info;
    if (stack_frame->ip >= (unw_word_t) libsimgrid_info->start_exec
        && stack_frame->ip < (unw_word_t) libsimgrid_info->end_exec)
      info = libsimgrid_info;
    else if (stack_frame->ip >= (unw_word_t) binary_info->start_exec
             && stack_frame->ip < (unw_word_t) binary_info->end_exec)
      info = binary_info;
    else
      continue;

    mc_hash_stack_frame(hash, info, &(stack_frame->unw_cursor),
                        stack_frame->frame, (void *) stack_frame->frame_base,
                        state);

  }
}

static void mc_hash_stacks(mc_hash_t * hash, mc_hashing_state * state,
                           xbt_dynar_t stacks)
{
  unsigned int cursor = 0;
  mc_snapshot_stack_t current_stack;

  MC_HASH(*hash, xbt_dynar_length(stacks_areas));

  int i = 0;
  xbt_dynar_foreach(stacks, cursor, current_stack) {
    XBT_DEBUG("Stack %i", i);
    mc_hash_stack(hash, current_stack, state);
    ++i;
  }
}
#endif

uint64_t mc_hash_processes_state(int num_state, xbt_dynar_t stacks)
{
  XBT_DEBUG("START hash %i", num_state);

  mc_hashing_state state;
  mc_hash_state_init(&state);

  mc_hash_t hash = MC_HASH_INIT;

  MC_HASH(hash, xbt_swag_size(simix_global->process_list));     // process count
  // mc_hash_object_globals(&hash, &state, binary_info);
  // mc_hash_object_globals(&hash, &state, libsimgrid_info);
  // mc_hash_stacks(&hash, &state, stacks);

  mc_hash_state_destroy(&state);

  XBT_DEBUG("END hash %i", num_state);
  return hash;
}
