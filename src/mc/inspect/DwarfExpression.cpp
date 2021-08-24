/* Copyright (c) 2014-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <cstddef>
#include <cstdint>
#include <unordered_set>

#include "src/mc/AddressSpace.hpp"
#include "src/mc/inspect/DwarfExpression.hpp"
#include "src/mc/inspect/Frame.hpp"
#include "src/mc/inspect/LocationList.hpp"
#include "src/mc/inspect/ObjectInformation.hpp"
#include "src/mc/inspect/mc_dwarf.hpp"
#include "src/mc/mc_private.hpp"

using simgrid::mc::remote;

namespace simgrid {
namespace dwarf {

void execute(const Dwarf_Op* ops, std::size_t n, const ExpressionContext& context, ExpressionStack& stack)
{
  for (size_t i = 0; i != n; ++i) {
    const Dwarf_Op* op = ops + i;
    std::uint8_t atom  = op->atom;
    intptr_t first;
    intptr_t second;

    switch (atom) {
        // Push the CFA (Canonical Frame Address):
      case DW_OP_call_frame_cfa:
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
        if (context.cursor) {
          // Get frame:
          unw_cursor_t cursor = *(context.cursor);
          unw_step(&cursor);

          unw_word_t res;
          unw_get_reg(&cursor, UNW_REG_SP, &res);
          stack.push(res);
          break;
        }
        throw evaluation_error("Missing cursor");

        // Frame base:
      case DW_OP_fbreg:
        stack.push((std::uintptr_t)context.frame_base + op->number);
        break;

        // Address from the base address of this ELF object.
        // Push the address on the stack (base_address + argument).
      case DW_OP_addr:
        if (context.object_info) {
          Dwarf_Off addr = (Dwarf_Off)(std::uintptr_t)context.object_info->base_address() + op->number;
          stack.push(addr);
          break;
        }
        throw evaluation_error("No base address");

        // ***** Stack manipulation:

        // Push another copy/duplicate the value at the top of the stack:
      case DW_OP_dup:
        stack.dup();
        break;

        // Pop/drop the top of the stack:
      case DW_OP_drop:
        (void)stack.pop();
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
        first  = stack.pop();
        second = stack.pop();
        stack.push(first + second);
        break;

      case DW_OP_mul:
        first  = stack.pop();
        second = stack.pop();
        stack.push(first * second);
        break;

      case DW_OP_plus_uconst:
        stack.top() += op->number;
        break;

      case DW_OP_not:
        stack.top() = ~stack.top();
        break;

      case DW_OP_neg:
        stack.top() = -(intptr_t)stack.top();
        break;

      case DW_OP_minus:
        first  = stack.pop();
        second = stack.pop();
        stack.push(second - first);
        break;

      case DW_OP_and:
        first  = stack.pop();
        second = stack.pop();
        stack.push(first & second);
        break;

      case DW_OP_or:
        first  = stack.pop();
        second = stack.pop();
        stack.push(first | second);
        break;

      case DW_OP_xor:
        first  = stack.pop();
        second = stack.pop();
        stack.push(first ^ second);
        break;

      case DW_OP_nop:
        break;

        // ***** Deference (memory fetch)

      case DW_OP_deref_size:
        throw evaluation_error("Unsupported operation");

      case DW_OP_deref:
        // Computed address:
        if (not context.address_space)
          throw evaluation_error("Missing address space");
        context.address_space->read_bytes(&stack.top(), sizeof(uintptr_t), remote(stack.top()));
        break;

      default:

        // Registers:
        static const std::unordered_set<uint8_t> registers = {
            DW_OP_breg0,  DW_OP_breg1,  DW_OP_breg2,  DW_OP_breg3,  DW_OP_breg4,  DW_OP_breg5,  DW_OP_breg6,
            DW_OP_breg7,  DW_OP_breg8,  DW_OP_breg9,  DW_OP_breg10, DW_OP_breg11, DW_OP_breg12, DW_OP_breg13,
            DW_OP_breg14, DW_OP_breg15, DW_OP_breg16, DW_OP_breg17, DW_OP_breg18, DW_OP_breg19, DW_OP_breg20,
            DW_OP_breg21, DW_OP_breg22, DW_OP_breg23, DW_OP_breg24, DW_OP_breg25, DW_OP_breg26, DW_OP_breg27,
            DW_OP_breg28, DW_OP_breg29, DW_OP_breg30, DW_OP_breg31};
        if (registers.count(atom) > 0) {
          // Push register + constant:
          int register_id = simgrid::dwarf::dwarf_register_to_libunwind(op->atom - DW_OP_breg0);
          unw_word_t res;
          if (not context.cursor)
            throw evaluation_error("Missing stack context");
          unw_get_reg(context.cursor, register_id, &res);
          stack.push(res + op->number);
          break;
        }

        // ***** Constants:

        // Short constant literals:
        static const std::unordered_set<uint8_t> literals = {
            DW_OP_lit0,  DW_OP_lit1,  DW_OP_lit2,  DW_OP_lit3,  DW_OP_lit4,  DW_OP_lit5,  DW_OP_lit6,  DW_OP_lit7,
            DW_OP_lit8,  DW_OP_lit9,  DW_OP_lit10, DW_OP_lit11, DW_OP_lit12, DW_OP_lit13, DW_OP_lit14, DW_OP_lit15,
            DW_OP_lit16, DW_OP_lit17, DW_OP_lit18, DW_OP_lit19, DW_OP_lit20, DW_OP_lit21, DW_OP_lit22, DW_OP_lit23,
            DW_OP_lit24, DW_OP_lit25, DW_OP_lit26, DW_OP_lit27, DW_OP_lit28, DW_OP_lit29, DW_OP_lit30, DW_OP_lit31};
        if (literals.count(atom) > 0) {
          // Push a literal/constant on the stack:
          stack.push(atom - DW_OP_lit0);
          break;
        }

        // General constants:
        static const std::unordered_set<uint8_t> constants = {
            DW_OP_const1u, DW_OP_const2u, DW_OP_const4u, DW_OP_const8u, DW_OP_const1s,
            DW_OP_const2s, DW_OP_const4s, DW_OP_const8s, DW_OP_constu,  DW_OP_consts};
        if (constants.count(atom) > 0) {
          // Push the constant argument on the stack.
          stack.push(op->number);
          break;
        }

        // Not handled:
        throw evaluation_error("Unsupported operation");
    }
  }
}

} // namespace dwarf
} // namespace simgrid
