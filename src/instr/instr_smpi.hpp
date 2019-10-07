/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef INSTR_SMPI_HPP
#define INSTR_SMPI_HPP

#include "smpi/smpi.h"
#include "src/instr/instr_private.hpp"
#include <string>

/* Helper functions */
XBT_PRIVATE container_t smpi_container(int rank);
XBT_PRIVATE void TRACE_smpi_setup_container(int rank, sg_host_t host);

XBT_PRIVATE void TRACE_smpi_computing_init(int rank);
XBT_PRIVATE void TRACE_smpi_computing_out(int rank);
XBT_PRIVATE void TRACE_smpi_computing_in(int rank, double amount);
XBT_PRIVATE void TRACE_smpi_sleeping_init(int rank);
XBT_PRIVATE void TRACE_smpi_sleeping_out(int rank);
XBT_PRIVATE void TRACE_smpi_sleeping_in(int rank, double duration);
XBT_PRIVATE void TRACE_smpi_comm_in(int rank, const char* operation, simgrid::instr::TIData* extra);
XBT_PRIVATE void TRACE_smpi_comm_out(int rank);
XBT_PRIVATE void TRACE_smpi_send(int rank, int src, int dst, int tag, int size);
XBT_PRIVATE void TRACE_smpi_recv(int src, int dst, int tag);
XBT_PRIVATE void TRACE_smpi_init(int rank);
/* SMPI + LB (load balancer) */
XBT_PRIVATE void TRACE_smpi_process_change_host(int rank, sg_host_t new_host);

class smpi_trace_call_location_t {
public:
  std::string filename;
  int linenumber;

  std::string previous_filename;
  int previous_linenumber;

  std::string get_composed_key()
  {
    return previous_filename + ':' + std::to_string(previous_linenumber) + ':' + filename + ':' +
           std::to_string(linenumber);
  }
};

#endif
