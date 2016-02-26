/* Copyright (c) 2014-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstddef>
#include <cstdint>

#include <dwarf.h>
#include <elfutils/libdw.h>

#include "src/mc/mc_private.h"
#include "src/mc/LocationList.hpp"
#include "src/mc/AddressSpace.hpp"
#include "src/mc/Frame.hpp"
#include "src/mc/ObjectInformation.hpp"
#include "src/mc/DwarfExpression.hpp"
#include "src/mc/mc_dwarf.hpp"

using simgrid::mc::remote;

namespace simgrid {
namespace dwarf {

evaluation_error::~evaluation_error() noexcept(true) {}

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
        // Push register + constant:
        int register_id = simgrid::dwarf::dwarf_register_to_libunwind(
          op->atom - DW_OP_breg0);
        unw_word_t res;
        if (!context.cursor)
          throw evaluation_error("Missing stack context");
        unw_get_reg(context.cursor, register_id, &res);
        stack.push(res + op->number);
        break;
      }

      // Push the CFA (Canonical Frame Addresse):
    case DW_OP_call_frame_cfa:
      {
        /* See 6.4 of DWARF4 (http://dwarfstd.org/doc/DWARF4.pdf#page=140):
         *
         * > Typically, the CFA is defined to be the value of the stack
         * > pointer at the call site in the previous frame (which may be
         * > different from its value on entry to the current frame).
         *
         * We need to unwind the frame in order to get the SP of the parent
         * frame.
         *
         * Warning: the CFA returned by libunwind (UNW_X86_64_RSP, etc.)
         * is the SP of the *current* frame. */

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
      // Push a literal/constant on the stack:
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

    case DW_OP_swap:
      stack.swap();
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
