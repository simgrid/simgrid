/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstdint>
#include <cstdarg>

#include <dwarf.h>
#include <elfutils/libdw.h>

#include "mc_object_info.h"
#include "mc_private.h"
#include "mc/LocationList.hpp"
#include "mc/AddressSpace.hpp"
#include "mc/Frame.hpp"
#include "mc/ObjectInformation.hpp"
#include "mc/DwarfExpression.hpp"
#include "mc_dwarf.hpp"

using simgrid::mc::remote;

namespace simgrid {
namespace dwarf {

evaluation_error::~evaluation_error() {}

}
}

namespace simgrid {
namespace dwarf {

void execute(
  const Dwarf_Op* ops, std::size_t n,
  const ExpressionContext& context, ExpressionStack& stack)
{
  for (size_t i = 0; i != n; ++i) {
    const Dwarf_Op *op = ops + i;
    std::uint8_t atom = op->atom;

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
        int register_id = simgrid::dwarf::dwarf_register_to_libunwind(
          op->atom - DW_OP_breg0);
        unw_word_t res;
        if (!context.cursor)
          throw evaluation_error("Missin stack context");
        unw_get_reg(context.cursor, register_id, &res);
        stack.push(res + op->number);
        break;
      }

      // Push the CFA (Canonical Frame Addresse):
    case DW_OP_call_frame_cfa:
      {
        // UNW_X86_64_CFA does not return the CFA DWARF expects
        // (it is a synonym for UNW_X86_64_RSP) so copy the cursor,
        // unwind it once in order to find the parent SP:

        if (!context.cursor)
          throw evaluation_error("Missint cursor");

        // Get frame:
        unw_cursor_t cursor = *(context.cursor);
        unw_step(&cursor);

        unw_word_t res;
        unw_get_reg(&cursor, UNW_REG_SP, &res);
        stack.push(res);
        break;
      }

      // Frame base:

    case DW_OP_fbreg:
      stack.push((std::uintptr_t) context.frame_base + op->number);
      break;

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
      stack.push(atom - DW_OP_lit0);
      break;

      // Address from the base address of this ELF object.
      // Push the address on the stack (base_address + argument).
    case DW_OP_addr: {
      if (!context.object_info)
        throw evaluation_error("No base address");
      Dwarf_Off addr = (Dwarf_Off) (std::uintptr_t)
        context.object_info->base_address() + op->number;
      stack.push(addr);
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
      stack.push(op->number);
      break;

      // ***** Stack manipulation:

      // Push another copy/duplicate the value at the top of the stack:
    case DW_OP_dup:
      stack.dup();
      break;

      // Pop/drop the top of the stack:
    case DW_OP_drop:
      stack.pop();
      break;

      // Swap the two top-most value of the stack:
    case DW_OP_swap:
      std::swap(stack.top(), stack.top(1));
      break;

      // Duplicate the value under the top of the stack:
    case DW_OP_over:
      stack.push(stack.top(1));
      break;

      // ***** Operations:
      // Those usually take the top of the stack and the next value as argument
      // and replace the top of the stack with the computed value
      // (stack.top() += stack.before_top()).

    case DW_OP_plus:
      stack.push(stack.pop() + stack.pop());
      break;

    case DW_OP_mul:
      stack.push(stack.pop() * stack.pop());
      break;

    case DW_OP_plus_uconst:
      stack.top() += op->number;
      break;

    case DW_OP_not:
      stack.top() = ~stack.top();
      break;

    case DW_OP_neg:
      stack.top() = - (intptr_t) stack.top();
      break;

    case DW_OP_minus:
      stack.push(stack.pop() - stack.pop());
      break;

    case DW_OP_and:
      stack.push(stack.pop() & stack.pop());
      break;

    case DW_OP_or:
      stack.push(stack.pop() | stack.pop());
      break;

    case DW_OP_xor:
      stack.push(stack.pop() ^ stack.pop());
      break;

    case DW_OP_nop:
      break;

      // ***** Deference (memory fetch)

    case DW_OP_deref_size:
      throw evaluation_error("Unsupported operation");

    case DW_OP_deref:
      // Computed address:
      if (!context.address_space)
        throw evaluation_error("Missing address space");
      context.address_space->read_bytes(
        &stack.top(), sizeof(uintptr_t), remote(stack.top()),
        context.process_index);
      break;

      // Not handled:
    default:
      throw evaluation_error("Unsupported operation");
    }

  }
}

}
}

extern "C" {

/** \brief Find the frame base of a given frame
 *
 *  \param frame
 *  \param unw_cursor
 */
void *mc_find_frame_base(simgrid::mc::Frame* frame, simgrid::mc::ObjectInformation* object_info,
                         unw_cursor_t * unw_cursor)
{
  simgrid::dwarf::Location location = simgrid::dwarf::resolve(
                             frame->frame_base, object_info,
                             unw_cursor, NULL, NULL, -1);
  if (location.in_memory())
    return location.address();
  else if (location.in_register()) {
    // This is a special case.
    // The register if not the location of the frame base
    // (a frame base cannot be located in a register)
    // Instead, DWARF defines this to mean that the register
    // contains the address of the frame base.
    unw_word_t word;
    unw_get_reg(unw_cursor, location.register_id(), &word);
    return (void*) word;
  }
  else xbt_die("Unexpected location type");

}

void mc_dwarf_location_list_init(
  simgrid::dwarf::LocationList* list, simgrid::mc::ObjectInformation* info,
  Dwarf_Die * die, Dwarf_Attribute * attr)
{
  list->clear();

  std::ptrdiff_t offset = 0;
  Dwarf_Addr base, start, end;
  Dwarf_Op *ops;
  std::size_t len;

  while (1) {

    offset = dwarf_getlocations(attr, offset, &base, &start, &end, &ops, &len);
    if (offset == 0)
      return;
    else if (offset == -1)
      xbt_die("Error while loading location list");

    simgrid::dwarf::LocationListEntry entry;
    entry.expression = simgrid::dwarf::DwarfExpression(ops, ops + len);

    void *base = info->base_address();
    // If start == 0, this is not a location list:
    entry.lowpc = start == 0 ? NULL : (char *) base + start;
    entry.highpc = start == 0 ? NULL : (char *) base + end;

    list->push_back(std::move(entry));
  }

}

}
