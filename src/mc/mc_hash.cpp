/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cinttypes>

#include <cstdint>

#include "mc_private.h"
#include "mc/datatypes.h"
#include <mc/mc.h>
#include "mc_hash.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_hash, mc, "Logging specific to mc_hash");

namespace simgrid {
namespace mc {

// This is djb2:
#define MC_HASH_INIT ((simgrid::mc::hash_type)5381)

template<class T>
static void hash_update(hash_type& hash, T const& value)
{
  hash = (hash << 5) + hash + (std::uint64_t) value;
}

// ***** Hash state

#if 0
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

static void mc_hash_binary(hash_type * hash, const void *s, size_t len)
{
  const char *p = (const char*) s;
  for (size_t i = 0; i != len; ++i) {
    hash_update(*hash, p[i]);
  }
}

/** \brief Compute a hash for a given value of a given type
 *
 *  We try to be very conservative (do not hash too ambiguous things).
 *
 *  \param address address of the variable
 *  \param type type of the variable
 * */
static void mc_hash_value(hash_type * hash, mc_hashing_state * state,
                          simgrid::mc::ObjectInformation* info, const void *address,
                          simgrid::mc::Type* type)
{
  simgrid::mc::Process* process = &mc_model_checker->process();
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
      simgrid::mc::Type* subtype = type->subtype;
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
      simgrid::mc::Type* member;
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
          || (pointed >= process->heap_address
              && pointed < (void *) ((const char *) process->heap_address + STD_HEAP_SIZE));
      if (!valid_pointer) {
        XBT_DEBUG("Hashed pointed data %p is in an ignored range", pointed);
        return;
      }

      if (type->subtype == NULL) {
        XBT_DEBUG("Missing type for %p (type=%s)",
          pointed, type->type_id.c_str());
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

static void mc_hash_object_globals(hash_type * hash, mc_hashing_state * state,
                                   simgrid::mc::ObjectInformation* info)
{
  unsigned int cursor = 0;
  simgrid::mc::Variable* variable;
  xbt_dynar_foreach(info->global_variables, cursor, variable) {
    XBT_DEBUG("Hash global variable %s", variable->name);

    if (variable->type_id == NULL) {
      // Nothing
      continue;
    }

    simgrid::mc::Type* type = variable->type;
    if (type == NULL) {
      // Nothing
      continue;
    }

    const char *address = variable->address;
    bool valid_pointer = (address >= binary_info->start_rw
                          && address <= binary_info->end_rw)
        || (address >= libsimgrid_info->start_rw
            && address <= libsimgrid_info->end_rw)
        || (address >= (const char *) process->heap_address
            && address < (const char *) process->heap_address + STD_HEAP_SIZE);
    if (!valid_pointer)
      continue;

    mc_hash_value(hash, state, info, variable->address, type);
  }
}

static void mc_hash_stack_frame(mc_hash_t * hash,
                                simgrid::mc::ObjectInformation* info,
                                unw_cursor_t * unw_cursor, simgrid::mc::Frame* frame,
                                char *frame_pointer, mc_hashing_state * state)
{

  // return; // TEMP

  unsigned int cursor = 0;
  simgrid::mc::Variable* variable;
  xbt_dynar_foreach(frame->variables, cursor, variable) {

    if (variable->type_id == NULL) {
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

    simgrid::mc::Type* type = variable->type;
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

  for(s_mc_stack_frame_t const& stack_frame : stack->stack_frames) {

    hash_update(*hash, stack_frame.ip);

    simgrid::mc::ObjectInformation* info;
    if (stack_frame.ip >= (unw_word_t) libsimgrid_info->start_exec
        && stack_frame.ip < (unw_word_t) libsimgrid_info->end_exec)
      info = libsimgrid_info;
    else if (stack_frame.ip >= (unw_word_t) binary_info->start_exec
             && stack_frame.ip < (unw_word_t) binary_info->end_exec)
      info = binary_info;
    else
      continue;

    mc_hash_stack_frame(hash, info, &(stack_frame.unw_cursor),
                        stack_frame.frame, (void *) stack_frame.frame_base,
                        state);

  }
}

static void mc_hash_stacks(mc_hash_t * hash, mc_hashing_state * state,
                           xbt_dynar_t stacks)
{
  unsigned int cursor = 0;
  mc_snapshot_stack_t current_stack;

  hash_update(*hash, xbt_dynar_length(stacks_areas));

  int i = 0;
  xbt_dynar_foreach(stacks, cursor, current_stack) {
    XBT_DEBUG("Stack %i", i);
    mc_hash_stack(hash, current_stack, state);
    ++i;
  }
}
#endif

static hash_type hash(std::vector<s_mc_snapshot_stack_t> const& stacks)
{
#if 0
  mc_hashing_state state;
  mc_hash_state_init(&state);
#endif

  hash_type hash = MC_HASH_INIT;

  hash_update(hash, xbt_swag_size(simix_global->process_list));
#if 0
  // mc_hash_object_globals(&hash, &state, binary_info);
  // mc_hash_object_globals(&hash, &state, libsimgrid_info);
  // mc_hash_stacks(&hash, &state, stacks);
  mc_hash_state_destroy(&state);
#endif


  return hash;
}

hash_type hash(Snapshot const& snapshot)
{
  XBT_DEBUG("START hash %i", snapshot.num_state);
  hash_type res = simgrid::mc::hash(snapshot.stacks);
  XBT_DEBUG("END hash %i", snapshot.num_state);
  return res;
}

}
}
