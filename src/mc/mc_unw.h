/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UNW_H
#define SIMGRID_MC_UNW_H

/** \file
 *  Libunwind implementation for the model-checker
 *
 *  Libunwind provides an pluggable stack unwinding API: the way the current
 *  registers and memory is accessed, the way unwinding informations is found
 *  is pluggable.
 *
 *  This component implements the libunwind API for he model-checker:
 *
 *    * reading memory from a simgrid::mc::AddressSpace*;
 *
 *    * reading stack registers from a saved snapshot (context).
 *
 *  Parts of the libunwind information fetching is currently handled by the
 *  standard `libunwind` implementations (either the local one or the ptrace one)
 *  because parsing `.eh_frame` section is not fun and `libdw` does not help
 *  much here.
 */

#include <sys/types.h>

#include <xbt/base.h>

#include <libunwind.h>

#include "src/mc/mc_forward.hpp"

namespace simgrid {
namespace unw {

XBT_PRIVATE unw_addr_space_t create_addr_space();
XBT_PRIVATE void* create_context(unw_addr_space_t as, pid_t pid);

}
}

namespace simgrid {
namespace mc {

class UnwindContext {
  simgrid::mc::AddressSpace* addressSpace_ = nullptr;
  simgrid::mc::Process*      process_ = nullptr;
  unw_context_t              unwindContext_;
public:
  UnwindContext() {}
  ~UnwindContext() { clear(); }
  void initialize(simgrid::mc::Process* process, unw_context_t* c);
  void clear();
  unw_cursor_t cursor();

private: // Methods and virtual table for libunwind
  static int find_proc_info(unw_addr_space_t as,
                unw_word_t ip, unw_proc_info_t *pip,
                int need_unwind_info, void* arg) noexcept;
  static void put_unwind_info(unw_addr_space_t as,
                unw_proc_info_t *pip, void* arg) noexcept;
  static int get_dyn_info_list_addr(unw_addr_space_t as,
                unw_word_t *dilap, void* arg) noexcept;
  static int access_mem(unw_addr_space_t as,
                unw_word_t addr, unw_word_t *valp,
                int write, void* arg) noexcept;
  static void* get_reg(unw_context_t* context, unw_regnum_t regnum) noexcept;
  static int access_reg(unw_addr_space_t as,
                unw_regnum_t regnum, unw_word_t *valp,
                int write, void* arg) noexcept;
  static int access_fpreg(unw_addr_space_t as,
                unw_regnum_t regnum, unw_fpreg_t *fpvalp,
                int write, void* arg) noexcept;
  static int resume(unw_addr_space_t as,
                unw_cursor_t *cp, void* arg) noexcept;
  static int get_proc_name(unw_addr_space_t as,
                unw_word_t addr, char *bufp,
                size_t buf_len, unw_word_t *offp,
                void* arg) noexcept;
  static unw_accessors_t accessors;
public:
  // Create a libunwind address space:
  static unw_addr_space_t createUnwindAddressSpace();
};

}
}

#endif
