/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_SMPI_HPP
#define INSTR_SMPI_HPP

#include "smpi/smpi.h"
#include "src/instr/instr_private.hpp"
#include <string>

/* Helper functions */
XBT_PRIVATE simgrid::instr::Container* smpi_container(aid_t pid);
XBT_PRIVATE void TRACE_smpi_setup_container(aid_t pid, const_sg_host_t host);

XBT_PRIVATE void TRACE_smpi_sleeping_out(aid_t pid);
XBT_PRIVATE void TRACE_smpi_sleeping_in(aid_t pid, double duration);
XBT_PRIVATE void TRACE_smpi_comm_in(aid_t pid, const char* operation, simgrid::instr::TIData* extra);
XBT_PRIVATE void TRACE_smpi_comm_out(aid_t pid);
XBT_PRIVATE void TRACE_smpi_send(aid_t rank, aid_t src, aid_t dst, int tag, int size);
XBT_PRIVATE void TRACE_smpi_recv(aid_t src, aid_t dst, int tag);
XBT_PRIVATE void TRACE_smpi_init(aid_t pid, const std::string& calling_func);

class smpi_trace_call_location_t {
public:
  std::string filename;
  int linenumber = 0;

  std::string previous_filename;
  int previous_linenumber = 0;

  std::string get_composed_key() const
  {
    return previous_filename + ':' + std::to_string(previous_linenumber) + ':' + filename + ':' +
           std::to_string(linenumber);
  }
};

#endif
