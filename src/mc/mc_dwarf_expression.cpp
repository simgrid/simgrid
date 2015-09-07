/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdint.h>
#include <stdarg.h>

#include <dwarf.h>
#include <elfutils/libdw.h>

#include "mc_object_info.h"
#include "mc_private.h"
#include "mc_location.h"
#include "mc/AddressSpace.hpp"
#include "mc/Frame.hpp"
#include "mc/ObjectInformation.hpp"

using simgrid::mc::remote;

extern "C" {

static int mc_dwarf_push_value(mc_expression_state_t state, Dwarf_Off value)
{
  if (state->stack_size >= MC_EXPRESSION_STACK_SIZE)
    return MC_EXPRESSION_E_STACK_OVERFLOW;

  state->stack[state->stack_size++] = value;
  return 0;
}

/** Convert a DWARF register into a libunwind register
 *
 *  DWARF and libunwind does not use the same convention for numbering the
 *  registers on some architectures. The function makes the necessary
 *  convertion.
 */
static int mc_dwarf_register_to_libunwind(int dwarf_register)
{
#if defined(__x86_64__)
  // It seems for this arch, DWARF and libunwind agree in the numbering:
  return dwarf_register;
#elif defined(__i386__)
  // Could't find the authoritative source of information for this.
  // This is inspired from http://source.winehq.org/source/dlls/dbghelp/cpu_i386.c#L517.
  switch (dwarf_register) {
  case 0:
    return UNW_X86_EAX;
  case 1:
    return UNW_X86_ECX;
  case 2:
    return UNW_X86_EDX;
  case 3:
    return UNW_X86_EBX;
  case 4:
    return UNW_X86_ESP;
  case 5:
    return UNW_X86_EBP;
  case 6:
    return UNW_X86_ESI;
  case 7:
    return UNW_X86_EDI;
  case 8:
    return UNW_X86_EIP;
  case 9:
    return UNW_X86_EFLAGS;
  case 10:
    return UNW_X86_CS;
  case 11:
    return UNW_X86_SS;
  case 12:
    return UNW_X86_DS;
  case 13:
    return UNW_X86_ES;
  case 14:
    return UNW_X86_FS;
  case 15:
    return UNW_X86_GS;
  case 16:
    return UNW_X86_ST0;
  case 17:
    return UNW_X86_ST1;
  case 18:
    return UNW_X86_ST2;
  case 19:
    return UNW_X86_ST3;
  case 20:
    return UNW_X86_ST4;
  case 21:
    return UNW_X86_ST5;
  case 22:
    return UNW_X86_ST6;
  case 23:
    return UNW_X86_ST7;
  default:
    xbt_die("Bad/unknown register number.");
  }
#else
#error This architecture is not supported yet for DWARF expression evaluation.
#endif
}

int mc_dwarf_execute_expression(size_t n, const Dwarf_Op * ops,
                                mc_expression_state_t state)
{
  for (size_t i = 0; i != n; ++i) {
    int error = 0;
    const Dwarf_Op *op = ops + i;
    uint8_t atom = op->atom;

    switch (atom) {

      // Registers:

    case DW_OP_breg0:
    case DW_OP_breg1:
    case DW_OP_breg2:
    case DW_OP_breg3:
    case DW_OP_breg4:
    case DW_OP_breg5:
    case DW_OP_breg6:
    case DW_OP_breg7:
    case DW_OP_breg8:
    case DW_OP_breg9:
    case DW_OP_breg10:
    case DW_OP_breg11:
    case DW_OP_breg12:
    case DW_OP_breg13:
    case DW_OP_breg14:
    case DW_OP_breg15:
    case DW_OP_breg16:
    case DW_OP_breg17:
    case DW_OP_breg18:
    case DW_OP_breg19:
    case DW_OP_breg20:
    case DW_OP_breg21:
    case DW_OP_breg22:
    case DW_OP_breg23:
    case DW_OP_breg24:
    case DW_OP_breg25:
    case DW_OP_breg26:
    case DW_OP_breg27:
    case DW_OP_breg28:
    case DW_OP_breg29:
    case DW_OP_breg30:
    case DW_OP_breg31:{
        int register_id =
            mc_dwarf_register_to_libunwind(op->atom - DW_OP_breg0);
        unw_word_t res;
        if (!state->cursor)
          return MC_EXPRESSION_E_MISSING_STACK_CONTEXT;
        unw_get_reg(state->cursor, register_id, &res);
        error = mc_dwarf_push_value(state, res + op->number);
        break;
      }

      // Push the CFA (Canonical Frame Addresse):
    case DW_OP_call_frame_cfa:
      {
        // UNW_X86_64_CFA does not return the CFA DWARF expects
        // (it is a synonym for UNW_X86_64_RSP) so copy the cursor,
        // unwind it once in order to find the parent SP:

        if (!state->cursor)
          return MC_EXPRESSION_E_MISSING_STACK_CONTEXT;

        // Get frame:
        unw_cursor_t cursor = *(state->cursor);
        unw_step(&cursor);

        unw_word_t res;
        unw_get_reg(&cursor, UNW_REG_SP, &res);
        error = mc_dwarf_push_value(state, res);
        break;
      }

      // Frame base:

    case DW_OP_fbreg:
      {
        if (!state->frame_base)
          return MC_EXPRESSION_E_MISSING_FRAME_BASE;
        uintptr_t fb = ((uintptr_t) state->frame_base) + op->number;
        error = mc_dwarf_push_value(state, fb);
        break;
      }


      // ***** Constants:

      // Short constant literals:
      // DW_OP_lit15 pushed the 15 on the stack.
    case DW_OP_lit0:
    case DW_OP_lit1:
    case DW_OP_lit2:
    case DW_OP_lit3:
    case DW_OP_lit4:
    case DW_OP_lit5:
    case DW_OP_lit6:
    case DW_OP_lit7:
    case DW_OP_lit8:
    case DW_OP_lit9:
    case DW_OP_lit10:
    case DW_OP_lit11:
    case DW_OP_lit12:
    case DW_OP_lit13:
    case DW_OP_lit14:
    case DW_OP_lit15:
    case DW_OP_lit16:
    case DW_OP_lit17:
    case DW_OP_lit18:
    case DW_OP_lit19:
    case DW_OP_lit20:
    case DW_OP_lit21:
    case DW_OP_lit22:
    case DW_OP_lit23:
    case DW_OP_lit24:
    case DW_OP_lit25:
    case DW_OP_lit26:
    case DW_OP_lit27:
    case DW_OP_lit28:
    case DW_OP_lit29:
    case DW_OP_lit30:
    case DW_OP_lit31:
      error = mc_dwarf_push_value(state, atom - DW_OP_lit0);
      break;

      // Address from the base address of this ELF object.
      // Push the address on the stack (base_address + argument).
    case DW_OP_addr: {
      if (!state->object_info)
        return MC_EXPRESSION_E_NO_BASE_ADDRESS;
      if (state->stack_size == MC_EXPRESSION_STACK_SIZE)
        return MC_EXPRESSION_E_STACK_OVERFLOW;
      Dwarf_Off addr = (Dwarf_Off) (uintptr_t)
        state->object_info->base_address() + op->number;
      error = mc_dwarf_push_value(state, addr);
      break;
    }

      // General constants:
      // Push the constant argument on the stack.
    case DW_OP_const1u:
    case DW_OP_const2u:
    case DW_OP_const4u:
    case DW_OP_const8u:
    case DW_OP_const1s:
    case DW_OP_const2s:
    case DW_OP_const4s:
    case DW_OP_const8s:
    case DW_OP_constu:
    case DW_OP_consts:
      if (state->stack_size == MC_EXPRESSION_STACK_SIZE)
        return MC_EXPRESSION_E_STACK_OVERFLOW;
      error = mc_dwarf_push_value(state, op->number);
      break;

      // ***** Stack manipulation:

      // Push another copy/duplicate the value at the top of the stack:
    case DW_OP_dup:
      if (state->stack_size == 0)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      else
        error = mc_dwarf_push_value(state, state->stack[state->stack_size - 1]);
      break;

      // Pop/drop the top of the stack:
    case DW_OP_drop:
      if (state->stack_size == 0)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      else
        state->stack_size--;
      break;

      // Swap the two top-most value of the stack:
    case DW_OP_swap:
      if (state->stack_size < 2)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      {
        uintptr_t temp = state->stack[state->stack_size - 2];
        state->stack[state->stack_size - 2] =
            state->stack[state->stack_size - 1];
        state->stack[state->stack_size - 1] = temp;
      }
      break;

      // Duplicate the value under the top of the stack:
    case DW_OP_over:
      if (state->stack_size < 2)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      error = mc_dwarf_push_value(state, state->stack[state->stack_size - 2]);
      break;

      // ***** Operations:
      // Those usually take the top of the stack and the next value as argument
      // and replace the top of the stack with the computed value
      // (stack.top() += stack.before_top()).

    case DW_OP_plus:
      if (state->stack_size < 2)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      {
        uintptr_t result =
            state->stack[state->stack_size - 2] +
            state->stack[state->stack_size - 1];
        state->stack[state->stack_size - 2] = result;
        state->stack_size--;
      }
      break;

    case DW_OP_mul:
      if (state->stack_size < 2)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      {
        uintptr_t result =
            state->stack[state->stack_size - 2] -
            state->stack[state->stack_size - 1];
        state->stack[state->stack_size - 2] = result;
        state->stack_size--;
      }
      break;

    case DW_OP_plus_uconst:
      if (state->stack_size == 0)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      state->stack[state->stack_size - 1] += op->number;
      break;

    case DW_OP_not:
      if (state->stack_size == 0)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      state->stack[state->stack_size - 1] =
          ~state->stack[state->stack_size - 1];
      break;

    case DW_OP_neg:
      if (state->stack_size == 0)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      {
        intptr_t value = state->stack[state->stack_size - 1];
        if (value < 0)
          value = -value;
        state->stack[state->stack_size - 1] = value;
      }
      break;

    case DW_OP_minus:
      if (state->stack_size < 2)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      {
        uintptr_t result =
            state->stack[state->stack_size - 2] -
            state->stack[state->stack_size - 1];
        state->stack[state->stack_size - 2] = result;
        state->stack_size--;
      }
      break;

    case DW_OP_and:
      if (state->stack_size < 2)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      {
        uintptr_t result =
            state->stack[state->stack_size -
                         2] & state->stack[state->stack_size - 1];
        state->stack[state->stack_size - 2] = result;
        state->stack_size--;
      }
      break;

    case DW_OP_or:
      if (state->stack_size < 2)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      {
        uintptr_t result =
            state->stack[state->stack_size -
                         2] | state->stack[state->stack_size - 1];
        state->stack[state->stack_size - 2] = result;
        state->stack_size--;
      }
      break;

    case DW_OP_xor:
      if (state->stack_size < 2)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      {
        uintptr_t result =
            state->stack[state->stack_size -
                         2] ^ state->stack[state->stack_size - 1];
        state->stack[state->stack_size - 2] = result;
        state->stack_size--;
      }
      break;

    case DW_OP_nop:
      break;

      // ***** Deference (memory fetch)

    case DW_OP_deref_size:
      return MC_EXPRESSION_E_UNSUPPORTED_OPERATION;

    case DW_OP_deref:
      if (state->stack_size == 0)
        return MC_EXPRESSION_E_STACK_UNDERFLOW;
      {
        // Computed address:
        uintptr_t address = (uintptr_t) state->stack[state->stack_size - 1];
        if (!state->address_space)
          xbt_die("Missing address space");
        state->address_space->read_bytes(
          &state->stack[state->stack_size - 1], sizeof(uintptr_t),
          remote(address), state->process_index);
      }
      break;

      // Not handled:
    default:
      return MC_EXPRESSION_E_UNSUPPORTED_OPERATION;
    }

    if (error)
      return error;
  }
  return 0;
}

// ***** Location

/** \brief Resolve a location expression
 *  \deprecated Use mc_dwarf_resolve_expression
 */
void mc_dwarf_resolve_location(mc_location_t location,
                               simgrid::mc::DwarfExpression* expression,
                               simgrid::mc::ObjectInformation* object_info,
                               unw_cursor_t * c,
                               void *frame_pointer_address,
                               simgrid::mc::AddressSpace* address_space, int process_index)
{
  s_mc_expression_state_t state;
  memset(&state, 0, sizeof(s_mc_expression_state_t));
  state.frame_base = frame_pointer_address;
  state.cursor = c;
  state.address_space = address_space;
  state.object_info = object_info;
  state.process_index = process_index;

  if (expression->size() >= 1
      && (*expression)[0].atom >=DW_OP_reg0
      && (*expression)[0].atom <= DW_OP_reg31) {
    int dwarf_register = (*expression)[0].atom - DW_OP_reg0;
    xbt_assert(c,
      "Missing frame context for register operation DW_OP_reg%i",
      dwarf_register);
    location->memory_location = NULL;
    location->cursor = c;
    location->register_id = mc_dwarf_register_to_libunwind(dwarf_register);
    return;
  }

  if (mc_dwarf_execute_expression(
      expression->size(), expression->data(), &state))
    xbt_die("Error evaluating DWARF expression");
  if (state.stack_size == 0)
    xbt_die("No value on the stack");
  else {
    location->memory_location = (void*) state.stack[state.stack_size - 1];
    location->cursor = NULL;
    location->register_id = 0;
  }
}

// TODO, move this in a method of LocationList
static simgrid::mc::DwarfExpression* mc_find_expression(
    simgrid::mc::LocationList* locations, unw_word_t ip)
{
  for (simgrid::mc::LocationListEntry& entry : *locations)
    if (entry.valid_for_ip(ip))
      return &entry.expression;
  return nullptr;
}

void mc_dwarf_resolve_locations(mc_location_t location,
                                simgrid::mc::LocationList* locations,
                                simgrid::mc::ObjectInformation* object_info,
                                unw_cursor_t * c,
                                void *frame_pointer_address,
                                simgrid::mc::AddressSpace* address_space,
                                int process_index)
{

  unw_word_t ip = 0;
  if (c) {
    if (unw_get_reg(c, UNW_REG_IP, &ip))
      xbt_die("Could not resolve IP");
  }

  simgrid::mc::DwarfExpression* expression = mc_find_expression(locations, ip);
  if (expression) {
    mc_dwarf_resolve_location(location,
                              expression, object_info, c,
                              frame_pointer_address, address_space, process_index);
  } else {
    xbt_die("Could not resolve location");
  }
}

/** \brief Find the frame base of a given frame
 *
 *  \param frame
 *  \param unw_cursor
 */
void *mc_find_frame_base(simgrid::mc::Frame* frame, simgrid::mc::ObjectInformation* object_info,
                         unw_cursor_t * unw_cursor)
{
  s_mc_location_t location;
  mc_dwarf_resolve_locations(&location,
                             &frame->frame_base, object_info,
                             unw_cursor, NULL, NULL, -1);
  switch(mc_get_location_type(&location)) {
  case MC_LOCATION_TYPE_ADDRESS:
    return location.memory_location;

  case MC_LOCATION_TYPE_REGISTER: {
      // This is a special case.
      // The register if not the location of the frame base
      // (a frame base cannot be located in a register)
      // Instead, DWARF defines this to mean that the register
      // contains the address of the frame base.
      unw_word_t word;
      unw_get_reg(location.cursor, location.register_id, &word);
      return (void*) word;
    }

  default:
    xbt_die("Cannot handle non-address frame base");
    return NULL; // Unreachable
  }
}

void mc_dwarf_location_list_init(
  simgrid::mc::LocationList* list, simgrid::mc::ObjectInformation* info,
  Dwarf_Die * die, Dwarf_Attribute * attr)
{
  list->clear();

  ptrdiff_t offset = 0;
  Dwarf_Addr base, start, end;
  Dwarf_Op *ops;
  size_t len;

  while (1) {

    offset = dwarf_getlocations(attr, offset, &base, &start, &end, &ops, &len);
    if (offset == 0)
      return;
    else if (offset == -1)
      xbt_die("Error while loading location list");

    simgrid::mc::LocationListEntry entry;
    entry.expression = simgrid::mc::DwarfExpression(ops, ops + len);

    void *base = info->base_address();
    // If start == 0, this is not a location list:
    entry.lowpc = start == 0 ? NULL : (char *) base + start;
    entry.highpc = start == 0 ? NULL : (char *) base + end;

    list->push_back(std::move(entry));
  }

}

}
