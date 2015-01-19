/* Copyright (c) 2004-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_OBJECT_LOCATION_H
#define MC_OBJECT_LOCATION_H

#include <stdint.h>

#include <libunwind.h>
#include <dwarf.h>
#include <elfutils/libdw.h>

#include <simgrid_config.h>
#include "mc_interface.h"
#include "mc_object_info.h"
#include "mc_forward.h"
#include "mc_address_space.h"

SG_BEGIN_DECL()

/** \brief a DWARF expression with optional validity contraints */
typedef struct s_mc_expression {
  size_t size;
  Dwarf_Op* ops;
  // Optional validity:
  void* lowpc, *highpc;
} s_mc_expression_t, *mc_expression_t;

/** A location list (list of location expressions) */
typedef struct s_mc_location_list {
  size_t size;
  mc_expression_t locations;
} s_mc_location_list_t, *mc_location_list_t;

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

void mc_dwarf_resolve_location(mc_location_t location, mc_expression_t expression, mc_object_info_t object_info, unw_cursor_t* c, void* frame_pointer_address, mc_address_space_t address_space, int process_index);
void mc_dwarf_resolve_locations(mc_location_t location, mc_location_list_t locations, mc_object_info_t object_info, unw_cursor_t* c, void* frame_pointer_address, mc_address_space_t address_space, int process_index);

void mc_dwarf_expression_clear(mc_expression_t expression);
void mc_dwarf_expression_init(mc_expression_t expression, size_t len, Dwarf_Op* ops);

void mc_dwarf_location_list_clear(mc_location_list_t list);

void mc_dwarf_location_list_init_from_expression(mc_location_list_t target, size_t len, Dwarf_Op* ops);
void mc_dwarf_location_list_init(mc_location_list_t target, mc_object_info_t info, Dwarf_Die* die, Dwarf_Attribute* attr);

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
  mc_address_space_t address_space;
  mc_object_info_t object_info;
  int process_index;
} s_mc_expression_state_t, *mc_expression_state_t;

int mc_dwarf_execute_expression(size_t n, const Dwarf_Op* ops, mc_expression_state_t state);

void* mc_find_frame_base(dw_frame_t frame, mc_object_info_t object_info, unw_cursor_t* unw_cursor);

SG_END_DECL()

#endif
