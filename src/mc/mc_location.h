/* Copyright (c) 2004-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_OBJECT_LOCATION_H
#define SIMGRID_MC_OBJECT_LOCATION_H

#include <stdint.h>

#include <vector>

#include <libunwind.h>
#include <dwarf.h>

#include <simgrid_config.h>
#include "mc_base.h"
#include "mc_forward.hpp"
#include "AddressSpace.hpp"

namespace simgrid {
namespace mc {

typedef struct
{
  uint8_t atom;
  std::uint64_t number;
  std::uint64_t number2;
  std::uint64_t offset;
} DwarfInstruction;

typedef std::vector<DwarfInstruction> DwarfExpression;

/** \brief A DWARF expression with optional validity contraints */
class LocationListEntry {
public:
  DwarfExpression expression;
  void* lowpc, *highpc;

  LocationListEntry() : lowpc(nullptr), highpc(nullptr) {}

  bool valid_for_ip(unw_word_t ip)
  {
    return (this->lowpc == nullptr && this->highpc == nullptr)
        || (ip >= (unw_word_t) this->lowpc
            && ip < (unw_word_t) this->highpc);
  }
};

typedef std::vector<LocationListEntry> LocationList;

}
}

SG_BEGIN_DECL()

/** A location is either a location in memory of a register location
 *
 *  Usage:
 *
 *    * mc_dwarf_resolve_locations or mc_dwarf_resolve_location is used
 *      to find the location of a given location expression or location list;
 *
 *    * mc_get_location_type MUST be used to find the location type;
 *
 *    * for MC_LOCATION_TYPE_ADDRESS, memory_address is the resulting address
 *
 *    * for MC_LOCATION_TYPE_REGISTER, unw_get_reg(l.cursor, l.register_id, value)
 *        and unw_get_reg(l.cursor, l.register_id, value) can be used to read/write
 *        the value.
 *  </ul>
 */
typedef struct s_mc_location {
  void* memory_location;
  unw_cursor_t* cursor;
  int register_id;
} s_mc_location_t, *mc_location_t;

/** Type of a given location
 *
 *  Use `mc_get_location_type(location)` to find the type.
 * */
typedef enum mc_location_type {
  MC_LOCATION_TYPE_ADDRESS,
  MC_LOCATION_TYPE_REGISTER
} mc_location_type;

/** Find the type of a location */
static inline __attribute__ ((always_inline))
enum mc_location_type mc_get_location_type(mc_location_t location) {
  if (location->cursor) {
    return MC_LOCATION_TYPE_REGISTER;
  } else {
    return MC_LOCATION_TYPE_ADDRESS;
  }
}

XBT_INTERNAL void mc_dwarf_resolve_location(
  mc_location_t location, simgrid::mc::DwarfExpression* expression,
  simgrid::mc::ObjectInformation* object_info, unw_cursor_t* c,
  void* frame_pointer_address, simgrid::mc::AddressSpace* address_space,
  int process_index);
MC_SHOULD_BE_INTERNAL void mc_dwarf_resolve_locations(
  mc_location_t location, simgrid::mc::LocationList* locations,
  simgrid::mc::ObjectInformation* object_info, unw_cursor_t* c,
  void* frame_pointer_address, simgrid::mc::AddressSpace* address_space,
  int process_index);

#define MC_EXPRESSION_STACK_SIZE 64

#define MC_EXPRESSION_OK 0
#define MC_EXPRESSION_E_UNSUPPORTED_OPERATION 1
#define MC_EXPRESSION_E_STACK_OVERFLOW 2
#define MC_EXPRESSION_E_STACK_UNDERFLOW 3
#define MC_EXPRESSION_E_MISSING_STACK_CONTEXT 4
#define MC_EXPRESSION_E_MISSING_FRAME_BASE 5
#define MC_EXPRESSION_E_NO_BASE_ADDRESS 6

typedef struct s_mc_expression_state {
  uintptr_t stack[MC_EXPRESSION_STACK_SIZE];
  size_t stack_size;

  unw_cursor_t* cursor;
  void* frame_base;
  simgrid::mc::AddressSpace* address_space;
  simgrid::mc::ObjectInformation* object_info;
  int process_index;
} s_mc_expression_state_t, *mc_expression_state_t;

MC_SHOULD_BE_INTERNAL int mc_dwarf_execute_expression(
  size_t n, const simgrid::mc::DwarfInstruction* ops, mc_expression_state_t state);

MC_SHOULD_BE_INTERNAL void* mc_find_frame_base(
  simgrid::mc::Frame* frame, simgrid::mc::ObjectInformation* object_info, unw_cursor_t* unw_cursor);

SG_END_DECL()

namespace simgrid {
namespace mc {

inline
int execute(DwarfExpression const& expression, mc_expression_state_t state)
{
  return mc_dwarf_execute_expression(
    expression.size(), expression.data(), state);
}

}
}

#endif
