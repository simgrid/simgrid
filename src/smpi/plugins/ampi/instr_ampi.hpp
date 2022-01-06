/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_INSTR_AMPI_HPP_
#define INSTR_INSTR_AMPI_HPP_

#include "smpi/smpi.h"
#include "src/instr/instr_private.hpp"

XBT_PRIVATE void TRACE_Iteration_in(aid_t pid, simgrid::instr::TIData* extra);
XBT_PRIVATE void TRACE_Iteration_out(aid_t pid, simgrid::instr::TIData* extra);
XBT_PRIVATE void TRACE_migration_call(aid_t pid, simgrid::instr::TIData* extra);

#endif
